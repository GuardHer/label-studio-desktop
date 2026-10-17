#include "apiclient.h"
#include <QRegularExpression>
#include <QUrlQuery>

void ApiClient::updateProjectDetails(const QString &title,const QString &description,const QString &config,const QString &color,const QString &sampling){
    if(!mayNavigate()||m_project.isEmpty()||m_demo)return;
    if(title.trimmed().isEmpty()){notify(tr("Project name is required."),true);return;}
    if(!QRegularExpression("^#[0-9A-Fa-f]{6}$").match(color).hasMatch()){notify(tr("Enter a color in #RRGGBB format."),true);return;}
    if(sampling!="Sequential sampling"&&sampling!="Uniform sampling"&&sampling!=m_project.value("sampling").toString()){notify(tr("Choose sequential or random task sampling."),true);return;}
    request("PATCH",QString("/api/projects/%1/").arg(m_project.value("id").toInt()),
        {{"title",title.trimmed()},{"description",description},{"label_config",config},{"color",color},{"sampling",sampling}},[this](const QJsonDocument &json){
            m_project=json.object();m_task={};m_taskId=-1;m_annotations={};m_imageSource.clear();m_samplingQueue=false;m_samplingHistory.clear();m_samplingIndex=-1;
            m_document->configure(m_project.value("label_config").toString());m_dataManager->load();notify(tr("Project settings saved."));emit projectUpdated();fetchProjects(1);
        });
}
void ApiClient::deleteProjectById(int id){
    if(!mayNavigate()||!m_connected||m_demo||id<1)return;
    request("DELETE",QString("/api/projects/%1/").arg(id),{},[this,id](const QJsonDocument &){
        m_catalog->forget(id);
        if(m_project.value("id").toInt()==id){resetTaskQuery();clearThumbnails();m_project={};m_task={};m_taskId=-1;m_totalTasks=0;m_tasks.clear();m_annotations={};m_imageSource.clear();m_document->configure({});m_account->activateProject(0);emit projectDeleted();}
        fetchProjects(1);notify(tr("Project deleted."));
    });
}
void ApiClient::resetProjectCache(){
    if(!mayNavigate()||m_project.isEmpty()||m_demo)return;
    request("POST",QString("/api/projects/%1/summary/reset/").arg(m_project.value("id").toInt()),{},[this](const QJsonDocument &){refreshProjectSummary();notify(tr("Cache reset requested. Large projects may finish in the background."));});
}
void ApiClient::dropProjectTabs(){
    if(!mayNavigate()||m_project.isEmpty()||m_demo)return;
    request("DELETE","/api/dm/views/reset/",{{"project",m_project.value("id")}},[this](const QJsonDocument &){resetTaskQuery();m_dataManager->load();fetchTasks(1);emit projectTabsReset();notify(tr("Data Manager tabs dropped. The task list has been reset."));});
}
void ApiClient::startLabeling(){
    if(!mayNavigate()||m_project.isEmpty()||m_demo)return;
    if(!m_document->editable()){notify(tr("Configure a supported labeling interface before starting."),true);return;}
    m_queueSelection=selectedTaskCount()>0?selectionQuery():QJsonObject{{"all",true},{"excluded",QJsonArray{}}};fetchSampledTask(true);
}
void ApiClient::fetchSampledTask(bool restart){
    auto query=taskQuery();QJsonArray excluded;if(!restart)for(int id:m_samplingHistory)excluded.append(id);
    auto selection=m_queueSelection;
    if(selection.isEmpty())selection={{"all",true},{"excluded",QJsonArray{}}};
    if(selection.value("all").toBool()){auto ids=selection.value("excluded").toArray();for(const auto &v:excluded)if(!ids.contains(v))ids.append(v);selection["excluded"]=ids;}
    else{QJsonArray ids;for(const auto &v:selection.value("included").toArray())if(!excluded.contains(v))ids.append(v);if(ids.isEmpty()){notify(tr("All selected tasks have been visited."));emit workflowFinished();return;}selection["included"]=ids;}
    query["selectedItems"]=selection;
    QUrl path(QString("/api/projects/%1/next/").arg(m_project.value("id").toInt()));QUrlQuery params;
    params.addQueryItem("query",QString::fromUtf8(QJsonDocument(query).toJson(QJsonDocument::Compact)));path.setQuery(params);
    request("GET",path.toString(QUrl::FullyEncoded).replace("+","%2B"),{},[this,restart](const QJsonDocument &json){
        const int id=json.object().value("id").toInt();if(id<1){notify(tr("The sampling response has no task ID."),true);return;}
        const bool previous=m_samplingQueue;m_samplingQueue=true;
        const auto previousHistory=m_samplingHistory;const int previousIndex=m_samplingIndex;
        if(restart)m_samplingHistory.clear();
        m_samplingHistory.append(id);m_samplingIndex=m_samplingHistory.size()-1;
        fetchTask(id,[this,previous,previousHistory,previousIndex]{m_samplingQueue=previous;m_samplingHistory=previousHistory;m_samplingIndex=previousIndex;});
    },true,[this](int status){if(status!=404)return false;notify(tr("No more tasks are available in the current project sampling queue."));emit workflowFinished();return true;});
}
