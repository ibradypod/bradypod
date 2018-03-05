#ifndef UTILS_H
#define UTILS_H

#include <QtGlobal>
#include "encoding.h"

class QWebFrame;

/**
 * Aggregate common utility functions.
 */

namespace Utils
{

void messageHandler(QtMsgType type,
                    const QMessageLogContext& context,
                    const QString& msg);
extern bool printDebugMessages;

bool injectJsInFrame(const QString& jsFilePath,
                     const QString& libraryPath,
                     QWebFrame* targetFrame,
                     const bool startingScript = false);

bool injectJsInFrame(const QString& jsFilePath,
                     const QString& jsFileLanguage,
                     const Encoding& jsFileEnc,
                     const QString& libraryPath,
                     QWebFrame* targetFrame,
                     const bool startingScript = false);

bool loadJSForDebug(const QString& jsFilePath,
                    const QString& libraryPath,
                    QWebFrame* targetFrame,
                    const bool autorun = false);

bool loadJSForDebug(const QString& jsFilePath,
                    const QString& jsFileLanguage,
                    const Encoding& jsFileEnc,
                    const QString& libraryPath,
                    QWebFrame* targetFrame,
                    const bool autorun = false);

QString readResourceFileUtf8(const QString& resourceFilePath);

};

#endif // UTILS_H
