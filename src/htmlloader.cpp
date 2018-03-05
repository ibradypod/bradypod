#include "htmlloader.h"
#include "bradypod.h"
#include "terminal.h"

#include <QJsonObject>
#include <QJsonDocument>
#include <QDebug>

using namespace QtPrivate;

HtmlLoader::HtmlLoader(QObject *parent, WebPage *page)
    : QObject(parent)
    , m_webpage(0)
    , m_domparser(0)
{
    m_bradypod = Bradypod::instance();
    if (page)
        m_webpage = page;
    else
        m_webpage = static_cast<WebPage*>(m_bradypod->page());
    m_webpage->setWaitAfterWindowOnloadTime(m_bradypod->config()->waitAfterWindowOnload());

    connect(m_webpage,SIGNAL(resourceRequested(QVariant,QObject*)),SLOT(on_resourceRequested(QVariant,QObject*)));
    connect(m_webpage,SIGNAL(resourceReceived(QVariant)),SLOT(on_resourceReceived(QVariant)));
    connect(m_webpage,SIGNAL(resourceError(QVariant)),SLOT(on_resourceError(QVariant)));
    connect(m_webpage,SIGNAL(resourceTimeout(QVariant)),SLOT(on_resourceTimeout(QVariant)));
#if QT_VERSION >= QT_VERSION_CHECK(5, 6, 0)
    connect(m_webpage,SIGNAL(resourceRedirect(QVariant)),SLOT(on_resourceRedirect(QVariant)));
#endif
    connect(m_webpage,SIGNAL(javaScriptConsoleMessageSent(QString)),SLOT(on_javaScriptConsoleMessageSent(QString)));

    connect(m_webpage,SIGNAL(loadStarted()),SLOT(on_loadStarted()));
    connect(m_webpage,SIGNAL(loadFinished(QString)),SLOT(on_loadFinished(QString)));
    connect(m_webpage,SIGNAL(urlChanged(QString)),SLOT(on_urlChanged(QString)));

    m_domparser = new DOMParser(this,m_webpage);
    connect(m_domparser,SIGNAL(parsedLinks(QVariant)),SLOT(on_parsedLink(QVariant)));
}

void HtmlLoader::loadUrl(const QUrl& url)
{
    loadUrl(url.toString());
}

void HtmlLoader::loadUrl(const QString& url)
{
    m_webpage->openUrl(url,m_bradypod->config()->getOperation(),m_bradypod->defaultPageSettings());
}

WebPage* HtmlLoader::webpage(void)
{
    return m_webpage;
}

QString HtmlLoader::getHtmlContent() const
{
    return m_html;
}

#define printResource(data) qDebug()<<BLUE<<__FUNCTION__<<" :: "<<NONE<<QJsonDocument::fromVariant(data).toJson(QJsonDocument::Indented)

void HtmlLoader::on_resourceRequested(const QVariant& data, QObject* jsNetworkRequest)
{
    (void)jsNetworkRequest;
    printResource(data);
    m_bradypod->addParsedData(data);
}

void HtmlLoader::on_resourceReceived(const QVariant& data)
{
    printResource(data);
    m_bradypod->addParsedData(data);
}

void HtmlLoader::on_resourceError(const QVariant& data)
{
    (void)data;
    printResource(data);
    m_bradypod->addParsedData(data);
}

void HtmlLoader::on_resourceTimeout(const QVariant& data)
{
    printResource(data);
    m_bradypod->addParsedData(data);
}

#if QT_VERSION >= QT_VERSION_CHECK(5, 6, 0)
void HtmlLoader::on_resourceRedirect(const QVariant& data)
{
    printResource(data);
    m_bradypod->addParsedData(data);
}
#endif

void HtmlLoader::on_loadFinished(const QString& status)
{
    static long int count = 0;
    if (count++ > 0) {
        qDebug()<<tr("COUNT:: %1 :: LOAD FINISHED: %2\n").arg(QString::number(count),status);
        return;
    }
    qDebug()<<(tr("LOAD FINISHED: %1\n").arg(status));

//    QEventLoop eventloop;
//    QTimer::singleShot(msec, &eventloop, SLOT(quit()));
//    eventloop.exec();

//    QTime dieTime = QTime::currentTime().addMSecs(m_bradypod->config()->waitAfterWindowOnload());
//    while(QTime::currentTime() < dieTime) {
//        QCoreApplication::processEvents(QEventLoop::AllEvents, 100);
//    }

    m_bradypod->config()->setAllowNetworkAccess(false);

    QString text = m_webpage->mainFrame()->toHtml();
    m_html = text;

    on_renderFinished();
}

void HtmlLoader::on_renderFinished()
{
    QString image_path = m_bradypod->config()->renderImagePath();

    if (!image_path.isEmpty())
        m_webpage->render(image_path);

    m_domparser->parse_traversal_dom();

    m_bradypod->exit(1);
}

void HtmlLoader::on_loadProgress(int progress)
{
    qDebug()<<(tr("%1 %").arg(progress));
}

void HtmlLoader::on_loadStarted()
{
    qDebug()<<(tr("Loading..."));
}

void HtmlLoader::on_statusBarMessage(const QString &text)
{
    qDebug()<<("statusMessage :: " + text);
}

void HtmlLoader::on_urlChanged(const QString &url)
{
    qDebug()<<url;
}

void HtmlLoader::on_windowTitleChanged(const QString &title)
{
     qDebug()<<(QString("Title :: %1").arg(title));
}

void HtmlLoader::on_javaScriptConsoleMessageSent(const QString& message)
{
    qDebug()<<"\t"<<RED<<__FUNCTION__<<" :: "<<NONE<<message;
}

void HtmlLoader::on_javaScriptErrorSent(const QString& message)
{
    qDebug()<<"\t"<<RED<<__FUNCTION__<<" :: "<<NONE<<message;
}

void HtmlLoader::on_javaScriptErrorSent(const QString& msg, int lineNumber, const QString& sourceID, const QString& stack)
{
    qDebug()<<"\t"<<RED<<__FUNCTION__<<" :: "<<NONE<<msg<<lineNumber<<sourceID<<stack;
}

void HtmlLoader::on_parsedLink(const QVariant& data)
{
    QByteArray json = QJsonDocument::fromVariant(data).toJson(QJsonDocument::Indented);
    qDebug()<<"\t"<<BLUE<<"on_parsedLink :: "<<NONE<<json;
    m_bradypod->addParsedData(data);
}
