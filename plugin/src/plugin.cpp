// SPDX-FileCopyrightText: 2026 ccontino84
// SPDX-License-Identifier: LicenseRef-TBD

#include "learnedwords.h"
#include "predictionengine.h"

#include <QQmlExtensionPlugin>
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
    }
};

#include "plugin.moc"
