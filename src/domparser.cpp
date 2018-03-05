#include "domparser.h"
#include "terminal.h"

#include <QDebug>
#include <QCoreApplication>
#include <QEventLoop>
#include <QCryptographicHash>
#include <QRegularExpression>
#include <QtWebKitWidgets>

// webkit private api
#include <private/qwebviewaccessible_p.h>
#include <private/qwebelement_p.h>
//#include "Element.h"

using namespace WebCore;

static const QStringList tags_base = QStringLiteral("html title body h1 h2 h3 h4 h5 h6 p br hr")
        .split(" ", QString::SkipEmptyParts);

static const QStringList tags_format = QStringLiteral("acronym abbr address b bdi bdo big blockquote center cite code del dfn em font i ins kbd mark meter pre progress q rp rt ruby s samp small strike strong sup sub time tt u var wbr")
        .split(" ", QString::SkipEmptyParts);

static const QStringList tags_form = QStringLiteral("form").split(" ", QString::SkipEmptyParts);
static const QStringList tags_form_element = QStringLiteral("input textarea button select optgroup option label fieldset legend isindex datalist keygen output").split(" ", QString::SkipEmptyParts);

static const QStringList tags_frame = QStringLiteral("frame frameset noframes iframe")
        .split(" ", QString::SkipEmptyParts);

static const QStringList tags_image = QStringLiteral("img map area canvas figcaption figure")
        .split(" ", QString::SkipEmptyParts);

static const QStringList tags_media = QStringLiteral("audio source track video")
        .split(" ", QString::SkipEmptyParts);

static const QStringList tags_hyperlink = QStringLiteral("a link nav")
        .split(" ", QString::SkipEmptyParts);

static const QStringList tags_list = QStringLiteral("ul ol li dir dl dt dd menu menuitem command")
        .split(" ", QString::SkipEmptyParts);

static const QStringList tags_table = QStringLiteral("table caption th tr td thead tbody tfoot col colgroup")
        .split(" ", QString::SkipEmptyParts);

static const QStringList tags_section = QStringLiteral("style div span header footer section article aside details dialog summary details")
        .split(" ", QString::SkipEmptyParts);

static const QStringList tags_meta = QStringLiteral("head meta base basefont")
        .split(" ", QString::SkipEmptyParts);

static const QStringList tags_script = QStringLiteral("script noscript applet embed object param")
        .split(" ", QString::SkipEmptyParts);

static inline QString elide(const QString& str, int max_len = 300)
{
    return str.length() <= max_len ? str : str.left(150) + "  ...  " + str.right(150);
}


#define printSimpleElement(element) qDebug()<<GREEN<<__FUNCTION__ <<" tag :: "<<element.tagName().trimmed()<<NONE<<" XML:: "<<elide(element.toOuterXml().trimmed())


DOMParser::DOMParser(QObject *parent, WebPage* webpage) : QObject(parent)
{
    m_webpage = webpage;
    m_webEelement = webpage->mainFrame()->documentElement();
}

void DOMParser::_traversal_dom(QWebElement &element)
{
    if (element.isNull()) {
        return;
    } else {
        qDebug()<<YELLOW<<__FUNCTION__ <<" tag :: "<<element.tagName().trimmed()<<NONE;
        // printSimpleElement(element);
        // parse `element`
        parse_element(element);
        // child
        QWebElement child = element.firstChild();
        _traversal_dom(child);

        // sibling
        QWebElement sibling = child.nextSibling();
        while(!sibling.isNull()) {
            _traversal_dom(sibling);
            sibling = sibling.nextSibling();
        }
    }
}

void DOMParser::parse_traversal_dom()
{
    qDebug()<<GREEN<<"BODY_LENGTH::"<<m_webpage->mainFrame()->toHtml().length()<<NONE<<"\n\n";
    QWebElement element = m_webpage->mainFrame()->documentElement();
//    qDebug()<<"document.readyState"<<m_webpage->mainFrame()->evaluateJavaScript("document.readyState;");
    // parser start
    _traversal_dom(element);
}

