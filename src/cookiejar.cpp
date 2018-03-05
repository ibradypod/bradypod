#include "bradypod.h"
#include "config.h"
#include "cookiejar.h"

#include <QDateTime>
#include <QDataStream>
#include <QSettings>
#include <QTimer>
#include <QDebug>

#define COOKIE_JAR_VERSION      1

// Operators needed for Cookie Serialization
QT_BEGIN_NAMESPACE
QDataStream& operator<<(QDataStream& stream, const QList<QNetworkCookie>& list)
{
    stream << COOKIE_JAR_VERSION;
    stream << quint32(list.size());
    for (int i = 0; i < list.size(); ++i) {
        stream << list.at(i).toRawForm();
    }
    return stream;
}

QDataStream& operator>>(QDataStream& stream, QList<QNetworkCookie>& list)
{
    list.clear();

    quint32 version;
    stream >> version;

    if (version != COOKIE_JAR_VERSION) {
        return stream;
    }

    quint32 count;
    stream >> count;
    for (quint32 i = 0; i < count; ++i) {
        QByteArray value;
        stream >> value;
        QList<QNetworkCookie> newCookies = QNetworkCookie::parseCookies(value);
        if (newCookies.count() == 0 && value.length() != 0) {
            qWarning() << "CookieJar: Unable to parse saved cookie:" << value;
        }
        for (int j = 0; j < newCookies.count(); ++j) {
            list.append(newCookies.at(j));
        }
        if (stream.atEnd()) {
            break;
        }
    }
    return stream;
}
QT_END_NAMESPACE

// public:
CookieJar::CookieJar(QString cookiesFile, QObject* parent)
    : QNetworkCookieJar(parent)
    , m_enabled(true)
{
    if (cookiesFile == "") {
        m_cookieStorage = 0;
    } else {
        m_cookieStorage = new QSettings(cookiesFile, QSettings::IniFormat, this);
        load();
        qDebug() << "CookieJar - Created and will store cookies in:" << cookiesFile;
    }
}

// private:
CookieJar::~CookieJar()
{
    // On destruction, before saving, clear all the session cookies
    purgeSessionCookies();
    save();
}

bool CookieJar::setCookiesFromUrl(const QList<QNetworkCookie>& cookieList, const QUrl& url)
{
    bool isCookieAdded = false;
    // Update cookies in memory
    if (isEnabled()) {
        isCookieAdded = QNetworkCookieJar::setCookiesFromUrl(cookieList, url);
        save();
    }
    // No changes occurred
    return isCookieAdded;
}

QList<QNetworkCookie> CookieJar::cookiesForUrl(const QUrl& url) const
{
    if (isEnabled()) {
        QList<QNetworkCookie> cookies = QNetworkCookieJar::cookiesForUrl(url);
        QMap<QString, int> cookie_path_stat;
        QList<QNetworkCookie> ret;
        foreach (QNetworkCookie c, cookies) {
            int len = c.path().length();
            QString name = c.name();
            if (cookie_path_stat.contains(name)) {
                int l1 = cookie_path_stat[name];
                if (l1 > len) len = l1;
            }
            cookie_path_stat[name] = len;
        }
        foreach (QNetworkCookie c, cookies) {
            int len = c.path().length();
            QString name = c.name();
            int l1 = cookie_path_stat[name];
            if (l1 == len) {
                ret.append(c);
            }
        }
        return ret;
    }
    // The CookieJar is disabled: don't return any cookie
    return QList<QNetworkCookie>();
}

bool CookieJar::addCookie(const QNetworkCookie& cookie, const QString& url)
{
    bool isCookieAdded = false;
    if (isEnabled() && (!url.isEmpty() || !cookie.domain().isEmpty())) {
        // Save a single cookie
        isCookieAdded = setCookiesFromUrl(
            QList<QNetworkCookie>() << cookie, //< unfortunately, "setCookiesFromUrl" requires a list
            !url.isEmpty() ?
            url :           //< use given URL
            QString(        //< mock-up a URL
                (cookie.isSecure() ? "https://" : "http://") +                              //< URL protocol
                QString(cookie.domain().startsWith('.') ? "www" : "") + cookie.domain() +   //< URL domain
                (cookie.path().isEmpty() ? "/" : cookie.path())));                          //< URL path

        // Return "true" if the cookie was really set
        if (contains(cookie)) {
            isCookieAdded = true;
        }
    }

    if (!isCookieAdded) {
        qDebug() << "CookieJar - Rejected Cookie" << cookie.toRawForm();
    }
    return isCookieAdded;
}

