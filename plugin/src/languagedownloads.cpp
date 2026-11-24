// SPDX-FileCopyrightText: 2026 ccontino84
// SPDX-License-Identifier: LGPL-2.1-or-later

// GLib's headers first: Qt's "signals" macro breaks them.
#include <dconf.h>

#include "languagedownloads.h"

#include "languages.h"

#include <QDateTime>
#include <QFile>

namespace {

const char *const LanguagesKey = "/apps/parrotfish-keyboard/languages";
const char *const ChangedKey = "/apps/parrotfish-keyboard/dictionaries-changed";

QString readString(const char *key)
{
    DConfClient *client = dconf_client_new();
    GVariant *v = dconf_client_read(client, key);
    QString out;
    if (v && g_variant_is_of_type(v, G_VARIANT_TYPE_STRING))
        out = QString::fromUtf8(g_variant_get_string(v, nullptr));
    if (v)
        g_variant_unref(v);
    g_object_unref(client);
    return out;
}

void writeString(const char *key, const QString &value)
{
    DConfClient *client = dconf_client_new();
    GError *error = nullptr;
    if (!dconf_client_write_sync(client, key, g_variant_new_string(value.toUtf8().constData()), nullptr, nullptr,
                                 &error)) {
        qWarning("parrotfish: %s not written: %s", key, error ? error->message : "?");
        g_clear_error(&error);
    }
    g_object_unref(client);
}

} // namespace

LanguageDownloads::LanguageDownloads(QObject *parent)
    : QObject(parent)
{
    connect(&m_job, &DownloadJob::changed, this, &LanguageDownloads::changed);
    connect(&m_job, &DownloadJob::finished, this, &LanguageDownloads::done);
}

QVariantMap LanguageDownloads::jobs() const
{
    QVariantMap out;
    for (auto it = m_failed.constBegin(); it != m_failed.constEnd(); ++it)
        out.insert(it.key(), QVariantMap{{QStringLiteral("state"), QStringLiteral("failed")},
                                         {QStringLiteral("error"), it.value()}});
    for (const QString &code : m_queue)
        out.insert(code, QVariantMap{{QStringLiteral("state"), QStringLiteral("queued")}});
    if (!m_current.isEmpty())
        out.insert(m_current, QVariantMap{{QStringLiteral("state"), m_job.state()},
                                          {QStringLiteral("received"), m_job.received()},
                                          {QStringLiteral("total"), m_job.total()}});
    return out;
}

void LanguageDownloads::download(const QString &code)
{
    if (code == m_current || m_queue.contains(code))
        return;
    m_failed.remove(code);
    m_queue << code;
    if (m_current.isEmpty())
        next();
    else
        emit changed();
}

void LanguageDownloads::updateAll()
{
    for (const QVariant &v : Languages::instance().all()) {
        const QVariantMap l = v.toMap();
        if (l.value(QStringLiteral("update")).toBool())
            download(l.value(QStringLiteral("code")).toString());
    }
}

void LanguageDownloads::cancel(const QString &code)
{
    m_failed.remove(code);
    m_queue.removeAll(code);
    if (code == m_current) {
        m_job.cancel();
        m_current.clear();
        next();
    }
    emit changed();
}

void LanguageDownloads::remove(const QString &code)
{
    cancel(code);
    const QStringList typed = typedLanguages();
    const QString base = Languages::downloadDir() + QLatin1Char('/') + code + QStringLiteral(".pfd");
    QFile::remove(base + QStringLiteral(".sha256"));   // first: not counted from now on
    for (const char *suffix : {"", ".part", ".gz", ".gz.part"})
        QFile::remove(base + QLatin1String(suffix));
    changedFiles(code, true, typed);
}

void LanguageDownloads::next()
{
    if (m_queue.isEmpty()) {
        emit changed();
        return;
    }
    m_current = m_queue.takeFirst();
    m_job.start({QStringLiteral("language"), m_current});
}

void LanguageDownloads::done(bool ok)
{
    const QString code = m_current;
    m_current.clear();
    if (ok)
        changedFiles(code, Languages::instance().isInstalled(code), typedLanguages());
    else
        m_failed.insert(code, m_job.error());
    next();
}

QStringList LanguageDownloads::typedLanguages() const
{
    const Languages &languages = Languages::instance();
    QStringList out;
    for (const QString &code : languages.selectedAll(readString(LanguagesKey)))
        if (languages.isInstalled(code))
            out << code;
    return out;
}

void LanguageDownloads::changedFiles(const QString &code, bool wasInstalled, const QStringList &typed)
{
    Languages &languages = Languages::instance();
    languages.refresh();
    const bool installed = languages.isInstalled(code);
    if (installed && !wasInstalled && !typed.contains(code))
        writeString(LanguagesKey, languages.toggled(typed, code));   // a new language: on
    else if (!installed && wasInstalled && typed.contains(code))
        writeString(LanguagesKey, languages.toggled(typed, code));   // gone: off
    writeString(ChangedKey, QString::number(QDateTime::currentMSecsSinceEpoch()));
    emit changed();
}
