#include <qpa/qplatformintegrationplugin.h>
#include "bradypodintegration.h"

QT_BEGIN_NAMESPACE

class BradypodIntegrationPlugin : public QPlatformIntegrationPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID QPlatformIntegrationFactoryInterface_iid FILE "bradypod.json")
public:
    BradypodIntegration *create(const QString&, const QStringList&);
};

BradypodIntegration *BradypodIntegrationPlugin::create(const QString& system, const QStringList& paramList)
{
    Q_UNUSED(paramList)
    if (!system.compare(QLatin1String("bradypod-qpa"), Qt::CaseInsensitive))
        return new BradypodIntegration();

    return 0;
}

QT_END_NAMESPACE

#include "main.moc"