bool CookieJar::addCookie(const QVariantMap& cookie)
{
    return addCookieFromMap(cookie);
}

bool CookieJar::addCookieFromMap(const QVariantMap& cookie, const QString& url)
{
    QNetworkCookie newCookie;

    // The cookie must have a non-empty "name" and a "value"
    if (!cookie["name"].isNull() && !cookie["name"].toString().isEmpty() && !cookie["value"].isNull()) {
        // Name & Value
        newCookie.setName(cookie["name"].toByteArray());
        newCookie.setValue(cookie["value"].toByteArray());

        // Domain, if provided
        if (!cookie["domain"].isNull() && !cookie["domain"].toString().isEmpty()) {
            newCookie.setDomain(cookie["domain"].toString());
        }

        // Path, if provided
        if (!cookie["path"].isNull() || !cookie["path"].toString().isEmpty()) {
            newCookie.setPath(cookie["path"].toString());
        }

        // HttpOnly, false by default
        newCookie.setHttpOnly(cookie["httponly"].isNull() ? false : cookie["httponly"].toBool());
        // Secure, false by default
        newCookie.setSecure(cookie["secure"].isNull() ? false : cookie["secure"].toBool());

        // Expiration Date, if provided, giving priority to "expires" over "expiry"
        QVariant expiresVar;
        if (!cookie["expires"].isNull()) {
            expiresVar = cookie["expires"];
        } else if (!cookie["expiry"].isNull()) {
            expiresVar = cookie["expiry"];
        }

        if (expiresVar.isValid()) {
            QDateTime expirationDate;
            if (expiresVar.type() == QVariant::String) {
                // Set cookie expire date via "classic" string format
                QString datetime = expiresVar.toString().replace(" GMT", "");
                expirationDate = QDateTime::fromString(datetime, "ddd, dd MMM yyyy hh:mm:ss");
            } else if (expiresVar.type() == QVariant::Double) {
                // Set cookie expire date via "number of milliseconds since epoch"
                // NOTE: Every JS number is a Double.
                // @see http://www.ecma-international.org/publications/files/ECMA-ST/Ecma-262.pdf
                expirationDate = QDateTime::fromMSecsSinceEpoch(expiresVar.toLongLong());
            }

            if (expirationDate.isValid()) {
                newCookie.setExpirationDate(expirationDate);
            }
        }

        return addCookie(newCookie, url);
    }

    qDebug() << "Cookie must have name and value";
    return false;
}

bool CookieJar::addCookies(const QList<QNetworkCookie>& cookiesList, const QString& url)
{
    bool added = false;
    for (int i = cookiesList.length() - 1; i >= 0; --i) {
        if (addCookie(cookiesList.at(i), url)) {
            // change it to "true" if at least 1 cookie was set
            added = true;
        }
    }
    return added;
}

bool CookieJar::addCookiesFromMap(const QVariantList& cookiesList, const QString& url)
{
    bool added = false;
    for (int i = cookiesList.length() - 1; i >= 0; --i) {
        if (addCookieFromMap(cookiesList.at(i).toMap(), url)) {
            // change it to "true" if at least 1 cookie was set
            added = true;
        }
    }
    return added;
}

QList<QNetworkCookie> CookieJar::cookies(const QString& url) const
{
    if (url.isEmpty()) {
        // No url provided: return all the cookies in this CookieJar
        return allCookies();
    } else {
        // Return ONLY the cookies that match this URL
        return cookiesForUrl(url);
    }
}

