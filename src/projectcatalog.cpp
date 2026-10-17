#include "projectcatalog.h"
#include "apiclient.h"
#include <QSettings>
#include <QCryptographicHash>
#include <QCollator>
#include <algorithm>

ProjectCatalog::ProjectCatalog(ApiClient *api):QObject(api),m_api(api) {
    connect(api,&ApiClient::changed,this,&ProjectCatalog::changed);
}
QString ProjectCatalog::historyKey() const {
    const auto identity=m_api->serverUrl()+"|"+QString::number(m_api->user().value("id").toInt());
    return "recentProjects/"+QString::fromLatin1(QCryptographicHash::hash(identity.toUtf8(),QCryptographicHash::Sha256).toHex());
}
void ProjectCatalog::setSearch(const QString &value){if(m_search==value)return;m_search=value;emit changed();}
void ProjectCatalog::setOrder(const QString &value){
    if(!QStringList{"newest","oldest","title","title_desc","recent"}.contains(value)||m_order==value)return;
    m_order=value;emit changed();
}
void ProjectCatalog::recordVisit(int id){
    if(m_api->demo()||!m_api->user().contains("id"))return;
    auto history=QSettings().value(historyKey()).toStringList();history.removeAll(QString::number(id));history.prepend(QString::number(id));
    while(history.size()>100)history.removeLast();QSettings().setValue(historyKey(),history);emit changed();
}
void ProjectCatalog::forget(int id){auto history=QSettings().value(historyKey()).toStringList();history.removeAll(QString::number(id));QSettings().setValue(historyKey(),history);emit changed();}
QVariantList ProjectCatalog::items()const{
    auto items=m_api->projects();const auto term=m_search.trimmed();
    items.erase(std::remove_if(items.begin(),items.end(),[&](const QVariant &item){const auto p=item.toMap();return !term.isEmpty()&&!p.value("title").toString().contains(term,Qt::CaseInsensitive)&&!p.value("description").toString().contains(term,Qt::CaseInsensitive)&&QString::number(p.value("id").toInt())!=term;}),items.end());
    const auto history=QSettings().value(historyKey()).toStringList();QCollator collator;collator.setCaseSensitivity(Qt::CaseInsensitive);collator.setNumericMode(true);
    std::stable_sort(items.begin(),items.end(),[&](const QVariant&a,const QVariant&b){
        const auto left=a.toMap(),right=b.toMap();const int lid=left.value("id").toInt(),rid=right.value("id").toInt();
        if(m_order=="recent") {const int ai=history.indexOf(QString::number(lid)),bi=history.indexOf(QString::number(rid));if(ai!=bi)return ai<0?false:bi<0?true:ai<bi;}
        if(m_order=="title"||m_order=="title_desc"){const int c=collator.compare(left.value("title").toString(),right.value("title").toString());if(c!=0)return m_order=="title"?c<0:c>0;}
        const int c=QString::compare(left.value("created_at").toString(),right.value("created_at").toString());
        if(c!=0)return m_order=="oldest"?c<0:c>0;return m_order=="oldest"?lid<rid:lid>rid;
    });return items;
}
QVariantList ProjectCatalog::recent()const{
    auto projects=m_api->projects();const auto history=QSettings().value(historyKey()).toStringList();
    std::stable_sort(projects.begin(),projects.end(),[&](const QVariant&a,const QVariant&b){const int ai=history.indexOf(QString::number(a.toMap().value("id").toInt())),bi=history.indexOf(QString::number(b.toMap().value("id").toInt()));if(ai==bi)return false;return ai<0?false:bi<0?true:ai<bi;});
    while(projects.size()>10)projects.removeLast();return projects;
}
