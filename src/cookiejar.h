#ifndef COOKIEJAR_H
#define COOKIEJAR_H

#include <QSettings>
#include <QNetworkCookie>
#include <QNetworkCookieJar>
#include <QVariantList>
#include <QVariantMap>

class CookieJar: public QNetworkCookieJar
{
    Q_OBJECT

    Q_PROPERTY(QVariantList cookies READ cookiesToMap WRITE addCookiesFromMap)

public:
    CookieJar(QString cookiesFile, QObject* parent = NULL);
    virtual ~CookieJar();

    bool setCookiesFromUrl(const QList<QNetworkCookie>& cookieList, const QUrl& url);
    QList<QNetworkCookie> cookiesForUrl(const QUrl& url) const;

    bool addCookie(const QNetworkCookie& cookie, const QString& url = QString());
    bool addCookies(const QList<QNetworkCookie>& cookiesList, const QString& url = QString());

    QList<QNetworkCookie> cookies(const QString& url = QString()) const;

    QNetworkCookie cookie(const QString& name, const QString& url = QString()) const;

    using QNetworkCookieJar::deleteCookie;
    bool deleteCookies(const QString& url = QString());

    void enable();
    void disable();
    bool isEnabled() const;

public slots:
    bool addCookie(const QVariantMap& cookie);
    bool addCookieFromMap(const QVariantMap& cookie, const QString& url = QString());
    bool addCookiesFromMap(const QVariantList& cookiesList, const QString& url = QString());
    QVariantList cookiesToMap(const QString& url = QString()) const;
    QVariantMap cookieToMap(const QString& name, const QString& url = QString()) const;
    bool deleteCookie(const QString& name, const QString& url = QString());
    void clearCookies();
    void close();

private slots:
    bool purgeExpiredCookies();
    bool purgeSessionCookies();
    void save();
    void load();

private:
    bool contains(const QNetworkCookie& cookie) const;

private:
    QSettings* m_cookieStorage;
    bool m_enabled;
};

#endif // COOKIEJAR_H
