// SPDX-FileCopyrightText: 2026 ccontino84
// SPDX-License-Identifier: LicenseRef-TBD

#include "predictionengine.h"

#include "parrotfish/utf8.h"

#include <QDate>
#include <QDir>
#include <QLocale>
#include <algorithm>
#include <cmath>
#include <QElapsedTimer>
#include <QFile>
#include <QtDebug>

namespace {

const int MaxSuggestions = 10;
// Undos of the same autocorrection that stop it (plan §6).
const int UndosToStop = 2;
// slips.log counts the words typed in batches of this many.
const int SlipsWordBatch = 50;

std::u32string toU32(const QString &s)
{
    const QVector<uint> ucs4 = s.toUcs4();
    return std::u32string(ucs4.begin(), ucs4.end());
}

parrotfish::EngineParams mvpParams()
{
    parrotfish::EngineParams params;
    params.strength = parrotfish::Strength::Normal;
    params.corrector.completions = true;
    return params;
}

QString fromU32(const std::u32string &s)
{
    return QString::fromUcs4(reinterpret_cast<const uint *>(s.data()), static_cast<int>(s.size()));
}

} // namespace

PredictionEngine::PredictionEngine(QObject *parent)
    : QAbstractListModel(parent)
    , m_engine(mvpParams())
{
    m_dataDir = qEnvironmentVariableIsSet("PARROTFISH_DATA_DIR")
            ? QString::fromLocal8Bit(qgetenv("PARROTFISH_DATA_DIR"))
            : QStringLiteral(PARROTFISH_DATA_DIR);
    // Timing logs (never the typed text) while this file exists.
    m_debug = QFile::exists(QDir::homePath() + QStringLiteral("/.local/share/parrotfish-keyboard/debug"));
    m_store.reset(new LearningStore(QDir::homePath()
                                    + QStringLiteral("/.local/share/parrotfish-keyboard/learning.db")));
    m_undos = m_store->undos();

    loadLanguages();
    loadFallbackLayout();
}

int PredictionEngine::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : m_items.size();
}

QVariant PredictionEngine::data(const QModelIndex &index, int role) const
{
    if (role != TextRole || index.row() < 0 || index.row() >= m_items.size())
        return QVariant();
    return m_items.at(index.row());
}

QHash<int, QByteArray> PredictionEngine::roleNames() const
{
    QHash<int, QByteArray> roles;
    roles.insert(TextRole, "text");
    return roles;
}

void PredictionEngine::setPreedit(const QString &preedit)
{
    if (preedit == m_preedit)
        return;
    m_preedit = preedit;
    emit preeditChanged();
    // Not now: the handler sends the letter to the app after setting
    // preedit, and the search (up to 15 ms on a phone between taps) must
    // not hold it back. The bar follows on the next pass of the event loop;
    // whatever needs the result earlier calls flush() first.
    if (!m_updatePending) {
        m_updatePending = true;
        QMetaObject::invokeMethod(this, "flush", Qt::QueuedConnection);
    }
}

void PredictionEngine::flush()
{
    if (!m_updatePending)
        return;
    m_updatePending = false;
    update();
}

void PredictionEngine::setPreeditKeys(const QStringList &keys)
{
    if (keys == m_preeditKeys)
        return;
    m_preeditKeys = keys;   // used at the next update (the handler sets preedit next)
    emit preeditKeysChanged();
}

void PredictionEngine::setLanguages(const QString &languages)
{
    flush();
    if (languages == m_languages)
        return;
    m_languages = languages;
    emit languagesChanged();
    loadLanguages();
    m_contextValid = false;
    // The undo memory stays: it is kept per language, for when a language
    // is used again (and it is loaded before them at start).
    setContext(m_context);
}

