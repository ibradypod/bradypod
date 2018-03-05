#include "consts.h"
#include "utils.h"
#include "bradypod.h"
#include "crashdump.h"

#include <QApplication>
#include <QtPlugin>
#include <QSslSocket>
#include <QWebSettings>
#include <QTextCodec>
#include <cstdio>

#include <QtPlugin>

static int inner_main(int argc, char** argv)
{
#ifdef Q_OS_WIN32
    QTextCodec::setCodecForLocale(QTextCodec::codecForName("GBK"));
#endif

    QApplication app(argc, argv);

    app.setWindowIcon(QIcon(":/bradypod-icon.png"));
    app.setApplicationName("Bradypod");
    app.setOrganizationName("bradypod");
    app.setOrganizationDomain("localhost");
    app.setApplicationVersion(BRADYPOD_VERSION_STRING);

    // Registering an alternative Message Handler
    qInstallMessageHandler(Utils::messageHandler);

#if defined(Q_OS_LINUX)
    if (QSslSocket::supportsSsl()) {
        // Don't perform on-demand loading of root certificates on Linux
        QSslSocket::addDefaultCaCertificates(QSslSocket::systemCaCertificates());
    }
#endif

    // Get the bradypod singleton
    Bradypod* bradypod = Bradypod::instance();

    // Start script execution
    if (bradypod->execute()) {
        app.exec();
    }

    // End script execution: delete the bradypod singleton and set
    // execution return value
    int retVal = bradypod->returnValue();

    bradypod->deleteLater();

#ifndef QT_NO_DEBUG
    // Clear all cached data before exiting, so it is not detected as
    // leaked.
    QWebSettings::clearMemoryCaches();
#endif

    return retVal;
}

int main(int argc, char** argv)
{
    try {
        init_crash_handler();
        return inner_main(argc, argv);

        // These last-ditch exception handlers write to the C stderr
        // because who knows what kind of state Qt is in.  And they avoid
        // using fprintf because _that_ might be in bad shape too.
        // (I would drop all the way down to write() but then I'd have to
        // write the code again for Windows.)
        //
        // print_crash_message includes a call to fflush(stderr).
    } catch (std::bad_alloc) {
        fputs("Memory exhausted.\n", stderr);
        fflush(stderr);
        return 1;

    } catch (std::exception& e) {
        fputs("Uncaught C++ exception: ", stderr);
        fputs(e.what(), stderr);
        putc('\n', stderr);
        print_crash_message();
        return 1;

    } catch (...) {
        fputs("Uncaught nonstandard exception.\n", stderr);
        print_crash_message();
        return 1;
    }
}
