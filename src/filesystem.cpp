#include "filesystem.h"

#include <QDir>
#include <QDebug>
#include <QDateTime>
#include <QFile>
#include <QFileInfo>

// File
// public:
File::File(QFile* openfile, QTextCodec* codec, QObject* parent) :
    QObject(parent),
    m_file(openfile),
    m_fileStream(0)
{
    if (codec) {
        m_fileStream = new QTextStream(m_file);
        m_fileStream->setCodec(codec);
    }
}

File::~File()
{
    this->close();
}

//NOTE: for binary files we want to use QString instead of QByteArray as the
//      latter is not really useable in javascript and e.g. window.btoa expects a string
//      and we need special code required since fromAsci() would stop as soon as it
//      encounters \0 or similar

// public slots:
QString File::read(const QVariant& n)
{
    // Default to 1024 (used when n is "null")
    qint64 bytesToRead = 1024;

    // If parameter can be converted to a qint64, do so and use that value instead
    if (n.canConvert(QVariant::LongLong)) {
        bytesToRead = n.toLongLong();
    }

    const bool isReadAll = 0 > bytesToRead;

    if (!m_file->isReadable()) {
        qDebug() << "File::read - " << "Couldn't read:" << m_file->fileName();
        return QString();
    }
    if (m_file->isWritable()) {
        // make sure we write everything to disk before reading
        flush();
    }
    if (m_fileStream) {
        // text file
        QString ret;
        if (isReadAll) {
            // This code, for some reason, reads the whole file from 0 to EOF,
            // and then resets to the position the file was at prior to reading
            const qint64 pos = m_fileStream->pos();
            m_fileStream->seek(0);
            ret = m_fileStream->readAll();
            m_fileStream->seek(pos);
        } else {
            ret = m_fileStream->read(bytesToRead);
        }
        return ret;
    } else {
        // binary file
        QByteArray data;
        if (isReadAll) {
            // This code, for some reason, reads the whole file from 0 to EOF,
            // and then resets to the position the file was at prior to reading
            const qint64 pos = m_file->pos();
            m_file->seek(0);
            data = m_file->readAll();
            m_file->seek(pos);
        } else {
            data = m_file->read(bytesToRead);
        }
        QString ret(data.size(), ' ');
        for (int i = 0; i < data.size(); ++i) {
            ret[i] = data.at(i);
        }
        return ret;
    }
}

bool File::write(const QString& data)
{
    if (!m_file->isWritable()) {
        qDebug() << "File::write - " << "Couldn't write:" << m_file->fileName();
        return true;
    }
    if (m_fileStream) {
        // text file
        (*m_fileStream) << data;
        if (_isUnbuffered()) {
            m_fileStream->flush();
        }
        return true;
    } else {
        // binary file
        QByteArray bytes(data.size(), Qt::Uninitialized);
        for (int i = 0; i < data.size(); ++i) {
            bytes[i] = data.at(i).toLatin1();
        }
        return m_file->write(bytes);
    }
}

bool File::seek(const qint64 pos)
{
    if (m_fileStream) {
        return m_fileStream->seek(pos);
    } else {
        return m_file->seek(pos);
    }
}

QString File::readLine()
{
    if (!m_file->isReadable()) {
        qDebug() << "File::readLine - " << "Couldn't read:" << m_file->fileName();
        return QString();
    }
    if (m_file->isWritable()) {
        // make sure we write everything to disk before reading
        flush();
    }
    if (m_fileStream) {
        // text file
        return m_fileStream->readLine();
    } else {
        // binary file - doesn't make much sense but well...
        return QString::fromLatin1(m_file->readLine());
    }
}

bool File::writeLine(const QString& data)
{
    if (write(data) && write("\n")) {
        return true;
    }
    qDebug() << "File::writeLine - " << "Couldn't write:" << m_file->fileName();
    return false;
}

bool File::atEnd() const
{
    if (m_file->isReadable()) {
        if (m_fileStream) {
            // text file
            return m_fileStream->atEnd();
        } else {
            // binary file
            return m_file->atEnd();
        }
    }
    qDebug() << "File::atEnd - " << "Couldn't read:" << m_file->fileName();
    return false;
}

void File::flush()
{
    if (m_file) {
        if (m_fileStream) {
            // text file
            m_fileStream->flush();
        }
        // binary or text file
        m_file->flush();
    }
}

