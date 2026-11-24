// SPDX-FileCopyrightText: 2026 ccontino84
// SPDX-License-Identifier: LGPL-2.1-or-later
//
// parrotfish-download: Parrotfish's only program that uses the network,
// started by the settings.
//   parrotfish-download voice MODEL_DIR
//     The speech model, from the sherpa-onnx project's Hugging Face
//     repository at a pinned revision. MODEL_DIR/revision is written last:
//     the model is complete when it exists.
//   parrotfish-download language CODE
//     A dictionary, as the installed languages.json lists it: CODE.pfd.gz
//     and the unpacked dictionary are each checked by SHA-256, then
//     CODE.pfd replaces the old one and its record CODE.pfd.sha256 is
//     written last (Languages, in the plugin, reads both).
// A stopped download continues where it stopped (FILE.part). Writes one
// line each to stdout:
//   progress <bytes> <total> | verifying | done | error space <bytes needed>
//   | error network <text> | error checksum <file> | error write <file>
//   | error unavailable

#include <QCoreApplication>
#include <QCryptographicHash>
#include <QDir>
#include <QElapsedTimer>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QStorageInfo>
#include <QTimer>
#include <QUrl>

#include <cstdio>
#include <functional>
#include <zlib.h>

namespace {

void say(const QString &line)
{
    std::fputs(line.toUtf8().constData(), stdout);
    std::fputc('\n', stdout);
    std::fflush(stdout);
}

struct Item {
    QString name;
    QUrl url;
    qint64 size;
    QByteArray sha256;   // hex
};

// Fetches `items` into `dir`, checks them, then runs `finish` (its error
// line, or "" when done).
class Download : public QObject
{
public:
    Download(const QString &dir, const QList<Item> &items, qint64 extraSpace, std::function<QString()> finish)
        : m_dir(dir)
        , m_items(items)
        , m_extraSpace(extraSpace)
        , m_finish(finish)
    {
    }

    void start()
    {
        if (!QDir().mkpath(m_dir)) {
            fail(QStringLiteral("error write ") + m_dir);
            return;
        }
        qint64 needed = m_extraSpace;
        for (const Item &f : m_items)
            needed += f.size - have(f);
        const QStorageInfo storage(m_dir);
        if (storage.isValid() && storage.bytesAvailable() < needed) {
            fail(QStringLiteral("error space %1").arg(needed));
            return;
        }
        next();
    }

private:
    // The bytes of `f` already here, complete or in FILE.part.
    qint64 have(const Item &f) const
    {
        const QFileInfo done(path(f));
        if (done.exists() && done.size() == f.size)
            return f.size;
        const QFileInfo part(path(f) + QStringLiteral(".part"));
        return part.exists() && part.size() <= f.size ? part.size() : 0;
    }

    QString path(const Item &f) const { return m_dir + QLatin1Char('/') + f.name; }

    qint64 doneBytes() const
    {
        qint64 sum = 0;
        for (const Item &f : m_items)
            sum += have(f);
        return sum;
    }

    qint64 total() const
    {
        qint64 sum = 0;
        for (const Item &f : m_items)
            sum += f.size;
        return sum;
    }

    void next()
    {
        for (; m_index < m_items.size(); ++m_index) {
            const Item &f = m_items.at(m_index);
            if (!QFileInfo::exists(path(f)) || QFileInfo(path(f)).size() != f.size) {
                fetch(f);
                return;
            }
        }
        verify();
    }

