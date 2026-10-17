#include "labelingconfig.h"
#include "apiclient.h"
#include <QFile>
#include <QJsonDocument>
#include <QJsonArray>
#include <QColor>
#include <QSyntaxHighlighter>
#include <QRegularExpression>
#include <QQmlEngine>
#include <QPointer>
#include <algorithm>

namespace {
const QStringList controlTags{"RectangleLabels","PolygonLabels","Labels","BrushLabels","KeyPointLabels","Choices","TextArea","Rating","Number","Rectangle","Polygon","EllipseLabels","HyperTextLabels","Taxonomy","Pairwise","Ranker","VideoRectangle","TimeSeriesLabels","AudioLabels"};
const QStringList objectTags{"Image","Text","HyperText","Audio","AudioPlus","Video","TimeSeries","Paragraphs","Pdf","Table"};
QVariantMap attrs(const QDomElement &element){QVariantMap map;const auto attributes=element.attributes();for(int i=0;i<attributes.size();++i){const auto a=attributes.item(i);map[a.nodeName()]=a.nodeValue();}return map;}
class Highlighter:public QSyntaxHighlighter {
public:
    explicit Highlighter(QTextDocument *document):QSyntaxHighlighter(document){}
    void highlightBlock(const QString &text)override{
        const auto apply=[&](const QString &pattern,const QColor &color,int group=0){QTextCharFormat format;format.setForeground(color);auto matches=QRegularExpression(pattern).globalMatch(text);while(matches.hasNext()){const auto m=matches.next();setFormat(m.capturedStart(group),m.capturedLength(group),format);}};
        apply("</?([A-Za-z][\\w:-]*)",QColor("#7351b5"),1);
        apply("([\\w:-]+)\\s*=",QColor("#225daa"),1);
        apply("\"[^\"]*\"|'[^']*'",QColor("#3c7d4c"));
        int start=previousBlockState()==1?0:text.indexOf("<!--");setCurrentBlockState(0);
        while(start>=0){const int end=text.indexOf("-->",start);const int length=end<0?text.size()-start:end-start+3;QTextCharFormat format;format.setForeground(QColor("#888780"));setFormat(start,length,format);if(end<0){setCurrentBlockState(1);break;}start=text.indexOf("<!--",end+3);}
    }
};
}

