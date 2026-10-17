#include "apiclient.h"
#include <QUrlQuery>
#include <algorithm>

void ApiClient::resetTaskQuery() {
    m_dataManager->reset();m_allSelected=false;m_excludedTasks.clear();m_queueSelection={};m_taskSearch.clear(); m_taskStatus = 0; m_taskDescending = false; m_selectedTasks.clear();
    m_samplingQueue=false;m_samplingHistory.clear();m_samplingIndex=-1;
}

QJsonObject ApiClient::taskQuery() const {
    QJsonArray filters;
    if (m_taskStatus != 0)
        filters.append(QJsonObject{{"filter", "filter:tasks:completed_at"}, {"operator", "empty"},
            {"type", "Datetime"}, {"value", m_taskStatus == 2}});
    if (!m_taskSearch.isEmpty()) {
        bool numeric = false; const int id = m_taskSearch.toInt(&numeric);
        filters.append(numeric
            ? QJsonObject{{"filter", "filter:tasks:id"}, {"operator", "equal"}, {"type", "Number"}, {"value", id}}
            : QJsonObject{{"filter", "filter:tasks:data." + (m_document->imageKey().isEmpty() ? "image" : m_document->imageKey())},
                {"operator", "contains"}, {"type", "String"}, {"value", m_taskSearch}});
    }
    auto query=m_dataManager->query();
    if(m_dataManager->m_advanced){
        auto group=query.value("filters").toObject();auto roots=group.value("items").toArray();
        if(roots.isEmpty())roots=filters;else if(!filters.isEmpty()){
            if(group.value("conjunction")=="and")for(const auto &v:filters)roots.append(v);
            else for(int i=0;i<roots.size();++i){auto row=roots[i].toObject();auto children=row.value("child_filters").toArray();for(const auto &v:filters)children.append(v);row["child_filters"]=children;roots[i]=row;}
        }
        group["items"]=roots;query["filters"]=group;
    }else{query["filters"]=QJsonObject{{"conjunction","and"},{"items",filters}};query["ordering"]=QJsonArray{m_taskDescending?"-tasks:id":"tasks:id"};}
    return query;
}

QString ApiClient::tasksPath(int page) const {
    QUrl url("/api/tasks/"); QUrlQuery query;
    query.addQueryItem("project", QString::number(m_project.value("id").toInt()));
    query.addQueryItem("page", QString::number(page)); query.addQueryItem("page_size", QString::number(m_dataManager->pageSize()));
    query.addQueryItem("query", QString::fromUtf8(QJsonDocument(taskQuery()).toJson(QJsonDocument::Compact)));
    url.setQuery(query); return url.toString(QUrl::FullyEncoded).replace("+", "%2B");
}

bool ApiClient::setTaskQuery(const QString &search, int status, bool descending) {
    if (!mayNavigate() || m_demo || m_project.isEmpty() || status < 0 || status > 2) return false;
    const auto previousSearch = m_taskSearch; const auto previousStatus = m_taskStatus;
    const auto previousDescending = m_taskDescending; const auto previousSelection = m_selectedTasks;const bool previousAdvanced=m_dataManager->m_advanced,previousAll=m_allSelected,previousViewDirty=m_dataManager->m_dirty;const auto previousExcluded=m_excludedTasks;
    m_taskSearch = search.trimmed(); m_taskStatus = status; m_taskDescending = descending;
    m_selectedTasks.clear();m_allSelected=false;m_excludedTasks.clear();
    m_dataManager->m_dirty=true;emit m_dataManager->changed();
    fetchTasks(1, {}, [=] {
        m_taskSearch = previousSearch; m_taskStatus = previousStatus; m_taskDescending = previousDescending;
        m_selectedTasks = previousSelection;m_dataManager->m_advanced=previousAdvanced;m_dataManager->m_dirty=previousViewDirty;emit m_dataManager->changed();m_allSelected=previousAll;m_excludedTasks=previousExcluded; emit taskQueryFailed(); emit changed();
    });
    return true;
}

