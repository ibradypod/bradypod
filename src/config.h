﻿#ifndef CONFIG_H
#define CONFIG_H

#include <QNetworkProxy>
#include <QVariant>
#include <QRegExp>

class QCommandLine;

class Config: public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString cookies READ cookies WRITE setCookies)
    Q_PROPERTY(QString cookiesFile READ cookiesFile WRITE setCookiesFile)
    Q_PROPERTY(bool autoLoadImages READ autoLoadImages WRITE setAutoLoadImages)
    Q_PROPERTY(QString renderImagePath READ renderImagePath WRITE setRenderImagePath)
    Q_PROPERTY(bool onlyLoadFirstRequest READ onlyLoadFirstRequest WRITE setOnlyLoadFirstRequest)
    Q_PROPERTY(int maxDiskCacheSize READ maxDiskCacheSize WRITE setMaxDiskCacheSize)
    Q_PROPERTY(QString diskCachePath READ diskCachePath WRITE setDiskCachePath)
    Q_PROPERTY(bool ignoreSslErrors READ ignoreSslErrors WRITE setIgnoreSslErrors)
    Q_PROPERTY(bool localUrlAccessEnabled READ localUrlAccessEnabled WRITE setLocalUrlAccessEnabled)
    Q_PROPERTY(bool localToRemoteUrlAccessEnabled READ localToRemoteUrlAccessEnabled WRITE setLocalToRemoteUrlAccessEnabled)
    Q_PROPERTY(QString outputEncoding READ outputEncoding WRITE setOutputEncoding)
    Q_PROPERTY(QString outputFile READ outputFile WRITE setOutputFile)
    Q_PROPERTY(QString outputFormat READ outputFormat WRITE setOutputFormat)
    Q_PROPERTY(QString proxyType READ proxyType WRITE setProxyType)
    Q_PROPERTY(QString proxy READ proxy WRITE setProxy)
    Q_PROPERTY(QString proxyAuth READ proxyAuth WRITE setProxyAuth)
    Q_PROPERTY(QString scriptEncoding READ scriptEncoding WRITE setScriptEncoding)
    Q_PROPERTY(bool webSecurityEnabled READ webSecurityEnabled WRITE setWebSecurityEnabled)
    Q_PROPERTY(QString offlineStoragePath READ offlineStoragePath WRITE setOfflineStoragePath)
    Q_PROPERTY(QString localStoragePath READ localStoragePath WRITE setLocalStoragePath)
    Q_PROPERTY(int localStorageDefaultQuota READ localStorageDefaultQuota WRITE setLocalStorageDefaultQuota)
    Q_PROPERTY(int offlineStorageDefaultQuota READ offlineStorageDefaultQuota WRITE setOfflineStorageDefaultQuota)
    Q_PROPERTY(bool printDebugMessages READ printDebugMessages WRITE setPrintDebugMessages)
    Q_PROPERTY(bool javascriptEnabled READ javascriptEnabled WRITE setJavascriptEnabled)
    Q_PROPERTY(bool javaEnabled READ javaEnabled WRITE setJavaEnabled)
    Q_PROPERTY(bool javascriptCanOpenWindows READ javascriptCanOpenWindows WRITE setJavascriptCanOpenWindows)
    Q_PROPERTY(bool javascriptCanCloseWindows READ javascriptCanCloseWindows WRITE setJavascriptCanCloseWindows)
    Q_PROPERTY(int waitAfterWindowOnload READ waitAfterWindowOnload WRITE setWaitAfterWindowOnload)
    Q_PROPERTY(int resourceTimeout READ resourceTimeout WRITE setResourceTimeout)
    Q_PROPERTY(QString userAgent READ userAgent WRITE setUserAgent)
    Q_PROPERTY(QString sslProtocol READ sslProtocol WRITE setSslProtocol)
    Q_PROPERTY(QString sslCiphers READ sslCiphers WRITE setSslCiphers)
    Q_PROPERTY(QString sslCertificatesPath READ sslCertificatesPath WRITE setSslCertificatesPath)
    Q_PROPERTY(QString sslClientCertificateFile READ sslClientCertificateFile WRITE setSslClientCertificateFile)
    Q_PROPERTY(QString sslClientKeyFile READ sslClientKeyFile WRITE setSslClientKeyFile)
    Q_PROPERTY(QByteArray sslClientKeyPassphrase READ sslClientKeyPassphrase WRITE setSslClientKeyPassphrase)

