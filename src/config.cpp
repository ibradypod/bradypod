#include "config.h"

#include <QDir>
#include <QFileInfo>
#include <QWebPage>
#include <QWebFrame>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QCoreApplication>

#include "terminal.h"
#include "qcommandline.h"
#include "utils.h"
#include "consts.h"


static const struct QCommandLineConfigEntry flags[] = {
    { QCommandLine::Option, '\0', "cookies", QStringLiteral("设置Cookie"), QCommandLine::Optional },
    { QCommandLine::Option, '\0', "cookies-file", QStringLiteral("设置输出Cookie的文件名"), QCommandLine::Optional },
    { QCommandLine::Option, '\0', "cookiejar", QStringLiteral("设置Cookiejar的json数据"), QCommandLine::Optional },
    { QCommandLine::Option, '\0', "method", QStringLiteral("请求方法，默认'GET'"), QCommandLine::Optional },
    { QCommandLine::Option, '\0', "body", QStringLiteral("请求体，默认为空"), QCommandLine::Optional },
    { QCommandLine::Option, '\0', "body-encoding", QStringLiteral("输入的请求体内容编码,'utf-8'或'latin'(默认)"), QCommandLine::Optional },
    { QCommandLine::Option, '\0', "header", QStringLiteral("额外的请求头条目,如'--header=Referer:\\ abc'。该选项可以使用多次"), QCommandLine::Flags(QCommandLine::Optional | QCommandLine::Multiple) },
    { QCommandLine::Option, '\0', "user-agent", QStringLiteral("User Agent"), QCommandLine::Optional },
    { QCommandLine::Option, '\0', "headers-attach-to-per-request", QStringLiteral("自定义的请求头附加到每一个http请求上,'true'或'false'(默认)"), QCommandLine::Optional },
    { QCommandLine::Option, '\0', "block-ip-and-domain", QStringLiteral("排除的域名或ip地址,允许通配符,如:*.example.com;*.gov"), QCommandLine::Optional },
    { QCommandLine::Option, '\0', "config", QStringLiteral("指定JSON格式的配置文件"), QCommandLine::Optional },
    { QCommandLine::Option, '\0', "debug", QStringLiteral("打印额外的警告和调试信息:'true'或'false'(默认)"), QCommandLine::Optional },
    { QCommandLine::Option, '\0', "disk-cache", QStringLiteral("启用磁盘缓存:'true'或'false'(默认)"), QCommandLine::Optional },
    { QCommandLine::Option, '\0', "disk-cache-path", QStringLiteral("指定磁盘缓存的位置"), QCommandLine::Optional },
    { QCommandLine::Option, '\0', "ignore-ssl-errors", QStringLiteral("忽略SSL错误(过期/自签名证书错误):'true'(默认)或'false'"), QCommandLine::Optional },
    { QCommandLine::Option, '\0', "load-images", QStringLiteral("加载图像:'true'或'false'(默认)"), QCommandLine::Optional },
    { QCommandLine::Option, '\0', "only-load-first-request", QStringLiteral("只加载URL的第一个请求，不请求其他资源:'true'或'false'(默认)"), QCommandLine::Optional },
    { QCommandLine::Option, '\0', "local-url-access", QStringLiteral("允许使用'file:///'格式的URL:'true'或'false'(默认)"), QCommandLine::Optional },
    { QCommandLine::Option, '\0', "local-storage-path", QStringLiteral("指定本地存储的位置"), QCommandLine::Optional },
    { QCommandLine::Option, '\0', "local-storage-quota", QStringLiteral("设置本地存储的最大大小(以KB为单位)"), QCommandLine::Optional },
    { QCommandLine::Option, '\0', "offline-storage-path", QStringLiteral("指定离线存储的位置"), QCommandLine::Optional },
    { QCommandLine::Option, '\0', "offline-storage-quota", QStringLiteral("设置脱机存储的最大大小(以KB为单位)"), QCommandLine::Optional },
    { QCommandLine::Option, '\0', "local-to-remote-url-access", QStringLiteral("允许本地内容访问远程URL:'true'或'false'(默认)"), QCommandLine::Optional },
    { QCommandLine::Option, '\0', "max-disk-cache-size", QStringLiteral("限制磁盘缓存的大小(以KB为单位)"), QCommandLine::Optional },
    { QCommandLine::Option, '\0', "output-encoding", QStringLiteral("设置终端输出的编码,默认为'utf8'"), QCommandLine::Optional },
    { QCommandLine::Option, '\0', "remote-debugger-P=port", QStringLiteral("启动调试工具中的脚本并侦听指定的端口"), QCommandLine::Optional },
    { QCommandLine::Option, '\0', "remote-rebugger-autorun", QStringLiteral("立即在调试器中运行脚本:'true'或'false'(默认)"), QCommandLine::Optional },
    { QCommandLine::Option, '\0', "proxy", QStringLiteral("设置代理服务器,例如'--proxy=http://proxy.company.com:8080'"), QCommandLine::Optional },
    { QCommandLine::Option, '\0', "proxy-auth", QStringLiteral("提供代理的身份验证信息,例如'-proxy-auth=username:password'"), QCommandLine::Optional },
    { QCommandLine::Option, '\0', "proxy-type", QStringLiteral("指定代理类型,'http'(默认), 'none'(完全禁用), 'socks5'"), QCommandLine::Optional },
    { QCommandLine::Option, '\0', "resource-timeout", QStringLiteral("设置资源请求超时时间(单位:s), 默认为8s"), QCommandLine::Optional },
    { QCommandLine::Option, '\0', "wait-window-onload-timeout", QStringLiteral("在windonw.onload事件后等待Javascript去加载的时间,值:1200(默认,单位:ms)"), QCommandLine::Optional },
    { QCommandLine::Option, '\0', "web-security", QStringLiteral("启用Web安全检查,'true' (默认值) 或'false'"), QCommandLine::Optional },
    { QCommandLine::Option, '\0', "javascript-enable", QStringLiteral("启用JavaScript,'true' (默认值) 或'false'"), QCommandLine::Optional },
//    { QCommandLine::Option, '\0', "java-enable", QStringLiteral("启用Java,'true'或 'false'(默认值)"), QCommandLine::Optional },
    { QCommandLine::Option, '\0', "ssl-protocol", QStringLiteral("选择要提供的特定SSL协议版本。tlsv1.2,tlsv1.1,...,sslv3或any,默认是any"), QCommandLine::Optional },
    { QCommandLine::Option, '\0', "ssl-ciphers", QStringLiteral("设置支持的TLS/SSL密码套件。参数是冒号分隔的OpenSSL密码列表,如:AES128-SHA:RC4-MD5"), QCommandLine::Optional },
    { QCommandLine::Option, '\0', "ssl-certificates-path", QStringLiteral("设置自定义CA证书的位置(如果没有设置,将使用环境变量SSL_CERT_DIR或系统默认值)"), QCommandLine::Optional },
    { QCommandLine::Option, '\0', "ssl-client-certificate-file", QStringLiteral("设置客户端证书的位置"), QCommandLine::Optional },
    { QCommandLine::Option, '\0', "ssl-client-key-file", QStringLiteral("设置客户端私钥的位置"), QCommandLine::Optional },
    { QCommandLine::Option, '\0', "ssl-client-key-passphrase", QStringLiteral("设置客户端私钥的密码"), QCommandLine::Optional },
    { QCommandLine::Param, '\0', "url", QStringLiteral("需要解析的URL"), QCommandLine::Flags(QCommandLine::Optional | QCommandLine::ParameterFence)},
    { QCommandLine::Option, 'o', "output", QStringLiteral("将结果输出到文件"), QCommandLine::Optional },
    { QCommandLine::Option, '\0', "output-format", QStringLiteral("将结果以指定格式输出,'json' (默认值) 或'xml'"), QCommandLine::Optional },
    { QCommandLine::Option, '\0', "render-image-path", QStringLiteral("内容加载完成后截图保存到指定路径,格式:png(默认),pdf"), QCommandLine::Optional },
    { QCommandLine::Switch, 'h', "help", QStringLiteral("显示帮助信息并退出"), QCommandLine::Optional },
    { QCommandLine::Switch, 'v', "version", QStringLiteral("版本"), QCommandLine::Optional },
    QCOMMANDLINE_CONFIG_ENTRY_END
};

