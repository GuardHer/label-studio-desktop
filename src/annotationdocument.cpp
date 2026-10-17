#include "annotationdocument.h"
#include <QColor>
#include <QJsonObject>
#include <QSet>
#include <QUuid>
#include <QXmlStreamReader>
#include <algorithm>
#include <cmath>

namespace {
bool validPolygon(const QJsonArray &points) {
    if (points.size() < 3) return false;
    double area = 0;
    for (int i=0; i<points.size(); ++i) {
        const auto point = points[i].toArray(), next = points[(i+1)%points.size()].toArray();
        if (point.size()!=2 || next.size()!=2) return false;
        for (const auto &coordinate : point)
            if (!coordinate.isDouble() || !std::isfinite(coordinate.toDouble()) || coordinate.toDouble()<0 || coordinate.toDouble()>100) return false;
        area += point[0].toDouble()*next[1].toDouble()-next[0].toDouble()*point[1].toDouble();
    }
    return std::abs(area) > 0.001;
}
}

AnnotationDocument::AnnotationDocument(QObject *p) : QObject(p) {}

bool AnnotationDocument::fail(const QString &message) {
    m_error = message; m_editable = false;
    m_results = {}; m_saved = {};
    m_history = {QJsonArray{}}; m_historyIndex = 0;
    emit changed();
    return false;
}

