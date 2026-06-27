#include "ReaderMode.h"
#include "Logger.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QRegularExpression>

namespace Nova {

ReaderMode& ReaderMode::instance() { static ReaderMode i; return i; }
ReaderMode::ReaderMode(QObject* parent) : QObject(parent) { initCss(); }

void ReaderMode::initCss() {
    m_readerCss = R"(
body{font-family:Georgia,'Times New Roman',serif;font-size:18px;line-height:1.8;max-width:680px;margin:40px auto;padding:0 20px;color:#222;background:#fafafa;}
h1,h2,h3,h4,h5,h6{font-family:'Helvetica Neue',Arial,sans-serif;font-weight:600;line-height:1.3;margin-top:2em;color:#111;}
h1{font-size:2em;border-bottom:1px solid #ddd;padding-bottom:.3em;}
h2{font-size:1.5em;}p{margin:1em 0;}
img{max-width:100%;height:auto;display:block;margin:1.5em auto;border-radius:4px;}
pre,code{font-family:monospace;font-size:.9em;background:#f0f0f0;border-radius:4px;}
pre{padding:1em;overflow-x:auto;border-left:4px solid #0078d4;}
code{padding:2px 5px;}
blockquote{border-left:4px solid #aaa;margin:1.5em 0;padding:.5em 1em;color:#555;font-style:italic;}
a{color:#0078d4;text-decoration:none;}a:hover{text-decoration:underline;}
.reader-title{font-size:2.2em;font-weight:700;margin-bottom:.2em;}
.reader-meta{color:#777;font-size:.9em;margin-bottom:2em;}
)";
}

void ReaderMode::extractContent(QWebEnginePage* page, std::function<void(const ReaderContent&)> callback) {
    if (!page) { callback({}); return; }
    page->runJavaScript(extractionScript(), [this, callback](const QVariant& result) {
        callback(parseExtractionResult(result));
    });
}

QString ReaderMode::generateReaderHtml(const ReaderContent& content, bool darkMode) const {
    QString darkCss;
    if (darkMode) darkCss = "body{background:#1a1a1a;color:#e0e0e0;}h1,h2,h3,h4,h5,h6{color:#f0f0f0;}pre,code{background:#2d2d2d;color:#ddd;}blockquote{color:#aaa;}a{color:#5aafff;}";
    return QString(R"(<!DOCTYPE html><html><head><meta charset="UTF-8"><meta name="viewport" content="width=device-width,initial-scale=1.0"><title>%1</title><style>%2%3</style></head><body><div class="reader-title">%1</div><div class="reader-meta"><span>%4</span> · %5 min read</div><article>%6</article></body></html>)")
        .arg(content.title.toHtmlEscaped(), m_readerCss, darkCss,
             content.byline.toHtmlEscaped(), QString::number(content.estimatedReadingTimeMinutes),
             content.htmlContent);
}

bool ReaderMode::isReaderModeAvailable(const QString& url) const {
    return url.startsWith("http://") || url.startsWith("https://");
}

QString ReaderMode::extractionScript() const {
    return R"JS(
(function(){
function getTitle(){var o=document.querySelector('meta[property="og:title"]');if(o)return o.content;var h=document.querySelector('article h1,h1');return h?h.textContent.trim():document.title||'';}
function getByline(){var s=['meta[name="author"]','[rel="author"]','.author','.byline','[itemprop="author"]'];for(var x of s){var e=document.querySelector(x);if(e)return(e.content||e.textContent||'').trim();}return'';}
function getContent(){var a=document.querySelector('article,[role="main"],main,.article-content,.post-content,.entry-content,#content')||document.body;var c=a.cloneNode(true);c.querySelectorAll('nav,header,footer,aside,.sidebar,.ads,.advertisement,.nav,.navigation,.menu,.popup,.modal,script,style,noscript,iframe,[class*="social"],[class*="share"],[class*="related"],[class*="ad-"],[id*="sidebar"],[id*="footer"],[id*="header"]').forEach(e=>e.remove());var out='';c.querySelectorAll('h1,h2,h3,h4,h5,h6,p,img,pre,blockquote').forEach(function(el){var t=el.tagName.toLowerCase();if(t==='img'){var s=el.src||el.dataset.src||'';if(s&&!s.startsWith('data:'))out+='<img src="'+s+'" alt="'+(el.alt||'')+'">';}else if(t==='p'){var tx=el.innerText.trim();if(tx.length>20)out+='<p>'+tx+'</p>';}else if(/^h[1-6]$/.test(t)){var tx=el.innerText.trim();if(tx)out+='<'+t+'>'+tx+'</'+t+'>';}else if(t==='pre')out+=el.outerHTML;else if(t==='blockquote')out+='<blockquote>'+el.innerText.trim()+'</blockquote>';});return out;}
var title=getTitle(),byline=getByline(),html=getContent();
var wc=html.replace(/<[^>]*>/g,' ').split(/\s+/).filter(w=>w.length>0).length;
return JSON.stringify({title:title,byline:byline,html:html,readingTime:Math.max(1,Math.round(wc/200))});
})();
)JS";
}

ReaderContent ReaderMode::parseExtractionResult(const QVariant& result) const {
    ReaderContent c;
    const QString json = result.toString();
    if (json.isEmpty()) return c;
    QJsonParseError err;
    auto doc = QJsonDocument::fromJson(json.toUtf8(), &err);
    if (err.error != QJsonParseError::NoError) { LOG_ERROR("ReaderMode", err.errorString()); return c; }
    auto obj = doc.object();
    c.title = obj["title"].toString();
    c.byline = obj["byline"].toString();
    c.htmlContent = obj["html"].toString();
    c.estimatedReadingTimeMinutes = obj["readingTime"].toInt(1);
    return c;
}

} // namespace Nova
