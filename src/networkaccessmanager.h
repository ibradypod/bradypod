#ifndef NETWORKACCESSMANAGER_H
#define NETWORKACCESSMANAGER_H

#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QSslConfiguration>
#include <QTimer>
#include <QStringList>
#include <QMutex>
#include <QDateTime>

class Config;
class QAuthenticator;
class QNetworkDiskCache;
class QSslConfiguration;


class TimeoutTimer : public QTimer
{
    Q_OBJECT

public:
    TimeoutTimer(QObject* parent = 0);
    QNetworkReply* reply;
    QVariantMap data;
};


class JsNetworkRequest : public QObject
{
    Q_OBJECT

public:
    JsNetworkRequest(QNetworkRequest* request, QObject* parent = 0);
    Q_INVOKABLE void abort();
    Q_INVOKABLE void changeUrl(const QString& url);
    Q_INVOKABLE bool setHeader(const QString& name, const QVariant& value);

private:
    QNetworkRequest* m_networkRequest;
};


class NoFileAccessReply : public QNetworkReply
{
    Q_OBJECT

public:
    NoFileAccessReply(QObject* parent, const QNetworkRequest& req, const QNetworkAccessManager::Operation op);
    ~NoFileAccessReply();
    void abort() Q_DECL_OVERRIDE {}
protected:
    qint64 readData(char*, qint64) Q_DECL_OVERRIDE { return -1; }
};


class NetworkAccessManager : public QNetworkAccessManager
{
    Q_OBJECT
public:
    NetworkAccessManager(QObject* parent, Config* config);
    void setUserName(const QString& userName);
    void setPassword(const QString& password);
    void setMaxAuthAttempts(int maxAttempts);
    void setResourceTimeout(int resourceTimeout);
    void setCustomHeaders(const QVariantList& headers);
    QVariantList customHeaders() const;

    void setCookieJar(QNetworkCookieJar* cookieJar);
    QString getCookieStringFromUrl(const QUrl &url);

    QDateTime getLastAccessTime();

protected:
    Config* m_config;
    bool m_ignoreSslErrors;
    int m_authAttempts;
    int m_maxAuthAttempts;
    int m_resourceTimeout;
    QString m_userName;
    QString m_password;

    QDateTime m_last_access;

    void setLastAccessTime();

    QNetworkReply* createRequest(Operation op, const QNetworkRequest& req, QIODevice* outgoingData) Q_DECL_OVERRIDE;
    void handleFinished(QNetworkReply* reply, const QVariant& status, const QVariant& statusText);
    QVariant getResponseBodyFromReply(QNetworkReply* reply);

Q_SIGNALS:
    void resourceRequested(const QVariant& data, QObject*);
    void resourceReceived(const QVariant& data);
    void resourceError(const QVariant& data);
    void resourceTimeout(const QVariant& data);

#if QT_VERSION >= QT_VERSION_CHECK(5, 6, 0)
    void resourceRedirect(const QVariant& data);
#endif

private slots:
    void handleStarted();
    void handleFinished(QNetworkReply* reply);
    void provideAuthentication(QNetworkReply* reply, QAuthenticator* authenticator);
    void handleSslErrors(const QList<QSslError>& errors);
    void handleNetworkError(QNetworkReply::NetworkError);
    void handleTimeout();

#if QT_VERSION >= QT_VERSION_CHECK(5, 6, 0)
    void handleRedirect(const QUrl& url);
#endif

private:
    void prepareSslConfiguration(const Config* config);
    QVariantList getHeadersFromReply(const QNetworkReply* reply);
    void setRequestHeaders(QNetworkRequest* request);
    bool isBlockDomainOrIP(const QString& domain);

    QHash<QNetworkReply*, int> m_ids;
    QSet<QNetworkReply*> m_started;
    QMutex m_mutex;
    int m_idCounter;
    QNetworkDiskCache* m_networkDiskCache;
    QVariantList m_customHeaders;
    QSslConfiguration m_sslConfiguration;
    QMultiMap<QString,QString> m_dns_cache;
};

#endif // NETWORKACCESSMANAGER_H
