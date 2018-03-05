#ifndef ENV_H
#define ENV_H

#include <QObject>
#include <QVariantMap>

class Env : public QObject
{
    Q_OBJECT

public:
    static Env* instance();

    QVariantMap asVariantMap() const;

private:
    Env();

    QVariantMap m_map;
};

#endif // ENV_H
