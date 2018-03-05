#ifndef SYSTEM_H
#define SYSTEM_H

#include <QApplication>
#include <QObject>
#include <QStringList>
#include <QMap>
#include <QVariant>

#include "filesystem.h"

// This class implements the CommonJS System/1.0 spec.
// See: http://wiki.commonjs.org/wiki/System/1.0
class System : public QObject
{
    Q_OBJECT
    Q_PROPERTY(qint64 pid READ pid)
    Q_PROPERTY(QStringList args READ args)
    Q_PROPERTY(QVariant env READ env)
    Q_PROPERTY(QVariant os READ os)
    Q_PROPERTY(bool isSSLSupported READ isSSLSupported)
    Q_PROPERTY(QObject* standardout READ _stdout)
    Q_PROPERTY(QObject* standarderr READ _stderr)
    Q_PROPERTY(QObject* standardin READ _stdin)

public:
    explicit System(QObject* parent = 0);
    virtual ~System();

    qint64 pid() const;

    void setArgs(const QStringList& args);
    QStringList args() const;

    QVariant env() const;

    QVariant os() const;

    bool isSSLSupported() const;

    // system.stdout
    QObject* _stdout();

    // system.stderr
    QObject* _stderr();

    // system.stdin
    QObject* _stdin();

private slots:
    void _onTerminalEncodingChanged(const QString& encoding);

private:
    File* createFileInstance(QFile* f);

    QStringList m_args;
    QVariant m_env;
    QMap<QString, QVariant> m_os;
    File* m_stdout;
    File* m_stderr;
    File* m_stdin;
};

#endif // SYSTEM_H