void PredictionEngine::loadLanguages()
{
    auto dict = [this](const QString &code) {
        return m_dataDir + QLatin1Char('/') + code + QStringLiteral(".pfd");
    };
    QStringList codes = m_languages.split(QLatin1Char(' '));
    codes.removeAll(QString());
    if (codes.isEmpty()) {
        // The system language only (pt_BR, or its language: it_IT -> it);
        // others are added in the settings. English if there is no
        // dictionary for it (below).
        const QString name = QLocale::system().name();
        QString lang = name.section(QLatin1Char('_'), 0, 0);
        if (lang == QLatin1String("no") || lang == QLatin1String("nn"))
            lang = QStringLiteral("nb");
        else if (lang == QLatin1String("pt"))
            lang = QStringLiteral("pt_PT");   // pt_AO, pt_MZ...: the Portugal spelling
        codes << (QFile::exists(dict(name)) ? name : lang);
    }
    m_engine.clearLanguages();
    for (const QString &code : codes) {
        if (!QFile::exists(dict(code)))
            continue;
        std::string error;
        if (!m_engine.addLanguage(dict(code).toStdString(), &error))
            qWarning("parrotfish: %s", error.c_str());
    }
    if (m_engine.languageCount() == 0) {
        std::string error;
        if (!m_engine.addLanguage(dict(QStringLiteral("en")).toStdString(), &error))
            qWarning("parrotfish: %s", error.c_str());
    }
    if (m_debug)
        qInfo("parrotfish: %d typing languages", static_cast<int>(m_engine.languageCount()));
}

void PredictionEngine::setLanguage(const QString &language)
{
    flush();
    if (language == m_language)
        return;
    m_language = language;
    emit languageChanged();
    if (!m_layoutKeys)
        loadFallbackLayout();
}

QString PredictionEngine::leadingLanguage() const
{
    const int i = m_engine.tracker().leading();
    return i < 0 ? QString() : QString::fromStdString(m_engine.language(i));
}

double PredictionEngine::confidence() const
{
    return m_engine.tracker().confidence();
}

void PredictionEngine::setContext(const QString &textBefore)
{
    flush();
    // Starting leans (plan §5): the language the last text was in (×3) and
    // the layout's language (×2). The text already in the field soon
    // outweighs both; one distinctive word of another language switches.
    std::vector<double> prior(m_engine.languageCount(), 1.0);
    const int remembered = m_engine.languageIndex(m_remembered.toStdString());
    if (remembered >= 0)
        prior[remembered] *= 3.0;
    const int hinted = m_engine.languageIndex(m_language.toLower().toStdString());
    if (hinted >= 0)
        prior[hinted] *= 2.0;
    // At Space the handler passes the same text up to four times (the
    // committed word, then the app's text and cursor, then the next word's
    // first letter): read it once.
    if (m_contextValid && textBefore == m_context && prior == m_contextPrior) {
        if (m_preedit.isEmpty() && !m_nextValid)
            update();
        return;
    }
    QElapsedTimer timer;
    timer.start();
    m_context = textBefore;
    m_contextValid = true;
    m_contextPrior = prior;
    m_nextValid = false;
    const QString lead = leadingLanguage();
    const double conf = confidence();
    m_engine.startText(toU32(textBefore), prior);
    if (m_debug)
        qInfo("parrotfish: context, %d characters, %.1f ms", static_cast<int>(textBefore.size()),
              timer.nsecsElapsed() / 1e6);
    if (leadingLanguage() != lead || std::abs(confidence() - conf) > 0.01)
        emit leadingLanguageChanged();
    if (m_preedit.isEmpty())
        update();
}