Config::Config(QObject* parent)
    : QObject(parent)
{
    m_cmdLine = new QCommandLine(this);
    m_cmdLine->setConfig(flags);
    connect(m_cmdLine, SIGNAL(switchFound(const QString&)), this, SLOT(handleSwitch(const QString&)));
    connect(m_cmdLine, SIGNAL(optionFound(const QString&, const QVariant&)), this, SLOT(handleOption(const QString&, const QVariant&)));
    connect(m_cmdLine, SIGNAL(paramFound(const QString&, const QVariant&)), this, SLOT(handleParam(const QString&, const QVariant&)));
    connect(m_cmdLine, SIGNAL(parseError(const QString&)), this, SLOT(handleError(const QString&)));

    // We will handle --help and --version ourselves in bradypod.cpp
    m_cmdLine->enableHelp(false);
    m_cmdLine->enableVersion(false);

    resetToDefaults();
}

void Config::init(const QStringList& args)
{
    resetToDefaults();

    QByteArray envSslCertDir = qgetenv("SSL_CERT_DIR");
    if (!envSslCertDir.isEmpty()) {
        setSslCertificatesPath(envSslCertDir);
    }

    processArgs(args);

    if (!m_configFile.isEmpty()) {
        loadJsonFile(m_configFile);
    }
}

