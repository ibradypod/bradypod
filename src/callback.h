#ifndef CALLBACK_H
#define CALLBACK_H

#include <QObject>
#include <QVariant>

class Callback : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QVariant returnValue READ returnValue WRITE setReturnValue)

public:
    Callback(QObject* parent);

    QVariant call(const QVariantList& arguments);

    QVariant returnValue() const;
    void setReturnValue(const QVariant& returnValue);

Q_SIGNALS:
    void called(const QVariantList& arguments);

private:
    QVariant m_returnValue;
};

#endif // CALLBACK_H