void File::close()
{
    flush();
    if (m_fileStream) {
        delete m_fileStream;
        m_fileStream = 0;
    }
    if (m_file) {
        m_file->close();
        delete m_file;
        m_file = NULL;
    }
    deleteLater();
}

bool File::setEncoding(const QString& encoding)
{
    if (encoding.isEmpty() || encoding.isNull()) {
        return false;
    }

    // "Binary" mode doesn't use/need text codecs
    if (!m_fileStream) {
        // TODO: Should we switch to "text" mode?
        return false;
    }

    // Since there can be multiple names for the same codec (i.e., "utf8" and
    // "utf-8"), we need to get the codec in the system first and use its
    // canonical name
    QTextCodec* codec = QTextCodec::codecForName(encoding.toLatin1());
    if (!codec) {
        return false;
    }

    // Check whether encoding actually needs to be changed
    const QString encodingBeforeUpdate(m_fileStream->codec()->name());
    if (0 == encodingBeforeUpdate.compare(QString(codec->name()), Qt::CaseInsensitive)) {
        return false;
    }

    m_fileStream->setCodec(codec);

    // Return whether update was successful
    const QString encodingAfterUpdate(m_fileStream->codec()->name());
    return 0 != encodingBeforeUpdate.compare(encodingAfterUpdate, Qt::CaseInsensitive);
}

QString File::getEncoding() const
{
    QString encoding;

    if (m_fileStream) {
        encoding = QString(m_fileStream->codec()->name());
    }

    return encoding;
}

// private:

bool File::_isUnbuffered() const
{
    return m_file->openMode() & QIODevice::Unbuffered;
}


// FileSystem
// public:
FileSystem::FileSystem(QObject* parent)
    : QObject(parent)
{ }

// public slots:

// Attributes
int FileSystem::_size(const QString& path) const
{
    QFileInfo fi(path);
    if (fi.exists()) {
        return fi.size();
    }
    return -1;
}

QVariant FileSystem::lastModified(const QString& path) const
{
    QFileInfo fi(path);
    if (fi.exists()) {
        return QVariant(fi.lastModified());
    }
    return QVariant(QDateTime());
}

// Links
QString FileSystem::readLink(const QString& path) const
{
    return QFileInfo(path).symLinkTarget();
}

// Tests
bool FileSystem::exists(const QString& path) const
{
    return QFile::exists(path);
}

bool FileSystem::isDirectory(const QString& path) const
{
    return QFileInfo(path).isDir();
}

bool FileSystem::isFile(const QString& path) const
{
    return QFileInfo(path).isFile();
}

bool FileSystem::isAbsolute(const QString& path) const
{
    return QFileInfo(path).isAbsolute();
}

bool FileSystem::isExecutable(const QString& path) const
{
    return QFileInfo(path).isExecutable();
}

bool FileSystem::isLink(const QString& path) const
{
    return QFileInfo(path).isSymLink();
}

bool FileSystem::isReadable(const QString& path) const
{
    return QFileInfo(path).isReadable();
}

bool FileSystem::isWritable(const QString& path) const
{
    return QFileInfo(path).isWritable();
}

// Directory
bool FileSystem::_copyTree(const QString& source, const QString& destination) const
{
    QDir sourceDir(source);
    QDir::Filters sourceDirFilter = QDir::NoDotAndDotDot | QDir::AllDirs | QDir::Files | QDir::NoSymLinks | QDir::Drives;

    if (sourceDir.exists()) {
        // Make the destination directory if it doesn't exist already
        if (!FileSystem::exists(destination) && !FileSystem::makeDirectory(destination)) {
            return false;
        }

        foreach(QFileInfo entry, sourceDir.entryInfoList(sourceDirFilter, QDir::DirsFirst)) {
            if (entry.isDir()) {
                if (!FileSystem::_copyTree(entry.absoluteFilePath(),
                                           destination + "/" + entry.fileName())) { //< directory: recursive call
                    return false;
                }
            } else {
                if (!FileSystem::_copy(entry.absoluteFilePath(),
                                       destination + "/" + entry.fileName())) { //< file: copy
                    return false;
                }
            }
        }
    }

    return true;
}

bool FileSystem::makeDirectory(const QString& path) const
{
    return QDir().mkdir(path);
}

