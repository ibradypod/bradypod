#include "bradypod.h"

#include <QApplication>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QMetaObject>
#include <QMetaProperty>
#include <QScreen>
#include <QStandardPaths>
#include <QWebPage>

#include "callback.h"
#include "consts.h"
#include "cookiejar.h"
#include "system.h"
#include "terminal.h"
#include "utils.h"
#include "webpage.h"
#include "htmlloader.h"

static Bradypod* bradypodInstance = NULL;

// private:
Bradypod::Bradypod(QObject* parent)
    : QObject(parent)
    , m_html_loader(0)
    , m_terminated(false)
    , m_returnValue(0)
    , m_filesystem(0)
    , m_system(0)
{
    QStringList args = QApplication::arguments();
    m_start_time = QDateTime::currentDateTime();

    // Prepare the configuration object based on the command line arguments.
    // Because this object will be used by other classes, it needs to be ready ASAP.
    m_config = new Config(this);
    m_config->init(args);
    // Apply debug configuration as early as possible
    Utils::printDebugMessages = m_config->printDebugMessages();

    m_parsedDataStore["start_time"] = m_start_time.toString(Qt::ISODateWithMs);
    m_parsedDataStore["url"] = m_config->resourceUrl();
    m_parsedDataStore["commandline"] = args;
    m_parsedDataStore["data"] = QVariantList();
}

void Bradypod::init()
{
    if (m_config->helpFlag()) {
        Terminal::instance()->cout(QString("%1").arg(m_config->helpText()));
//        Terminal::instance()->cout("Any of the options that accept boolean values ('true'/'false') can also accept 'yes'/'no'.");
        Terminal::instance()->cout("");
        m_terminated = true;
        return;
    }

    if (m_config->versionFlag()) {
        m_terminated = true;
        Terminal::instance()->cout(QString("%1").arg(BRADYPOD_VERSION_STRING));
        return;
    }

    if (!m_config->unknownOption().isEmpty()) {
        Terminal::instance()->cerr(m_config->unknownOption());
        m_terminated = true;
        return;
    }

    // Initialize the CookieJar
    m_defaultCookieJar = new CookieJar(m_config->cookiesFile());
    if (m_config->cookies().length() > 0) {
        setCookies(m_config->cookies());
    }
    if (m_config->cookiejarData().length() > 0) {
        m_defaultCookieJar->addCookiesFromMap(m_config->cookiejarData());
    }

    // set the default DPI
    m_defaultDpi = qRound(QApplication::primaryScreen()->logicalDotsPerInch());

    if ( m_config->offlineStorageDefaultQuota() > 0) {
        QWebSettings::setOfflineWebApplicationCachePath(QStandardPaths::writableLocation(QStandardPaths::DataLocation));
        if (m_config->offlineStoragePath().isEmpty()) {
            QWebSettings::setOfflineStoragePath(QStandardPaths::writableLocation(QStandardPaths::DataLocation));
        } else {
            QWebSettings::setOfflineStoragePath(m_config->offlineStoragePath());
        }
        QWebSettings::setOfflineStorageDefaultQuota(m_config->offlineStorageDefaultQuota());
    }

    m_page = new WebPage(this, m_config->resourceUrl());
    m_page->setCookieJar(m_defaultCookieJar);
    m_pages.append(m_page);

    // Set up proxy if required
    QString proxyType = m_config->proxyType();
    if (proxyType != "none") {
        setProxy(m_config->proxyHost(), m_config->proxyPort(), proxyType, m_config->proxyAuthUser(), m_config->proxyAuthPass());
    }

    // Set output encoding
    Terminal::instance()->setEncoding(m_config->outputEncoding());

    // Set script file encoding
    m_scriptFileEnc.setEncoding(m_config->scriptEncoding());

    connect(m_page, SIGNAL(javaScriptConsoleMessageSent(QString)),
            SLOT(printConsoleMessage(QString)));
    connect(m_page, SIGNAL(initialized()),
            SLOT(onInitialized()));

    m_defaultPageSettings[PAGE_SETTINGS_LOAD_IMAGES] = QVariant::fromValue(m_config->autoLoadImages());
    m_defaultPageSettings[PAGE_SETTINGS_JS_ENABLED] = QVariant::fromValue(m_config->javascriptEnabled());
    m_defaultPageSettings[PAGE_SETTINGS_XSS_AUDITING] = QVariant::fromValue(m_config->webSecurityEnabled());
    m_defaultPageSettings[PAGE_SETTINGS_USER_AGENT] = QVariant::fromValue(m_config->userAgent());
    m_defaultPageSettings[PAGE_SETTINGS_LOCAL_ACCESS_REMOTE] = QVariant::fromValue(m_config->localToRemoteUrlAccessEnabled());
    m_defaultPageSettings[PAGE_SETTINGS_WEB_SECURITY_ENABLED] = QVariant::fromValue(m_config->webSecurityEnabled());
    m_defaultPageSettings[PAGE_SETTINGS_JS_CAN_OPEN_WINDOWS] = QVariant::fromValue(m_config->javascriptCanOpenWindows());
    m_defaultPageSettings[PAGE_SETTINGS_JS_CAN_CLOSE_WINDOWS] = QVariant::fromValue(m_config->javascriptCanCloseWindows());
    m_defaultPageSettings[PAGE_SETTINGS_RESOURCE_TIMEOUT] = QVariant::fromValue(m_config->resourceTimeout());
    m_defaultPageSettings[PAGE_SETTINGS_DPI] = QVariant::fromValue(m_defaultDpi);
    m_page->applySettings(m_defaultPageSettings);

    setLibraryPath(QDir::currentPath());
}