    void fetch(const Item &f)
    {
        m_file.reset(new QFile(path(f) + QStringLiteral(".part")));
        if (!m_file->open(QIODevice::ReadWrite)) {
            fail(QStringLiteral("error write ") + m_file->fileName());
            return;
        }
        if (m_file->size() > f.size)
            m_file->resize(0);
        m_file->seek(m_file->size());
        QNetworkRequest request(f.url);
        request.setAttribute(QNetworkRequest::FollowRedirectsAttribute, true);
        request.setRawHeader("User-Agent", "parrotfish-download");
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
            // Without a partial answer (206) the file starts over.
            const int status = m_reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
            if (status != 206 && m_file->size() > 0) {
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
        const Item &f = m_items.at(m_index);
        m_file->flush();
        if (m_file->size() != f.size) {
            m_file->close();
            fail(QStringLiteral("error network incomplete ") + f.name);
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
        for (const Item &f : m_items) {
            QFile file(path(f));
            QCryptographicHash hash(QCryptographicHash::Sha256);
            if (!file.open(QIODevice::ReadOnly) || !hash.addData(&file) || hash.result().toHex() != f.sha256) {
                file.close();
                QFile::remove(path(f));   // fetched again next time
                fail(QStringLiteral("error checksum ") + f.name);
                return;
            }
        }
        const QString error = m_finish();
        if (!error.isEmpty()) {
            fail(error);
            return;
        }
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
    QList<Item> m_items;
    qint64 m_extraSpace;
    std::function<QString()> m_finish;
    QNetworkAccessManager m_network;
    QScopedPointer<QFile> m_file;
    QNetworkReply *m_reply = nullptr;
    QElapsedTimer m_progress;
    int m_index = 0;
    bool m_started = false;
    bool m_failed = false;
};

// The speech model.

const char *const Repository = "https://huggingface.co/csukuangfj/sherpa-onnx-nemo-parakeet-tdt-0.6b-ultra-int8";
const char *const Revision = "402e1190ce85b9793118ee223db21fb0d12f52db";

struct ModelFile {
    const char *name;
    qint64 size;
    const char *sha256;
};

const ModelFile ModelFiles[] = {
    {"tokens.txt", 93939, "d58544679ea4bc6ac563d1f545eb7d474bd6cfa467f0a6e2c1dc1c7d37e3c35d"},
    {"joiner.int8.onnx", 6355277, "8a71aaccfdba3d451775507a889afd2257d2e754463fd81095a8c8f290f924c2"},
    {"decoder.int8.onnx", 11845274, "1fab98fe6c12aded87d2da66272cc9e148d0a0044ce3850a12fe56302ec4a922"},
    {"encoder.int8.onnx", 652282345, "b43072e1b721f18a3df03fcfeb763ff7c4debdf29843598a3aba2f703e2780a3"},
};

Download *voice(const QString &dir)
{
    QFile::remove(dir + QStringLiteral("/revision"));
    QList<Item> items;
    for (const ModelFile &f : ModelFiles)
        items << Item{QLatin1String(f.name),
                      QUrl(QStringLiteral("%1/resolve/%2/%3")
                               .arg(QLatin1String(Repository), QLatin1String(Revision), QLatin1String(f.name))),
                      f.size, QByteArray(f.sha256)};
    return new Download(dir, items, 50 * 1024 * 1024, [dir] {
        QFile revision(dir + QStringLiteral("/revision"));
        if (!revision.open(QIODevice::WriteOnly) || revision.write(QByteArray(Revision) + '\n') < 0)
            return QStringLiteral("error write ") + revision.fileName();
        return QString();
    });
}

// A dictionary.

QString dataDir()
{
    return qEnvironmentVariableIsSet("PARROTFISH_DATA_DIR") ? QString::fromLocal8Bit(qgetenv("PARROTFISH_DATA_DIR"))
                                                             : QStringLiteral(PARROTFISH_DATA_DIR);
}

// Unpacks the gzip file `from` into `to`; the SHA-256 of what was written
// (hex), or "" if it failed.
QByteArray gunzip(const QString &from, const QString &to)
{
    QFile in(from), out(to);
    if (!in.open(QIODevice::ReadOnly) || !out.open(QIODevice::WriteOnly | QIODevice::Truncate))
        return QByteArray();
    z_stream z = {};
    if (inflateInit2(&z, 16 + MAX_WBITS) != Z_OK)   // 16: the gzip wrapper
        return QByteArray();
    QCryptographicHash hash(QCryptographicHash::Sha256);
    QByteArray buffer(1 << 16, Qt::Uninitialized);
    int status = Z_OK;
    while (status != Z_STREAM_END) {
        QByteArray chunk = in.read(1 << 16);
        if (chunk.isEmpty())
            break;
        z.next_in = reinterpret_cast<Bytef *>(chunk.data());
        z.avail_in = static_cast<uInt>(chunk.size());
        while (z.avail_in > 0 && status != Z_STREAM_END) {
            z.next_out = reinterpret_cast<Bytef *>(buffer.data());
            z.avail_out = static_cast<uInt>(buffer.size());
            status = inflate(&z, Z_NO_FLUSH);
            if (status != Z_OK && status != Z_STREAM_END) {
                inflateEnd(&z);
                return QByteArray();
            }
            const int n = buffer.size() - static_cast<int>(z.avail_out);
            hash.addData(buffer.constData(), n);
            if (out.write(buffer.constData(), n) != n) {
                inflateEnd(&z);
                return QByteArray();
            }
        }
    }
    inflateEnd(&z);
    return status == Z_STREAM_END && out.flush() ? hash.result().toHex() : QByteArray();
}

Download *language(const QString &code)
{
    QFile catalog(dataDir() + QStringLiteral("/languages.json"));
    if (!catalog.open(QIODevice::ReadOnly))
        return nullptr;
    const QJsonObject d = QJsonDocument::fromJson(catalog.readAll())
                                  .object().value(code).toObject().value(QStringLiteral("download")).toObject();
    const QUrl url(d.value(QStringLiteral("url")).toString());
    const qint64 size = static_cast<qint64>(d.value(QStringLiteral("size")).toDouble());
    const qint64 pfdSize = static_cast<qint64>(d.value(QStringLiteral("pfdSize")).toDouble());
    const QByteArray sha256 = d.value(QStringLiteral("sha256")).toString().toLatin1();
    const QByteArray pfdSha256 = d.value(QStringLiteral("pfdSha256")).toString().toLatin1();
    if (!url.isValid() || size <= 0 || sha256.size() != 64 || pfdSha256.size() != 64)
        return nullptr;
    // As Languages::downloadDir() in the plugin.
    const QString dir = QDir::homePath() + QStringLiteral("/.local/share/parrotfish-keyboard/dictionaries");
    const QString gz = dir + QLatin1Char('/') + code + QStringLiteral(".pfd.gz");
    const QString pfd = dir + QLatin1Char('/') + code + QStringLiteral(".pfd");
    return new Download(dir, {Item{code + QStringLiteral(".pfd.gz"), url, size, sha256}}, pfdSize + 5 * 1024 * 1024,
                        [gz, pfd, pfdSha256] {
        // The old dictionary stays in use until the new one is checked.
        const QString part = pfd + QStringLiteral(".part");
        const QByteArray got = gunzip(gz, part);
        QFile::remove(gz);
        if (got != pfdSha256) {
            QFile::remove(part);
            return got.isEmpty() ? QStringLiteral("error write ") + part
                                 : QStringLiteral("error checksum ") + QFileInfo(pfd).fileName();
        }
        // No record while the file is replaced: never a record of another file.
        QFile::remove(pfd + QStringLiteral(".sha256"));
        QFile::remove(pfd);
        QFile record(pfd + QStringLiteral(".sha256"));
        if (!QFile::rename(part, pfd) || !record.open(QIODevice::WriteOnly) || record.write(pfdSha256 + '\n') < 0)
            return QStringLiteral("error write ") + pfd;
        return QString();
    });
}

} // namespace

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);
    const QString mode = QString::fromLocal8Bit(argc > 1 ? argv[1] : "");
    if (argc != 3 || (mode != QLatin1String("voice") && mode != QLatin1String("language"))) {
        std::fprintf(stderr, "usage: %s voice MODEL_DIR | language CODE\n", argv[0]);
        return 2;
    }
    const QString arg = QString::fromLocal8Bit(argv[2]);
    QScopedPointer<Download> download(mode == QLatin1String("voice") ? voice(arg) : language(arg));
    if (!download) {
        say(QStringLiteral("error unavailable"));
        return 1;
    }
    QTimer::singleShot(0, download.data(), [&download] { download->start(); });
    return app.exec();
}