public:
    Config(QObject* parent = 0);

    void init(const QStringList& args);
    void processArgs(const QStringList& args);
    void loadJsonFile(const QString& filePath);

    QString helpText() const;

    QVariantMap getOperation() const;
    void setOperation(QString method="GET", QString body="",const QVariantMap& header=QVariantMap(),QString encoding="latin");
    void setMethod(QString method);
    void setBody(QString body);
    void setBodyEncoding(QString encoding);
    void setHeaders(const QVariantMap & header);
    void addHeader(const QString& name, const QString& value);
    void setHeadersToPerRequest(const bool value);

    bool autoLoadImages() const;
    void setAutoLoadImages(const bool value);

    QString renderImagePath() const;
    void setRenderImagePath(const QString& value);

    bool onlyLoadFirstRequest() const;
    void setOnlyLoadFirstRequest(const bool value);

    int waitAfterWindowOnload() const;
    void setWaitAfterWindowOnload(const int millisecond);

    bool isBlockedIpDomain(const QString& domain) const;
    bool hasSetBlockDomain() const;
    QList<QRegExp> blockedIpAndDomains() const;
    void addBlockIpAndDomain(const QString& value);

    QString configFile() const;
    void setConfigFile(const QString& value);

    QString cookies() const;
    void setCookies(const QString& cookies);

    QVariantList cookiejarData() const;
    void setCookiejarData(const QString& cookiejar);

    QString cookiesFile() const;
    void setCookiesFile(const QString& cookiesFile);

    QString offlineStoragePath() const;
    void setOfflineStoragePath(const QString& value);

    int offlineStorageDefaultQuota() const;
    void setOfflineStorageDefaultQuota(int offlineStorageDefaultQuota);

    QString localStoragePath() const;
    void setLocalStoragePath(const QString& value);

    int localStorageDefaultQuota() const;
    void setLocalStorageDefaultQuota(int localStorageDefaultQuota);

    bool diskCacheEnabled() const;
    void setDiskCacheEnabled(const bool value);

    int maxDiskCacheSize() const;
    void setMaxDiskCacheSize(int maxDiskCacheSize);

    QString diskCachePath() const;
    void setDiskCachePath(const QString& value);

    bool ignoreSslErrors() const;
    void setIgnoreSslErrors(const bool value);

    bool localUrlAccessEnabled() const;
    void setLocalUrlAccessEnabled(const bool value);

    bool allowNetworkAccess() const;
    void setAllowNetworkAccess(const bool value);

    bool localToRemoteUrlAccessEnabled() const;
    void setLocalToRemoteUrlAccessEnabled(const bool value);

    QString outputEncoding() const;
    void setOutputEncoding(const QString& value);

    QString outputFile() const;
    void setOutputFile(const QString& value);

    QString outputFormat() const;
    void setOutputFormat(const QString& value);

    QString proxyType() const;
    void setProxyType(const QString& value);

    QString proxy() const;
    void setProxy(const QString& value);
    QString proxyHost() const;
    int proxyPort() const;

    QString proxyAuth() const;
    void setProxyAuth(const QString& value);
    QString proxyAuthUser() const;
    QString proxyAuthPass() const;
    void setProxyAuthUser(const QString& value);
    void setProxyAuthPass(const QString& value);

    QStringList scriptArgs() const;
    void setScriptArgs(const QStringList& value);

    QString scriptEncoding() const;
    void setScriptEncoding(const QString& value);

    QString scriptLanguage() const;
    void setScriptLanguage(const QString& value);

    QString resourceUrl() const;
    void setResourceUrl(const QString& value);

    QString userAgent() const;
    void setUserAgent(const QString& value);

    QString unknownOption() const;
    void setUnknownOption(const QString& value);

    bool versionFlag() const;
    void setVersionFlag(const bool value);

    void setDebug(const bool value);
    bool debug() const;

    void setRemoteDebugPort(const int port);
    int remoteDebugPort() const;

    void setRemoteDebugAutorun(const bool value);
    bool remoteDebugAutorun() const;

    bool webSecurityEnabled() const;
    void setWebSecurityEnabled(const bool value);

    bool helpFlag() const;
    void setHelpFlag(const bool value);

    void setPrintDebugMessages(const bool value);
    bool printDebugMessages() const;

    bool javascriptEnabled() const;
    void setJavascriptEnabled(const bool value);

    bool javaEnabled() const;
    void setJavaEnabled(const bool value);

    void setJavascriptCanOpenWindows(const bool value);
    bool javascriptCanOpenWindows() const;

    void setJavascriptCanCloseWindows(const bool value);
    bool javascriptCanCloseWindows() const;

    void setResourceTimeout(const int value);
    int resourceTimeout() const;

    void setSslProtocol(const QString& sslProtocolName);
    QString sslProtocol() const;

    void setSslCiphers(const QString& sslCiphersName);
    QString sslCiphers() const;

    void setSslCertificatesPath(const QString& sslCertificatesPath);
    QString sslCertificatesPath() const;

    void setSslClientCertificateFile(const QString& sslClientCertificateFile);
    QString sslClientCertificateFile() const;

    void setSslClientKeyFile(const QString& sslClientKeyFile);
    QString sslClientKeyFile() const;

    void setSslClientKeyPassphrase(const QByteArray& sslClientKeyPassphrase);
    QByteArray sslClientKeyPassphrase() const;

