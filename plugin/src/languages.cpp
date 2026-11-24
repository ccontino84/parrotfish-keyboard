// SPDX-FileCopyrightText: 2026 ccontino84
// SPDX-License-Identifier: LGPL-2.1-or-later

#include "languages.h"

#include <QDir>
#include <QFile>
#include <QHash>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLocale>
#include <algorithm>

namespace {

QString dataDir()
{
    return qEnvironmentVariableIsSet("PARROTFISH_DATA_DIR") ? QString::fromLocal8Bit(qgetenv("PARROTFISH_DATA_DIR"))
                                                             : QStringLiteral(PARROTFISH_DATA_DIR);
}

// The hash in `path`.sha256 if `path` exists, else "" (none: `exists` false).
QString record(const QString &path, bool *exists)
{
    *exists = QFile::exists(path);
    QFile f(path + QStringLiteral(".sha256"));
    if (!*exists || !f.open(QIODevice::ReadOnly))
        return QString();
    return QString::fromLatin1(f.read(64)).toLower();
}

} // namespace

Languages::Languages(QObject *parent)
    : QObject(parent)
    , m_dataDir(dataDir())
    , m_locale(QLocale::system().name())
{
    QFile f(m_dataDir + QStringLiteral("/languages.json"));
    if (!f.open(QIODevice::ReadOnly)) {
        qWarning("parrotfish: no %s", qPrintable(f.fileName()));
        return;
    }
    const QJsonObject root = QJsonDocument::fromJson(f.readAll()).object();
    for (auto it = root.constBegin(); it != root.constEnd(); ++it) {
        const QJsonObject o = it.value().toObject();
        Language l;
        l.code = it.key();
        l.name = o.value(QStringLiteral("name")).toString();
        l.script = o.value(QStringLiteral("script")).toString(QStringLiteral("Latn"));
        l.experimental = o.value(QStringLiteral("experimental")).toBool();
        l.fallback = o.value(QStringLiteral("fallback")).toBool();
        l.menuLetters = o.value(QStringLiteral("menuLetters")).toString();
        l.extraKeys = o.value(QStringLiteral("extraKeys")).toString();
        l.commaExtra = o.value(QStringLiteral("commaExtra")).toString();
        l.periodBefore = o.value(QStringLiteral("periodBefore")).toString();
        l.periodAfter = o.value(QStringLiteral("periodAfter")).toString();
        l.systemLanguages = o.value(QStringLiteral("systemLanguages")).toVariant().toStringList();
        const QJsonObject download = o.value(QStringLiteral("download")).toObject();
        l.downloadSize = static_cast<qint64>(download.value(QStringLiteral("size")).toDouble());
        l.releaseHash = download.value(QStringLiteral("pfdSha256")).toString().toLower();
        m_languages << l;
    }
    std::sort(m_languages.begin(), m_languages.end(), [](const Language &a, const Language &b) {
        return QString::localeAwareCompare(a.name, b.name) < 0;
    });
    refresh();
}

Languages &Languages::instance()
{
    static Languages languages;
    return languages;
}

QString Languages::downloadDir()
{
    return QDir::homePath() + QStringLiteral("/.local/share/parrotfish-keyboard/dictionaries");
}

QString Languages::packagePath(const QString &code) const
{
    return m_dataDir + QLatin1Char('/') + code + QStringLiteral(".pfd");
}

void Languages::refresh()
{
    bool any = false;
    for (Language &l : m_languages) {
        Language before = l;
        l.packageHash = record(packagePath(l.code), &l.packaged);
        bool file = false;
        l.downloadHash = record(downloadDir() + QLatin1Char('/') + l.code + QStringLiteral(".pfd"), &file);
        l.downloaded = file && !l.downloadHash.isEmpty();   // no record: not complete
        if (!l.downloaded)
            l.downloadHash.clear();
        any = any || l.packaged != before.packaged || l.downloaded != before.downloaded
                || l.packageHash != before.packageHash || l.downloadHash != before.downloadHash;
    }
    if (any || m_all.isEmpty())
        updateAll();
}

