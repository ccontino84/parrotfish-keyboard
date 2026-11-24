// SPDX-FileCopyrightText: 2026 ccontino84
// SPDX-License-Identifier: LGPL-2.1-or-later

#include "languages.h"

#include <QFile>
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

} // namespace

Languages::Languages(QObject *parent)
    : QObject(parent)
    , m_locale(QLocale::system().name())
{
    const QString dir = dataDir();
    QFile f(dir + QStringLiteral("/languages.json"));
    if (!f.open(QIODevice::ReadOnly)) {
        qWarning("parrotfish: no %s", qPrintable(f.fileName()));
        return;
    }
    const QJsonObject root = QJsonDocument::fromJson(f.readAll()).object();
    for (auto it = root.constBegin(); it != root.constEnd(); ++it) {
        if (!QFile::exists(dir + QLatin1Char('/') + it.key() + QStringLiteral(".pfd")))
            continue;   // listed, not installed
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
        m_languages << l;
    }
    std::sort(m_languages.begin(), m_languages.end(), [](const Language &a, const Language &b) {
        return QString::localeAwareCompare(a.name, b.name) < 0;
    });
    for (const Language &l : m_languages)
        m_all << QVariantMap{{QStringLiteral("code"), l.code}, {QStringLiteral("name"), l.name},
                             {QStringLiteral("experimental"), l.experimental}, {QStringLiteral("script"), l.script}};
}

const Languages &Languages::instance()
{
    static const Languages languages;
    return languages;
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

QStringList Languages::defaultLanguages(const QString &script) const
{
    // The locale itself (pt_BR), its language (it_IT: it), or the language
    // it stands for (no: nb).
    const auto inScript = [&](const Language *l) { return l && l->script == script; };
    if (inScript(find(m_locale)))
        return {m_locale};
    const QString lang = m_locale.section(QLatin1Char('_'), 0, 0);
    if (inScript(find(lang)))
        return {lang};
    for (const Language &l : m_languages)
        if (l.script == script && l.systemLanguages.contains(lang))
            return {l.code};
    if (script != QLatin1String("Latn"))
        return {};   // chosen in the settings
    for (const Language &l : m_languages)
        if (l.fallback && l.script == script)
            return {l.code};
    for (const Language &l : m_languages)
        if (l.script == script)
            return {l.code};
    return {};
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
