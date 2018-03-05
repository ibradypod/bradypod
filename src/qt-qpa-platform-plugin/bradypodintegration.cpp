#include "bradypodintegration.h"
#include "bradypodbackingstore.h"

#include <private/qpixmap_raster_p.h>

#include <QtFontDatabaseSupport/QtFontDatabaseSupport>

#if defined(Q_OS_MAC)
# include <QtPlatformSupport/private/qcoretextfontdatabase_p.h>
#else
#include <QtFontDatabaseSupport/private/qgenericunixfontdatabase_p.h>
#endif

#include <QtEventDispatcherSupport/private/qgenericunixeventdispatcher_p.h>

#include <qpa/qplatformnativeinterface.h>
#include <qpa/qplatformscreen.h>

QT_BEGIN_NAMESPACE

class BradypodNativeInterface : public QPlatformNativeInterface
{
public:
};

class PhantomScreen : public QPlatformScreen
{
public:
    PhantomScreen()
        : mDepth(32), mFormat(QImage::Format_ARGB32_Premultiplied) {}

    QRect geometry() const { return mGeometry; }
    QSizeF physicalSize() const { return mPhysicalSize; }
    int depth() const { return mDepth; }
    QImage::Format format() const { return mFormat; }

    void setGeometry(const QRect& rect) { mGeometry = rect; }
    void setPhysicalSize(const QSizeF& physicalSize) { mPhysicalSize = physicalSize; }
    void setDepth(int depth) { mDepth = depth; }
    void setFormat(QImage::Format format) { mFormat = format; }

private:
    QRect mGeometry;
    int mDepth;
    QImage::Format mFormat;
    QSizeF mPhysicalSize;
};

BradypodIntegration::BradypodIntegration()
  : m_nativeInterface(new BradypodNativeInterface)
{
    PhantomScreen *screen = new PhantomScreen();

    // Simulate typical desktop screen
    int width = 1024;
    int height = 768;
    int dpi = 72;
    qreal physicalWidth = width * 25.4 / dpi;
    qreal physicalHeight = height * 25.4 / dpi;
    screen->setGeometry(QRect(0, 0, width, height));
    screen->setPhysicalSize(QSizeF(physicalWidth, physicalHeight));

    screen->setDepth(32);
    screen->setFormat(QImage::Format_ARGB32_Premultiplied);

    screenAdded(screen);
}

BradypodIntegration::~BradypodIntegration()
{
}

bool BradypodIntegration::hasCapability(QPlatformIntegration::Capability cap) const
{
    switch (cap) {
    case ThreadedPixmaps: return true;
    default: return QPlatformIntegration::hasCapability(cap);
    }
}

QPlatformWindow* BradypodIntegration::createPlatformWindow(QWindow* window) const
{
    return new QPlatformWindow(window);
}

QPlatformBackingStore* BradypodIntegration::createPlatformBackingStore(QWindow* window) const
{
    return new BradypodBackingStore(window);
}

QPlatformFontDatabase *BradypodIntegration::fontDatabase() const
{
    static QPlatformFontDatabase *db = 0;
    if (!db) {
#if defined(Q_OS_MAC)
        db = new QCoreTextFontDatabase();
#else
        db = new QGenericUnixFontDatabase();
#endif
    }
    return db;
}

QAbstractEventDispatcher *BradypodIntegration::createEventDispatcher() const
{
    return createUnixEventDispatcher();
}

QPlatformNativeInterface *BradypodIntegration::nativeInterface() const
{
    return m_nativeInterface.data();
}

QT_END_NAMESPACE