void Languages::updateAll()
{
    m_all.clear();
    for (const Language &l : m_languages) {
        const bool current = !l.releaseHash.isEmpty()
                && (l.downloadHash == l.releaseHash || l.packageHash == l.releaseHash);
        // A package's dictionary of unknown hash isn't reported as old.
        const bool known = l.downloaded || !l.packageHash.isEmpty();
        const QStringList files = dictionaryFiles(l.code);
        const QString source = files.isEmpty() ? QString()
                             : files.first().startsWith(downloadDir()) ? QStringLiteral("download")
                                                                       : QStringLiteral("package");
        m_all << QVariantMap{{QStringLiteral("code"), l.code}, {QStringLiteral("name"), l.name},
                             {QStringLiteral("script"), l.script},
                             {QStringLiteral("source"), source},
                             {QStringLiteral("experimental"), l.experimental},
                             {QStringLiteral("packaged"), l.packaged}, {QStringLiteral("downloaded"), l.downloaded},
                             {QStringLiteral("installed"), l.packaged || l.downloaded},
                             {QStringLiteral("update"), (l.packaged || l.downloaded) && known && !current
                                                                && l.downloadSize > 0},
                             {QStringLiteral("size"), l.downloadSize}};
    }
    ++m_revision;
    emit changed();
}

QStringList Languages::dictionaryFiles(const QString &code) const
{
    const Language *l = find(code);
    if (!l)
        return {};
    QStringList current, others;
    if (l->downloaded)
        (l->downloadHash == l->releaseHash && !l->releaseHash.isEmpty() ? current : others)
                << downloadDir() + QLatin1Char('/') + code + QStringLiteral(".pfd");
    if (l->packaged)
        (l->packageHash == l->releaseHash && !l->releaseHash.isEmpty() ? current : others) << packagePath(code);
    return current + others;
}

bool Languages::updatesAvailable() const
{
    for (const QVariant &v : m_all)
        if (v.toMap().value(QStringLiteral("update")).toBool())
            return true;
    return false;
}

QStringList Languages::typing(const QString &value, const QString &script) const
{
    const QStringList codes = installedOf(selected(value, script));
    return codes.isEmpty() ? installedOf(defaultLanguages(script)) : codes;
}

QStringList Languages::scriptsOn(const QVariant &layouts) const
{
    static const QHash<QString, QString> scriptOf{
        {QStringLiteral("parrotfish.qml"), QStringLiteral("Latn")},
        {QStringLiteral("parrotfish_qwertz.qml"), QStringLiteral("Latn")},
        {QStringLiteral("parrotfish_azerty.qml"), QStringLiteral("Latn")},
        {QStringLiteral("parrotfish_jcuken.qml"), QStringLiteral("Cyrl")},
        {QStringLiteral("parrotfish_greek.qml"), QStringLiteral("Grek")}};
    QStringList scripts;
    for (const QString &layout : layouts.toStringList()) {
        const QString script = scriptOf.value(layout);
        if (!script.isEmpty() && !scripts.contains(script))
            scripts << script;
    }
    if (scripts.isEmpty())
        for (const Language &l : m_languages)
            if (!scripts.contains(l.script))
                scripts << l.script;
    return scripts;
}

QStringList Languages::typingAll(const QString &value, const QVariant &layouts) const
{
    QStringList codes;
    const QStringList scripts = scriptsOn(layouts);
    for (const QString &script : scripts)
        codes += typing(value, script);
    QStringList out;
    for (const Language &l : m_languages)
        if (codes.contains(l.code))
            out << l.code;
    return out;
}

QStringList Languages::installedOf(const QStringList &codes) const
{
    QStringList out;
    for (const QString &code : codes)
        if (isInstalled(code))
            out << code;
    return out;
}

bool Languages::isInstalled(const QString &code) const
{
    const Language *l = find(code);
    return l && (l->packaged || l->downloaded);
}

