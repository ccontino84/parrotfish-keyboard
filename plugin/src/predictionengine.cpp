// SPDX-FileCopyrightText: 2026 ccontino84
// SPDX-License-Identifier: LicenseRef-TBD

#include "predictionengine.h"

#include "parrotfish/utf8.h"

#include <QDir>
#include <algorithm>
#include <cmath>
#include <QElapsedTimer>
#include <QFile>
#include <QtDebug>

namespace {

const char *const Languages[] = {"en", "it", "de"};
const int MaxSuggestions = 10;
// Undos of the same autocorrection that stop it (plan §6).
const int UndosToStop = 2;

std::u32string toU32(const QString &s)
{
    const QVector<uint> ucs4 = s.toUcs4();
    return std::u32string(ucs4.begin(), ucs4.end());
}

parrotfish::EngineParams mvpParams()
{
    parrotfish::EngineParams params;
    params.strength = parrotfish::Strength::Gentle;
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

    for (const char *lang : Languages) {
        std::string error;
        const QString path = m_dataDir + QLatin1Char('/') + QLatin1String(lang) + QStringLiteral(".pfd");
        if (!m_engine.addLanguage(path.toStdString(), &error))
            qWarning("parrotfish: %s", error.c_str());
    }
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
    update();
}

void PredictionEngine::setLanguage(const QString &language)
{
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
    m_context = textBefore;
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
    const QString lead = leadingLanguage();
    const double conf = confidence();
    m_engine.startText(toU32(textBefore), prior);
    if (leadingLanguage() != lead || std::abs(confidence() - conf) > 0.01)
        emit leadingLanguageChanged();
    if (m_preedit.isEmpty())
        update();
}

int PredictionEngine::setKeys(const QJSValue &keys)
{
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
    if (strength == m_strength)
        return;
    m_strength = strength;
    emit strengthChanged();
    applyParams();
}

void PredictionEngine::setTuning(const QString &tuning)
{
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
                    : m_strength == QLatin1String("normal") ? parrotfish::Strength::Normal
                                                            : parrotfish::Strength::Gentle;
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

void PredictionEngine::rejectCorrection(const QString &typed)
{
    const int undos = ++m_undos[typed.toLower()];
    if (m_debug)
        qInfo("parrotfish: autocorrection undone (%d of %d)", undos, UndosToStop);
}

void PredictionEngine::acceptCorrection(const QString &typed)
{
    if (m_undos.remove(typed.toLower()) && m_debug)
        qInfo("parrotfish: undos of an autocorrection forgotten");
}

void PredictionEngine::logSlips(const QString &typed, const QString &meant, const QString &how)
{
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
    if (t.size() == m.size()) {
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
    QFile f(QDir::homePath() + QStringLiteral("/.local/share/parrotfish-keyboard/slips.log"));
    if (!f.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text))
        return;
    for (const QString &s : slips)
        f.write((how + QLatin1Char(' ') + s + QLatin1Char('\n')).toUtf8());
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
        const parrotfish::Result r = m_engine.process(parrotfish::tapsFor(typed));
        for (const parrotfish::Suggestion &s : r.suggestions) {
            if (items.size() >= MaxSuggestions)
                break;
            items.append(fromU32(s.word));
        }
        if (r.autocorrect == 0 && m_undos.value(m_preedit.toLower()) < UndosToStop)
            autocorrection = items.value(0);
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