bool AnnotationDocument::configure(const QString &xml) {
    m_configured=false; m_labels.clear(); m_controls.clear(); m_images.clear(); m_frames.clear(); m_layout.clear();
    m_taskData.clear(); m_activeImage=m_activeControl=-1; m_from.clear(); m_to.clear(); m_imageKey.clear(); m_shapeType.clear();
    m_width=m_height=0; m_id=-1; m_validationError.clear();
    QXmlStreamReader reader(xml);
    QList<QVariantMap> nodes;
    QStringList stack;
    QSet<QString> names, objects;
    const QSet<QString> shapeTags{"RectangleLabels","PolygonLabels"};
    const QSet<QString> controlTags{"RectangleLabels","PolygonLabels","Choices","TextArea","Rating","Number"};
    const QSet<QString> allowed{"View","Header","Text","Style","Image","RectangleLabels","PolygonLabels","Label","Choices","Choice","TextArea","Rating","Number"};
    const QSet<QString> layoutAttrs{"name","value","style","className","size"};
    const QSet<QString> labelAttrs{"value","alias","showAlias","background","hotkey","hint","maxUsages","selected","selectedColor","size","aliasStyle","category"};
    const QSet<QString> imageAttrs{"name","value","valueList","zoom","zoomControl","rotateControl","width","height","style","crosshair","crosshairColor","selectionEnabled"};
    const QSet<QString> controlAttrs{"name","toName","required","requiredMessage","min","max","step","maxRating","icon","size","maxUsages","value","choice","showInline","placeholder","rows","maxSubmissions","editable","allowSubmit","hotkey"};
    int currentControl=-1;
    while (!reader.atEnd()) {
        reader.readNext();
        if(reader.isCharacters() && !nodes.isEmpty()) nodes.last()["text"]=nodes.last().value("text").toString()+reader.text().toString();
        if(reader.isEndElement()) {
            if(nodes.isEmpty()) continue;
            const auto node=nodes.takeLast(); const auto tag=stack.takeLast();
            if(controlTags.contains(tag)) currentControl=-1;
            if(nodes.isEmpty()) m_layout.append(node);
            else { auto children=nodes.last().value("children").toList(); children.append(node); nodes.last()["children"]=children; }
            continue;
        }
        if(!reader.isStartElement()) continue;
        const QString tag=reader.name().toString();
        if(!allowed.contains(tag)) return fail(tr("Unsupported annotation control <%1>. Editing and saving are disabled.").arg(tag));
        QVariantMap attrs;
        const auto accepted=(tag=="Label"||tag=="Choice")?labelAttrs:(tag=="Image"?imageAttrs:(controlTags.contains(tag)?controlAttrs:layoutAttrs));
        for(const auto &a:reader.attributes()) {
            const auto key=a.name().toString();
            if(!accepted.contains(key)) return fail(tr("Unsupported attribute %1 on <%2>. Editing and saving are disabled.").arg(key,tag));
            attrs[key]=a.value().toString();
        }
        const auto name=attrs.value("name").toString();
        if((tag=="Image"||tag=="Text"||controlTags.contains(tag)) && (name.isEmpty()||names.contains(name))) return fail(tr("Object and control names must be present and unique."));
        if(!name.isEmpty()) names.insert(name);
        if(tag=="Image"||tag=="Text") objects.insert(name);
        for(const auto &key:{"required","selected","showAlias","editable","allowSubmit","showInline","selectionEnabled"})
            if(attrs.contains(key)&&attrs.value(key)!="true"&&attrs.value(key)!="false") return fail(tr("Invalid boolean attribute %1 on <%2>.").arg(key,tag));
        if(controlTags.contains(tag)) {
            QSet<QString> semantic{"name","toName","required","requiredMessage","hotkey"};
            if(shapeTags.contains(tag)) semantic.unite({"value","maxUsages"});
            if(tag=="Choices") semantic.unite({"choice","showInline","value"});
            if(tag=="TextArea") semantic.unite({"placeholder","rows","maxSubmissions","editable","allowSubmit"});
            if(tag=="Number") semantic.unite({"min","max","step"});
            if(tag=="Rating") semantic.unite({"maxRating","icon","size"});
            for(auto it=attrs.cbegin();it!=attrs.cend();++it) if(!semantic.contains(it.key())) return fail(tr("Unsupported attribute %1 on <%2>. Editing and saving are disabled.").arg(it.key(),tag));
        }
        if(tag=="Image") {
            const auto source=attrs.value("valueList",attrs.value("value")).toString();
            if(!source.startsWith('$')||source.size()<2) return fail(tr("Image value must refer to a task field, for example $image."));
            if(attrs.value("selectionEnabled")=="true") return fail(tr("Image selectionEnabled is not supported by the native editor."));
            m_images.append(QVariantMap{{"name",name},{"key",source.mid(1)},{"multi",attrs.contains("valueList")}});
        } else if(controlTags.contains(tag)) {
            if(currentControl>=0) return fail(tr("Annotation controls cannot be nested."));
            QVariantMap c{{"name",name},{"type",tag.toLower()},{"toName",attrs.value("toName")},{"attributes",attrs},{"labels",QVariantList{}}};
            for(const auto &key:{"min","max","step","maxRating","maxUsages","maxSubmissions"}) if(attrs.contains(key)) {
                bool ok=false; const auto v=attrs.value(key).toString().toDouble(&ok);
                const bool positive=QString(key)=="step"||QString(key)=="maxRating"||QString(key)=="maxUsages"||QString(key)=="maxSubmissions";
                if(!ok||!std::isfinite(v)||(positive&&v<=0)||((QString(key)=="maxRating"||QString(key)=="maxUsages"||QString(key)=="maxSubmissions")&&std::floor(v)!=v)) return fail(tr("Invalid numeric constraint %1.").arg(key));
            }
            if(attrs.contains("min")&&attrs.contains("max")&&attrs.value("min").toDouble()>attrs.value("max").toDouble()) return fail(tr("Minimum exceeds maximum."));
            if(tag=="Choices"&&attrs.contains("choice")&&attrs.value("choice")!="single"&&attrs.value("choice")!="multiple"&&attrs.value("choice")!="single-radio") return fail(tr("Unsupported choice mode."));
            if(attrs.value("editable")=="false"||attrs.value("allowSubmit")=="false") return fail(tr("Read-only text controls are not supported by the native editor."));
            m_controls.append(c); currentControl=m_controls.size()-1;
        } else if(tag=="Label"||tag=="Choice") {
            if(currentControl<0||stack.isEmpty()||((tag=="Label")?!shapeTags.contains(stack.last()):stack.last()!="Choices")) return fail(tr("Labels and choices must be direct children of their controls."));
            auto c=m_controls[currentControl].toMap(); auto labels=c.value("labels").toList();
            if(tag=="Choice"&&attrs.value("selected")=="true") return fail(tr("Preselected choices are not supported by the native editor."));
            if(attrs.contains("maxUsages")) {bool ok=false;const auto n=attrs.value("maxUsages").toString().toInt(&ok);if(!ok||n<1)return fail(tr("Invalid numeric constraint %1.").arg("maxUsages"));}
            auto label=attrs; label["name"]=attrs.value("value");
            if(label.value("name").toString().isEmpty()) return fail(tr("A label name is empty or duplicated."));
            for(const auto &existing:labels) if(existing.toMap().value("name")==label.value("name")) return fail(tr("A label name is empty or duplicated."));
            labels.append(label); c["labels"]=labels; m_controls[currentControl]=c;
        }
        nodes.append(QVariantMap{{"tag",tag},{"attributes",attrs},{"text",QString{}},{"children",QVariantList{}}}); stack.append(tag);
    }
    if(reader.hasError()) return fail(tr("Unable to parse labeling configuration XML: %1").arg(reader.errorString()));
    if(m_layout.size()!=1||m_layout[0].toMap().value("tag")!="View"||m_controls.isEmpty()) return fail(tr("Use a View containing named objects and supported annotation controls."));
    for(const auto &entry:m_controls) {
        const auto c=entry.toMap(); const auto target=c.value("toName").toString();
        if(target.isEmpty()||!objects.contains(target)) return fail(tr("Each control must target an existing object."));
        if(c.value("type").toString().endsWith("labels")) {
            bool image=false; for(const auto &v:m_images) if(v.toMap().value("name")==target) image=true;
            if(!image) return fail(tr("Shape controls must target an Image."));
        }
        if((c.value("type").toString().endsWith("labels")||c.value("type")=="choices")&&c.value("labels").toList().isEmpty()&&!c.value("attributes").toMap().value("value").toString().startsWith('$')) return fail(tr("Annotation controls need labels or a dynamic task field."));
    }
    m_configured=true;
    setTaskData({});
    return load({});
}