QString Languages::systemLanguage() const
{
    // The locale itself (pt_BR), its language (it_IT: it), or the language
    // it stands for (no: nb).
    if (find(m_locale))
        return m_locale;
    const QString lang = m_locale.section(QLatin1Char('_'), 0, 0);
    if (find(lang))
        return lang;
    for (const Language &l : m_languages)
        if (l.systemLanguages.contains(lang))
            return l.code;
    return QString();
}

const Languages::Language *Languages::find(const QString &code) const
{
    for (const Language &l : m_languages)
        if (l.code == code)
            return &l;
    return nullptr;
}

void Languages::setSystemLocale(const QString &name)
{
    m_locale = name;
    updateAll();
}

QStringList Languages::selected(const QString &value, const QString &script) const
{
    const QStringList parts = value.split(QLatin1Char(' '));   // (Qt 5.6: no Qt::SkipEmptyParts)
    QStringList codes;
    for (const Language &l : m_languages)
        if (l.script == script && parts.contains(l.code))
            codes << l.code;
    return codes.isEmpty() ? defaultLanguages(script) : codes;
}

QStringList Languages::selectedAll(const QString &value) const
{
    QStringList scripts, codes;
    for (const Language &l : m_languages)
        if (!scripts.contains(l.script))
            scripts << l.script;
    for (const QString &script : scripts)
        codes += selected(value, script);
    QStringList out;
    for (const Language &l : m_languages)
        if (codes.contains(l.code))
            out << l.code;
    return out;
}

QStringList Languages::defaultLanguages(const QString &script) const
{
    const QString system = systemLanguage();
    const bool systemHere = !system.isEmpty() && find(system) && find(system)->script == script;
    if (script != QLatin1String("Latn")) {
        // All installed ones, the system language first: the layout's
        // switch key picks among them.
        QStringList out;
        if (systemHere && isInstalled(system))
            out << system;
        for (const Language &l : m_languages)
            if (l.script == script && l.code != system && isInstalled(l.code))
                out << l.code;
        return out;
    }
    if (systemHere && isInstalled(system))
        return {system};
    for (const Language &l : m_languages)
        if (l.fallback && l.script == script && isInstalled(l.code))
            return {l.code};
    for (const Language &l : m_languages)
        if (l.script == script && isInstalled(l.code))
            return {l.code};
    if (systemHere)
        return {system};
    for (const Language &l : m_languages)
        if (l.fallback)
            return {l.code};
    return m_languages.isEmpty() ? QStringList() : QStringList{m_languages.first().code};
}

QString Languages::script(const QString &code) const
{
    const Language *l = find(code);
    return l ? l->script : QStringLiteral("Latn");
}

QString Languages::names(const QStringList &codes) const
{
    QStringList out;
    for (const Language &l : m_languages)
        if (codes.contains(l.code))
            out << l.name;
    return out.join(QStringLiteral(", "));
}

QString Languages::toggled(const QStringList &codes, const QString &code) const
{
    QStringList out;
    for (const Language &l : m_languages)
        if (codes.contains(l.code) != (l.code == code))
            out << l.code;
    return out.join(QLatin1Char(' '));
}

QString Languages::menuLetters(const QStringList &codes) const
{
    QString out;
    for (const QString &code : codes)
        if (const Language *l = find(code))
            for (const QChar c : l->menuLetters)
                if (!out.contains(c))
                    out += c;
    return out;
}

QString Languages::extraKeys(const QString &code) const
{
    const Language *l = find(code);
    return l ? l->extraKeys : QString();
}

QVariantMap Languages::punctuation(const QStringList &codes) const
{
    QString comma = QStringLiteral(":,;"), before, after;
    const auto add = [](QString &to, const QString &signs) {
        for (const QChar c : signs)
            if (!to.contains(c))
                to += c;
    };
    for (const QString &code : codes)
        if (const Language *l = find(code)) {
            add(comma, l->commaExtra);
            add(before, l->periodBefore);
            add(after, l->periodAfter);
        }
    return {{QStringLiteral("comma"), comma}, {QStringLiteral("period"), before + QStringLiteral("!.?") + after}};
}