// public:
Bradypod* Bradypod::instance()
{
    if (NULL == bradypodInstance) {
        bradypodInstance = new Bradypod();
        bradypodInstance->init();
    }
    return bradypodInstance;
}

Bradypod::~Bradypod()
{
    // Nothing to do: cleanup is handled by QObject relationships
}

QVariantMap Bradypod::defaultPageSettings() const
{
    return m_defaultPageSettings;
}

QString Bradypod::outputEncoding() const
{
    return Terminal::instance()->getEncoding();
}

void Bradypod::setOutputEncoding(const QString& encoding)
{
    Terminal::instance()->setEncoding(encoding);
}

bool Bradypod::execute()
{
    if (m_terminated) {
        return false;
    }

#ifndef QT_NO_DEBUG_OUTPUT
    qDebug() << "Bradypod - execute: Configuration";
    const QMetaObject* configMetaObj = m_config->metaObject();
    for (int i = 0, ilen = configMetaObj->propertyCount(); i < ilen; ++i) {
        qDebug() << "    " << i << configMetaObj->property(i).name() << ":" << m_config->property(configMetaObj->property(i).name()).toString();
    }

    qDebug() << "Bradypod - execute: URL";
    qDebug() << "    " << "URL:" << m_config->resourceUrl();
#endif

    if (m_config->resourceUrl().isEmpty()) {
        Terminal::instance()->cout(m_config->helpText());
        return false;
    }

    if (m_config->debug() && m_config->remoteDebugPort() != 0) {
#ifndef QT_NO_DEBUG
        // Debug enabled
        m_page->showInspector(m_config->remoteDebugPort());
#endif
    }

    html_loader()->loadUrl(m_config->resourceUrl());

    return !m_terminated;
}

int Bradypod::returnValue() const
{
    return m_returnValue;
}

QString Bradypod::libraryPath() const
{
    return m_page->libraryPath();
}

void Bradypod::setLibraryPath(const QString& libraryPath)
{
    m_page->setLibraryPath(libraryPath);
}