int PredictionEngine::setKeys(const QJSValue &keys)
{
    flush();
    std::vector<parrotfish::Key> out;
    const int n = keys.property(QStringLiteral("length")).toInt();
    for (int i = 0; i < n; ++i) {
        const QJSValue k = keys.property(static_cast<quint32>(i));
        const std::u32string c = toU32(k.property(QStringLiteral("ch")).toString().toLower());
        if (c.size() != 1)
            continue;
        out.push_back({c[0],
                       static_cast<float>(k.property(QStringLiteral("x")).toNumber()),
                       static_cast<float>(k.property(QStringLiteral("y")).toNumber()),
                       static_cast<float>(k.property(QStringLiteral("width")).toNumber()),
                       static_cast<float>(k.property(QStringLiteral("height")).toNumber())});
    }
    if (out.size() < 10)
        return 0;
    auto same = [](const parrotfish::Key &a, const parrotfish::Key &b) {
        return a.ch == b.ch && a.x == b.x && a.y == b.y && a.width == b.width && a.height == b.height;
    };
    if (m_layoutKeys && std::equal(out.begin(), out.end(), m_keys.begin(), m_keys.end(), same))
        return static_cast<int>(out.size());
    if (m_debug) {
        // Key positions only, to check the geometry the engine works with.
        QString keys;
        for (const parrotfish::Key &k : out)
            keys += QStringLiteral(" %1:%2,%3/%4x%5").arg(fromU32(std::u32string(1, k.ch)))
                    .arg(qRound(k.x)).arg(qRound(k.y)).arg(qRound(k.width)).arg(qRound(k.height));
        qInfo("parrotfish: %d of %d keys from the layout:%s", static_cast<int>(out.size()), n, qPrintable(keys));
    }
    m_keys = out;
    m_engine.setLayout(parrotfish::KeyModel(out));
    m_layoutKeys = true;
    update();
    return static_cast<int>(out.size());
}

void PredictionEngine::setStrength(const QString &strength)
{
    flush();
    if (strength == m_strength)
        return;
    m_strength = strength;
    emit strengthChanged();
    applyParams();
}

void PredictionEngine::setTuning(const QString &tuning)
{
    flush();
    if (tuning == m_tuning)
        return;
    m_tuning = tuning;
    emit tuningChanged();
    applyParams();
}

void PredictionEngine::applyParams()
{
    parrotfish::EngineParams params = mvpParams();
    params.strength = m_strength == QLatin1String("off") ? parrotfish::Strength::Off
                    : m_strength == QLatin1String("gentle") ? parrotfish::Strength::Gentle
                    : m_strength == QLatin1String("strong") ? parrotfish::Strength::Strong
                                                            : parrotfish::Strength::Normal;
    const QStringList parts = m_tuning.split(QLatin1Char(' '));
    for (const QString &part : parts) {
        if (part.isEmpty())
            continue;
        bool ok = false;
        try {
            ok = parrotfish::setParam(params, part.toStdString());
        } catch (const std::exception &) {
            ok = false;   // not a number
        }
        if (!ok)
            qWarning("parrotfish: ignoring tuning \"%s\"", qPrintable(part));
    }
    m_engine.setParams(params);
    m_layoutKeys = false;          // the handler passes the keys again
    loadFallbackLayout();
    m_contextValid = false;
    setContext(m_context);
}

void PredictionEngine::setRememberedLanguage(const QString &language)
{
    if (language == m_remembered)
        return;
    m_remembered = language;
    emit rememberedLanguageChanged();
}

QString PredictionEngine::resolveTap(const QString &preedit, const QString &lifted,
                                     const QString &initial) const
{
    const std::u32string l = toU32(lifted), i = toU32(initial);
    if (l.size() != 1 || i.size() != 1)
        return lifted;
    const char32_t c = m_engine.resolveTap(toU32(preedit), l[0], i[0]);
    if (m_debug && c != l[0])
        qInfo("parrotfish: uncertain tap took the touch-down key");
    return c == l[0] ? lifted : initial;
}

// The language `typed` was corrected in: the text's leading language then
// (the corrected word has been counted since). A word picked from the bar
// is still being typed: the leading language now, even if it was corrected
// in another language before.
QString PredictionEngine::fixLanguage(const QString &typed) const
{
    if (typed.compare(m_preedit, Qt::CaseInsensitive) == 0)
        return leadingLanguage();
    return m_fixLanguages.value(typed.toLower(), leadingLanguage());
}

