// SPDX-FileCopyrightText: 2026 ccontino84
// SPDX-License-Identifier: LGPL-2.1-or-later
//
// Languages: the dictionaries from packages and downloads, which file is
// tried first, updates, the default language.

#include "languages.h"

#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QTemporaryDir>
#include <cstdio>

namespace {

int failures = 0;

void check(bool ok, const char *what, const QString &got = QString())
{
    if (!ok) {
        ++failures;
        std::printf("FAIL %s%s%s\n", what, got.isEmpty() ? "" : ": ", qPrintable(got));
    }
}

QString hashOf(const QByteArray &content)
{
    return QString::fromLatin1(QCryptographicHash::hash(content, QCryptographicHash::Sha256).toHex());
}

void write(const QString &path, const QByteArray &content)
{
    QFile f(path);
    if (!f.open(QIODevice::WriteOnly))
        check(false, "write", path);
    f.write(content);
}

// A dictionary file, with its record unless `record` is false.
void dictionary(const QString &dir, const char *code, const QByteArray &content, bool record = true)
{
    QDir().mkpath(dir);
    const QString path = dir + QLatin1Char('/') + QLatin1String(code) + QStringLiteral(".pfd");
    write(path, content);
    if (record)
        write(path + QStringLiteral(".sha256"), hashOf(content).toLatin1() + '\n');
}

QVariantMap entry(const Languages &l, const char *code)
{
    for (const QVariant &v : l.all())
        if (v.toMap().value(QStringLiteral("code")).toString() == QLatin1String(code))
            return v.toMap();
    return {};
}

QString states(const Languages &l, const char *code)
{
    const QVariantMap e = entry(l, code);
    QStringList out;
    for (const char *key : {"packaged", "downloaded", "installed", "update"})
        if (e.value(QLatin1String(key)).toBool())
            out << QLatin1String(key);
    return out.join(QLatin1Char(' '));
}

const QByteArray Current = "this release's en";
const QByteArray Old = "an older en";

} // namespace

