#include <QCoreApplication>
#include <QAuthenticator>
#include <QDateTime>
#include <QDesktopServices>
#include <QNetworkDiskCache>
#include <QNetworkRequest>
#include <QSslSocket>
#include <QSslCertificate>
#include <QSslCipher>
#include <QSslKey>
#include <QRegExp>
#include <QFile>
#include <QHostInfo>

#include "bradypod.h"
#include "config.h"
#include "cookiejar.h"
#include "networkaccessmanager.h"

#include <private/qnetworkreplyhttpimpl_p.h>

// 10 MB
const qint64 MAX_REQUEST_POST_BODY_SIZE = 10 * 1000 * 1000;

static const char* toString(QNetworkAccessManager::Operation op)
{
    const char* str = 0;
    switch (op) {
    case QNetworkAccessManager::HeadOperation:
        str = "HEAD";
        break;
    case QNetworkAccessManager::GetOperation:
        str = "GET";
        break;
    case QNetworkAccessManager::PutOperation:
        str = "PUT";
        break;
    case QNetworkAccessManager::PostOperation:
        str = "POST";
        break;
    case QNetworkAccessManager::DeleteOperation:
        str = "DELETE";
        break;
    default:
        str = "?";
        break;
    }
    return str;
}

// Stub QNetworkReply used when file:/// URLs are disabled.
// Somewhat cargo-culted from QDisabledNetworkReply.

NoFileAccessReply::NoFileAccessReply(QObject* parent, const QNetworkRequest& req, const QNetworkAccessManager::Operation op)
    : QNetworkReply(parent)
{
    setRequest(req);
    setUrl(req.url());
    setOperation(op);

    qRegisterMetaType<QNetworkReply::NetworkError>();
    QString msg = (QCoreApplication::translate("QNetworkReply", "access deny"));
    setError(OperationCanceledError, msg);

    QMetaObject::invokeMethod(this, "error", Qt::QueuedConnection,
                              Q_ARG(QNetworkReply::NetworkError, OperationCanceledError));
    QMetaObject::invokeMethod(this, "finished", Qt::QueuedConnection);
}

// The destructor must be out-of-line in order to trigger generation of the vtable.
NoFileAccessReply::~NoFileAccessReply() {}


TimeoutTimer::TimeoutTimer(QObject* parent)
    : QTimer(parent)
{
}


JsNetworkRequest::JsNetworkRequest(QNetworkRequest* request, QObject* parent)
    : QObject(parent)
{
    m_networkRequest = request;
}

void JsNetworkRequest::abort()
{
    if (m_networkRequest) {
        m_networkRequest->setUrl(QUrl());
    }
}

bool JsNetworkRequest::setHeader(const QString& name, const QVariant& value)
{
    if (!m_networkRequest) {
        return false;
    }

    // Pass `null` as the second argument to remove a HTTP header
    m_networkRequest->setRawHeader(name.toLatin1(), value.toByteArray());
    return true;
}

void JsNetworkRequest::changeUrl(const QString& address)
{
    if (m_networkRequest) {
        QUrl url = QUrl::fromEncoded(address.toLatin1());
        m_networkRequest->setUrl(url);
    }
}

struct ssl_protocol_option {
    const char* name;
    QSsl::SslProtocol proto;
};
const ssl_protocol_option ssl_protocol_options[] = {
    { "default", QSsl::SecureProtocols },
    { "tlsv1.2", QSsl::TlsV1_2 },
    { "tlsv1.1", QSsl::TlsV1_1 },
    { "tlsv1.0", QSsl::TlsV1_0 },
    { "tlsv1",   QSsl::TlsV1_0 },
    { "sslv3",   QSsl::SslV3 },
    { "any",     QSsl::AnyProtocol },
    { 0,         QSsl::UnknownProtocol }
};