void PredictionEngine::rejectCorrection(const QString &typed)
{
    flush();
    const QString language = fixLanguage(typed), lower = typed.toLower();
    // A valid word fixed by Strong is left alone after one undo.
    int &count = m_undos[qMakePair(language, lower)];
    count = m_strongFixes.contains(lower) ? UndosToStop : count + 1;
    const int undos = count;
    m_store->setUndos(language, lower, undos);
    if (m_debug)
        qInfo("parrotfish: autocorrection undone (%d of %d) in %s", undos, UndosToStop, qPrintable(language));
}

void PredictionEngine::acceptCorrection(const QString &typed)
{
    flush();
    const QString language = fixLanguage(typed), lower = typed.toLower();
    if (!m_undos.remove(qMakePair(language, lower)))
        return;
    m_store->setUndos(language, lower, 0);
    if (m_debug)
        qInfo("parrotfish: undos of an autocorrection forgotten in %s", qPrintable(language));
}

void PredictionEngine::logSlips(const QString &typed, const QString &meant, const QString &how)
{
    flush();
    if (!m_debug)
        return;
    const std::u32string t = toU32(typed.toLower()), m = toU32(meant.toLower());
    if (m.size() >= t.size() && m.compare(0, t.size(), t) == 0)
        return;   // a completion, not a correction
    auto isKey = [this](char32_t c) {
        return std::any_of(m_keys.begin(), m_keys.end(), [c](const parrotfish::Key &k) { return k.ch == c; });
    };
    auto letter = [](char32_t c) { return fromU32(std::u32string(1, c)); };
    // `longer` without its letter at j is `shorter`.
    auto dropped = [](const std::u32string &longer, const std::u32string &shorter) -> int {
        for (size_t j = 0; j < longer.size(); ++j)
            if (longer.substr(0, j) + longer.substr(j + 1) == shorter)
                return static_cast<int>(j);
        return -1;
    };
    // A letter next to an equal one: one of a double.
    auto doubled = [](const std::u32string &w, int j) {
        return (j > 0 && w[j - 1] == w[j]) || (j + 1 < static_cast<int>(w.size()) && w[j + 1] == w[j]);
    };
    QStringList slips;
    if (m.find(U' ') != std::u32string::npos && t.find(U' ') == std::u32string::npos) {
        // Two words typed as one: the space left out, or typed as a key.
        std::u32string joined = m;
        joined.erase(std::remove(joined.begin(), joined.end(), U' '), joined.end());
        const size_t at = m.find(U' ');
        slips << (joined == t ? QStringLiteral("space missing")
                  : t.size() == m.size() && at < t.size() ? QStringLiteral("space key %1").arg(letter(t[at]))
                  : QStringLiteral("other"));
    } else if (t.size() == m.size()) {
        std::vector<size_t> diff;
        for (size_t i = 0; i < t.size(); ++i)
            if (t[i] != m[i])
                diff.push_back(i);
        if (diff.size() == 2 && diff[1] == diff[0] + 1 && t[diff[0]] == m[diff[1]] && t[diff[1]] == m[diff[0]])
            slips << QStringLiteral("swap");
        else if (diff.size() > 2)
            slips << QStringLiteral("other");
        else
            for (size_t i : diff)
                slips << (isKey(t[i]) && isKey(m[i])
                          ? QStringLiteral("key %1 %2").arg(letter(t[i]), letter(m[i]))
                          : QStringLiteral("accent"));
    } else if (m.size() == t.size() + 1) {
        const int j = dropped(m, t);
        slips << (j < 0 ? QStringLiteral("other") : doubled(m, j) ? QStringLiteral("missing-double")
                                                                  : QStringLiteral("missing"));
    } else if (t.size() == m.size() + 1) {
        const int j = dropped(t, m);
        slips << (j < 0 ? QStringLiteral("other") : doubled(t, j) ? QStringLiteral("extra-double")
                                                                  : QStringLiteral("extra"));
    } else {
        slips << QStringLiteral("other");
    }
    // Strong's fixes of valid words: their own lines, with the margin.
    QString prefix = how, suffix;
    const auto strong = m_strongFixes.constFind(typed.toLower());
    if (how != QLatin1String("picked") && strong != m_strongFixes.constEnd()
            && strong->first == meant.toLower()) {
        prefix = QStringLiteral("strong ") + how;
        suffix = QStringLiteral(" margin=") + QString::number(strong->second, 'f', 1);
    }
    // A word changed by hand into a quite different one is a new word, not
    // a slip.
    if (how == QLatin1String("manual"))
        slips.removeAll(QStringLiteral("other"));
    if (slips.isEmpty())
        return;
    for (QString &s : slips)
        s = prefix + QLatin1Char(' ') + s + suffix;
    appendSlips(slips);
}

