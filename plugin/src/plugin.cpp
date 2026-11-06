// SPDX-FileCopyrightText: 2026 ccontino84
// SPDX-License-Identifier: LGPL-2.1-or-later

#include "keyboardinfo.h"
#include "languages.h"
#include "learnedwords.h"
#include "predictionengine.h"

#include <QCoreApplication>
#include <QLocale>
#include <QQmlExtensionPlugin>
#include <QTranslator>
#include <qqml.h>

class ParrotfishKeyboardPlugin : public QQmlExtensionPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID QQmlExtensionInterface_iid)

public:
    void registerTypes(const char *uri) override
    {
        Q_ASSERT(QLatin1String(uri) == QLatin1String("Parrotfish.Keyboard"));
        qmlRegisterType<PredictionEngine>(uri, 1, 0, "PredictionEngine");
        qmlRegisterType<LearnedWords>(uri, 1, 0, "LearnedWords");
        qmlRegisterType<KeyboardInfo>(uri, 1, 0, "KeyboardInfo");
        qmlRegisterSingletonType<Languages>(uri, 1, 0, "Languages", [](QQmlEngine *, QJSEngine *) -> QObject * {
            return new Languages;
        });
    }

    // The settings' translations (translations/), in the process that
    // imports us: Jolla's settings, which load our pages without a
    // catalogue of their own, and the keyboard. Installed once, before the
    // pages' texts are evaluated; the contexts are our QML files' names.
    void initializeEngine(QQmlEngine *, const char *) override
    {
        static bool installed = false;
        if (installed)
            return;
        installed = true;
        auto *translator = new QTranslator(QCoreApplication::instance());
        if (translator->load(QLocale(), QStringLiteral("parrotfish-keyboard"), QStringLiteral("-"),
                             QStringLiteral(PARROTFISH_TRANSLATIONS_DIR)))
            QCoreApplication::installTranslator(translator);
        else
            delete translator;
    }
};

#include "plugin.moc"