QVariantList CookieJar::cookiesToMap(const QString& url) const
{
    QVariantList result;
    QNetworkCookie c;
    QVariantMap cookie;

    QList<QNetworkCookie> cookiesList = cookies(url);
    for (int i = cookiesList.length() - 1; i >= 0; --i) {
        c = cookiesList.at(i);

        cookie.clear();
        cookie["domain"] = QVariant(c.domain());
        cookie["name"] = QVariant(QString(c.name()));
        cookie["value"] = QVariant(QString(c.value()));
        cookie["path"] = (c.path().isNull() || c.path().isEmpty()) ? QVariant("/") : QVariant(c.path());
        cookie["httponly"] = QVariant(c.isHttpOnly());
        cookie["secure"] = QVariant(c.isSecure());
        if (c.expirationDate().isValid()) {
            cookie["expires"] = QVariant(QString(c.expirationDate().toString("ddd, dd MMM yyyy hh:mm:ss")).append(" GMT"));
            cookie["expiry"] = QVariant(c.expirationDate().toMSecsSinceEpoch() / 1000);
        }

        result.append(cookie);
    }

    return result;
}

QNetworkCookie CookieJar::cookie(const QString& name, const QString& url) const
{
    QList<QNetworkCookie> cookiesList = cookies(url);
    for (int i = cookiesList.length() - 1; i >= 0; --i) {
        if (cookiesList.at(i).name() == name) {
            return cookiesList.at(i);
        }
    }
    return QNetworkCookie();
}

QVariantMap CookieJar::cookieToMap(const QString& name, const QString& url) const
{
    QVariantMap cookie;

    QVariantList cookiesList = cookiesToMap(url);
    for (int i = cookiesList.length() - 1; i >= 0; --i) {
        cookie = cookiesList.at(i).toMap();
        if (cookie["name"].toString() == name) {
            return cookie;
        }
    }
    return QVariantMap();
}

bool CookieJar::deleteCookie(const QString& name, const QString& url)
{
    bool deleted = false;
    if (isEnabled()) {

        // NOTE: This code has been written in an "extended form" to make it
        // easy to understand. Surely this could be "shrinked", but it
        // would probably look uglier.

        QList<QNetworkCookie> cookiesListAll;

        if (url.isEmpty()) {
            if (name.isEmpty()) {           //< Neither "name" or "url" provided
                // This method has been used wrong:
                // "redirecting" to the right method for the job
                clearCookies();
            } else {                        //< Only "name" provided
                // Delete all cookies with the given name from the CookieJar
                cookiesListAll = allCookies();
                for (int i = cookiesListAll.length() - 1; i >= 0; --i) {
                    if (cookiesListAll.at(i).name() == name) {
                        // Remove this cookie
                        qDebug() << "CookieJar - Deleted" << cookiesListAll.at(i).toRawForm();
                        cookiesListAll.removeAt(i);
                        deleted = true;
                    }
                }
            }
        } else {
            // Delete cookie(s) from the ones visible to the given "url".
            // Use the "name" to delete only the right one, otherwise all of them.
            QList<QNetworkCookie> cookiesListUrl = cookies(url);
            cookiesListAll = allCookies();
            for (int i = cookiesListAll.length() - 1; i >= 0; --i) {
                if (cookiesListUrl.contains(cookiesListAll.at(i)) &&            //< if it part of the set of cookies visible at URL
                        (cookiesListAll.at(i).name() == name || name.isEmpty())) {  //< and if the name matches, or no name provided
                    // Remove this cookie
                    qDebug() << "CookieJar - Deleted" << cookiesListAll.at(i).toRawForm();
                    cookiesListAll.removeAt(i);
                    deleted = true;

                    if (!name.isEmpty()) {
                        // Only one cookie was supposed to be deleted: we are done here!
                        break;
                    }
                }
            }
        }

        // Put back the remaining cookies
        setAllCookies(cookiesListAll);
    }
    return deleted;
}