QVariant AnnotationDocument::resolve(const QString &value) const {
    if(!value.startsWith('$')) return value;
    QVariant current=m_taskData;
    for(const auto &part:value.mid(1).split('.')) current=current.toMap().value(part);
    return current;
}
QVariantMap AnnotationDocument::control(const QString &name) const {
    for(const auto &v:m_controls) if(v.toMap().value("name")==name) return v.toMap();
    return {};
}
QVariantList AnnotationDocument::labelsFor(const QVariantMap &c) const {
    auto input=c.value("labels").toList();
    const auto dynamic=c.value("attributes").toMap().value("value").toString();
    if(dynamic.startsWith('$')) input=resolve(dynamic).toList();
    QVariantList labels; QSet<QString> used;
    static const QStringList colors{"#6C63FF","#F59E42","#24B89A","#EC6B8C","#47A3EB"};
    for(const auto &v:input) {
        auto row=v.toMap(); if(row.isEmpty()) row["value"]=v.toString();
        if(row.contains("showalias")&&!row.contains("showAlias")) row["showAlias"]=row.value("showalias");
        if(row.contains("maxusages")&&!row.contains("maxUsages")) row["maxUsages"]=row.value("maxusages");
        const auto name=row.value("name",row.value("value")).toString();
        if(name.isEmpty()||used.contains(name)) continue;
        used.insert(name); row["name"]=name;
        const auto alias=row.value("alias").toString();
        row["resultValue"]=(c.value("type")=="choices"?row.contains("alias"):!alias.isEmpty())?alias:name;
        row["display"]=name+((row.value("showAlias").toString()=="true"||row.value("showAlias").toBool())&&!row.value("alias").toString().isEmpty()?" "+row.value("alias").toString():QString{});
        auto color=row.value("background",row.value("color")).toString(); if(!QColor(color).isValid()) color=colors[labels.size()%colors.size()];
        row["color"]=color; labels.append(row);
    }
    return labels;
}
QString AnnotationDocument::resultLabel(const QVariantMap &c,const QString &name) const {
    for(const auto &entry:labelsFor(c)) {const auto label=entry.toMap();if(label.value("name").toString()==name)return label.value("resultValue").toString();}
    return name;
}
void AnnotationDocument::setTaskData(const QVariantMap &data) {
    m_taskData=data; m_frames.clear();
    for(const auto &entry:m_images) {
        const auto image=entry.toMap(); const auto value=resolve('$'+image.value("key").toString());
        auto sources=image.value("multi").toBool()?value.toList():QVariantList{value};
        if(sources.isEmpty()) sources.append(QString{});
        for(int i=0;i<sources.size();++i) { auto frame=image; frame["source"]=sources[i].toString(); frame["itemIndex"]=i; m_frames.append(frame); }
    }
    m_activeImage=m_frames.isEmpty()?-1:0; m_activeControl=m_controls.isEmpty()?-1:0;
    for(int i=0;i<m_controls.size();++i) { auto c=m_controls[i].toMap(); c["resolvedLabels"]=labelsFor(c); m_controls[i]=c; }
    refreshActive(); emit changed(); emit activeImageChanged();
}
void AnnotationDocument::refreshActive() {
    const auto frame=m_activeImage>=0&&m_activeImage<m_frames.size()?m_frames[m_activeImage].toMap():QVariantMap{};
    m_to=frame.value("name").toString(); m_imageKey=frame.value("key").toString();
    if(m_activeControl>=0&&m_activeControl<m_controls.size()) {
        const auto c=m_controls[m_activeControl].toMap();
        if(c.value("toName")!=m_to||!c.value("type").toString().endsWith("labels")) m_activeControl=-1;
    }
    if(m_activeControl<0) for(int i=0;i<m_controls.size();++i) { const auto c=m_controls[i].toMap(); if(c.value("toName")==m_to&&c.value("type").toString().endsWith("labels")) {m_activeControl=i;break;} }
    const auto c=m_activeControl>=0?m_controls[m_activeControl].toMap():QVariantMap{};
    m_from=c.value("name").toString(); m_shapeType=c.value("type").toString(); m_labels=labelsFor(c);
}
void AnnotationDocument::selectImage(int index) {
    if(index<0||index>=m_frames.size()||index==m_activeImage) return;
    m_activeImage=index; m_activeControl=-1; m_width=m_height=0; refreshActive(); emit changed(); emit activeImageChanged();
}
void AnnotationDocument::selectControl(int index) {
    if(index<0||index>=m_controls.size()) return;
    const auto c=m_controls[index].toMap();
    if(!c.value("type").toString().endsWith("labels")) return;
    if(c.value("toName")!=m_to) { for(int i=0;i<m_frames.size();++i) if(m_frames[i].toMap().value("name")==c.value("toName")) {selectImage(i);break;} }
    m_activeControl=index; refreshActive(); emit changed();
}
QString AnnotationDocument::activeImageSource() const { return m_activeImage>=0?m_frames[m_activeImage].toMap().value("source").toString():QString{}; }