// public:
NetworkAccessManager::NetworkAccessManager(QObject* parent, Config* config)
    : QNetworkAccessManager(parent)
    , m_config(config)
    , m_ignoreSslErrors(config->ignoreSslErrors())
    , m_authAttempts(0)
    , m_maxAuthAttempts(3)
    , m_resourceTimeout(30000)
    , m_idCounter(0)
    , m_networkDiskCache(0)
    , m_sslConfiguration(QSslConfiguration::defaultConfiguration())
{
    if (config->diskCacheEnabled()) {
        m_networkDiskCache = new QNetworkDiskCache(this);

        if (config->diskCachePath().isEmpty()) {
            m_networkDiskCache->setCacheDirectory(QStandardPaths::writableLocation(QStandardPaths::CacheLocation));
        } else {
            m_networkDiskCache->setCacheDirectory(config->diskCachePath());
        }

        if (config->maxDiskCacheSize() >= 0) {
            m_networkDiskCache->setMaximumCacheSize(qint64(config->maxDiskCacheSize()) * 1024);
        }
        setCache(m_networkDiskCache);
    }

    if (QSslSocket::supportsSsl()) {
        prepareSslConfiguration(config);
    }

    setLastAccessTime();

    connect(this, SIGNAL(authenticationRequired(QNetworkReply*, QAuthenticator*)), SLOT(provideAuthentication(QNetworkReply*, QAuthenticator*)));
    connect(this, SIGNAL(finished(QNetworkReply*)), SLOT(handleFinished(QNetworkReply*)));
}

void NetworkAccessManager::prepareSslConfiguration(const Config* config)
{
    m_sslConfiguration = QSslConfiguration::defaultConfiguration();

    if (config->ignoreSslErrors()) {
        m_sslConfiguration.setPeerVerifyMode(QSslSocket::VerifyNone);
    }

    bool setProtocol = false;
    for (const ssl_protocol_option* proto_opt = ssl_protocol_options;
            proto_opt->name;
            proto_opt++) {
        if (config->sslProtocol() == proto_opt->name) {
            m_sslConfiguration.setProtocol(proto_opt->proto);
            setProtocol = true;
            break;
        }
    }
    // FIXME: actually object to an invalid setting.
    if (!setProtocol) {
        m_sslConfiguration.setProtocol(QSsl::SecureProtocols);
    }

    // Essentially the same as what QSslSocket::setCiphers(QString) does.
    // That overload isn't available on QSslConfiguration.
    if (!config->sslCiphers().isEmpty()) {
        QList<QSslCipher> cipherList;
        foreach(const QString & cipherName,
                config->sslCiphers().split(QLatin1String(":"),
                                           QString::SkipEmptyParts)) {
            QSslCipher cipher(cipherName);
            if (!cipher.isNull()) {
                cipherList << cipher;
            }
        }
        if (!cipherList.isEmpty()) {
            m_sslConfiguration.setCiphers(cipherList);
        }
    }

    if (!config->sslCertificatesPath().isEmpty()) {
        QList<QSslCertificate> caCerts = QSslCertificate::fromPath(
                                             config->sslCertificatesPath(), QSsl::Pem, QRegExp::Wildcard);

        m_sslConfiguration.setCaCertificates(caCerts);
    }

    if (!config->sslClientCertificateFile().isEmpty()) {
        QList<QSslCertificate> clientCerts = QSslCertificate::fromPath(
                config->sslClientCertificateFile(), QSsl::Pem, QRegExp::Wildcard);

        if (!clientCerts.isEmpty()) {
            QSslCertificate clientCert = clientCerts.first();

            QList<QSslCertificate> caCerts = m_sslConfiguration.caCertificates();
            caCerts.append(clientCert);
            m_sslConfiguration.setCaCertificates(caCerts);
            m_sslConfiguration.setLocalCertificate(clientCert);

            QFile* keyFile = NULL;
            if (config->sslClientKeyFile().isEmpty()) {
                keyFile = new QFile(config->sslClientCertificateFile());
            } else {
                keyFile = new QFile(config->sslClientKeyFile());
            }

            if (keyFile->open(QIODevice::ReadOnly | QIODevice::Text)) {
                QSslKey key(keyFile->readAll(), QSsl::Rsa, QSsl::Pem, QSsl::PrivateKey, config->sslClientKeyPassphrase());

                m_sslConfiguration.setPrivateKey(key);
                keyFile->close();
            }
        }
    }
}