void DOMParser::parse_element(QWebElement& element)
{
    QString tag = element.tagName().toLower();
    if (tags_base.contains(tag))                // base
        handle_tag_bases(element);
    else if (tags_format.contains(tag))         // format
        handle_tag_formats(element);
    else if (tags_form.contains(tag))           // form
        handle_tag_forms(element);
    else if (tags_form_element.contains(tag))   // form element
        (void)element;  // ignore
    else if (tags_frame.contains(tag))          // frame
        handle_tag_frames(element);
    else if (tags_image.contains(tag))          // image
        handle_tag_images(element);
    else if (tags_media.contains(tag))          // media
        handle_tag_media(element);
    else if (tags_hyperlink.contains(tag))      // hyperlinks
        handle_tag_hyperlinks(element);
    else if (tags_list.contains(tag))           // list
        handle_tag_list(element);
    else if (tags_table.contains(tag))          // table
        handle_tag_table(element);
    else if (tags_section.contains(tag))        // section
        handle_tag_sections(element);
    else if (tags_meta.contains(tag))           // meta
        handle_tag_meta(element);
    else if (tags_script.contains(tag))         // script
        handle_tag_script(element);
    else                                        // other
        handle_tag_other(element);
}

void DOMParser::emulate_click(QWebElement &element,QString jscode)
{
    static long int count = 1;

    qDebug()<<RED<<"Special operation count: "<<NONE<< count++;
//    printSimpleElement(element);

    if (element.tagName().compare("form",Qt::CaseInsensitive) == 0 ) {
        // TODO: form click
    } else {
        element.evaluateJavaScript(jscode);
    }
}

static QByteArray toDigest(const QString& data)
{
    return QCryptographicHash::hash(data.toUtf8(), QCryptographicHash::Sha512).toHex();
}

void DOMParser::submit_uri(const QString& uri, const QWebElement& element, const QString& method, const QVariantMap& body)
{
    static long int count = 0;
    QVariantMap result;
    QString submit_method = method.length() > 0 ? method : element.attribute("method").trimmed();
    submit_method = submit_method.length() > 0 ? submit_method : "GET";
    QString mime_type = element.attribute("type").trimmed();

    QByteArray hash = toDigest(uri+"-method-"+method+"-mime_type-"+mime_type);

    if (!m_rescheduling.contains(hash)) {
        m_rescheduling.insert(hash);

        count ++;
        result["id"] = "dom_parser_"+QString::number(count);
        result["type"] = "dom_parser";
        result["uri"] = uri;
        result["method"] = submit_method;
        if (!body.isEmpty()) {
            result["body"] = body;
        }
        result["mime_type"] = mime_type;
        result["tag_name"] = element.tagName();
        result["xml"] = element.toOuterXml().trimmed();

        emit parsedLinks(result);
    }
}

/* 标签: 基础
 * html title body h1 ... h6 p br hr
 */
void DOMParser::handle_tag_bases(QWebElement& element)
{
    (void)element;
    // ignore
}

/* 标签: 格式
 * acronym abbr address b bdi bdo big blockquote center cite code del dfn em font i ins kbd mark
 * meter pre progress q rp rt ruby s samp small strike strong sup sub time tt u var wbr
 */
void DOMParser::handle_tag_formats(QWebElement& element)
{
    (void)element;
    // ignore
}

static QVariantMap getFormInputAttr(const QWebElement& element, const QString& name)
{
    QVariantMap res;

    foreach (QString name, element.attributeNames()) {
        res[name] = element.attribute(name);
    }
    res["tagName"] = element.tagName();
    if (!element.hasAttribute("name")) {
        res["name"] = name;
    }
    return res;
}