bool CookieJar::deleteCookies(const QString& url)
{
    if (isEnabled()) {
        if (url.isEmpty()) {
            // No URL provided: delete ALL the cookies in the CookieJar
            clearCookies();
            return true;
        }

        // No cookie name provided: delete all the cookies visible by this URL
        qDebug() << "Delete all cookies for URL:" << url;
        return deleteCookie("", url);
    }
    return false;
}

void CookieJar::clearCookies()
{
    if (isEnabled()) {
        setAllCookies(QList<QNetworkCookie>());
    }
}

void CookieJar::enable()
{
    m_enabled = true;
}

void CookieJar::disable()
{
    m_enabled = false;
}

bool CookieJar::isEnabled() const
{
    return m_enabled;
}

void CookieJar::close()
{
    deleteLater();
}

// private:
bool CookieJar::purgeExpiredCookies()
{
    QList<QNetworkCookie> cookiesList = allCookies();

    // If empty, there is nothing to purge
    if (cookiesList.isEmpty()) {
        return false;
    }

    // Check if any cookie has expired
    int prePurgeCookiesCount = cookiesList.count();
    QDateTime now = QDateTime::currentDateTime();
    for (int i = cookiesList.count() - 1; i >= 0; --i) {
        if (!cookiesList.at(i).isSessionCookie() && cookiesList.at(i).expirationDate() < now) {
            qDebug() << "CookieJar - Purged (expired)" << cookiesList.at(i).toRawForm();
            cookiesList.removeAt(i);
        }
    }

    // Set cookies and returns "true" if at least 1 cookie expired and has been removed
    if (prePurgeCookiesCount != cookiesList.count()) {
        setAllCookies(cookiesList);
        return true;
    }
    return false;
}

bool CookieJar::purgeSessionCookies()
{
    QList<QNetworkCookie> cookiesList = allCookies();

    // If empty, there is nothing to purge
    if (cookiesList.isEmpty()) {
        return false;
    }

    // Check if any cookie has expired
    int prePurgeCookiesCount = cookiesList.count();
    for (int i = cookiesList.count() - 1; i >= 0; --i) {
        if (cookiesList.at(i).isSessionCookie()) {
            qDebug() << "CookieJar - Purged (session)" << cookiesList.at(i).toRawForm();
            cookiesList.removeAt(i);
        }
    }

    // Set cookies and returns "true" if at least 1 session cookie was found and removed
    if (prePurgeCookiesCount != cookiesList.count()) {
        setAllCookies(cookiesList);
        return true;
    }
    return false;
}

void CookieJar::save()
{
    if (isEnabled()) {
        // Get rid of all the Cookies that have expired
        purgeExpiredCookies();

#ifndef QT_NO_DEBUG_OUTPUT
        foreach(QNetworkCookie cookie, allCookies()) {
            qDebug() << "CookieJar - Saved" << cookie.toRawForm();
        }
#endif

        // Store cookies
        if (m_cookieStorage) {
            m_cookieStorage->setValue(QLatin1String("cookies"), QVariant::fromValue<QList<QNetworkCookie> >(allCookies()));
        }
    }
}

void CookieJar::load()
{
    if (isEnabled()) {
        // Register a "StreamOperator" for this Meta Type, so we can easily serialize/deserialize the cookies
        qRegisterMetaTypeStreamOperators<QList<QNetworkCookie> >("QList<QNetworkCookie>");

        // Load all the cookies
        if (m_cookieStorage) {
            setAllCookies(qvariant_cast<QList<QNetworkCookie> >(m_cookieStorage->value(QLatin1String("cookies"))));
        }

        // If any cookie has expired since last execution, purge and save before going any further
        if (purgeExpiredCookies()) {
            save();
        }

#ifndef QT_NO_DEBUG_OUTPUT
        foreach(QNetworkCookie cookie, allCookies()) {
            qDebug() << "CookieJar - Loaded" << cookie.toRawForm();
        }
#endif
    }
}

bool CookieJar::contains(const QNetworkCookie& cookieToFind) const
{
    QList<QNetworkCookie> cookiesList = allCookies();
    foreach(QNetworkCookie cookie, cookiesList) {
        if (cookieToFind == cookie) {
            return true;
        }
    }

    return false;
}