QVariantMap Bradypod::version() const
{
    QVariantMap result;
    result["major"] = BRADYPOD_VERSION_MAJOR;
    result["minor"] = BRADYPOD_VERSION_MINOR;
    result["patch"] = BRADYPOD_VERSION_PATCH;
    return result;
}

QObject* Bradypod::page()
{
    return m_page;
}

HtmlLoader* Bradypod::html_loader()
{
    if (m_html_loader)
        return m_html_loader;
    else
        m_html_loader = new HtmlLoader(this,0);
    return m_html_loader;
}

Config* Bradypod::config() const
{
    return m_config;
}

bool Bradypod::printDebugMessages() const
{
    return m_config->printDebugMessages();
}

bool Bradypod::areCookiesEnabled() const
{
    return m_defaultCookieJar->isEnabled();
}

void Bradypod::setCookiesEnabled(const bool value)
{
    if (value) {
        m_defaultCookieJar->enable();
    } else {
        m_defaultCookieJar->disable();
    }
}

// public slots:
QObject* Bradypod::createCookieJar(const QString& filePath)
{
    return new CookieJar(filePath, this);
}

QObject* Bradypod::createWebPage()
{
    WebPage* page = new WebPage(this);
    page->setCookieJar(m_defaultCookieJar);

    // Store pointer to the page for later cleanup
    m_pages.append(page);
    // Apply default settings to the page
    page->applySettings(m_defaultPageSettings);

    // Show web-inspector if in debug mode
    if (m_config->debug()) {
        page->showInspector(m_config->remoteDebugPort());
    }

    return page;
}

QObject* Bradypod::createFilesystem()
{
    if (!m_filesystem) {
        m_filesystem = new FileSystem(this);
    }

    return m_filesystem;
}

QObject* Bradypod::createSystem()
{
    if (!m_system) {
        m_system = new System(this);

        QStringList systemArgs;
        systemArgs += m_config->resourceUrl();
        systemArgs += m_config->scriptArgs();
        m_system->setArgs(systemArgs);
    }

    return m_system;
}

QObject* Bradypod::createCallback()
{
    return new Callback(this);
}

void Bradypod::loadModule(const QString& moduleSource, const QString& filename)
{
    if (m_terminated) {
        return;
    }

    QString scriptSource =
        "(function(require, exports, module) {\n" +
        moduleSource +
        "\n}.call({}," +
        "require.cache['" + filename + "']._getRequire()," +
        "require.cache['" + filename + "'].exports," +
        "require.cache['" + filename + "']" +
        "));";
    m_page->mainFrame()->evaluateJavaScript(scriptSource);
}

bool Bradypod::injectJs(const QString& jsFilePath)
{
    QString pre = "";
    qDebug() << "Bradypod - injectJs:" << jsFilePath;

    if (m_terminated) {
        return false;
    }

    return Utils::injectJsInFrame(pre + jsFilePath, libraryPath(), m_page->mainFrame());
}

void Bradypod::setProxy(const QString& ip, const qint64& port, const QString& proxyType, const QString& user, const QString& password)
{
    qDebug() << "Set " << proxyType << " proxy to: " << ip << ":" << port;
    if (ip.isEmpty()) {
        QNetworkProxyFactory::setUseSystemConfiguration(true);
    } else {
        QNetworkProxy::ProxyType networkProxyType = QNetworkProxy::HttpProxy;

        if (proxyType == "socks5") {
            networkProxyType = QNetworkProxy::Socks5Proxy;
        }
        // Checking for passed proxy user and password, allow empty password
        if (!user.isEmpty()) {
            QNetworkProxy proxy(networkProxyType, ip, port, user, password);
            QNetworkProxy::setApplicationProxy(proxy);
        } else {
            QNetworkProxy proxy(networkProxyType, ip, port);
            QNetworkProxy::setApplicationProxy(proxy);
        }
    }
}

QString Bradypod::proxy()
{
    QNetworkProxy proxy = QNetworkProxy::applicationProxy();
    if (proxy.hostName().isEmpty()) {
        return NULL;
    }
    return proxy.hostName() + ":" + QString::number(proxy.port());
}

