// SPDX-FileCopyrightText: 2026 ccontino84
// SPDX-License-Identifier: LGPL-2.1-or-later

// GLib's headers first: Qt's "signals" macro breaks them. (dconf.h
// declares itself C; an extern "C" around it breaks GLib's C++ templates.)
#include <dconf.h>

#include "keyboardinfo.h"
#include "languages.h"
#include "voicepaths.h"
#include "learningstore.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
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

// The installed dictionaries: p from a package, d downloaded, ! an update.
QString dictionaries()
{
    Languages::instance().refresh();
    QStringList out;
    for (const QVariant &v : Languages::instance().all()) {
        const QVariantMap l = v.toMap();
        if (l.value(QStringLiteral("installed")).toBool())
            out << l.value(QStringLiteral("code")).toString() + QLatin1Char(':')
                           + QLatin1String(l.value(QStringLiteral("packaged")).toBool() ? "p" : "")
                           + QLatin1String(l.value(QStringLiteral("downloaded")).toBool() ? "d" : "")
                           + QLatin1String(l.value(QStringLiteral("update")).toBool() ? "!" : "");
    }
    return QStringLiteral("dictionaries ") + (out.isEmpty() ? QStringLiteral("-") : out.join(QLatin1Char(' ')));
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
            QStringLiteral("numpad"),
            QStringLiteral("sticky"), QStringLiteral("sticky-side"), QStringLiteral("swipe"),
            QStringLiteral("language-keys"), QStringLiteral("language-keys-side"),
            QStringLiteral("key-height"), QStringLiteral("key-height-landscape"), QStringLiteral("key-gap"),
            QStringLiteral("key-fill"), QStringLiteral("key-border"), QStringLiteral("accent-tint"),
            QStringLiteral("long-press-hint"),
            QStringLiteral("automatic-capitals"), QStringLiteral("double-space-period"),
            QStringLiteral("space-before-punctuation"), QStringLiteral("replace-emoticons"), QStringLiteral("voice")};
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
        QString line = key + QLatin1Char(' ') + (value.isEmpty() ? QStringLiteral("-") : value);
        if (key == QLatin1String("languages")) {
            // Codes kept after their package was removed.
            Languages::instance().refresh();
            QStringList missing;
            for (const QString &code : value.split(QLatin1Char(' ')))
                if (!code.isEmpty() && !Languages::instance().isInstalled(code))
                    missing << code;
            if (!missing.isEmpty())
                line += QStringLiteral(" (no dictionary: ") + missing.join(QLatin1Char(' ')) + QLatin1Char(')');
        }
        if (key == QLatin1String("voice"))
            line += !QFileInfo(VoicePaths::helper()).isExecutable() ? QStringLiteral(", not in this build")
                  : VoicePaths::modelReady() ? QStringLiteral(", model ") + QFileInfo(VoicePaths::modelDir()).fileName()
                                             : QStringLiteral(", no model");
        lines << line;
        if (key == QLatin1String("languages"))
            lines << dictionaries();
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