int main()
{
    QTemporaryDir tmp;
    const QString data = tmp.path() + QStringLiteral("/data");
    const QString home = tmp.path() + QStringLiteral("/home");
    QDir().mkpath(data);
    qputenv("PARROTFISH_DATA_DIR", data.toLocal8Bit());
    qputenv("HOME", home.toLocal8Bit());
    write(data + QStringLiteral("/languages.json"),
          "{\"en\": {\"name\": \"English\", \"fallback\": true, \"download\": {\"size\": 3000000, \"pfdSha256\": \""
                  + hashOf(Current).toLatin1() + "\"}},"
                  " \"it\": {\"name\": \"Italiano\", \"download\": {\"size\": 2000000, \"pfdSha256\": \"abc\"}},"
                  " \"fi\": {\"name\": \"Suomi\", \"experimental\": true}}");
    const QString downloads = Languages::downloadDir();
    check(downloads.startsWith(home), "downloads in the home", downloads);

    Languages l;
    l.setSystemLocale(QStringLiteral("it_IT"));
    check(l.all().size() == 3, "every language listed, none installed");
    check(states(l, "en").isEmpty(), "en not installed", states(l, "en"));
    check(entry(l, "en").value(QStringLiteral("size")).toLongLong() == 3000000, "download size");
    check(l.systemLanguage() == QLatin1String("it"), "system language", l.systemLanguage());
    check(l.defaultLanguages() == QStringList{QStringLiteral("it")}, "none installed: the system language");
    check(l.dictionaryFiles(QStringLiteral("en")).isEmpty(), "no files");

    // A package's dictionary from before 1.4: no record.
    dictionary(data, "en", Old, false);
    int revision = l.revision();
    l.refresh();
    check(l.revision() == revision + 1, "a change bumps the revision");
    l.refresh();
    check(l.revision() == revision + 1, "no change, same revision");
    check(states(l, "en") == QLatin1String("packaged installed"), "package without record", states(l, "en"));
    check(l.defaultLanguages() == QStringList{QStringLiteral("en")}, "system's not installed: the fallback");
    check(!l.updatesAvailable(), "no record: never old");

    // A download without its record is unfinished.
    dictionary(downloads, "en", Current, false);
    l.refresh();
    check(!entry(l, "en").value(QStringLiteral("downloaded")).toBool(), "no record, not downloaded");
    dictionary(downloads, "en", Current);
    l.refresh();
    check(states(l, "en") == QLatin1String("packaged downloaded installed"), "both, download current",
          states(l, "en"));
    check(l.dictionaryFiles(QStringLiteral("en"))
                  == QStringList({downloads + QStringLiteral("/en.pfd"), data + QStringLiteral("/en.pfd")}),
          "the download first", l.dictionaryFiles(QStringLiteral("en")).join(QLatin1Char(' ')));

    // A package of this release and an older download: the package first.
    dictionary(data, "en", Current);
    dictionary(downloads, "en", Old);
    l.refresh();
    check(states(l, "en") == QLatin1String("packaged downloaded installed"), "package current",
          states(l, "en"));
    check(l.dictionaryFiles(QStringLiteral("en")).first() == data + QStringLiteral("/en.pfd"),
          "this release's first", l.dictionaryFiles(QStringLiteral("en")).join(QLatin1Char(' ')));

    // Both older: an update; the download still before the package.
    dictionary(data, "en", Old);
    l.refresh();
    check(states(l, "en") == QLatin1String("packaged downloaded installed update"), "both older",
          states(l, "en"));
    check(entry(l, "en").value(QStringLiteral("source")).toString() == QLatin1String("download"), "source download");
    check(l.dictionaryFiles(QStringLiteral("en")).first() == downloads + QStringLiteral("/en.pfd"),
          "download overrides", l.dictionaryFiles(QStringLiteral("en")).join(QLatin1Char(' ')));

    // Downloaded only; no download for fi: never an update.
    dictionary(downloads, "it", "italiano");
    dictionary(data, "fi", "suomi");
    l.refresh();
    check(l.updatesAvailable(), "updatesAvailable");
    check(l.installedOf({QStringLiteral("fi"), QStringLiteral("xx"), QStringLiteral("it")})
                  == QStringList({QStringLiteral("fi"), QStringLiteral("it")}),
          "installedOf");
    check(states(l, "it") == QLatin1String("downloaded installed update"), "it downloaded, older",
          states(l, "it"));
    check(states(l, "fi") == QLatin1String("packaged installed"), "fi: nothing to download", states(l, "fi"));
    check(l.defaultLanguages() == QStringList{QStringLiteral("it")}, "the system language, downloaded");
    check(l.isInstalled(QStringLiteral("fi")) && !l.isInstalled(QStringLiteral("xx")), "isInstalled");

    // Deleted: back to the package's.
    QFile::remove(downloads + QStringLiteral("/en.pfd"));
    QFile::remove(downloads + QStringLiteral("/en.pfd.sha256"));
    l.refresh();
    check(entry(l, "en").value(QStringLiteral("source")).toString() == QLatin1String("package"), "source package");
    // Typed languages all gone: the keyboard's fallback (en), not none.
    check(l.typing(QStringLiteral("sv da")) == QStringList{QStringLiteral("it")}, "typing: the default",
          l.typing(QStringLiteral("sv da")).join(QLatin1Char(' ')));
    check(l.typing(QStringLiteral("fi en")) == QStringList({QStringLiteral("en"), QStringLiteral("fi")}),
          "typing: installed ones", l.typing(QStringLiteral("fi en")).join(QLatin1Char(' ')));
    check(l.dictionaryFiles(QStringLiteral("en")) == QStringList{data + QStringLiteral("/en.pfd")},
          "deleted download", l.dictionaryFiles(QStringLiteral("en")).join(QLatin1Char(' ')));

    std::printf(failures ? "%d failures\n" : "all passed\n", failures);
    return failures ? 1 : 0;
}
