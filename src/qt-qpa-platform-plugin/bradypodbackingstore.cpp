#include "bradypodbackingstore.h"
#include <qpa/qplatformscreen.h>
#include <private/qguiapplication_p.h>

QT_BEGIN_NAMESPACE

BradypodBackingStore::BradypodBackingStore(QWindow *window)
    : QPlatformBackingStore(window)
{
}

BradypodBackingStore::~BradypodBackingStore()
{
}

QPaintDevice *BradypodBackingStore::paintDevice()
{
    return &mImage;
}

void BradypodBackingStore::flush(QWindow *window, const QRegion &region, const QPoint &offset)
{
    Q_UNUSED(window);
    Q_UNUSED(region);
    Q_UNUSED(offset);
}

void BradypodBackingStore::resize(const QSize &size, const QRegion &)
{
    QImage::Format format = QGuiApplication::primaryScreen()->handle()->format();
    if (mImage.size() != size)
        mImage = QImage(size, format);
}

QT_END_NAMESPACE
