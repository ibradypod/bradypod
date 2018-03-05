#include "env.h"

#include <QCoreApplication>
#include <QString>
#include <QVariantMap>
#include <QProcessEnvironment>

static Env* env_instance = NULL;

Env* Env::instance()
{
    if (NULL == env_instance) {
        env_instance = new Env();
    }

    return env_instance;
}

Env::Env()
    : QObject(QCoreApplication::instance())
{
    const QProcessEnvironment& env = QProcessEnvironment::systemEnvironment();
    foreach(const QString & key, env.keys()) {
        m_map[key] = env.value(key);
    }
}

// public:

QVariantMap Env::asVariantMap() const
{
    return m_map;
}
