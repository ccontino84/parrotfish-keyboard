// SPDX-FileCopyrightText: 2026 ccontino84
// SPDX-License-Identifier: LGPL-2.1-or-later

// GLib's headers first: Qt's "signals" macro breaks them. (dconf.h
// declares itself C; an extern "C" around it breaks GLib's C++ templates.)
#include <dconf.h>

#include "keyboardinfo.h"
#include "learningstore.h"

#include <QDir>
#include <QFile>
#include <QRegularExpression>
#include <QtDebug>

namespace {

const char *const Path = "/apps/parrotfish-keyboard/";

// The value of a dconf key as text ("" if unset): strings without quotes.
QString readKey(DConfClient *client, const QString &key)
{
    GVariant *v = dconf_client_read(client, (QLatin1String(Path) + key).toUtf8().constData());
    if (!v)
        return QString();
    QString out;
    if (g_variant_is_of_type(v, G_VARIANT_TYPE_STRING)) {
        out = QString::fromUtf8(g_variant_get_string(v, nullptr));
    } else {
        gchar *printed = g_variant_print(v, FALSE);
        out = QString::fromUtf8(printed);
        g_free(printed);
    }
    g_variant_unref(v);
    return out;
}

QString systemName()
{
    QFile f(QStringLiteral("/etc/os-release"));
    if (!f.open(QIODevice::ReadOnly))
        return QStringLiteral("unknown");
    const QRegularExpression re(QStringLiteral("^PRETTY_NAME=\"?([^\"\\n]*)"),
                                QRegularExpression::MultilineOption);
    const QRegularExpressionMatch m = re.match(QString::fromUtf8(f.readAll()));
    return m.hasMatch() && !m.captured(1).isEmpty() ? m.captured(1) : QStringLiteral("unknown");
}

} // namespace

KeyboardInfo::KeyboardInfo(QObject *parent)
    : QObject(parent)
{
}

QString KeyboardInfo::version() const
{
    return QStringLiteral(PARROTFISH_VERSION);
}

QStringList KeyboardInfo::settingKeys()
{
    return {QStringLiteral("languages"), QStringLiteral("autocorrect"),
            QStringLiteral("learning"), QStringLiteral("number-row"), QStringLiteral("number-row-height"),
            QStringLiteral("sticky"), QStringLiteral("sticky-side"), QStringLiteral("swipe"),
            QStringLiteral("key-height"), QStringLiteral("key-height-landscape"), QStringLiteral("key-gap"),
            QStringLiteral("key-fill"), QStringLiteral("key-border"), QStringLiteral("long-press-hint"),
            QStringLiteral("dividers"), QStringLiteral("space-before-punctuation")};
}

QString KeyboardInfo::diagnostics() const
{
    const bool debug = QFile::exists(QDir::homePath() + QStringLiteral("/.local/share/parrotfish-keyboard/debug"));
    QStringList lines{QStringLiteral("Parrotfish ") + version(), QStringLiteral("system ") + systemName(),
                      QStringLiteral("debug ") + QLatin1String(debug ? "on" : "off"), QString()};
    DConfClient *client = dconf_client_new();
    // Jolla's: the layouts in its list (ours among them) and the one in use.
    for (const char *key : {"/sailfish/text_input/enabled_layouts", "/sailfish/text_input/active_layout"}) {
        GVariant *v = dconf_client_read(client, key);
        gchar *printed = v ? g_variant_print(v, FALSE) : nullptr;
        lines << QString::fromLatin1(key).section(QLatin1Char('/'), -1) + QLatin1Char(' ')
                        + (printed ? QString::fromUtf8(printed) : QStringLiteral("-"));
        g_free(printed);
        if (v)
            g_variant_unref(v);
    }
    for (const QString &key : settingKeys()) {
        const QString value = readKey(client, key);
        lines << key + QLatin1Char(' ') + (value.isEmpty() ? QStringLiteral("-") : value);
    }
    g_object_unref(client);
    lines << QString();
    lines << LearningStore(LearningStore::defaultPath()).summary();
    return lines.join(QLatin1Char('\n'));
}

void KeyboardInfo::resetSettings() const
{
    DConfClient *client = dconf_client_new();
    DConfChangeset *changes = dconf_changeset_new();
    dconf_changeset_set(changes, Path, nullptr);   // a directory: everything under it
    GError *error = nullptr;
    if (!dconf_client_change_sync(client, changes, nullptr, nullptr, &error)) {
        qWarning("parrotfish: settings not reset: %s", error ? error->message : "?");
        g_clear_error(&error);
    }
    dconf_changeset_unref(changes);
    g_object_unref(client);
}