void Config::processArgs(const QStringList& args)
{
    m_cmdLine->setArguments(args);
    m_cmdLine->parse();
}

void Config::loadJsonFile(const QString& filePath)
{
    QByteArray jsonConfig;
    QFile f(filePath);

    // Check file exists and is readable
    if (!f.exists() || !f.open(QFile::ReadOnly | QFile::Text)) {
        Terminal::instance()->cerr("Unable to open config: \"" + filePath + "\"");
        return;
    }

    // Read content
    jsonConfig = f.readAll().trimmed();
    f.close();

    QJsonParseError error;
    QJsonDocument json = QJsonDocument::fromJson(jsonConfig,&error);
    if (error.error != QJsonParseError::NoError) {  // Check it's a valid JSON format
        Terminal::instance()->cerr("JSON parse error: " + error.errorString());
        return;
    }
    QStringList args;
    QVariantMap config = json.object().toVariantMap();
    QMapIterator<QString, QVariant> i(config);
    args << QCoreApplication::arguments()[0];
    while (i.hasNext()) {
        i.next();
        QString key = i.key();
        QVariant value = i.value();
        if (key == "config") {
            // don't load recursively
            continue;
        }
        if (value.type() == QVariant::String) {
            args.append("--" + key + "=" + value.toString());
        } else if (value.type() == QVariant::Bool) {
            args.append("--" + key + "=" + value.toString());
        } else if (value.type() == QVariant::List) {
            QVariantList items = value.toList();
            foreach (QVariant item, items) {
                args.append("--" + key + "=" + item.toString());
            }
        } else {
            Terminal::instance()->cerr("JSON format error: " + key + " expect list, string or boolean.");
            return;
        }
    }
    // process args
    processArgs(args);
}

QString Config::helpText() const
{
    return m_cmdLine->help();
}

QVariantMap Config::getOperation() const
{
    return m_operation;
}

void Config::setOperation(QString method, QString body,const QVariantMap& headers, QString encoding)
{
    m_operation["method"] = method;
    m_operation["data"] = body;
    m_operation["encoding"] = encoding;
    m_operation["headers"] = headers;
}

void Config::setMethod(QString method)
{
    m_operation["method"] = method;
}

void Config::setBody(QString body)
{
    m_operation["data"] = body;
}

void Config::setBodyEncoding(QString encoding)
{
    m_operation["encoding"] = encoding;
}

void Config::setHeaders(const QVariantMap & header)
{
    m_operation["headers"] = header;
}

void Config::addHeader(const QString& name, const QString& value)
{
    if (!name.isEmpty() && !value.isEmpty()) {
        QVariantMap headers = m_operation["headers"].toMap();
        headers[name] = value;
        m_operation["headers"] = headers;
    }
}

void Config::setHeadersToPerRequest(const bool value)
{
    m_operation["headers_attach_to_per_request"] = value;
}

bool Config::autoLoadImages() const
{
    return m_autoLoadImages;
}

void Config::setAutoLoadImages(const bool value)
{
    m_autoLoadImages = value;
}

QString Config::renderImagePath() const
{
    return m_renderImagePath;
}

void Config::setRenderImagePath(const QString& value)
{
    m_renderImagePath = value;
}

bool Config::onlyLoadFirstRequest() const
{
    return m_only_load_first_request;
}

void Config::setOnlyLoadFirstRequest(const bool value)
{
    m_only_load_first_request = value;
}

