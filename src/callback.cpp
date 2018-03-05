#include "callback.h"

#include <QDebug>

Callback::Callback(QObject* parent)
    : QObject(parent)
{
}

QVariant Callback::call(const QVariantList& arguments)
{
    emit called(arguments);

    qDebug() << "Callback - call result:" << m_returnValue;
    return m_returnValue;
}

QVariant Callback::returnValue() const
{
    return m_returnValue;
}

void Callback::setReturnValue(const QVariant& returnValue)
{
    m_returnValue = returnValue;
}