void NetworkAccessManager::setUserName(const QString& userName)
{
    m_userName = userName;
}

void NetworkAccessManager::setPassword(const QString& password)
{
    m_password = password;
}

void NetworkAccessManager::setResourceTimeout(int resourceTimeout)
{
    m_resourceTimeout = resourceTimeout;
}

void NetworkAccessManager::setMaxAuthAttempts(int maxAttempts)
{
    m_maxAuthAttempts = maxAttempts;
}

void NetworkAccessManager::setCustomHeaders(const QVariantList& headers)
{
    m_customHeaders = headers;
}

QVariantList NetworkAccessManager::customHeaders() const
{
    return m_customHeaders;
}

QDateTime NetworkAccessManager::getLastAccessTime()
{
    return m_last_access;
}

void NetworkAccessManager::setLastAccessTime()
{
    m_last_access = QDateTime::currentDateTime();
}

void NetworkAccessManager::setCookieJar(QNetworkCookieJar* cookieJar)
{
    QNetworkAccessManager::setCookieJar(cookieJar);
    // Remove NetworkAccessManager's ownership of this CookieJar
    // CookieJar is shared between multiple instances of NetworkAccessManager.
    // It shouldn't be deleted when the NetworkAccessManager is deleted, but
    // only when close is called on the cookie jar.
    cookieJar->setParent(Bradypod::instance());
}

QString NetworkAccessManager::getCookieStringFromUrl(const QUrl &url)
{
    QStringList cookies;
    foreach (QNetworkCookie cookie, cookieJar()->cookiesForUrl(url)) {
        cookies.append(cookie.toRawForm(QNetworkCookie::NameAndValueOnly));
    }
    return cookies.join("; ");
}

