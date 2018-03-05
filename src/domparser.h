#ifndef DOMPARSER_H
#define DOMPARSER_H

#include <QObject>
#include <QWebElement>
#include "webpage.h"
#include "consts.h"

class DOMParser : public QObject
{
    Q_OBJECT
public:
    explicit DOMParser(QObject *parent, WebPage* webpage);

    void parse_traversal_dom();

    void parse_element(QWebElement& element);

    void emulate_click(QWebElement &element,QString jscode=JS_ELEMENT_CLICK);

    void submit_uri(const QString& uri, const QWebElement& element, const QString& method="", const QVariantMap& body=QVariantMap());

    /* 标签: 基础
     * html title body h1 ... h6 p br hr
     */
    void handle_tag_bases(QWebElement& element);

    /* 标签: 格式
     * acronym abbr address b bdi bdo big blockquote center cite code del dfn em font i ins kbd mark
     * meter pre progress q rp rt ruby s samp small strike strong sup sub time tt u var wbr
     */
    void handle_tag_formats(QWebElement& element);

    /* 标签: 表单
     * form input textarea button select optgroup option label fieldset legend isindex datalist keygen output
     */
    void handle_tag_forms(QWebElement& element);

    /* 标签: 框架
     * frame frameset noframes iframe
     */
    void handle_tag_frames(QWebElement& element);

    /* 标签: 图像
     * img map area canvas figcaption figure
     */
    void handle_tag_images(QWebElement& element);

    /* 标签: 音频/视频
     * audio source track video
     */
    void handle_tag_media(QWebElement& element);

    /* 标签: 链接
     * a link nav
     */
    void handle_tag_hyperlinks(QWebElement& element);

    /* 标签: 列表
     * ul ol li dir dl dt dd menu menuitem command
     */
    void handle_tag_list(QWebElement& element);

    /* 标签: 表格
     * table caption th tr td thead tbody tfoot col colgroup
     */
    void handle_tag_table(QWebElement& element);

    /* 标签: 样式/节
     * style div span header footer section article aside details dialog summary details
     */
    void handle_tag_sections(QWebElement& element);

    /* 标签: 元信息
     * head meta base basefont
     */
    void handle_tag_meta(QWebElement& element);

    /* 标签: 编程
     * script noscript applet embed object param
     */
    void handle_tag_script(QWebElement& element);

    /* 标签: 其他
     * ...
     */
    void handle_tag_other(QWebElement& element);

signals:
    void parsedLinks(const QVariant& resource);

public slots:

private:
    WebPage* m_webpage;
    DOMParser* domparser;
    QWebElement m_webEelement;
    QSet<QByteArray> m_rescheduling;

    void _traversal_dom(QWebElement &curElement);

    void wait(int msec, int per_cost = 80);
};

#endif // DOMPARSER_H