static void traverseFormDom(const QWebElement& parent, const QWebElement& element ,
                            QVariantMap& result, QString name=QString())
{
    static const QStringList form_parse_tags =
            QStringList()<<"input"<<"textarea"<<"button"<<"select"<<"optgroup"<<"option"<<"output"<<"datalist"<<"keygen";
    static const QStringList form_submit_tags = QStringList()<<"input"<<"textarea"<<"button"<<"option";
    if (element.isNull()) {
        return;
    }
    printSimpleElement(element);
    if (parent != element) {
        if (element.tagName().compare("FORM",Qt::CaseInsensitive) == 0) {
            return;
        }
    }
    // process current element
    QString tagName = element.tagName().toLower();
    if (form_parse_tags.contains(tagName,Qt::CaseInsensitive) && element.hasAttribute("name")) {
        name = element.attribute("name");
        if (tagName == "datalist") {
            name = element.attribute("id");
        }
    }
    if (form_submit_tags.contains(tagName,Qt::CaseInsensitive)) {
        // submit
        if (!result.contains(name)) {
            result[name] = QVariantList();
        }
        QVariantList values = result[name].toList();
        values.append(getFormInputAttr(element,name));
        result[name] = values;
    }

    // child
    QWebElement child = element.firstChild();
    traverseFormDom(element,child,result,name);

    // sibling
    QWebElement sibling = child.nextSibling();
    while(!sibling.isNull()) {
        traverseFormDom(element,sibling,result,name);
        sibling = sibling.nextSibling();
    }

}

static QVariantMap staticParserForm(QWebElement& element)
{
    QVariantMap result;
    traverseFormDom(element,element,result);
    return result;
}

static QVariantMap dynamicParserForm(QWebElement& element)
{
    // TODO:
    (void)element;
    return QVariantMap();
}


/* 标签: 表单
 * form input textarea button select optgroup option label fieldset legend isindex datalist keygen output
 */
void DOMParser::handle_tag_forms(QWebElement& element)
{
    // TODO:
    printSimpleElement(element);
    QString result = element.evaluateJavaScript("this.action").toString().trimmed();
    qDebug()<<"Form: action: "<<result;

    // extra url
    if (!result.isEmpty() && QUrl(result).isValid()) {
        submit_uri(result,element,"GET");
    }
    // static parser
    QVariantMap static_body = staticParserForm(element);
    if (!static_body.isEmpty()) {
        submit_uri(result,element,"",static_body);
    }
    // dynamic parser
    QVariantMap dynamic_body = dynamicParserForm(element);
    if (!dynamic_body.isEmpty()) {
        submit_uri(result,element,"",dynamic_body);
    }
}


/* 标签: 框架
 * frame frameset noframes iframe
 */
void DOMParser::handle_tag_frames(QWebElement& element)
{
    printSimpleElement(element);
    QString src = element.evaluateJavaScript("this.src").toString();
    if (!src.isEmpty() && QUrl(src).isValid()) {
        submit_uri(src,element);
    }
}

// Private API BUGFIX:
//QWebElement QtWebElementRuntime::create(Element* element)
//{
//    return QWebElement(element);
//}

//Element* QtWebElementRuntime::get(const QWebElement& element)
//{
//    return element.m_element;
//}

/* 标签: 图像
 * img map area canvas figcaption figure
 */
void DOMParser::handle_tag_images(QWebElement& element)
{
    printSimpleElement(element);
//    WebCore::Element* wkt_element = QtWebElementRuntime::get(element);
    QString js = element.tagName().toLower() == "area" ? "this.href" : "this.src";
    QString result = element.evaluateJavaScript(js).toString().trimmed();
    if (!result.isEmpty() && QUrl(result).isValid()) {
        submit_uri(result,element);
    }
}


/* 标签: 音频/视频
 * audio source track video
 */
void DOMParser::handle_tag_media(QWebElement& element)
{
    printSimpleElement(element);
    QString src = element.evaluateJavaScript("this.src").toString().trimmed();
    if (!src.isEmpty() && QUrl(src).isValid()) {
        submit_uri(src,element);
    }
}


/* 标签: 链接
 * a link nav
 */