// protected:
QNetworkReply* NetworkAccessManager::createRequest(Operation op, const QNetworkRequest& request, QIODevice* outgoingData)
{
    QNetworkRequest req(request);
    req.setAttribute(QNetworkRequest::HttpPipeliningAllowedAttribute,true);
#if QT_VERSION >= QT_VERSION_CHECK(5, 6, 0)
    req.setAttribute(QNetworkRequest::FollowRedirectsAttribute, false);
#endif

    QUrl url = req.url();
    QString scheme = url.scheme().toLower();

    if (!QSslSocket::supportsSsl()) {
        if (scheme == QLatin1String("https")) {
            qWarning() << "Request using https scheme without SSL support";
        }
    } else {
        req.setSslConfiguration(m_sslConfiguration);
    }

    // Get the URL string before calling the superclass. Seems to work around
    // segfaults in Qt 4.8: https://gist.github.com/1430393
    QString strUrl = url.toString();
    QByteArray postData;

    if (op == PostOperation) {
        if (outgoingData) { postData = outgoingData->peek(MAX_REQUEST_POST_BODY_SIZE); }
        QString contentType = req.header(QNetworkRequest::ContentTypeHeader).toString();
        if (contentType.isEmpty()) {
            req.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");
        }
    }

    // set custom HTTP headers
    setRequestHeaders(&req);

    // FIXME: need lock?
//    m_mutex.lock();
    int idCount = ++m_idCounter;
//    m_mutex.unlock();

    QVariantList headers;
    foreach(QByteArray headerName, req.rawHeaderList()) {
        QVariantMap header;
        header["name"] = QString::fromUtf8(headerName);
        header["value"] = QString::fromUtf8(req.rawHeader(headerName));
        headers += header;
    }

    // get Cookie from cookiejar
    if (!req.hasRawHeader("Cookie")) {
    // FIXME: if (req.header(QNetworkRequest::CookieHeader).toString().isEmpty()) {
        QString cookie = getCookieStringFromUrl(req.url());
        if (!cookie.isEmpty()) {
            QVariantMap header;
            header["name"] = "Cookie";
            header["value"] = cookie;
            headers += header;
        }
    }

    QVariantMap data;
    data["id"] = idCount;
    data["url"] = strUrl;
    data["type"] = "request";
    data["method"] = toString(op);
    data["headers"] = headers;
    if (op == PostOperation) { data["postData"] = postData.data(); }
    data["time"] = QDateTime::currentDateTime().toString(Qt::ISODateWithMs);

    JsNetworkRequest jsNetworkRequest(&req, this);

    emit resourceRequested(data, &jsNetworkRequest);

    // file: URLs may be disabled.
    // The second half of this conditional must match
    // QNetworkAccessManager's own idea of what a local file URL is.
    QNetworkReply* reply;
    bool blocked = isBlockDomainOrIP(url.host());

    // reply action
    if (!m_config->localUrlAccessEnabled() &&
            (url.isLocalFile() || scheme == QLatin1String("qrc"))) {
        reply = new NoFileAccessReply(this, req, op);
    } else if (blocked) {
        reply = new NoFileAccessReply(this, req, op);
    } else if (m_config->allowNetworkAccess()) {
        if (m_config->onlyLoadFirstRequest()) {
            m_config->setAllowNetworkAccess(false);
        }
        reply = QNetworkAccessManager::createRequest(op, req, outgoingData);
    } else {
        reply = new NoFileAccessReply(this, req, op);
    }

    m_ids[reply] = idCount;

    // reparent jsNetworkRequest to make sure that it will be destroyed with QNetworkReply
    jsNetworkRequest.setParent(reply);

    // If there is a timeout set, create a TimeoutTimer
    if (m_resourceTimeout > 0) {
        TimeoutTimer* nt = new TimeoutTimer(reply);
        nt->reply = reply; // We need the reply object in order to abort it later on.
        nt->data = data;
        nt->setInterval(m_resourceTimeout);
        nt->setSingleShot(true);
        nt->start();

        connect(nt, SIGNAL(timeout()), this, SLOT(handleTimeout()));
    }

#if QT_VERSION >= QT_VERSION_CHECK(5, 6, 0)
    connect(reply, &QNetworkReply::redirected, this, &NetworkAccessManager::handleRedirect);
#endif
    connect(reply, &QNetworkReply::readyRead, this, &NetworkAccessManager::handleStarted);
    connect(reply, &QNetworkReply::sslErrors, this, &NetworkAccessManager::handleSslErrors);
    connect(reply, static_cast<void(QNetworkReply::*)(QNetworkReply::NetworkError)>(&QNetworkReply::error), this, &NetworkAccessManager::handleNetworkError);

    // synchronous requests will be finished at this point
    if (reply->isFinished()) {
        handleFinished(reply);
        return reply;
    }

    return reply;
}

QVariant NetworkAccessManager::getResponseBodyFromReply(QNetworkReply* reply)
{
    QByteArray data;
    data += reply->peek(reply->bytesAvailable());
    return QString::fromUtf8(data.data(), data.length());
}

void NetworkAccessManager::handleTimeout()
{
    TimeoutTimer* nt = qobject_cast<TimeoutTimer*>(sender());

    if (!nt->reply) {
        return;
    }

    nt->data["errorCode"] = 408;
    nt->data["type"] = "timeout";
    nt->data["errorString"] = "Network timeout on resource.";

    emit resourceTimeout(nt->data);

    // Abort the reply that we attached to the Network Timeout
    nt->reply->abort();
}