public slots:
    void handleSwitch(const QString& sw);
    void handleOption(const QString& option, const QVariant& value);
    void handleParam(const QString& param, const QVariant& value);
    void handleError(const QString& error);

private:
    void resetToDefaults();
    void setProxyHost(const QString& value);
    void setProxyPort(const int value);

    QCommandLine* m_cmdLine;
    QVariantMap m_operation;
    QList<QRegExp> m_blockIpAndDomains;
    bool m_autoLoadImages;
    QString m_renderImagePath;
    int m_waitAfterWindowOnload;
    bool m_only_load_first_request;
    QString m_configFile;
    QString m_cookies;
    QVariantList m_cookiejar;
    QString m_cookiesFile;
    QString m_offlineStoragePath;
    int m_offlineStorageDefaultQuota;
    QString m_localStoragePath;
    int m_localStorageDefaultQuota;
    bool m_diskCacheEnabled;
    int m_maxDiskCacheSize;
    QString m_diskCachePath;
    bool m_ignoreSslErrors;
    bool m_localUrlAccessEnabled;
    bool m_localToRemoteUrlAccessEnabled;
    bool m_allowNetworkAccess;
    QString m_outputEncoding;
    QString m_outputFile;
    QString m_outputFormat;
    QString m_proxyType;
    QString m_proxyHost;
    int m_proxyPort;
    QString m_proxyAuthUser;
    QString m_proxyAuthPass;
    QStringList m_scriptArgs;
    QString m_scriptEncoding;
    QString m_scriptLanguage;
    QString m_resourceUrl;
    QString m_userAgent;
    QString m_unknownOption;
    bool m_versionFlag;
    QString m_authUser;
    QString m_authPass;
    bool m_debug;
    int m_remoteDebugPort;
    bool m_remoteDebugAutorun;
    bool m_webSecurityEnabled;
    bool m_helpFlag;
    bool m_printDebugMessages;
    bool m_javascriptEnabled;
    bool m_javaEnabled;
    bool m_javascriptCanOpenWindows;
    bool m_javascriptCanCloseWindows;
    int m_resourceTimeout;
    QString m_sslProtocol;
    QString m_sslCiphers;
    QString m_sslCertificatesPath;
    QString m_sslClientCertificateFile;
    QString m_sslClientKeyFile;
    QByteArray m_sslClientKeyPassphrase;
    QString m_webdriverIp;
    QString m_webdriverPort;
    QString m_webdriverLogFile;
    QString m_webdriverLogLevel;
    QString m_webdriverSeleniumGridHub;
};

#endif // CONFIG_H