void ApiClient::moveTask(int delta) {
    if (!mayNavigate() || m_demo || m_taskId < 0 || (delta != 1 && delta != -1)) return;
    if(m_samplingQueue){
        const int index=m_samplingIndex+delta;
        if(index>=0&&index<m_samplingHistory.size()){const int previous=m_samplingIndex;m_samplingIndex=index;fetchTask(m_samplingHistory[index],[this,previous]{m_samplingIndex=previous;});}
        else if(delta>0)fetchSampledTask();else notify(tr("No earlier task in this labeling session."));
        return;
    }
    int index = -1;
    for (int i = 0; i < m_tasks.size(); ++i) if (m_tasks[i].toMap().value("id").toInt() == m_taskId) index = i;
    if (index < 0) { notify(tr("This task is outside the current filter. Return to the task list.")); return; }
    const int next = index + delta;
    if (next >= 0 && next < m_tasks.size()) fetchTask(m_tasks[next].toMap().value("id").toInt());
    else if ((delta > 0 && m_page * m_dataManager->pageSize() < m_totalTasks) || (delta < 0 && m_page > 1)) {
        const auto previousTasks = m_tasks; const int previousPage = m_page, previousTotal = m_totalTasks;
        fetchTasks(m_page + delta, [=] {
            if (!m_tasks.isEmpty()) fetchTask((delta > 0 ? m_tasks.first() : m_tasks.last()).toMap().value("id").toInt(), [=] {
                clearThumbnails(); m_tasks = previousTasks; m_page = previousPage; m_totalTasks = previousTotal; emit changed();
            });
        });
    }
    else notify(tr("No more tasks in this direction."));
}

void ApiClient::advanceAfterWrite(int previousIndex) {
    if(m_samplingQueue){
        if(m_samplingIndex+1<m_samplingHistory.size()){
            const int previous=m_samplingIndex;++m_samplingIndex;
            fetchTask(m_samplingHistory[m_samplingIndex],[this,previous]{m_samplingIndex=previous;});
        }else fetchSampledTask();
        fetchTasks(m_page);refreshProjectSummary();return;
    }
    const int previousTask = m_taskId, previousPage = m_page, previousTotal = m_totalTasks;
    const auto previousTasks = m_tasks;
    const auto restore = [=] {
        clearThumbnails(); m_tasks = previousTasks; m_page = previousPage; m_totalTasks = previousTotal; emit changed();
    };
    fetchTasks(m_page, [=] {
        int index = -1;
        for (int i = 0; i < m_tasks.size(); ++i) if (m_tasks[i].toMap().value("id").toInt() == previousTask) index = i;
        const int next = index >= 0 ? index + 1 : previousPage == m_page ? previousIndex : m_tasks.size();
        if (next >= 0 && next < m_tasks.size()) fetchTask(m_tasks[next].toMap().value("id").toInt(), restore);
        else if (m_page * m_dataManager->pageSize() < m_totalTasks) fetchTasks(m_page + 1, [=] {
            if (!m_tasks.isEmpty()) fetchTask(m_tasks.first().toMap().value("id").toInt(), restore);
        }, restore);
        else { notify(tr("All tasks in the current list have been visited. Return to the task list to change filters.")); emit workflowFinished(); }
    });
    refreshProjectSummary();
}

void ApiClient::saveAndNext() {
    if (m_demo) { save(); return; }
    if(m_samplingQueue){saveCurrent([this]{advanceAfterWrite(-1);});return;}
    int index = -1;
    for (int i = 0; i < m_tasks.size(); ++i) if (m_tasks[i].toMap().value("id").toInt() == m_taskId) index = i;
    if (index < 0) { notify(tr("Return to the task list before submitting and continuing."), true); return; }
    saveCurrent([=] { advanceAfterWrite(index); });
}