int Config::waitAfterWindowOnload() const
{
    return m_waitAfterWindowOnload;
}

void Config::setWaitAfterWindowOnload(const int millisecond)
{
    m_waitAfterWindowOnload = millisecond > 200 ? millisecond : 200;
}

bool Config::isBlockedIpDomain(const QString& domain) const
{
    foreach (QRegExp wc, m_blockIpAndDomains) {
        if (wc.exactMatch(domain))
            return true;
    }
    return false;
}

bool Config::hasSetBlockDomain() const
{
    return m_blockIpAndDomains.length() > 0 ? true : false;
}

QList<QRegExp> Config::blockedIpAndDomains() const
{
    return m_blockIpAndDomains;
}

void Config::addBlockIpAndDomain(const QString& value)
{
    QString text = value.trimmed().toLower();
    if (!text.isEmpty()) {
        QRegExp wc = QRegExp(text,Qt::CaseInsensitive,QRegExp::Wildcard);
        if (!m_blockIpAndDomains.contains(wc)) {
            m_blockIpAndDomains.append(wc);
        }
    }
}

QString Config::configFile() const
{
    return m_configFile;
}

void Config::setConfigFile(const QString& value)
{
    m_configFile = value.trimmed();
}

QString Config::cookies() const
{
    return m_cookies;
}

void Config::setCookies(const QString& cookies)
{
    m_cookies = cookies;
}

QVariantList Config::cookiejarData() const
{
    return m_cookiejar;
}

void Config::setCookiejarData(const QString& cookiejar)
{
    QJsonParseError error;
    QJsonDocument json = QJsonDocument::fromJson(cookiejar.trimmed().toUtf8(), &error);
    if (error.error != QJsonParseError::NoError) {  // Check it's a valid JSON format
        Terminal::instance()->cerr("JSON parse error: " + error.errorString());
        return;
    }
    if (!json.isArray() || json.array().size() < 1) return;

    m_cookiejar = json.array().toVariantList();
}

QString Config::cookiesFile() const
{
    return m_cookiesFile;
}

void Config::setCookiesFile(const QString& value)
{
    m_cookiesFile = value;
}

QString Config::offlineStoragePath() const
{
    return m_offlineStoragePath;
}

void Config::setOfflineStoragePath(const QString& value)
{
    QDir dir(value);
    m_offlineStoragePath = dir.absolutePath();
}

int Config::offlineStorageDefaultQuota() const
{
    return m_offlineStorageDefaultQuota;
}

void Config::setOfflineStorageDefaultQuota(int offlineStorageDefaultQuota)
{
    m_offlineStorageDefaultQuota = offlineStorageDefaultQuota * 1024;
}


QString Config::localStoragePath() const
{
    return m_localStoragePath;
}

void Config::setLocalStoragePath(const QString& value)
{
    QDir dir(value);
    m_localStoragePath = dir.absolutePath();
}

int Config::localStorageDefaultQuota() const
{
    return m_localStorageDefaultQuota;
}

void Config::setLocalStorageDefaultQuota(int localStorageDefaultQuota)
{
    m_localStorageDefaultQuota = localStorageDefaultQuota * 1024;
}

bool Config::diskCacheEnabled() const
{
    return m_diskCacheEnabled;
}

void Config::setDiskCacheEnabled(const bool value)
{
    m_diskCacheEnabled = value;
}

int Config::maxDiskCacheSize() const
{
    return m_maxDiskCacheSize;
}

void Config::setMaxDiskCacheSize(int maxDiskCacheSize)
{
    m_maxDiskCacheSize = maxDiskCacheSize;
}

QString Config::diskCachePath() const
{
    return m_diskCachePath;
}

void Config::setDiskCachePath(const QString& value)
{
    QDir dir(value);
    m_diskCachePath = dir.absolutePath();
}

bool Config::ignoreSslErrors() const
{
    return m_ignoreSslErrors;
}

void Config::setIgnoreSslErrors(const bool value)
{
    m_ignoreSslErrors = value;
}

bool Config::localUrlAccessEnabled() const
{
    return m_localUrlAccessEnabled;
}

void Config::setLocalUrlAccessEnabled(const bool value)
{
    m_localUrlAccessEnabled = value;
}

bool Config::allowNetworkAccess() const
{
    return m_allowNetworkAccess;
}

void Config::setAllowNetworkAccess(const bool value)
{
    m_allowNetworkAccess = value;
}