bool AnnotationDocument::knownLabel(const QString &name) const {
    return std::any_of(m_labels.cbegin(), m_labels.cend(), [&](const QVariant &v) { return v.toMap().value("name").toString() == name; });
}

bool AnnotationDocument::validControlValue(const QVariantMap &c, const QJsonObject &v) const {
    const auto type=c.value("type").toString(); const auto a=c.value("attributes").toMap();
    const auto valueKey=type=="textarea"?QStringLiteral("text"):type;
    if(v.size()!=1||!v.contains(valueKey)) return false;
    if(type=="choices") {
        if(!v.value("choices").isArray()) return false;
        const auto selected=v.value("choices").toArray(); QSet<QString> unique;
        if(a.value("choice","single")!="multiple"&&selected.size()>1) return false;
        const auto labels=labelsFor(c);
        for(const auto &value:selected) { bool known=false; for(const auto &l:labels) if(l.toMap().value("resultValue")==value.toString()) known=true;
            if(!known||!value.isString()||unique.contains(value.toString())) return false; unique.insert(value.toString()); }
        return true;
    }
    if(type=="textarea") {
        if(!v.value("text").isArray()) return false;
        const auto texts=v.value("text").toArray(); if(a.contains("maxSubmissions")&&texts.size()>a.value("maxSubmissions").toInt()) return false;
        for(const auto &text:texts) if(!text.isString()) return false;
        return true;
    }
    if(type=="number"||type=="rating") {
        if(!v.value(type).isDouble()) return false;
        const double n=v.value(type).toDouble(); if(!std::isfinite(n)) return false;
        const double lo=type=="rating"?1:a.value("min",-1e100).toDouble(),hi=type=="rating"?a.value("maxRating",5).toDouble():a.value("max",1e100).toDouble();
        if(n<lo||n>hi||(type=="rating"&&std::floor(n)!=n)) return false;
        if(a.contains("step")) { const double steps=(n-(a.contains("min")?lo:0))/a.value("step").toDouble(); if(std::abs(steps-std::round(steps))>1e-6) return false; }
        return true;
    }
    return false;
}
bool AnnotationDocument::load(const QJsonArray &items, int id) {
    m_id=id;
    if(!m_configured) return fail(tr("Load a valid project labeling configuration first."));
    QSet<QString> ids,globals;
    for(const auto &item:items) {
        if(!item.isObject()) return fail(tr("Invalid annotation result."));
        const auto r=item.toObject(); const auto v=r.value("value").toObject(); const auto c=control(r.value("from_name").toString()); const auto type=c.value("type").toString();
        if(c.isEmpty()||r.value("type")!=type||r.value("to_name").toString()!=c.value("toName").toString()||!r.value("value").isObject()||r.contains("parentID")||r.contains("parent_id")||r.value("readonly").toBool()) return fail(tr("This annotation contains unsupported controls or region relationships. Editing and saving are disabled."));
        if(!type.endsWith("labels")) {
            if(globals.contains(c.value("name").toString())||!validControlValue(c,v)) return fail(tr("Invalid or unsupported control value. Editing and saving are disabled."));
            globals.insert(c.value("name").toString()); continue;
        }
        const auto labels=v.value(type).toArray(); bool known=false;
        if(!labels.isEmpty()) for(const auto &l:labelsFor(c)) if(l.toMap().value("resultValue")==labels.first().toString()||l.toMap().value("name")==labels.first().toString()) known=true;
        if((r.contains("image_rotation")&&!r.value("image_rotation").isDouble())||(v.contains("rotation")&&!v.value("rotation").isDouble())||r.value("image_rotation").toDouble()!=0||v.value("rotation").toDouble()!=0||labels.size()!=1||!known) return fail(tr("This annotation contains unsupported regions, rotation or labels. Editing and saving are disabled."));
        bool multi=false; for(const auto &img:m_images) if(img.toMap().value("name")==c.value("toName")) multi=img.toMap().value("multi").toBool();
        if(r.contains("item_index")&&(!multi||!r.value("item_index").isDouble()||r.value("item_index").toDouble()<0||std::floor(r.value("item_index").toDouble())!=r.value("item_index").toDouble())) return fail(tr("Invalid image list frame index."));
        if(multi&&r.contains("item_index")&&!m_taskData.isEmpty()) {
            bool found=false; for(const auto &f:m_frames) if(f.toMap().value("name")==c.value("toName")&&f.toMap().value("itemIndex").toInt()==r.value("item_index").toInt()) found=true;
            if(!found) return fail(tr("Annotation refers to a missing image list frame."));
        }
        const QString regionId=r.value("id").toString(); const auto unique=regionId+'|'+r.value("from_name").toString()+'|'+r.value("to_name").toString()+'|'+QString::number(r.value("item_index").toInt());
        if(regionId.isEmpty()||ids.contains(unique)) return fail(tr("Annotation region IDs are missing or duplicated.")); ids.insert(unique);
        if(type=="polygonlabels") { if(!validPolygon(v.value("points").toArray())) return fail(tr("Invalid polygon points. Editing and saving are disabled.")); continue; }
        for(const auto &key:{"x","y","width","height"}) if(!v.value(key).isDouble()||!std::isfinite(v.value(key).toDouble())) return fail(tr("Invalid annotation coordinates."));
        const auto x=v.value("x").toDouble(),y=v.value("y").toDouble(),w=v.value("width").toDouble(),h=v.value("height").toDouble();
        if(x<0||y<0||w<=0||h<=0||x+w>100.001||y+h>100.001) return fail(tr("The annotation extends beyond the image and cannot be safely edited."));
    }
    m_results=items; m_saved=items; m_history={items}; m_historyIndex=0; m_error.clear(); m_validationError.clear(); m_editable=true;
    emit changed(); return true;
}
bool AnnotationDocument::visibleResult(const QJsonObject &r) const {
    if(m_activeImage<0||!r.value("type").toString().endsWith("labels")) return false;
    const auto frame=m_frames[m_activeImage].toMap();
    return r.value("to_name").toString()==frame.value("name").toString()&&r.value("item_index").toInt()==frame.value("itemIndex").toInt();
}
int AnnotationDocument::resultIndex(int visibleIndex) const {
    for(int i=0;i<m_results.size();++i) if(visibleResult(m_results[i].toObject())&&visibleIndex--==0) return i;
    return -1;
}
QVariantMap AnnotationDocument::controlValues() const {
    QVariantMap values;
    for(const auto &item:m_results) {const auto r=item.toObject(); const auto type=r.value("type").toString(); if(type.endsWith("labels")) continue; const auto key=type=="textarea"?QStringLiteral("text"):type; values[r.value("from_name").toString()]=r.value("value").toObject().value(key).toVariant();}
    return values;
}
void AnnotationDocument::setControlValue(const QString &name,const QVariant &value) {
    const auto c=control(name); const auto type=c.value("type").toString(); if(!m_editable||c.isEmpty()||type.endsWith("labels")) return;
    const auto key=type=="textarea"?QStringLiteral("text"):type;
    QJsonObject v{{key,QJsonValue::fromVariant(value)}};
    const bool clear=!value.isValid()||value.isNull()||((type=="textarea"||type=="choices")&&value.toList().isEmpty())||(type=="rating"&&value.toDouble()==0);
    if(!clear&&!validControlValue(c,v)) {m_validationError=tr("The value for %1 does not satisfy its configuration.").arg(name);emit changed();return;}
    auto next=m_results; int at=-1; for(int i=0;i<next.size();++i) if(next[i].toObject().value("from_name").toString()==name) {at=i;break;}
    if(clear) {if(at>=0) next.removeAt(at);}
    else {auto r=at>=0?next[at].toObject():QJsonObject{{"id",QUuid::createUuid().toString(QUuid::Id128).left(10)},{"from_name",name},{"to_name",c.value("toName").toString()},{"type",type}};r["value"]=v;if(at>=0)next[at]=r;else next.append(r);}
    m_validationError.clear();commit(next);
}
bool AnnotationDocument::validate() {
    m_validationError.clear();
    if(!m_editable) m_validationError=m_error;
    for(const auto &entry:m_controls) {
        const auto c=entry.toMap(); const auto attrs=c.value("attributes").toMap(); int count=0;QMap<QString,int> labelCount;
        for(const auto &item:m_results) {const auto r=item.toObject();if(r.value("from_name").toString()!=c.value("name").toString())continue;const auto v=r.value("value").toObject();
            if(c.value("type").toString().endsWith("labels")) {++count;for(const auto &l:v.value(c.value("type").toString()).toArray())++labelCount[l.toString()];}
            else if(c.value("type")=="textarea") {for(const auto &text:v.value("text").toArray())if(!text.toString().trimmed().isEmpty())++count;}
            else if(c.value("type")=="choices") count+=v.value("choices").toArray().size();else ++count;
        }
        if(attrs.value("required")=="true"&&count==0) {m_validationError=attrs.value("requiredMessage").toString();if(m_validationError.isEmpty())m_validationError=tr("%1 is required.").arg(c.value("name").toString());break;}
        if(attrs.contains("maxUsages")&&count>attrs.value("maxUsages").toInt()) {m_validationError=tr("%1 exceeds its maximum usage count.").arg(c.value("name").toString());break;}
        for(const auto &label:labelsFor(c)) {const auto l=label.toMap();const auto canonical=l.value("name").toString(),serialized=l.value("resultValue").toString();const int uses=labelCount.value(serialized)+(canonical==serialized?0:labelCount.value(canonical));if(l.contains("maxUsages")&&uses>l.value("maxUsages").toInt()) {m_validationError=tr("%1 exceeds its maximum usage count.").arg(canonical);break;}}
        if(!m_validationError.isEmpty())break;
    }
    emit changed();return m_validationError.isEmpty();
}