void ApiClient::skipTask() {
    if (!mayNavigate() || m_demo || m_taskId < 0) return;
    int index = -1;
    for (int i = 0; i < m_tasks.size(); ++i) if (m_tasks[i].toMap().value("id").toInt() == m_taskId) index = i;
    if (index < 0 && !m_samplingQueue) return;
    request("POST", QString("/api/tasks/%1/annotations/").arg(m_taskId),
        {{"result", QJsonArray{}}, {"was_cancelled", true}}, [=](const QJsonDocument &) { advanceAfterWrite(index); });
}

QVariantList ApiClient::selectedTasks() const {
    QList<int> ids = m_selectedTasks.values(); std::sort(ids.begin(), ids.end());
    QVariantList result; for (int id : ids) result.append(id); return result;
}
QJsonObject ApiClient::selectionQuery()const {
    QJsonArray ids;const auto values=m_allSelected?m_excludedTasks:m_selectedTasks;QList<int> sorted=values.values();std::sort(sorted.begin(),sorted.end());for(int id:sorted)ids.append(id);
    return m_allSelected?QJsonObject{{"all",true},{"excluded",ids}}:QJsonObject{{"all",false},{"included",ids}};
}
void ApiClient::selectAllResults(){if(busy()||m_demo)return;m_selectedTasks.clear();m_excludedTasks.clear();m_allSelected=true;emit changed();}
void ApiClient::selectTask(int id,bool selected){if(busy())return;bool exists=false;for(const auto &row:m_tasks)if(row.toMap().value("id").toInt()==id)exists=true;if(!exists)return;if(m_allSelected){if(selected)m_excludedTasks.remove(id);else m_excludedTasks.insert(id);}else{if(selected)m_selectedTasks.insert(id);else m_selectedTasks.remove(id);}emit changed();}
void ApiClient::selectPage(bool selected){if(busy())return;for(const auto &row:m_tasks){int id=row.toMap().value("id").toInt();if(m_allSelected){if(selected)m_excludedTasks.remove(id);else m_excludedTasks.insert(id);}else{if(selected)m_selectedTasks.insert(id);else m_selectedTasks.remove(id);}}emit changed();}
void ApiClient::clearSelection(){if(busy())return;m_allSelected=false;m_selectedTasks.clear();m_excludedTasks.clear();emit changed();}

void ApiClient::refreshProjectSummary() {
    if (m_project.isEmpty()) return;
    request("GET", QString("/api/projects/%1/").arg(m_project.value("id").toInt()), {}, [this](const QJsonDocument &json) {
        m_project = json.object(); emit changed(); fetchProjects(1);
    });
}

void ApiClient::runTaskAction(const QString &action){m_dataManager->executeAction(action,{});}

bool ApiClient::groundTruth() const {
    for (const auto &item : m_annotations) if (item.toObject().value("id").toInt() == m_document->annotationId())
        return item.toObject().value("ground_truth").toBool();
    return false;
}
void ApiClient::setGroundTruth(bool enabled) {
    if (!mayNavigate() || m_demo || m_document->annotationId() < 0) return;
    const int id = m_document->annotationId();
    request("PATCH", QString("/api/annotations/%1/").arg(id), {{"ground_truth", enabled}}, [=](const QJsonDocument &) {
        for (int i = 0; i < m_annotations.size(); ++i) {
            auto annotation = m_annotations[i].toObject();
            if (annotation.value("id").toInt() == id) annotation["ground_truth"] = enabled;
            else if (enabled) annotation["ground_truth"] = false;
            m_annotations[i] = annotation;
        }
        notify(tr("Ground truth updated."));
    });
}
void ApiClient::deleteAnnotation() {
    if (!mayNavigate() || m_demo || m_document->annotationId() < 0) return;
    const int id = m_document->annotationId();
    request("DELETE", QString("/api/annotations/%1/").arg(id), {}, [=](const QJsonDocument &) {
        for (int i = m_annotations.size() - 1; i >= 0; --i) if (m_annotations[i].toObject().value("id").toInt() == id) m_annotations.removeAt(i);
        if (m_annotations.isEmpty()) m_document->load({}); else m_document->loadAnnotation(m_annotations.first().toObject());
        fetchTasks(m_page); refreshProjectSummary(); notify(tr("Annotation deleted."));
    });
}