bool Config::localToRemoteUrlAccessEnabled() const
{
    return m_localToRemoteUrlAccessEnabled;
}

void Config::setLocalToRemoteUrlAccessEnabled(const bool value)
{
    m_localToRemoteUrlAccessEnabled = value;
}

QString Config::outputEncoding() const
{
    return m_outputEncoding;
}

void Config::setOutputEncoding(const QString& value)
{
    if (value.isEmpty()) {
        return;
    }

    m_outputEncoding = value;
}

QString Config::outputFile() const
{
    return m_outputFile;
}

void Config::setOutputFile(const QString& value)
{
    m_outputFile = value.trimmed();
}

QString Config::outputFormat() const
{
    return m_outputFormat;
}

void Config::setOutputFormat(const QString& value)
{
    m_outputFormat = value.trimmed().toLower() == "xml" ? "xml" : "json";
}

QString Config::proxyType() const
{
    return m_proxyType;
}

void Config::setProxyType(const QString& value)
{
    m_proxyType = value;
}

QString Config::proxy() const
{
    return m_proxyHost + ":" + QString::number(m_proxyPort);
}

void Config::setProxy(const QString& value)
{
    QUrl proxyUrl = QUrl::fromUserInput(value);

    if (proxyUrl.isValid()) {
        setProxyHost(proxyUrl.host());
        setProxyPort(proxyUrl.port(1080));
    }
}

void Config::setProxyAuth(const QString& value)
{
    QString proxyUser = value;
    QString proxyPass = "";

    if (proxyUser.lastIndexOf(':') > 0) {
        proxyPass = proxyUser.mid(proxyUser.lastIndexOf(':') + 1).trimmed();
        proxyUser = proxyUser.left(proxyUser.lastIndexOf(':')).trimmed();

        setProxyAuthUser(proxyUser);
        setProxyAuthPass(proxyPass);
    }
}

QString Config::proxyAuth() const
{
    return proxyAuthUser() + ":" + proxyAuthPass();
}

QString Config::proxyAuthUser() const
{
    return m_proxyAuthUser;
}

QString Config::proxyAuthPass() const
{
    return m_proxyAuthPass;
}

QString Config::proxyHost() const
{
    return m_proxyHost;
}

int Config::proxyPort() const
{
    return m_proxyPort;
}

QStringList Config::scriptArgs() const
{
    return m_scriptArgs;
}

void Config::setScriptArgs(const QStringList& value)
{
    m_scriptArgs.clear();

    QStringListIterator it(value);
    while (it.hasNext()) {
        m_scriptArgs.append(it.next());
    }
}

QString Config::scriptEncoding() const
{
    return m_scriptEncoding;
}

void Config::setScriptEncoding(const QString& value)
{
    if (value.isEmpty()) {
        return;
    }

    m_scriptEncoding = value;
}

QString Config::scriptLanguage() const
{
    return m_scriptLanguage;
}

void Config::setScriptLanguage(const QString& value)
{
    if (value.isEmpty()) {
        return;
    }

    m_scriptLanguage = value;
}

QString Config::resourceUrl() const
{
    return m_resourceUrl;
}

void Config::setResourceUrl(const QString& value)
{
    m_resourceUrl = value;
}

QString Config::userAgent() const
{
    return m_userAgent;
}

void Config::setUserAgent(const QString& value)
{
    m_userAgent = value;
}

QString Config::unknownOption() const
{
    return m_unknownOption;
}

void Config::setUnknownOption(const QString& value)
{
    m_unknownOption = value;
}

bool Config::versionFlag() const
{
    return m_versionFlag;
}

void Config::setVersionFlag(const bool value)
{
    m_versionFlag = value;
}

bool Config::debug() const
{
    return m_debug;
}

void Config::setDebug(const bool value)
{
    m_debug = value;
}

int Config::remoteDebugPort() const
{
    return m_remoteDebugPort;
}

void Config::setRemoteDebugPort(const int port)
{
    m_remoteDebugPort = port;
}

bool Config::remoteDebugAutorun() const
{
    return m_remoteDebugAutorun;
}

void Config::setRemoteDebugAutorun(const bool value)
{
    m_remoteDebugAutorun = value;
}

bool Config::webSecurityEnabled() const
{
    return m_webSecurityEnabled;
}

void Config::setWebSecurityEnabled(const bool value)
{
    m_webSecurityEnabled = value;
}