int Bradypod::remoteDebugPort() const
{
    return m_config->remoteDebugPort();
}

static bool writeData2File(const QByteArray& data, const QString& fileName)
{
    if (fileName.isEmpty()) {
        Terminal::instance()->cout("========= BRADYPOD =========\n" + data);
    } else {
        QFile file(fileName);
        if(!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            Terminal::instance()->cout(QString("Open File[%1] error: %2").arg(fileName,file.errorString()));
            return false;
        }
        QTextStream textOutput(&file);
        textOutput.setCodec("utf-8");
        textOutput << data;
        file.close();
    }
    return true;
}

void Bradypod::exit(int code)
{
    if (m_config->debug() && m_config->remoteDebugPort() != 0) {
        qDebug()<<"Bradypod::exit() called but not quitting in debug mode.";
    } else {
        doExit(code);
    }

    m_end_time = QDateTime::currentDateTime();
    m_parsedDataStore["end_time"] = m_end_time.toString(Qt::ISODateWithMs);
    m_parsedDataStore["time_cost"] = m_start_time.msecsTo(m_end_time);

    // export result
    if (m_config->outputFormat() == "json") {
        QJsonObject json = storeToJson();
        QJsonDocument jdoc(json);
        QByteArray data = jdoc.toJson(QJsonDocument::Indented);
        writeData2File(data,m_config->outputFile());
    } else {
        QDomDocument xml = storeToXml();
        writeData2File(xml.toByteArray(),m_config->outputFile());
    }
}

void Bradypod::debugExit(int code)
{
    doExit(code);
}

QString Bradypod::resolveRelativeUrl(QString url, QString base)
{
    QUrl u = QUrl::fromEncoded(url.toLatin1());
    QUrl b = QUrl::fromEncoded(base.toLatin1());

    return b.resolved(u).toEncoded();
}

QString Bradypod::fullyDecodeUrl(QString url)
{
    return QUrl::fromEncoded(url.toLatin1()).toDisplayString();
}

// private slots:
void Bradypod::printConsoleMessage(const QString& message)
{
    qDebug()<<"console: "<<message;
}

void Bradypod::onInitialized()
{
    // Add 'bradypod' object to the global scope
    m_page->mainFrame()->addToJavaScriptWindowObject("bradypod", this);

    // Bootstrap
//    m_page->mainFrame()->evaluateJavaScript(Utils::readResourceFileUtf8(":/bootstrap.js"));
}

bool Bradypod::setCookies(const QVariantList& cookies)
{
    // Delete all the cookies from the CookieJar
    m_defaultCookieJar->clearCookies();
    // Add a new set of cookies
    return m_defaultCookieJar->addCookiesFromMap(cookies);
}

static QList<QNetworkCookie> parseSimpleCookie(const QString &raw)
{
    QList<QNetworkCookie> result;
    QList<QByteArray> cookieList = raw.toLocal8Bit().split(';');
    foreach (const QByteArray &name_value, cookieList) {
        QNetworkCookie cookie;
        QList<QByteArray> nv_pair = name_value.split('=');
        if (nv_pair.length() == 2) {
            cookie.setName(nv_pair[0].trimmed());
            cookie.setValue(nv_pair[1].trimmed());
            result += cookie;
        }
    }
    return result;
}


bool Bradypod::setCookies(const QString& cookies_str)
{
    QList<QNetworkCookie> cookies = parseSimpleCookie(cookies_str);
    m_defaultCookieJar->setCookiesFromUrl(cookies,QUrl(m_config->resourceUrl()));
    return true;
}

QVariantList Bradypod::cookies() const
{
    // Return all the Cookies in the CookieJar, as a list of Maps (aka JSON in JS space)
    return m_defaultCookieJar->cookiesToMap();
}

bool Bradypod::addCookie(const QVariantMap& cookie)
{
    return m_defaultCookieJar->addCookieFromMap(cookie);
}