LabelingConfig::LabelingConfig(QObject *parent):QObject(parent){
    QFile file(":/templates/catalog.json");if(file.open(QIODevice::ReadOnly)){const auto catalog=QJsonDocument::fromJson(file.readAll()).object();m_templates=catalog.value("templates").toArray().toVariantList();m_groups=catalog.value("groups").toVariant().toStringList();}
    std::stable_sort(m_templates.begin(),m_templates.end(),[this](const QVariant&a,const QVariant&b){const auto x=a.toMap(),y=b.toMap();const int gx=m_groups.indexOf(x.value("group").toString()),gy=m_groups.indexOf(y.value("group").toString());if(gx!=gy)return gx<gy;return x.value("order",100).toInt()<y.value("order",100).toInt();});
}
void LabelingConfig::setXml(const QString &xml){
    if(m_xml==xml)return;m_xml=xml;m_error.clear();m_message.clear();m_serverStatus.clear();
    m_doc.clear();if(!xml.trimmed().isEmpty()){const auto result=m_doc.setContent(xml);if(!result)m_error=tr("XML error at line %1, column %2: %3").arg(result.errorLine).arg(result.errorColumn).arg(result.errorMessage);else if(m_doc.documentElement().tagName()!="View")m_error=tr("The root element must be View.");}
    const int count=elements(controlTags).size();if(m_selected>=count)m_selected=qMax(0,count-1);emit changed();
}
QList<QDomElement> LabelingConfig::elements(const QStringList &tags)const{
    QList<QDomElement> result;if(!m_error.isEmpty())return result;
    const auto walk=[&](auto &&self,const QDomNode &parent)->void{for(auto n=parent.firstChild();!n.isNull();n=n.nextSibling()){if(n.isElement()){const auto e=n.toElement();if(tags.contains(e.tagName()))result.append(e);self(self,n);}}};walk(walk,m_doc);return result;
}
QVariantList LabelingConfig::controls()const{QVariantList result;for(const auto &e:elements(controlTags))result.append(QVariantMap{{"name",e.attribute("name")},{"tag",e.tagName()},{"toName",e.attribute("toName")},{"attributes",attrs(e)}});return result;}
QVariantList LabelingConfig::objects()const{QVariantList result;for(const auto &e:elements(objectTags))result.append(QVariantMap{{"name",e.attribute("name")},{"tag",e.tagName()},{"value",e.attribute("value",e.attribute("valueList"))},{"list",e.hasAttribute("valueList")}});return result;}
void LabelingConfig::setSelectedControl(int index){if(index==m_selected||index<0||index>=elements(controlTags).size())return;m_selected=index;emit changed();}
QDomElement LabelingConfig::selected()const{return elements(controlTags).value(m_selected);}
QList<QDomElement> LabelingConfig::labelElements()const{QList<QDomElement> result;for(auto e=selected().firstChildElement();!e.isNull();e=e.nextSiblingElement())if(e.tagName()=="Label"||e.tagName()=="Choice")result.append(e);return result;}
QVariantList LabelingConfig::labels()const{QVariantList result;for(const auto &e:labelElements())result.append(QVariantMap{{"value",e.attribute("value")},{"color",e.attribute("background","#6c63ff")},{"alias",e.attribute("alias")},{"hotkey",e.attribute("hotkey")}});return result;}
void LabelingConfig::commit(){const auto xml=m_doc.toString(2);m_xml.clear();setXml(xml);}
void LabelingConfig::setLabel(int index,const QString &value,const QString &color,const QString &alias,const QString &hotkey){
    auto labels=labelElements();if(index<0||index>=labels.size())return;
    if(value.trimmed().isEmpty()||!QColor(color).isValid()){m_message=tr("Enter a label name and a valid color.");emit changed();return;}
    for(int i=0;i<labels.size();++i)if(i!=index&&(labels[i].attribute("value")==value.trimmed()||(!alias.isEmpty()&&labels[i].attribute("alias")==alias))){m_message=tr("Label names and aliases must be unique.");emit changed();return;}
    auto label=labels[index];label.setAttribute("value",value.trimmed());label.setAttribute("background",color);
    for(const auto &key:QStringList{"alias","hotkey"}){const auto v=key=="alias"?alias:hotkey;if(v.isEmpty())label.removeAttribute(key);else label.setAttribute(key,v);}
    commit();
}
void LabelingConfig::removeLabel(int index){auto labels=labelElements();if(index<0||index>=labels.size())return;auto e=selected();e.removeChild(labels[index]);commit();}
void LabelingConfig::addLabels(const QString &lines){
    auto parent=selected();if(parent.isNull())return;const auto tag=parent.tagName()=="Choices"?"Choice":"Label";
    if(!parent.tagName().endsWith("Labels")&&parent.tagName()!="Choices"&&parent.tagName()!="Labels"){m_message=tr("This control does not contain editable labels.");emit changed();return;}
    auto existing=labelElements();QStringList names;for(const auto &e:existing)names.append(e.attribute("value"));
    const QStringList colors{"#008000","#0000ff","#e89b24","#ae59bf","#d84d4d"};bool changed=false;
    for(const auto &line:lines.split('\n')){const auto name=line.trimmed();if(name.isEmpty()||names.contains(name))continue;auto e=m_doc.createElement(tag);e.setAttribute("value",name);e.setAttribute("background",colors[names.size()%colors.size()]);parent.appendChild(e);names.append(name);changed=true;}
    if(changed)commit();
}
void LabelingConfig::setObjectValue(int index,const QString &value){auto objects=elements(objectTags);if(index<0||index>=objects.size())return;objects[index].setAttribute(objects[index].hasAttribute("valueList")?"valueList":"value",value);commit();}
void LabelingConfig::setControlAttribute(const QString &key,const QString &value){
    static const QStringList editable{"required","min","max","step","maxRating","maxSubmissions","placeholder","rows","maxUsages","choice","showInline","visibleWhen","whenTagName","whenChoiceValue"};if(!editable.contains(key)||selected().isNull())return;
    auto element=selected();if(value.isEmpty())element.removeAttribute(key);else element.setAttribute(key,value);commit();
}
void LabelingConfig::formatXml(){if(!m_doc.isNull()&&m_error.isEmpty())commit();}
QStringList LabelingConfig::completions(const QString &prefix)const{QStringList tags=controlTags+objectTags+QStringList{"View","Header","Style","Label","Choice","Filter","Relations","Relation"};tags.removeDuplicates();tags.sort(Qt::CaseInsensitive);QStringList result;for(const auto &tag:tags)if(tag.startsWith(prefix,Qt::CaseInsensitive))result.append(tag);return result;}
QStringList LabelingConfig::attributeCompletions(const QString &tag,const QString &prefix)const{
    QStringList keys{"name","style","className"};
    if(objectTags.contains(tag)||tag=="Header")keys<<"value";
    if(tag=="Image")keys<<"valueList"<<"zoom"<<"zoomControl"<<"rotateControl"<<"width"<<"height"<<"crosshair";
    if(controlTags.contains(tag))keys<<"toName"<<"required"<<"requiredMessage"<<"hotkey";
    if(tag.endsWith("Labels")||tag=="Label"||tag=="Choice")keys<<"value"<<"alias"<<"showAlias"<<"background"<<"hotkey"<<"maxUsages";
    if(tag=="Choices")keys<<"choice"<<"showInline"<<"value";
    if(tag=="TextArea")keys<<"placeholder"<<"rows"<<"maxSubmissions";
    if(tag=="Number")keys<<"min"<<"max"<<"step";
    if(tag=="Rating")keys<<"maxRating"<<"icon"<<"size";
    keys.removeDuplicates();keys.sort();QStringList result;for(const auto &key:keys)if(key.startsWith(prefix,Qt::CaseInsensitive))result<<key+"=\"\"";return result;
}
QString LabelingConfig::templateTitle(const QString &title)const{return QCoreApplication::translate("TemplateCatalog",title.toUtf8().constData());}
QVariantMap LabelingConfig::sampleData()const{
    QVariantMap data;
    const auto insert=[](auto &&self,QVariantMap &target,const QStringList &path,int depth,const QVariant &value)->void{
        if(depth==path.size()-1){target[path[depth]]=value;return;}
        auto child=target.value(path[depth]).toMap();self(self,child,path,depth+1,value);target[path[depth]]=child;
    };
    for(const auto &e:elements(objectTags)){const auto value=e.attribute("value",e.attribute("valueList"));if(!value.startsWith('$')||value.size()<2)continue;const auto path=value.mid(1).split('.');
        QVariant sample=tr("This is sample data. Edit the configuration to update the preview.");
        if(e.tagName()=="Image")sample=e.hasAttribute("valueList")?QVariant(QVariantList{QString("qrc:/templates/sample.jpg"),QString("qrc:/templates/sample.jpg")}):QVariant("qrc:/templates/sample.jpg");
        insert(insert,data,path,0,sample);}
    return data;
}
void LabelingConfig::validateOnServer(QObject *object,bool existingProject){
    auto *api=qobject_cast<ApiClient*>(object);if(!api||api->busy()||!api->connected()||api->demo())return;
    if(!m_error.isEmpty()||m_xml.trimmed().isEmpty()){m_serverStatus=tr("Fix the XML before validating.");emit changed();return;}
    const auto snapshot=m_xml;const int id=api->project().value("id").toInt();const auto path=existingProject&&id>0?QString("/api/projects/%1/validate/").arg(id):QString("/api/projects/validate/");
    m_serverStatus=tr("Validating…");emit changed();
    QPointer<LabelingConfig> self(this);
    api->request("POST",path,{{"label_config",snapshot}},[self,snapshot](const QJsonDocument &){if(self&&self->m_xml==snapshot){self->m_serverStatus=tr("Django validation passed.");emit self->changed();}},true,[self,snapshot](int){if(self&&self->m_xml==snapshot){self->m_serverStatus=tr("Django rejected the configuration. See the server message.");emit self->changed();}return false;});
}
void XmlSyntax::setDocument(QQuickTextDocument *document){if(m_document==document)return;delete m_highlighter;m_document=document;m_highlighter=document?new Highlighter(document->textDocument()):nullptr;emit documentChanged();}
void registerLabelingTypes(){static bool done=false;if(done)return;done=true;qmlRegisterType<LabelingConfig>("Studio.Native",1,0,"LabelingConfig");qmlRegisterType<XmlSyntax>("Studio.Native",1,0,"XmlSyntax");qmlRegisterType<AnnotationDocument>("Studio.Native",1,0,"AnnotationDocument");}