void DOMParser::handle_tag_hyperlinks(QWebElement& element)
{
    printSimpleElement(element);
    QString href = element.evaluateJavaScript("this.href").toString().trimmed();
    if (href.isEmpty())
        return;
    submit_uri(href,element);
    if (href.startsWith("javascript",Qt::CaseInsensitive)) {
        emulate_click(element);
        wait(50,50);
    } else {
        QUrl url(href);
        if (url.hasFragment() && !url.fragment().isEmpty()) {
            qDebug()<<"hasFragment::"<<url.fragment();
            emulate_click(element);
            wait(50,50);
//        } else if (!element.attribute("onclick").trimmed().isEmpty()) {
        } else if (element.evaluateJavaScript("this.onclick == null ? \"false\" : \"true\"").toBool()) {
            qDebug()<<"onclick::"<<element.attribute("onclick");
            emulate_click(element);
            wait(50,50);
        }
    }
}


/* 标签: 列表
 * ul ol li dir dl dt dd menu menuitem command
 */
void DOMParser::handle_tag_list(QWebElement& element)
{
    (void)element;
    // ignore
}


/* 标签: 表格
 * table caption th tr td thead tbody tfoot col colgroup
 */
void DOMParser::handle_tag_table(QWebElement& element)
{
    (void)element;
    // ignore
}


/* 标签: 样式/节
 * style div span header footer section article aside details dialog summary details
 */
void DOMParser::handle_tag_sections(QWebElement& element)
{
    (void)element;
    // ignore
}


/* 标签: 元信息
 * head meta base basefont
 */
void DOMParser::handle_tag_meta(QWebElement& element)
{
    printSimpleElement(element);
    QString tagName = element.tagName().toLower();
    if (tagName == "base") {
        QString result = element.evaluateJavaScript("this.href").toString().trimmed();
        if (result.isEmpty())
            return;
        submit_uri(result,element);
    } else if (tagName == "meta") {
        QString httpEquiv = element.attribute("http-equiv").toLower().trimmed();
        if (httpEquiv == "refresh") {
            QStringList sec_url = element.attribute("content").trimmed().split("=");
            if (sec_url.length() < 1)
                return;
            QString url = sec_url[sec_url.length() - 1];    // last one
            bool proto_exist = QRegularExpression("^(ht|f)tps?://",QRegularExpression::InvertedGreedinessOption|
                                                  QRegularExpression::UseUnicodePropertiesOption).
                    match(url).hasMatch();
            if (!proto_exist) {
                url = element.webFrame()->baseUrl().resolved(QUrl(url)).toString();
            }
            if (!url.isEmpty() && QUrl(url).isValid())
                submit_uri(url,element);
        }
    } else {
        return;
    }
}


/* 标签: 编程
 * script noscript applet embed object param
 */
void DOMParser::handle_tag_script(QWebElement& element)
{
    printSimpleElement(element);
    QString tagName = element.tagName().toLower();
    static const QString src_tags = "script,noscript,embed";
    if (src_tags.contains(tagName)) {
        QString src = element.evaluateJavaScript("this.src").toString().trimmed();
        if (!src.isEmpty() && QUrl(src).isValid())
            submit_uri(src,element);
    } else if (tagName == "applet") {
        QStringList js_codes;
        js_codes << "this.code" << "this.codebase" << "this.data" << "this.usemap";
        foreach (QString code, js_codes) {
            QString src = element.evaluateJavaScript(code).toString().trimmed();
            if (!src.isEmpty() && QUrl(src).isValid())
                submit_uri(src,element);
        }
    } else if (tagName == "object") {
        QStringList js_codes;
        js_codes << "this.archive" << "this.codebase" << "this.data" << "this.usemap";
        foreach (QString code, js_codes) {
            QString src = element.evaluateJavaScript(code).toString().trimmed();
            if (!src.isEmpty() && QUrl(src).isValid())
                submit_uri(src,element);
        }
    } else {
        return;
    }
}


/* 标签: 其他
 * ...
 */
void DOMParser::handle_tag_other(QWebElement& element)
{
    (void)element;
    printSimpleElement(element);
}

void DOMParser::wait(int msec, int per_cost)
{
    QTime dieTime = QTime::currentTime().addMSecs(msec);
    while (QTime::currentTime() < dieTime)
        QCoreApplication::processEvents(QEventLoop::AllEvents, per_cost);
}
