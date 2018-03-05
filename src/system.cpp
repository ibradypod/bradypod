#include "system.h"

#include <QSslSocket>
#include <QSysInfo>
#include <QVariantMap>
#include <QTextCodec>

#include "env.h"
#include "terminal.h"

#if defined(Q_OS_LINUX) || defined(Q_OS_MAC)
#include <sys/utsname.h>
QString getOSRelease()
{
    QString release;
    struct utsname un;
    if (uname(&un) != -1) {
        release = QString::fromLatin1(un.release);
    }

    return release;
}
#endif

System::System(QObject* parent) :
    QObject(parent)
    , m_stdout((File*)NULL)
    , m_stderr((File*)NULL)
    , m_stdin((File*)NULL)
{
    // Populate "env"
    m_env = Env::instance()->asVariantMap();

    // Populate "os"
    // "osarchitecture" word size
    m_os.insert("architecture", QString("%1bit").arg(QSysInfo::WordSize));

    // "os.name" and "os.version"
#if defined(Q_OS_WIN)
    m_os.insert("name", "windows");
    switch (QSysInfo::WindowsVersion) {
    case QSysInfo::WV_32s:
        m_os.insert("version", "3.1");
        break;
    case QSysInfo::WV_95:
        m_os.insert("version", "95");
        break;
    case QSysInfo::WV_98:
        m_os.insert("version", "98");
        break;
    case QSysInfo::WV_Me:
        m_os.insert("version", "Me");
        break;
    case QSysInfo::WV_NT:
        m_os.insert("version", "NT");
        break;
    case QSysInfo::WV_2000:
        m_os.insert("version", "2000");
        break;
    case QSysInfo::WV_XP:
        m_os.insert("version", "XP");
        break;
    case QSysInfo::WV_2003:
        m_os.insert("version", "2003");
        break;
    case QSysInfo::WV_VISTA:
        m_os.insert("version", "Vista");
        break;
    case QSysInfo::WV_WINDOWS7:
        m_os.insert("version", "7");
        break;
    case QSysInfo::WV_WINDOWS8:
        m_os.insert("version", "8");
        break;
    case QSysInfo::WV_WINDOWS8_1:
        m_os.insert("version", "8.1");
        break;
    case QSysInfo::WV_WINDOWS10:
        m_os.insert("version", "10");
        break;
    default:
        m_os.insert("version", "unknown");
        break;
    }
#elif defined(Q_OS_MAC)
    m_os.insert("name", "mac");

    QString osRelease = getOSRelease();
    m_os.insert("release", osRelease);

    switch (QSysInfo::MacintoshVersion) {
    case QSysInfo::MV_10_3:
        m_os.insert("version", "10.3 (Panther)");
        break;
    case QSysInfo::MV_10_4:
        m_os.insert("version", "10.4 (Tiger)");
        break;
    case QSysInfo::MV_10_5:
        m_os.insert("version", "10.5 (Leopard)");
        break;
    case QSysInfo::MV_10_6:
        m_os.insert("version", "10.6 (Snow Leopard)");
        break;
    case QSysInfo::MV_10_7:
        m_os.insert("version", "10.7 (Lion)");
        break;
    case QSysInfo::MV_10_8:
        m_os.insert("version", "10.8 (Mountain Lion)");
        break;
    case QSysInfo::MV_10_9:
        m_os.insert("version", "10.9 (Mavericks)");
        break;
    case QSysInfo::MV_10_10:
        m_os.insert("version", "10.10 (Yosemite)");
        break;
    case QSysInfo::MV_10_11:
        m_os.insert("version", "10.11 (El Capitan)");
        break;
    default:
        m_os.insert("version", "unknown");
        break;
    }
#elif defined(Q_OS_LINUX)
    m_os.insert("name", "linux");
    m_os.insert("version", "unknown");
    m_os.insert("release", getOSRelease());
#else
    m_os.insert("name", "unknown");
    m_os.insert("version", "unknown");
#endif

    connect(Terminal::instance(), SIGNAL(encodingChanged(QString)), this, SLOT(_onTerminalEncodingChanged(QString)));
}

System::~System()
{
    // Clean-up standard streams
    if ((File*)NULL != m_stdout) {
        delete m_stdout;
        m_stdout = (File*)NULL;
    }
    if ((File*)NULL != m_stderr) {
        delete m_stderr;
        m_stderr = (File*)NULL;
    }
    if ((File*)NULL != m_stdin) {
        delete m_stdin;
        m_stdin = (File*)NULL;
    }
}

qint64 System::pid() const
{
    return QApplication::applicationPid();
}

void System::setArgs(const QStringList& args)
{
    m_args = args;
}

QStringList System::args() const
{
    return m_args;
}

QVariant System::env() const
{
    return m_env;
}

QVariant System::os() const
{
    return m_os;
}

bool System::isSSLSupported() const
{
    return QSslSocket::supportsSsl();
}

QObject* System::_stdout()
{
    if ((File*)NULL == m_stdout) {
        QFile* f = new QFile();
        f->open(stdout, QIODevice::WriteOnly | QIODevice::Unbuffered);
        m_stdout = createFileInstance(f);
    }

    return m_stdout;
}

QObject* System::_stderr()
{
    if ((File*)NULL == m_stderr) {
        QFile* f = new QFile();
        f->open(stderr, QIODevice::WriteOnly | QIODevice::Unbuffered);
        m_stderr = createFileInstance(f);
    }

    return m_stderr;
}

QObject* System::_stdin()
{
    if ((File*)NULL == m_stdin) {
        QFile* f = new QFile();
        f->open(stdin, QIODevice::ReadOnly | QIODevice::Unbuffered);
        m_stdin = createFileInstance(f);
    }

    return m_stdin;
}

// private slots:

void System::_onTerminalEncodingChanged(const QString& encoding)
{
    if ((File*)NULL != m_stdin) {
        m_stdin->setEncoding(encoding);
    }

    if ((File*)NULL != m_stdout) {
        m_stdout->setEncoding(encoding);
    }

    if ((File*)NULL != m_stderr) {
        m_stderr->setEncoding(encoding);
    }
}

// private:

File* System::createFileInstance(QFile* f)
{
    // Get the Encoding used by the Terminal at this point in time
    Encoding e(Terminal::instance()->getEncoding());
    QTextCodec* codec = e.getCodec();
    return new File(f, codec, this);
}