bool Config::javascriptEnabled() const
{
    return m_javascriptEnabled;
}

void Config::setJavascriptEnabled(const bool value)
{
    m_javascriptEnabled = value;
}

bool Config::javaEnabled() const
{
    return m_javaEnabled;
}

void Config::setJavaEnabled(const bool value)
{
    m_javaEnabled = value;
}

void Config::setJavascriptCanOpenWindows(const bool value)
{
    m_javascriptCanOpenWindows = value;
}

bool Config::javascriptCanOpenWindows() const
{
    return m_javascriptCanOpenWindows;
}

void Config::setJavascriptCanCloseWindows(const bool value)
{
    m_javascriptCanCloseWindows = value;
}

bool Config::javascriptCanCloseWindows() const
{
    return m_javascriptCanCloseWindows;
}

void Config::setResourceTimeout(const int value)
{
    m_resourceTimeout = value * 1000;
}

int Config::resourceTimeout() const
{
    return m_resourceTimeout;
}

// private:
void Config::resetToDefaults()
{
    m_operation["method"] = "GET";
    m_operation["data"] = "";
    m_operation["encoding"] = "latin";
    m_operation["headers_attach_to_per_request"] = false;
    m_operation["headers"] = QVariantMap();
    m_autoLoadImages = false;
    m_waitAfterWindowOnload = 1200;
#ifdef QT_NO_DEBUG
    m_renderImagePath = QString();
#else
    m_renderImagePath = "test.png";
#endif
    m_only_load_first_request = false;
    m_blockIpAndDomains.clear();
    m_configFile = QString();
    m_cookiejar.clear();
    m_cookiesFile = QString();
    m_offlineStoragePath = QString();
    m_offlineStorageDefaultQuota = -1;
    m_localStoragePath = QString();
    m_localStorageDefaultQuota = -1;
    m_diskCacheEnabled = false;
    m_maxDiskCacheSize = -1;
    m_diskCachePath = QString();
    m_ignoreSslErrors = true;
    m_localUrlAccessEnabled = false;
    m_localToRemoteUrlAccessEnabled = false;
    m_allowNetworkAccess = true;
#ifdef Q_OS_WIN32
    m_outputEncoding = "GBK";
#else
    m_outputEncoding = "UTF-8";
#endif
    m_outputFile = "";
    m_outputFormat = "json";
    m_proxyType = "http";
    m_proxyHost.clear();
    m_proxyPort = 1080;
    m_proxyAuthUser.clear();
    m_proxyAuthPass.clear();
    m_scriptArgs.clear();
#ifdef Q_OS_WIN32
    m_scriptEncoding = "GBK";
#else
    m_scriptEncoding = "UTF-8";
#endif

    m_scriptLanguage.clear();
    m_resourceUrl.clear();
    m_userAgent = "Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/538.1 (KHTML, like Gecko) Bradypod/1 Safari/538.1";
    m_unknownOption.clear();
    m_versionFlag = false;
    m_debug = false;
    m_remoteDebugPort = 0;
    m_remoteDebugAutorun = false;
    m_webSecurityEnabled = false;
    m_javascriptEnabled = true;
    m_javaEnabled = true;   // not support yet
    m_javascriptCanOpenWindows = false;
    m_javascriptCanCloseWindows = false;
    m_resourceTimeout = 8*1000;
    m_helpFlag = false;
    m_printDebugMessages = false;
    m_sslProtocol = "any";
    // Default taken from Chromium 35.0.1916.153
    m_sslCiphers = ("ECDHE-ECDSA-AES128-GCM-SHA256"
                    ":ECDHE-RSA-AES128-GCM-SHA256"
                    ":DHE-RSA-AES128-GCM-SHA256"
                    ":ECDHE-ECDSA-AES256-SHA"
                    ":ECDHE-ECDSA-AES128-SHA"
                    ":ECDHE-RSA-AES128-SHA"
                    ":ECDHE-RSA-AES256-SHA"
                    ":ECDHE-ECDSA-RC4-SHA"
                    ":ECDHE-RSA-RC4-SHA"
                    ":DHE-RSA-AES128-SHA"
                    ":DHE-DSS-AES128-SHA"
                    ":DHE-RSA-AES256-SHA"
                    ":AES128-GCM-SHA256"
                    ":AES128-SHA"
                    ":AES256-SHA"
                    ":DES-CBC3-SHA"
                    ":RC4-SHA"
                    ":RC4-MD5");
    m_sslCertificatesPath.clear();
    m_sslClientCertificateFile.clear();
    m_sslClientKeyFile.clear();
    m_sslClientKeyPassphrase.clear();
    m_webdriverIp = QString();
    m_webdriverPort = QString();
    m_webdriverLogFile = QString();
    m_webdriverLogLevel = "INFO";
    m_webdriverSeleniumGridHub = QString();
}