void PredictionEngine::countWord()
{
    flush();
    if (!m_debug)
        return;
    if (slipsHeader() != m_slipsHeader)
        appendSlips(QStringList());   // the words so far belong to the old settings
    if (++m_slipsWords >= SlipsWordBatch) {
        m_slipsWords = 0;
        appendSlips(QStringList() << QStringLiteral("words %1").arg(SlipsWordBatch));
    }
}

QString PredictionEngine::slipsHeader() const
{
    // Rows apart, in key widths (the letter keys' median width): how close
    // vertical neighbours are.
    std::vector<float> widths, ys;
    for (const parrotfish::Key &k : m_keys) {
        if (k.ch >= U'0' && k.ch <= U'9')
            continue;   // the letter rows only, with or without the number row
        widths.push_back(k.width);
        if (std::none_of(ys.begin(), ys.end(), [&](float y) { return std::abs(y - k.y) < k.height / 2; }))
            ys.push_back(k.y);
    }
    QString pitch = QStringLiteral("?");
    if (!widths.empty() && ys.size() >= 2) {
        std::nth_element(widths.begin(), widths.begin() + widths.size() / 2, widths.end());
        std::sort(ys.begin(), ys.end());
        float row = ys[1] - ys[0];
        for (size_t i = 2; i < ys.size(); ++i)
            row = std::min(row, ys[i] - ys[i - 1]);
        if (widths[widths.size() / 2] > 0)
            pitch = QString::number(row / widths[widths.size() / 2], 'f', 2);
    }
    QString tuning = m_tuning.simplified();
    tuning.replace(QLatin1Char(' '), QLatin1Char(','));
    return QStringLiteral("# %1 %2 %3 %4 pitch=%5%6")
            .arg(QDate::currentDate().toString(Qt::ISODate), QStringLiteral(PARROTFISH_VERSION),
                 m_slipsConfig.isEmpty() ? QStringLiteral("?") : m_slipsConfig, m_strength, pitch,
                 tuning.isEmpty() ? QString() : QStringLiteral(" tuning=") + tuning);
}

void PredictionEngine::appendSlips(QStringList lines)
{
    const QString header = slipsHeader();
    if (header != m_slipsHeader) {
        lines.prepend(header);
        if (m_slipsWords > 0 && !m_slipsHeader.isEmpty())
            lines.prepend(QStringLiteral("words %1").arg(m_slipsWords));
        m_slipsWords = 0;
        m_slipsHeader = header;
    }
    QFile f(QDir::homePath() + QStringLiteral("/.local/share/parrotfish-keyboard/slips.log"));
    if (!f.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text))
        return;
    for (const QString &line : lines)
        f.write((line + QLatin1Char('\n')).toUtf8());
}