void NetworkAccessManager::handleStarted()
{
    QNetworkReply* reply = qobject_cast<QNetworkReply*>(sender());
    if (!reply) {
        return;
    }
    if (m_started.contains(reply)) {
        return;
    }

    m_started += reply;

    QVariantList headers = getHeadersFromReply(reply);

    QVariantMap data;
    data["stage"] = "start";
    data["type"] = "response";
    data["id"] = m_ids.value(reply);
    data["url"] = reply->url().toEncoded().data();
    data["status"] = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute);
    data["statusText"] = reply->attribute(QNetworkRequest::HttpReasonPhraseAttribute);
    data["contentType"] = reply->header(QNetworkRequest::ContentTypeHeader);
    data["bodySize"] = reply->size();
    data["redirectURL"] = reply->header(QNetworkRequest::LocationHeader);
    data["headers"] = headers;
    data["time"] = QDateTime::currentDateTime().toString(Qt::ISODateWithMs);
    data["body"] = getResponseBodyFromReply(reply);

    emit resourceReceived(data);
}

void NetworkAccessManager::handleFinished(QNetworkReply* reply)
{
    if (!m_ids.contains(reply)) {
        return;
    }

    QVariant status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute);
    QVariant statusText = reply->attribute(QNetworkRequest::HttpReasonPhraseAttribute);

    this->handleFinished(reply, status, statusText);
}

void NetworkAccessManager::provideAuthentication(QNetworkReply* reply, QAuthenticator* authenticator)
{
    if (m_authAttempts++ < m_maxAuthAttempts) {
        authenticator->setUser(m_userName);
        authenticator->setPassword(m_password);
    } else {
        m_authAttempts = 0;
        this->handleFinished(reply, 401, "Authorization Required");
        reply->close();
    }
}

void NetworkAccessManager::handleFinished(QNetworkReply* reply, const QVariant& status, const QVariant& statusText)
{
    QVariantList headers = getHeadersFromReply(reply);

    QVariantMap data;
    data["stage"] = "end";
    data["type"] = "finished";
    data["id"] = m_ids.value(reply);
    data["url"] = reply->url().toEncoded().data();
    data["status"] = status;
    data["statusText"] = statusText;
    data["contentType"] = reply->header(QNetworkRequest::ContentTypeHeader);
    data["redirectURL"] = reply->header(QNetworkRequest::LocationHeader);
    data["headers"] = headers;
    data["time"] = QDateTime::currentDateTime().toString(Qt::ISODateWithMs);
    data["body"] = "";

    m_ids.remove(reply);
    m_started.remove(reply);
    reply->deleteLater();

    emit resourceReceived(data);
}

void NetworkAccessManager::handleSslErrors(const QList<QSslError>& errors)
{
    QNetworkReply* reply = qobject_cast<QNetworkReply*>(sender());
    foreach(QSslError e, errors) {
        qDebug() << "Network - SSL Error:" << e;
    }

    if (m_ignoreSslErrors) {
        reply->ignoreSslErrors();
    }
}

void NetworkAccessManager::handleNetworkError(QNetworkReply::NetworkError error)
{
    (void)error;
    QNetworkReply* reply = qobject_cast<QNetworkReply*>(sender());

    qDebug() << "Network - Resource request error:"
             << error
             << "(" << reply->errorString() << ")"
             << "URL:" << reply->url().toEncoded();

    QVariant status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute);
    QVariantMap data;
    data["id"] = m_ids.value(reply);
    data["type"] = "error";
    data["url"] = reply->url().toEncoded().data();
    data["errorCode"] = reply->error();
    data["errorString"] = reply->errorString();
    data["status"] = status;
    data["statusText"] = reply->attribute(QNetworkRequest::HttpReasonPhraseAttribute);
    if(!status.isValid())
        emit resourceError(data);
}

QVariantList NetworkAccessManager::getHeadersFromReply(const QNetworkReply* reply)
{
    QVariantList headers;
    foreach(QByteArray headerName, reply->rawHeaderList()) {
        QVariantMap header;
        header["name"] = QString::fromUtf8(headerName);
        header["value"] = QString::fromUtf8(reply->rawHeader(headerName));
        headers += header;
    }

    return headers;
}

