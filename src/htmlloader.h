#ifndef HTMLLOADER_H
#define HTMLLOADER_H

#include <QObject>
#include <QWebElement>
#include <QWebElementCollection>
#include <QTimer>

#include "webpage.h"
#include "bradypod.h"
#include "consts.h"
#include "domparser.h"


class HtmlLoader : public QObject
{
    Q_OBJECT
public:
    explicit HtmlLoader(QObject *parent, WebPage *page);
    void loadUrl(const QUrl& url);
    void loadUrl(const QString& url);
    WebPage* webpage(void);

    QString getHtmlContent() const;

signals:
    void finished();

public slots:
    void on_resourceRequested(const QVariant& data, QObject* jsNetworkRequest);
    void on_resourceReceived(const QVariant& data);
    void on_resourceError(const QVariant& data);
    void on_resourceTimeout(const QVariant& data);
#if QT_VERSION >= QT_VERSION_CHECK(5, 6, 0)
    void on_resourceRedirect(const QVariant& data);
#endif

    void on_loadFinished(const QString& arg1);
    void on_renderFinished();
    void on_loadProgress(int progress);
    void on_loadStarted();
    void on_statusBarMessage(const QString &text);
    void on_urlChanged(const QString &arg1);
    void on_windowTitleChanged(const QString &title);

    void on_parsedLink(const QVariant& data);

    void on_javaScriptConsoleMessageSent(const QString& message);
    void on_javaScriptErrorSent(const QString& message);
    void on_javaScriptErrorSent(const QString& msg, int lineNumber, const QString& sourceID, const QString& stack);

private:
    WebPage* m_webpage;
    Bradypod* m_bradypod;
    DOMParser* m_domparser;
    QString m_html;
};

#endif // HTMLLOADER_H
