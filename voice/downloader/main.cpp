// SPDX-FileCopyrightText: 2026 ccontino84
// SPDX-License-Identifier: LGPL-2.1-or-later
//
// parrotfish-voice-download MODEL_DIR: downloads the speech model into
// MODEL_DIR from the sherpa-onnx project's Hugging Face repository, at a
// pinned revision, and checks each file's SHA-256. Started by the settings
// page (VoiceModel); a stopped download continues where it stopped
// (FILE.part). Writes one line each to stdout:
//   progress <bytes> <total> | verifying | done | error space <bytes needed>
//   | error network <text> | error checksum <file> | error write <file>
// MODEL_DIR/revision is written last: the model is complete when it exists.

#include <QCoreApplication>
#include <QCryptographicHash>
#include <QDir>
#include <QElapsedTimer>
#include <QFile>
#include <QFileInfo>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QStorageInfo>
#include <QTextStream>
#include <QTimer>

#include <cstdio>

namespace {

const char *const Repository = "https://huggingface.co/csukuangfj/sherpa-onnx-nemo-parakeet-tdt-0.6b-ultra-int8";
const char *const Revision = "402e1190ce85b9793118ee223db21fb0d12f52db";

struct ModelFile {
    const char *name;
    qint64 size;
    const char *sha256;
};

const ModelFile Files[] = {
    {"tokens.txt", 93939, "d58544679ea4bc6ac563d1f545eb7d474bd6cfa467f0a6e2c1dc1c7d37e3c35d"},
    {"joiner.int8.onnx", 6355277, "8a71aaccfdba3d451775507a889afd2257d2e754463fd81095a8c8f290f924c2"},
    {"decoder.int8.onnx", 11845274, "1fab98fe6c12aded87d2da66272cc9e148d0a0044ce3850a12fe56302ec4a922"},
    {"encoder.int8.onnx", 652282345, "b43072e1b721f18a3df03fcfeb763ff7c4debdf29843598a3aba2f703e2780a3"},
};

void say(const QString &line)
{
    std::fputs(line.toUtf8().constData(), stdout);
    std::fputc('\n', stdout);
    std::fflush(stdout);
}

qint64 total()
{
    qint64 sum = 0;
    for (const ModelFile &f : Files)
        sum += f.size;
    return sum;
}

class Download : public QObject
{
public:
    explicit Download(const QString &dir)
        : m_dir(dir)
    {
    }

    void start()
    {
        if (!QDir().mkpath(m_dir)) {
            fail(QStringLiteral("error write ") + m_dir);
            return;
        }
        QFile::remove(m_dir + QStringLiteral("/revision"));
        qint64 needed = 0;
        for (const ModelFile &f : Files)
            needed += f.size - have(f);
        const QStorageInfo storage(m_dir);
        if (storage.isValid() && storage.bytesAvailable() < needed + 50 * 1024 * 1024) {
            fail(QStringLiteral("error space %1").arg(needed));
            return;
        }
        next();
    }

private:
    // The bytes of `f` already here, complete or in FILE.part.
    qint64 have(const ModelFile &f) const
    {
        const QFileInfo done(path(f));
        if (done.exists() && done.size() == f.size)
            return f.size;
        const QFileInfo part(path(f) + QStringLiteral(".part"));
        return part.exists() && part.size() <= f.size ? part.size() : 0;
    }

    QString path(const ModelFile &f) const { return m_dir + QLatin1Char('/') + QLatin1String(f.name); }

    qint64 doneBytes() const
    {
        qint64 sum = 0;
        for (const ModelFile &f : Files)
            sum += have(f);
        return sum;
    }

    void next()
    {
        for (; m_index < int(sizeof(Files) / sizeof(Files[0])); ++m_index) {
            const ModelFile &f = Files[m_index];
            if (QFileInfo(path(f)).size() != f.size || !QFileInfo::exists(path(f))) {
                fetch(f);
                return;
            }
        }
        verify();
    }

