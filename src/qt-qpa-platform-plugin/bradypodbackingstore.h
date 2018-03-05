#ifndef PHANTOMBACKINGSTORE_H
#define PHANTOMBACKINGSTORE_H

#include <qpa/qplatformbackingstore.h>
#include <qpa/qplatformwindow.h>
#include <QImage>

QT_BEGIN_NAMESPACE

class BradypodBackingStore : public QPlatformBackingStore
{
public:
    BradypodBackingStore(QWindow *window);
    ~BradypodBackingStore();

    QPaintDevice *paintDevice();
    void flush(QWindow *window, const QRegion &region, const QPoint &offset);
    void resize(const QSize &size, const QRegion &staticContents);

private:
    QImage mImage;
};

QT_END_NAMESPACE

#endif // BRADYPODBACKINGSTORE_H