void Config::setProxyAuthPass(const QString& value)
{
    m_proxyAuthPass = value;
}

void Config::setProxyAuthUser(const QString& value)
{
    m_proxyAuthUser = value;
}

void Config::setProxyHost(const QString& value)
{
    m_proxyHost = value;
}

void Config::setProxyPort(const int value)
{
    m_proxyPort = value;
}

bool Config::helpFlag() const
{
    return m_helpFlag;
}

void Config::setHelpFlag(const bool value)
{
    m_helpFlag = value;
}

bool Config::printDebugMessages() const
{
    return m_printDebugMessages;
}

void Config::setPrintDebugMessages(const bool value)
{
    m_printDebugMessages = value;
}

void Config::handleSwitch(const QString& sw)
{
    setHelpFlag(sw == "help");
    setVersionFlag(sw == "version");
}

void Config::handleOption(const QString& option, const QVariant& value)
{
    bool boolValue = false;

    QStringList booleanFlags;
    booleanFlags << "headers-attach-to-per-request";
    booleanFlags << "debug";
    booleanFlags << "disk-cache";
    booleanFlags << "ignore-ssl-errors";
    booleanFlags << "load-images";
    booleanFlags << "only-load-first-request";
    booleanFlags << "local-url-access";
    booleanFlags << "local-to-remote-url-access";
    booleanFlags << "Remote-Debugger-Autorun";
    booleanFlags << "web-security";
    booleanFlags << "javascript-enable";
    booleanFlags << "java-enable";
    if (booleanFlags.contains(option)) {
        if ((value != "true") && (value != "yes") && (value != "false") && (value != "no")) {
            setUnknownOption(QString("Invalid values for '%1' option.").arg(option));
            return;
        }
        boolValue = (value == "true") || (value == "yes");
    }

    if (option == "url") {
        setResourceUrl(value.toString());
    } else if (option == "method") {
        setMethod(value.toString());
    } else if (option == "body") {
        setBody(value.toString());
    } else if (option == "body-encoding") {
        setBodyEncoding(value.toString());
    } else if (option == "header") {
        QString header = value.toString();
        int index = header.indexOf(':');
        if (index > 0)
            addHeader(header.mid(0,index).trimmed(),header.mid(index+1).trimmed());
    } else if (option == "headers-attach-to-per-request") {
        setHeadersToPerRequest(boolValue);
    } else if (option == "user-agent") {
        setUserAgent(value.toString());
    } else if (option == "cookies") {
        setCookies(value.toString());
    } else if (option == "cookiejar") {
        setCookiejarData(value.toString());
    } else if (option == "cookies-file") {
        setCookiesFile(value.toString());
    } else if (option == "block-ip-and-domain") {
        QStringList domainIPs = value.toString().split(";");
        foreach (QString dominIP, domainIPs) {
            addBlockIpAndDomain(dominIP);
        }
    } else if (option == "config") {
        setConfigFile(value.toString());
    } else if (option == "debug") {
        setDebug(boolValue);
        setPrintDebugMessages(boolValue);
    } else if (option == "disk-cache") {
        setDiskCacheEnabled(boolValue);
    } else if (option == "disk-cache-path") {
        setDiskCachePath(value.toString());
    } else if (option == "ignore-ssl-errors") {
        setIgnoreSslErrors(boolValue);
    } else if (option == "load-images") {
        setAutoLoadImages(boolValue);
    } else if (option == "wait-window-onload-timeout") {
        setWaitAfterWindowOnload(value.toInt());
    } else if (option == "render-image-path") {
        setRenderImagePath(value.toString());
    } else if (option == "only-load-first-request") {
        setOnlyLoadFirstRequest(boolValue);
    } else if (option == "local-storage-path") {
        setLocalStoragePath(value.toString());
    } else if (option == "local-storage-quota") {
        setLocalStorageDefaultQuota(value.toInt());
    } else if (option == "offline-storage-path") {
        setOfflineStoragePath(value.toString());
    } else if (option == "offline-storage-quota") {
        setOfflineStorageDefaultQuota(value.toInt());
    } else if (option == "local-url-access") {
        setLocalUrlAccessEnabled(boolValue);
    } else if (option == "local-to-remote-url-access") {
        setLocalToRemoteUrlAccessEnabled(boolValue);
    } else if (option == "max-disk-cache-size") {
        setMaxDiskCacheSize(value.toInt());
    } else if (option == "output-encoding") {
        setOutputEncoding(value.toString());
    } else if (option == "output") {
        setOutputFile(value.toString());
    } else if (option == "output-format") {
        setOutputFormat(value.toString());
    } else if (option == "remote-debugger-autorun") {
        setRemoteDebugAutorun(boolValue);
    } else if (option == "remote-debugger-port") {
        setDebug(true);
        setRemoteDebugPort(value.toInt());
    } else if (option == "proxy") {
        setProxy(value.toString());
    } else if (option == "proxy-type") {
        setProxyType(value.toString());
    } else if (option == "proxy-auth") {
        setProxyAuth(value.toString());
    } else if (option == "resource-timeout") {
        setResourceTimeout(value.toInt());
    } else if (option == "script-encoding") {
        setScriptEncoding(value.toString());
    } else if (option == "script-language") {
        setScriptLanguage(value.toString());
    } else if (option == "web-security") {
        setWebSecurityEnabled(boolValue);
    }  else if (option == "javascript-enable") {
        setJavascriptEnabled(boolValue);
    }  else if (option == "java-enable") {
        setJavaEnabled(boolValue);
    } else if (option == "ssl-protocol") {
        setSslProtocol(value.toString());
    } else if (option == "ssl-ciphers") {
        setSslCiphers(value.toString());
    } else if (option == "ssl-certificates-path") {
        setSslCertificatesPath(value.toString());
    } else if (option == "ssl-client-certificate-file") {
        setSslClientCertificateFile(value.toString());
    } else if (option == "ssl-client-key-file") {
        setSslClientKeyFile(value.toString());
    } else if (option == "ssl-client-key-passphrase") {
        setSslClientKeyPassphrase(value.toByteArray());
    }
}