    void fetch(const ModelFile &f)
    {
        m_file.reset(new QFile(path(f) + QStringLiteral(".part")));
        if (!m_file->open(QIODevice::ReadWrite)) {
            fail(QStringLiteral("error write ") + m_file->fileName());
            return;
        }
        if (m_file->size() > f.size)
            m_file->resize(0);
        m_file->seek(m_file->size());
        QNetworkRequest request(QUrl(QStringLiteral("%1/resolve/%2/%3")
                                         .arg(QLatin1String(Repository), QLatin1String(Revision), QLatin1String(f.name))));
        request.setAttribute(QNetworkRequest::FollowRedirectsAttribute, true);
        request.setRawHeader("User-Agent", "parrotfish-voice-download");
        if (m_file->size() > 0)
            request.setRawHeader("Range", "bytes=" + QByteArray::number(m_file->size()) + '-');
        m_reply = m_network.get(request);
        connect(m_reply, &QNetworkReply::readyRead, this, [this] { write(); });
        connect(m_reply, &QNetworkReply::finished, this, [this] { finished(); });
    }

    void write()
    {
        if (!m_started) {
            m_started = true;
            // Without a partial answer the file starts over.
            const int status = m_reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
            if (status == 200 && m_file->size() > 0) {
                m_file->resize(0);
                m_file->seek(0);
            }
        }
        if (m_file->write(m_reply->readAll()) < 0) {
            m_reply->abort();
            fail(QStringLiteral("error write ") + m_file->fileName());
            return;
        }
        if (!m_progress.isValid() || m_progress.elapsed() > 250) {
            m_progress.start();
            say(QStringLiteral("progress %1 %2").arg(doneBytes()).arg(total()));
        }
    }

    void finished()
    {
        if (!m_failed && m_reply->error() == QNetworkReply::NoError)
            write();
        QNetworkReply *reply = m_reply;
        m_reply = nullptr;
        reply->deleteLater();
        m_started = false;
        if (m_failed)
            return;
        if (reply->error() != QNetworkReply::NoError) {
            fail(QStringLiteral("error network ") + reply->errorString());
            return;
        }
        const ModelFile &f = Files[m_index];
        m_file->flush();
        if (m_file->size() != f.size) {
            m_file->close();
            fail(QStringLiteral("error network incomplete ") + QLatin1String(f.name));
            return;
        }
        m_file->close();
        QFile::remove(path(f));
        if (!m_file->rename(path(f))) {
            fail(QStringLiteral("error write ") + path(f));
            return;
        }
        say(QStringLiteral("progress %1 %2").arg(doneBytes()).arg(total()));
        ++m_index;
        next();
    }

    void verify()
    {
        say(QStringLiteral("verifying"));
        for (const ModelFile &f : Files) {
            QFile file(path(f));
            QCryptographicHash hash(QCryptographicHash::Sha256);
            if (!file.open(QIODevice::ReadOnly) || !hash.addData(&file)
                    || hash.result().toHex() != QByteArray(f.sha256)) {
                file.close();
                QFile::remove(path(f));   // fetched again next time
                fail(QStringLiteral("error checksum ") + QLatin1String(f.name));
                return;
            }
        }
        QFile revision(m_dir + QStringLiteral("/revision"));
        if (!revision.open(QIODevice::WriteOnly) || revision.write(QByteArray(Revision) + '\n') < 0) {
            fail(QStringLiteral("error write ") + revision.fileName());
            return;
        }
        revision.close();
        say(QStringLiteral("done"));
        QCoreApplication::exit(0);
    }

    void fail(const QString &line)
    {
        m_failed = true;
        say(line);
        QCoreApplication::exit(1);
    }

    QString m_dir;
    QNetworkAccessManager m_network;
    QScopedPointer<QFile> m_file;
    QNetworkReply *m_reply = nullptr;
    QElapsedTimer m_progress;
    int m_index = 0;
    bool m_started = false;
    bool m_failed = false;
};

} // namespace

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);
    if (argc < 2) {
        std::fprintf(stderr, "usage: %s MODEL_DIR\n", argv[0]);
        return 2;
    }
    Download download(QString::fromLocal8Bit(argv[1]));
    QTimer::singleShot(0, &download, [&download] { download.start(); });
    return app.exec();
}