bool AnnotationDocument::loadAnnotation(const QJsonObject &annotation) {
    m_id = annotation.value("id").toInt(-1);
    if (m_id <= 0 || !annotation.value("result").isArray())
        return fail(tr("The annotation response has no valid ID or result array. Editing and saving are disabled."));
    return load(annotation.value("result").toArray(), m_id);
}

QJsonArray AnnotationDocument::results() const { return m_results; }
QVariantList AnnotationDocument::labels() const { return m_labels; }
QVariantList AnnotationDocument::controls() const {
    QVariantList rows; for(const auto &item:m_controls) {auto c=item.toMap();c["labels"]=labelsFor(c);rows.append(c);}return rows;
}
bool AnnotationDocument::dirty() const { return m_results != m_saved; }
bool AnnotationDocument::editable() const { return m_editable; }
bool AnnotationDocument::canUndo() const { return m_editable && m_historyIndex > 0; }
bool AnnotationDocument::canRedo() const { return m_editable && m_historyIndex+1 < m_history.size(); }
QString AnnotationDocument::error() const { return m_error; }
QString AnnotationDocument::imageKey() const { return m_imageKey; }
int AnnotationDocument::annotationId() const { return m_id; }

QVariantList AnnotationDocument::rectangles() const {
    QVariantList rows;
    for(int i=0;i<m_results.size();++i) {
        const auto r=m_results[i].toObject(); if(!visibleResult(r))continue;
        const auto type=r.value("type").toString();const auto v=r.value("value").toObject();auto row=v.toVariantMap();const auto name=v.value(type).toArray().first().toString();
        if(type=="polygonlabels") {double x=100,y=100,right=0,bottom=0;for(const auto &point:v.value("points").toArray()){const auto p=point.toArray();x=std::min(x,p[0].toDouble());y=std::min(y,p[1].toDouble());right=std::max(right,p[0].toDouble());bottom=std::max(bottom,p[1].toDouble());}row["x"]=x;row["y"]=y;row["width"]=right-x;row["height"]=bottom-y;}
        row["label"]=name;row["displayLabel"]=name;row["id"]=r.value("id").toString();row["shapeType"]=type;row["fromName"]=r.value("from_name").toString();row["resultIndex"]=i;
        for(const auto &l:labelsFor(control(r.value("from_name").toString())))if(l.toMap().value("resultValue")==name||l.toMap().value("name")==name){row["label"]=l.toMap().value("name");row["color"]=l.toMap().value("color");row["displayLabel"]=l.toMap().value("display");break;}
        rows.append(row);
    }
    return rows;
}
QJsonObject AnnotationDocument::regionBase(const QString &type) const {
    QJsonObject r{{"id",QUuid::createUuid().toString(QUuid::Id128).left(10)},{"from_name",m_from},{"to_name",m_to},{"type",type},{"original_width",m_width},{"original_height",m_height},{"image_rotation",0}};
    if(m_activeImage>=0&&m_frames[m_activeImage].toMap().value("multi").toBool())r["item_index"]=m_frames[m_activeImage].toMap().value("itemIndex").toInt();
    return r;
}

