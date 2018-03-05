#ifndef PHANTOMINTEGRATION_H
#define PHANTOMINTEGRATION_H

#include <qpa/qplatformintegration.h>
#include <qpa/qplatformnativeinterface.h>

#include <QScopedPointer>

QT_BEGIN_NAMESPACE

class QWindowSurface;
class BradypodNativeInterface;

class BradypodIntegration : public QPlatformIntegration
{
public:
    BradypodIntegration();
    ~BradypodIntegration();

    bool hasCapability(QPlatformIntegration::Capability cap) const;

    QPlatformWindow *createPlatformWindow(QWindow *window) const;
    QPlatformBackingStore *createPlatformBackingStore(QWindow *window) const;
    QAbstractEventDispatcher *createEventDispatcher() const;

    QPlatformFontDatabase *fontDatabase() const;
    QPlatformNativeInterface *nativeInterface() const;

private:
    QScopedPointer<BradypodNativeInterface> m_nativeInterface;
};

QT_END_NAMESPACE

#endif // BRADYPODINTEGRATION_H