void Config::handleParam(const QString& param, const QVariant& value)
{
    Q_UNUSED(param);

    if (param == "url") {
        m_resourceUrl = value.toString();
    }
}

void Config::handleError(const QString& error)
{
    setUnknownOption(QString("Error: %1").arg(error));
}

QString Config::sslProtocol() const
{
    return m_sslProtocol;
}

void Config::setSslProtocol(const QString& sslProtocolName)
{
    m_sslProtocol = sslProtocolName.toLower();
}

QString Config::sslCiphers() const
{
    return m_sslCiphers;
}

void Config::setSslCiphers(const QString& sslCiphersName)
{
    // OpenSSL cipher strings are case sensitive.
    m_sslCiphers = sslCiphersName;
}

QString Config::sslCertificatesPath() const
{
    return m_sslCertificatesPath;
}

void Config::setSslCertificatesPath(const QString& sslCertificatesPath)
{
    QFileInfo sslPathInfo = QFileInfo(sslCertificatesPath);
    if (sslPathInfo.isDir()) {
        if (sslCertificatesPath.endsWith('/')) {
            m_sslCertificatesPath = sslCertificatesPath + "*";
        } else {
            m_sslCertificatesPath = sslCertificatesPath + "/*";
        }
    } else {
        m_sslCertificatesPath = sslCertificatesPath;
    }
}

QString Config::sslClientCertificateFile() const
{
    return m_sslClientCertificateFile;
}

void Config::setSslClientCertificateFile(const QString& sslClientCertificateFile)
{
    m_sslClientCertificateFile = sslClientCertificateFile;
}

QString Config::sslClientKeyFile() const
{
    return m_sslClientKeyFile;
}

void Config::setSslClientKeyFile(const QString& sslClientKeyFile)
{
    m_sslClientKeyFile = sslClientKeyFile;
}

QByteArray Config::sslClientKeyPassphrase() const
{
    return m_sslClientKeyPassphrase;
}

void Config::setSslClientKeyPassphrase(const QByteArray& sslClientKeyPassphrase)
{
    m_sslClientKeyPassphrase = sslClientKeyPassphrase;
}