bool Bradypod::deleteCookie(const QString& cookieName)
{
    if (!cookieName.isEmpty()) {
        return m_defaultCookieJar->deleteCookie(cookieName);
    }
    return false;
}

void Bradypod::clearCookies()
{
    m_defaultCookieJar->clearCookies();
}


// private:
void Bradypod::doExit(int code)
{
    emit aboutToExit(code);
    m_terminated = true;
    m_returnValue = code;

    // Iterate in reverse order so the first page is the last one scheduled for deletion.
    // The first page is the root object, which will be invalidated when it is deleted.
    // This causes an assertion to go off in BridgeJSC.cpp Instance::createRuntimeObject.
    QListIterator<QPointer<WebPage> > i(m_pages);
    i.toBack();
    while (i.hasPrevious()) {
        const QPointer<WebPage> page = i.previous();

        if (!page) {
            continue;
        }

        // stop processing of JavaScript code by loading a blank page
        page->mainFrame()->setUrl(QUrl(QStringLiteral("about:blank")));
        // delay deletion into the event loop, direct deletion can trigger crashes
        page->deleteLater();
    }
    m_pages.clear();
    m_page = 0;
    QApplication::instance()->exit(code);
}

QVariantMap Bradypod::getParsedDataStore() const
{
    QVariantMap data(m_parsedDataStore);
    data["data"] = m_requestData;
    data["cookiejar"] = m_defaultCookieJar->cookiesToMap();
    data["page_content"] = m_html_loader->getHtmlContent();
    return data;
}

void Bradypod::addParsedData(const QVariant& data)
{
    QVariantMap dataMap = data.toMap();
//    qDebug()<<"data:"<<data;
    QString id = dataMap["id"].toString();
    if (!m_requestData.contains(id)) {
        m_requestData[id] = QVariantMap();
    }
    QVariantMap req = m_requestData[id].toMap();
    QString type = dataMap["type"].toString();
    if (type == "finished")
    {
        if (req.contains("response")){
            // ignore
            return;
        } else {
            type = "response";
        }
    }
    dataMap.remove("id");
    dataMap.remove("type");
    dataMap.remove("stage");
    qDebug()<<"type: "<< type;
    req[type] = dataMap;
//    m_requestData.remove(id);
    m_requestData[id] = req;
}

QJsonObject Bradypod::storeToJson() const
{
    return QJsonObject::fromVariantMap(getParsedDataStore());
}

static void _json2xml(QDomDocument& doc,QDomElement& parent,const QVariant& data)
{
    if (data.isNull()) {
        return;
    }
    QVariant::Type type = data.type();
    switch (type) {
    case QVariant::Map:
    {
        QMapIterator<QString, QVariant> i(data.toMap());
        while (i.hasNext()) {
            i.next();
            QDomElement node = doc.createElement("element");
            node.setAttribute("name",i.key());
            parent.appendChild(node);
            _json2xml(doc,node,i.value());
        }
    }
        break;
    case QVariant::List:
    case QVariant::StringList:
    {
        QVariantList items = data.toList();
        foreach (QVariant item, items) {
            QDomElement node = doc.createElement("list");
            parent.appendChild(node);
            _json2xml(doc,node,item);
        }
    }
        break;
    default:
        parent.setAttribute("value",data.toString());
        break;
    }
}

static QDomDocument json2xml(const QVariant& data)
{
    QDomDocument doc;
    QDomProcessingInstruction instruction = doc.createProcessingInstruction("xml", "version=\"1.0\" encoding=\"UTF-8\"");
    doc.appendChild(instruction);

    QDomElement root = doc.createElement("bradypod");
    doc.appendChild(root);

    _json2xml(doc,root,data);

    return doc;
}

QDomDocument Bradypod::storeToXml() const
{
    QVariantMap data = getParsedDataStore();
    QDomDocument doc = json2xml(data);
    return doc;
}
