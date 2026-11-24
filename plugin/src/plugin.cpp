// SPDX-FileCopyrightText: 2026 ccontino84
// SPDX-License-Identifier: LGPL-2.1-or-later

#include "emojiimageprovider.h"
#include "keyboardinfo.h"
#include "languagedownloads.h"
#include "languages.h"
#include "learnedwords.h"
#include "predictionengine.h"
#include "voiceinput.h"
#include "voicemodel.h"
#include "voicewave.h"

#include <QCoreApplication>
#include <QLocale>
#include <QQmlEngine>
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
        qmlRegisterType<VoiceWave>(uri, 1, 0, "VoiceWave");
        qmlRegisterSingletonType<Languages>(uri, 1, 0, "Languages", [](QQmlEngine *, QJSEngine *) -> QObject * {
            // The engine's own, so that a rescan reaches both.
            QQmlEngine::setObjectOwnership(&Languages::instance(), QQmlEngine::CppOwnership);
            return &Languages::instance();
        });
        qmlRegisterSingletonType<VoiceInput>(uri, 1, 0, "Voice", [](QQmlEngine *, QJSEngine *) -> QObject * {
            return new VoiceInput;
        });
        // A singleton: the download goes on when its settings page closes.
        qmlRegisterSingletonType<LanguageDownloads>(uri, 1, 0, "LanguageDownloads",
                                                     [](QQmlEngine *, QJSEngine *) -> QObject * {
            return new LanguageDownloads;
        });
        qmlRegisterSingletonType<VoiceModel>(uri, 1, 0, "VoiceModel", [](QQmlEngine *, QJSEngine *) -> QObject * {
            return new VoiceModel;
        });
    }

    // Our translations, in whichever process imports us (Jolla's settings
    // load our pages without a catalogue of their own), before the pages'
    // texts are evaluated; and the colour emoji images.
    void initializeEngine(QQmlEngine *engine, const char *) override
    {
        if (engine && !engine->imageProvider(QStringLiteral("parrotfish-emoji")))
            engine->addImageProvider(QStringLiteral("parrotfish-emoji"), new EmojiImageProvider);
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
