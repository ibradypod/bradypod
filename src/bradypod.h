#ifndef BRODYPOD_H
#define BRODYPOD_H

#include <QPointer>
#include <QJsonObject>
#include <QJsonDocument>
#include <QDomDocument>
#include <QDateTime>

#include "filesystem.h"
#include "encoding.h"
#include "config.h"
#include "system.h"
#include "cookiejar.h"

class WebPage;
class HtmlLoader;
class CustomWebPage;
class WebServer;

class Bradypod : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QVariantMap defaultPageSettings READ defaultPageSettings)
    Q_PROPERTY(QString libraryPath READ libraryPath WRITE setLibraryPath)
    Q_PROPERTY(QString outputEncoding READ outputEncoding WRITE setOutputEncoding)
    Q_PROPERTY(QVariantMap version READ version)
    Q_PROPERTY(QObject* page READ page)
    Q_PROPERTY(bool cookiesEnabled READ areCookiesEnabled WRITE setCookiesEnabled)
    Q_PROPERTY(QVariantList cookies READ cookies WRITE setCookies)
    Q_PROPERTY(int remoteDebugPort READ remoteDebugPort)

private:
    // Private constructor: the Bradypod class is a singleton
    Bradypod(QObject* parent = 0);
    void init();

public:
    static Bradypod* instance();
    virtual ~Bradypod();

    QVariantMap defaultPageSettings() const;

    QString outputEncoding() const;
    void setOutputEncoding(const QString& encoding);

    bool execute();
    int returnValue() const;

    QString libraryPath() const;
    void setLibraryPath(const QString& libraryPath);

    QVariantMap version() const;

    QObject* page();

    HtmlLoader* html_loader();

    /**
     * Pointer to the Config loaded at startup.
     * The configuration is determined by the commandline parameters.
     *
     * @brief config
     * @return Pointer to the current Config(uration)
     */
    Config* config() const;

    bool printDebugMessages() const;

    bool areCookiesEnabled() const;
    void setCookiesEnabled(const bool value);

    int remoteDebugPort() const;

    QVariantMap getParsedDataStore() const;
    QJsonObject storeToJson() const;
    QDomDocument storeToXml() const;
    void addParsedData(const QVariant& data);

public slots:
    QObject* createCookieJar(const QString& filePath);
    QObject* createWebPage();
    QObject* createFilesystem();
    QObject* createSystem();
    QObject* createCallback();
    void loadModule(const QString& moduleSource, const QString& filename);
    bool injectJs(const QString& jsFilePath);

    /**
     * Allows to set cookies into the CookieJar.
     * Pages will be able to access only the cookies they are supposed to see given their URL.
     *
     * Cookies are expected in the format:
     * <pre>
     * {
     *   "name"     : "cookie name (string)",
     *   "value"    : "cookie value (string)",
     *   "domain"   : "cookie domain (string)",
     *   "path"     : "cookie path (string, optional)",
     *   "httponly" : "http only cookie (boolean, optional)",
     *   "secure"   : "secure cookie (boolean, optional)",
     *   "expires"  : "expiration date (string, GMT format, optional)"
     * }
     * </pre>
     * @brief setCookies
     * @param cookies Expects a QList of QVariantMaps
     * @return Boolean "true" if at least 1 cookie was set
     */
    bool setCookies(const QVariantList& cookies);
    bool setCookies(const QString& cookies_str);
    /**
     * All the Cookies in the CookieJar
     *
     * @see WebPage::setCookies for details on the format
     * @brief cookies
     * @return QList of QVariantMap cookies visible to this Page, at the current URL.
     */
    QVariantList cookies() const;
    /**
     * Add a Cookie (in QVariantMap or format) into the CookieJar
     * @see WebPage::setCookies for details on the format
     * @brief addCookie
     * @param cookie Cookie in QVariantMap format
     * @return Boolean "true" if cookie was added
     */
    bool addCookie(const QVariantMap& cookie);
    /**
     * Delete cookie by name from the CookieJar
     * @brief deleteCookie
     * @param cookieName Name of the Cookie to delete
     * @return Boolean "true" if cookie was deleted
     */
    bool deleteCookie(const QString& cookieName);
    /**
     * Delete All Cookies from the CookieJar
     * @brief clearCookies
     */
    void clearCookies();

    /**
     * Set the application proxy
     * @brief setProxy
     * @param ip The proxy ip
     * @param port The proxy port
     * @param proxyType The type of this proxy
     */
    void setProxy(const QString& ip, const qint64& port = 80, const QString& proxyType = "http", const QString& user = NULL, const QString& password = NULL);

    QString proxy();

    // exit() will not exit in debug mode. debugExit() will always exit.
    void exit(int code = 0);
    void debugExit(int code = 0);

    // URL utilities

    /**
     * Resolve a URL relative to a base.
     */
    QString resolveRelativeUrl(QString url, QString base);

    /**
     * Decode a URL to human-readable form.
     * @param url The URL to be decoded.
     *
     * This operation potentially destroys information.  It should only be
     * used when displaying URLs to the user, not when recording URLs for
     * later processing.  Quoting http://qt-project.org/doc/qt-5/qurl.html:
     *
     *   _Full decoding_
     *
     *   This [operation] should be used with care, since there are
     *   two conditions that cannot be reliably represented in the
     *   returned QString. They are:
     *
     *   + Non-UTF-8 sequences: URLs may contain sequences of
     *     percent-encoded characters that do not form valid UTF-8
     *     sequences. Since URLs need to be decoded using UTF-8, any
     *     decoder failure will result in the QString containing one or
     *     more replacement characters where the sequence existed.
     *
     *   + Encoded delimiters: URLs are also allowed to make a
     *     distinction between a delimiter found in its literal form and
     *     its equivalent in percent-encoded form. This is most commonly
     *     found in the query, but is permitted in most parts of the URL.
     */
    QString fullyDecodeUrl(QString url);

signals:
    void aboutToExit(int code);

private slots:
    void printConsoleMessage(const QString& msg);

    void onInitialized();

private:
    void doExit(int code);

    Encoding m_scriptFileEnc;
    WebPage* m_page;
    HtmlLoader* m_html_loader;
    bool m_terminated;
    int m_returnValue;
    QString m_script;
    QVariantMap m_defaultPageSettings;
    FileSystem* m_filesystem;
    System* m_system;
    QList<QPointer<WebPage> > m_pages;
    QList<QPointer<WebServer> > m_servers;
    QPointer<Config> m_config;
    CookieJar* m_defaultCookieJar;
    qreal m_defaultDpi;
    QVariantMap m_parsedDataStore;
    QVariantMap m_requestData;
    QDateTime m_start_time;
    QDateTime m_end_time;
    friend class CustomWebPage;
};

#endif // BRODYPOD_H
