#include "utils.h"
#include "consts.h"
#include "terminal.h"

#include <QFile>
#include <QDebug>
#include <QDateTime>
#include <QDir>
#include <QWebFrame>

static QString findScript(const QString& jsFilePath, const QString& libraryPath)
{
    if (!jsFilePath.isEmpty()) {
        QFile jsFile;

        // Is file in the PWD?
        jsFile.setFileName(QDir::fromNativeSeparators(jsFilePath)); //< Normalise User-provided path
        if (!jsFile.exists()) {
            // File is not in the PWD. Is it in the lookup directory?
            jsFile.setFileName(libraryPath + '/' + QDir::fromNativeSeparators(jsFilePath));
        }

        return jsFile.fileName();
    }
    return QString();
}

static QString jsFromScriptFile(const QString& scriptPath, const QString& scriptLanguage, const Encoding& enc)
{
    QFile jsFile(scriptPath);
    if (jsFile.exists() && jsFile.open(QFile::ReadOnly)) {
        QString scriptBody = enc.decode(jsFile.readAll());
        jsFile.close();

        // Remove CLI script heading
        if (scriptBody.startsWith("#!")) {
            int len = scriptBody.indexOf(QRegExp("[\r\n]"));
            if (len == -1) { len = scriptBody.length(); }
            scriptBody.remove(0, len);
        }

        // If a language is specified and is not "javascript", reject it.
        if (scriptLanguage != "javascript" && !scriptLanguage.isNull()) {
            QString errMessage = QString("Unsupported language: %1").arg(scriptLanguage);
            Terminal::instance()->cerr(errMessage);
            qWarning("%s", qPrintable(errMessage));
            return QString();
        }

        return scriptBody;
    } else {
        return QString();
    }
}

namespace Utils
{

bool printDebugMessages = false;

void messageHandler(QtMsgType type, const QMessageLogContext& context, const QString& msg)
{
    Q_UNUSED(context);
    QString now = QDateTime::currentDateTime().toString(Qt::ISODate);

    switch (type) {
    case QtInfoMsg:
        fprintf(stderr, "%s [I] %s\n", qPrintable(now), qPrintable(msg));
        break;
    case QtDebugMsg:
        if (printDebugMessages) {
            fprintf(stderr, "%s [D] %s\n", qPrintable(now), qPrintable(msg));
        }
        break;
    case QtWarningMsg:
        if (printDebugMessages) {
            fprintf(stderr, "%s [W] %s\n", qPrintable(now), qPrintable(msg));
        }
        break;
    case QtCriticalMsg:
        fprintf(stderr, "%s [C] %s\n", qPrintable(now), qPrintable(msg));
        break;
    case QtFatalMsg:
        fprintf(stderr, "%s [F] %s\n", qPrintable(now), qPrintable(msg));
        abort();
    }
}

bool injectJsInFrame(const QString& jsFilePath, const QString& libraryPath, QWebFrame* targetFrame, const bool startingScript)
{
    return injectJsInFrame(jsFilePath, QString(), Encoding::UTF8, libraryPath, targetFrame, startingScript);
}

bool injectJsInFrame(const QString& jsFilePath, const QString& jsFileLanguage, const Encoding& jsFileEnc, const QString& libraryPath, QWebFrame* targetFrame, const bool startingScript)
{
    // Don't do anything if an empty string is passed
    QString scriptPath = findScript(jsFilePath, libraryPath);
    QString scriptBody = jsFromScriptFile(scriptPath, jsFileLanguage, jsFileEnc);
    if (scriptBody.isEmpty()) {
        if (startingScript) {
            Terminal::instance()->cerr(QString("Can't open '%1'").arg(jsFilePath));
        } else {
            qWarning("Can't open '%s'", qPrintable(jsFilePath));
        }
        return false;
    }
    // Execute JS code in the context of the document
    targetFrame->evaluateJavaScript(scriptBody);
    return true;
}

bool loadJSForDebug(const QString& jsFilePath, const QString& libraryPath, QWebFrame* targetFrame, const bool autorun)
{
    return loadJSForDebug(jsFilePath, QString(), Encoding::UTF8, libraryPath, targetFrame, autorun);
}

bool loadJSForDebug(const QString& jsFilePath, const QString& jsFileLanguage, const Encoding& jsFileEnc, const QString& libraryPath, QWebFrame* targetFrame, const bool autorun)
{
    QString scriptPath = findScript(jsFilePath, libraryPath);
    QString scriptBody = jsFromScriptFile(scriptPath, jsFileLanguage, jsFileEnc);

    scriptBody = QString("function __run() {\n%1\n}").arg(scriptBody);
    targetFrame->evaluateJavaScript(scriptBody);

    if (autorun) {
        targetFrame->evaluateJavaScript("__run()");
    }

    return true;
}

QString readResourceFileUtf8(const QString& resourceFilePath)
{
    QFile f(resourceFilePath);
    f.open(QFile::ReadOnly); //< It's OK to assume this succeed. If it doesn't, we have a bigger problem.
    return QString::fromUtf8(f.readAll());
}

}; // namespace Utils