void AnnotationDocument::setImageSize(int w, int h) { m_width = w; m_height = h; }
void AnnotationDocument::markSaved(int id) { m_id = id; m_saved = m_results; emit changed(); }
void AnnotationDocument::commit(const QJsonArray &next) {
    if (!m_editable || next == m_results) return;
    while (m_history.size() > m_historyIndex+1) m_history.removeLast();
    m_results = next; m_history.append(next); ++m_historyIndex; emit changed();
}

void AnnotationDocument::addRectangle(double x1, double y1, double x2, double y2, const QString &label) {
    if (m_shapeType != "rectanglelabels" || !m_editable || !knownLabel(label) || m_width <= 0 || m_height <= 0 || !std::isfinite(x1) || !std::isfinite(y1) || !std::isfinite(x2) || !std::isfinite(y2)) return;
    const double x = std::clamp(std::min(x1, x2), 0.0, 100.0), y = std::clamp(std::min(y1, y2), 0.0, 100.0);
    const double w = std::clamp(std::max(x1, x2), 0.0, 100.0)-x, h = std::clamp(std::max(y1, y2), 0.0, 100.0)-y;
    if (w < 0.05 || h < 0.05) return;
    QJsonObject value{{"x", x}, {"y", y}, {"width", w}, {"height", h}, {"rotation", 0}, {"rectanglelabels", QJsonArray{resultLabel(control(m_from),label)}}};
    auto region=regionBase("rectanglelabels");region["value"]=value;
    auto next = m_results; next.append(region); commit(next);
}