bool FileSystem::makeTree(const QString& path) const
{
    return QDir().mkpath(path);
}

bool FileSystem::_removeDirectory(const QString& path) const
{
    return QDir().rmdir(path);
}

bool FileSystem::_removeTree(const QString& path) const
{
    QDir dir(path);
    QDir::Filters dirFilter = QDir::NoDotAndDotDot | QDir::System | QDir::Hidden | QDir::AllDirs | QDir::Files;

    if (dir.exists()) {
        foreach(QFileInfo info, dir.entryInfoList(dirFilter, QDir::DirsFirst)) {
            if (info.isDir()) {
                if (!FileSystem::_removeTree(info.absoluteFilePath())) { //< directory: recursive call
                    return false;
                }
            } else {
                if (!FileSystem::_remove(info.absoluteFilePath())) { //< file: remove
                    return false;
                }
            }
        }
        if (!FileSystem::_removeDirectory(path)) { //< delete the top tree directory
            return false;
        }
    }

    return true;
}

QStringList FileSystem::list(const QString& path) const
{
    return QDir(path).entryList();
}

// Paths
QString FileSystem::separator() const
{
    return QDir::separator();
}

QString FileSystem::workingDirectory() const
{
    return QDir::currentPath();
}

bool FileSystem::changeWorkingDirectory(const QString& path) const
{
    return QDir::setCurrent(path);
}

QString FileSystem::absolute(const QString& relativePath) const
{
    return QFileInfo(relativePath).absoluteFilePath();
}

QString FileSystem::fromNativeSeparators(const QString& path) const
{
    return QDir::fromNativeSeparators(path);
}

QString FileSystem::toNativeSeparators(const QString& path) const
{
    return QDir::toNativeSeparators(path);
}

// Files
QObject* FileSystem::_open(const QString& path, const QVariantMap& opts) const
{
    qDebug() << "FileSystem - _open:" << path << opts;

    const QVariant modeVar = opts["mode"];
    // Ensure only strings
    if (modeVar.type() != QVariant::String) {
        qDebug() << "FileSystem::open - " << "Mode must be a string!" << modeVar;
        return 0;
    }

    bool isBinary = false;
    QFile::OpenMode modeCode = QFile::NotOpen;

    // Determine the OpenMode
    foreach(const QChar & c, modeVar.toString()) {
        switch (c.toLatin1()) {
        case 'r': case 'R': {
            modeCode |= QFile::ReadOnly;
            break;
        }
        case 'a': case 'A': case '+': {
            modeCode |= QFile::Append;
            modeCode |= QFile::WriteOnly;
            break;
        }
        case 'w': case 'W': {
            modeCode |= QFile::WriteOnly;
            break;
        }
        case 'b': case 'B': {
            isBinary = true;
            break;
        }
        default: {
            qDebug() << "FileSystem::open - " << "Wrong Mode:" << c;
            return 0;
        }
        }
    }

    // Make sure the file exists OR it can be created at the required path
    if (!QFile::exists(path) && modeCode & QFile::WriteOnly) {
        if (!makeTree(QFileInfo(path).dir().absolutePath())) {
            qDebug() << "FileSystem::open - " << "Full path coulnd't be created:" << path;
            return 0;
        }
    }

    // Make sure there is something to read
    if (!QFile::exists(path) && modeCode & QFile::ReadOnly) {
        qDebug() << "FileSystem::open - " << "Trying to read a file that doesn't exist:" << path;
        return 0;
    }

    QTextCodec* codec = 0;
    if (!isBinary) {
        // default to UTF-8 encoded files
        const QString charset = opts.value("charset", "UTF-8").toString();
        codec = QTextCodec::codecForName(charset.toLatin1());
        if (!codec) {
            qDebug() << "FileSystem::open - " << "Unknown charset:" << charset;
            return 0;
        }
    }

    // Try to Open
    QFile* file = new QFile(path);
    if (!file->open(modeCode)) {
        // Return "NULL" if the file couldn't be opened as requested
        delete file;
        qDebug() << "FileSystem::open - " << "Couldn't be opened:" << path;
        return 0;
    }

    return new File(file, codec);
}

bool FileSystem::_remove(const QString& path) const
{
    return QFile::remove(path);
}

bool FileSystem::_copy(const QString& source, const QString& destination) const
{
    return QFile(source).copy(destination);
}
