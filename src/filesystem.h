#ifndef FILESYSTEM_H
#define FILESYSTEM_H

#include <QStringList>
#include <QFile>
#include <QTextCodec>
#include <QTextStream>
#include <QVariant>

class File : public QObject
{
    Q_OBJECT

public:
    // handle a textfile with given codec
    // if @p codec is null, the file is considered to be binary
    File(QFile* openfile, QTextCodec* codec, QObject* parent = 0);
    virtual ~File();

public slots:
    /**
     * @param n Number of bytes to read (a negative value means read up to EOF)
     * NOTE: The use of QVariant here is necessary to catch JavaScript `null`.
     * @see <a href="http://wiki.commonjs.org/wiki/IO/A#Instance_Methods">IO/A spec</a>
     */
    QString read(const QVariant& n = -1);
    bool write(const QString& data);

    bool seek(const qint64 pos);

    QString readLine();
    bool writeLine(const QString& data);

    bool atEnd() const;
    void flush();
    void close();

    QString getEncoding() const;
    bool setEncoding(const QString& encoding);

private:
    bool _isUnbuffered() const;

    QFile* m_file;
    QTextStream* m_fileStream;
};


class FileSystem : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString workingDirectory READ workingDirectory)
    Q_PROPERTY(QString separator READ separator)

public:
    FileSystem(QObject* parent = 0);

public slots:
    // Attributes
    // 'size(path)' implemented in "filesystem-shim.js" using '_size(path)'
    int _size(const QString& path) const;
    QVariant lastModified(const QString& path) const;

    // Directory
    // 'copyTree(source, destination)' implemented in "filesystem-shim.js" using '_copyTree(source, destination)'
    bool _copyTree(const QString& source, const QString& destination) const;
    bool makeDirectory(const QString& path) const;
    bool makeTree(const QString& path) const;
    // 'removeDirectory(path)' implemented in "filesystem-shim.js" using '_removeDirectory(path)'
    bool _removeDirectory(const QString& path) const;
    // 'removeTree(path)' implemented in "filesystem-shim.js" using '_removeTree(path)'
    bool _removeTree(const QString& path) const;

    // Files
    // 'open(path, mode|options)' implemented in "filesystem-shim.js" using '_open(path, opts)'
    QObject* _open(const QString& path, const QVariantMap& opts) const;
    // 'read(path, options)' implemented in "filesystem-shim.js"
    // 'readRaw(path, options)' implemented in "filesystem-shim.js"
    // 'write(path, mode|options)' implemented in the "filesystem-shim.js"
    // 'writeRaw(path, mode|options)' implemented in the "filesystem-shim.js"
    // 'remove(path)' implemented in "filesystem-shim.js" using '_remove(path)'
    bool _remove(const QString& path) const;
    // 'copy(source, destination)' implemented in "filesystem-shim.js" using '_copy(source, destination)'
    bool _copy(const QString& source, const QString& destination) const;
    // 'move(source, destination)' implemented in "filesystem-shim.js"
    // 'touch(path)' implemented in "filesystem-shim.js"

    // Listing
    QStringList list(const QString& path) const;

    // Paths
    QString separator() const;
    QString workingDirectory() const;
    bool changeWorkingDirectory(const QString& path) const;
    QString absolute(const QString& relativePath) const;
    // 'join(...)' implemented in "fs.js"
    // 'split(path)' implemented in "fs.js"
    QString fromNativeSeparators(const QString& path) const;
    QString toNativeSeparators(const QString& path) const;

    // Links
    QString readLink(const QString& path) const;

    // Tests
    bool exists(const QString& path) const;
    bool isDirectory(const QString& path) const;
    bool isFile(const QString& path) const;
    bool isAbsolute(const QString& path) const;
    bool isExecutable(const QString& path) const;
    bool isReadable(const QString& path) const;
    bool isWritable(const QString& path) const;
    bool isLink(const QString& path) const;
};

#endif // FILESYSTEM_H