void AnnotationDocument::addPolygon(const QVariantList &points, const QString &label) {
    const auto vertices = QJsonArray::fromVariantList(points);
    if (m_shapeType != "polygonlabels" || !m_editable || !knownLabel(label) || m_width<=0 || m_height<=0 || !validPolygon(vertices)) return;
    QJsonObject value{{"points",vertices},{"polygonlabels",QJsonArray{resultLabel(control(m_from),label)}}};
    auto region=regionBase("polygonlabels");region["value"]=value;
    auto next=m_results; next.append(region); commit(next);
}
void AnnotationDocument::updatePolygon(int index, const QVariantList &points) {
    const auto vertices = QJsonArray::fromVariantList(points);
    index=resultIndex(index);
    if (!m_editable || index<0 || index>=m_results.size() || m_results[index].toObject().value("type")!="polygonlabels" || !validPolygon(vertices)) return;
    auto next=m_results; auto region=next[index].toObject(); auto value=region.value("value").toObject();
    value["points"]=vertices; region["value"]=value; next[index]=region; commit(next);
}

void AnnotationDocument::updateRectangle(int i, double x, double y, double w, double h) {
    i=resultIndex(i);
    if (!m_editable || i < 0 || i >= m_results.size() || m_results[i].toObject().value("type")!="rectanglelabels" || !std::isfinite(x) || !std::isfinite(y) || !std::isfinite(w) || !std::isfinite(h)) return;
    w = std::clamp(w, 0.05, 100.0); h = std::clamp(h, 0.05, 100.0);
    x = std::clamp(x, 0.0, 100.0-w); y = std::clamp(y, 0.0, 100.0-h);
    auto next = m_results; auto region = next[i].toObject(); auto v = region.value("value").toObject();
    v["x"] = x; v["y"] = y; v["width"] = w; v["height"] = h; region["value"] = v; next[i] = region; commit(next);
}
void AnnotationDocument::setLabel(int i,const QString &label) {
    i=resultIndex(i);if(!m_editable||i<0)return;
    auto next=m_results;auto r=next[i].toObject();const auto c=control(r.value("from_name").toString());bool known=false;
    for(const auto &l:labelsFor(c))if(l.toMap().value("name")==label)known=true;if(!known)return;
    auto v=r.value("value").toObject();v[r.value("type").toString()]=QJsonArray{resultLabel(c,label)};r["value"]=v;next[i]=r;commit(next);
}
void AnnotationDocument::removeRectangle(int i) {i=resultIndex(i);if(!m_editable||i<0)return;auto next=m_results;next.removeAt(i);commit(next);}
void AnnotationDocument::undo() { if (canUndo()) { m_results = m_history[--m_historyIndex]; emit changed(); } }
void AnnotationDocument::redo() { if (canRedo()) { m_results = m_history[++m_historyIndex]; emit changed(); } }