#if QT_VERSION >= QT_VERSION_CHECK(5, 6, 0)
void NetworkAccessManager::handleRedirect(const QUrl& url)
{
    QNetworkReply* reply = qobject_cast<QNetworkReply*>(sender());

    qDebug() << "Network - Redirecting to " << url.toEncoded();

    QVariantMap data;
    data["id"] = m_ids.value(reply);
    data["url"] = url.toEncoded().data();
    data["stage"] = "start";
    data["type"] = "response";
    data["status"] = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute);
    data["statusText"] = reply->attribute(QNetworkRequest::HttpReasonPhraseAttribute);
    data["contentType"] = reply->header(QNetworkRequest::ContentTypeHeader);
    data["bodySize"] = reply->size();
    data["redirectURL"] = reply->header(QNetworkRequest::LocationHeader);
    data["headers"] = getHeadersFromReply(reply);
    data["time"] = QDateTime::currentDateTime().toString(Qt::ISODateWithMs);
    data["body"] = getResponseBodyFromReply(reply);

    emit resourceRedirect(data);

    get(QNetworkRequest(url));
}
#endif

void NetworkAccessManager::setRequestHeaders(QNetworkRequest* request)
{
    if (m_customHeaders.empty()) {
        return;
    }

    // save and clear existing headers
    QHash<QByteArray, QByteArray> originalHeaders;
    foreach (QByteArray headerName, request->rawHeaderList()) {
        originalHeaders[headerName] = request->rawHeader(headerName);
        request->setRawHeader(headerName, QByteArray());
    }

    // set custom headers
    QVariantList::const_iterator it = m_customHeaders.begin();

    while (it != m_customHeaders.end()) {
        QVariant item = *it;
        if (item.isValid()) {
            if (item.type() == QVariant::String) {

                // use the existing header
                QByteArray headerName = item.toString().toLatin1();
                QByteArray headerValue = originalHeaders[headerName];

                if (headerValue.isEmpty()) {
                    headerValue = "preserve";
                }

                request->setRawHeader(headerName, headerValue);
                QString msg = QString("Network - Using the existing header \"%1\": \"%2\"").arg(QString(headerName), QString(headerValue));
                qDebug() << qPrintable(msg);

                // remove the header from old headers
                originalHeaders.remove(headerName);
            } else if (item.type() == QVariant::Map) {
                QVariantMap obj = item.toMap();

                foreach (QVariant key, obj.keys()) {
                    QByteArray headerName = key.toString().toLatin1();
                    QVariant objValue = obj.value(headerName);
                    if (objValue.type() == QVariant::String) {
                        QByteArray headerValue = objValue.toString().toLatin1();

                        QString msg = QString("Network - Adding custom header \"%1\": \"%2\"").arg(QString(headerName), QString(headerValue));
                        qDebug() << qPrintable(msg);

                        request->setRawHeader(headerName, headerValue);

                        // remove the header from old headers
                        originalHeaders.remove(headerName);
                    }
                }
            } else {
                qDebug() << "Network - Unrecognized type of a header: " << item.type();
            }
        }
        ++it;
    }

    // set the remaining original headers
    foreach (QByteArray headerName, originalHeaders.keys()) {
        request->setRawHeader(headerName, originalHeaders.value(headerName));
    }
}

bool NetworkAccessManager::isBlockDomainOrIP(const QString& domain)
{
    bool blocked = false;
    // check blocked domin list
    if (m_config->hasSetBlockDomain()) {
        blocked = m_config->isBlockedIpDomain(domain);
        if (!blocked){
            if (!m_dns_cache.contains(domain)) {
                // FIXME: The function blocks during the lookup which means that execution of the program
                // is suspended until the results of the lookup are ready.
                const QList<QHostAddress>& hostAddresses = QHostInfo::fromName(domain).addresses();
                foreach (QHostAddress host, hostAddresses) {
                    m_dns_cache.insert(domain,host.toString());
                }
            }
            // from cache
            const QList<QString>& values = m_dns_cache.values(domain);
            foreach (QString value, values) {
                if (m_config->isBlockedIpDomain(value)) {
                    blocked = true;
                    break;
                }
            }
        }
    }
    return blocked;
}