void PredictionEngine::setSlipsConfig(const QString &config)
{
    if (config == m_slipsConfig)
        return;
    m_slipsConfig = config;
    emit slipsConfigChanged();
}

bool PredictionEngine::isLetter(const QString &character) const
{
    const QVector<uint> ucs4 = character.toUcs4();
    return !ucs4.isEmpty() && QChar::isLetter(ucs4.first());
}

void PredictionEngine::log(const QString &message) const
{
    if (m_debug)
        qInfo("parrotfish: %s", qPrintable(message));
}

void PredictionEngine::loadFallbackLayout()
{
    // Approximate rows, used until the handler passes the real keys.
    QString lang = m_language.toLower();
    QFile f(m_dataDir + QStringLiteral("/layouts/") + lang + QStringLiteral(".layout"));
    if (!f.exists())
        f.setFileName(m_dataDir + QStringLiteral("/layouts/en.layout"));
    if (!f.open(QIODevice::ReadOnly)) {
        qWarning("parrotfish: no layout file in %s", qPrintable(m_dataDir));
        return;
    }
    m_engine.setLayout(parrotfish::KeyModel::parse(f.readAll().toStdString()));
}

void PredictionEngine::update()
{
    QStringList items;
    QString autocorrection;
    m_nextValid = m_preedit.isEmpty();
    if (m_preedit.isEmpty()) {
        QElapsedTimer timer;
        timer.start();
        for (const parrotfish::Suggestion &s : m_engine.predict(toU32(m_context), MaxSuggestions))
            items.append(fromU32(s.word));
        if (m_debug)
            qInfo("parrotfish: next word, %.1f ms, %d suggestions, lead %s %.2f",
                  timer.nsecsElapsed() / 1e6, items.size(), qPrintable(leadingLanguage()), confidence());
    } else {
        QElapsedTimer timer;
        timer.start();
        const std::u32string typed = toU32(m_preedit);
        std::vector<parrotfish::Tap> taps = parrotfish::tapsFor(typed);
        if (static_cast<size_t>(m_preeditKeys.size()) == taps.size()) {
            for (size_t i = 0; i < taps.size(); ++i) {
                const QString &k = m_preeditKeys.at(static_cast<int>(i));
                const std::u32string other = toU32(k.left(k.size() - 1).toLower());
                if (k.size() >= 2 && other.size() == 1 && other[0] != taps[i].ch) {
                    taps[i].initial = other[0];
                    taps[i].chLifted = k.endsWith(QLatin1Char('1'));
                }
            }
        }
        const parrotfish::Result r = m_engine.process(taps);
        for (const parrotfish::Suggestion &s : r.suggestions) {
            if (items.size() >= MaxSuggestions)
                break;
            items.append(fromU32(s.word));
        }
        const QString language = leadingLanguage(), lower = m_preedit.toLower();
        if (r.autocorrect == 0 && m_undos.value(qMakePair(language, lower)) < UndosToStop) {
            autocorrection = items.value(0);
            if (m_fixLanguages.size() > 64)
                m_fixLanguages.clear();
            m_fixLanguages.insert(lower, language);
            if (r.realWordFix) {
                if (m_strongFixes.size() > 32)
                    m_strongFixes.clear();
                m_strongFixes.insert(lower, qMakePair(autocorrection.toLower(), r.margin));
            }
        }
        if (m_debug)
            qInfo("parrotfish: %d letters, %.1f ms, %d suggestions%s, lead %s %.2f",
                  m_preedit.size(), timer.nsecsElapsed() / 1e6, items.size(),
                  autocorrection.isEmpty() ? "" : ", autocorrect",
                  qPrintable(leadingLanguage()), m_engine.tracker().confidence());
    }
    if (items == m_items && autocorrection == m_autocorrection)
        return;
    if (items != m_items) {
        beginResetModel();
        m_items = items;
        endResetModel();
    }
    m_autocorrection = autocorrection;
    emit predictionsChanged();
}
