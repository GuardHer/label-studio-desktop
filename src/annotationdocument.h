#pragma once
#include <QObject>
#include <QJsonArray>
#include <QVariantList>

class AnnotationDocument : public QObject {
    Q_OBJECT
    Q_PROPERTY(QVariantList rectangles READ rectangles NOTIFY changed)
    Q_PROPERTY(QVariantList labels READ labels NOTIFY changed)
    Q_PROPERTY(bool dirty READ dirty NOTIFY changed)
    Q_PROPERTY(bool editable READ editable NOTIFY changed)
    Q_PROPERTY(bool canUndo READ canUndo NOTIFY changed)
    Q_PROPERTY(bool canRedo READ canRedo NOTIFY changed)
    Q_PROPERTY(QString error READ error NOTIFY changed)
    Q_PROPERTY(QString imageKey READ imageKey NOTIFY changed)
    Q_PROPERTY(QString shapeType READ shapeType NOTIFY changed)
    Q_PROPERTY(int annotationId READ annotationId NOTIFY changed)
    Q_PROPERTY(QVariantList controls READ controls NOTIFY changed)
    Q_PROPERTY(QVariantList imageFrames READ imageFrames NOTIFY changed)
    Q_PROPERTY(QVariantList layoutTree READ layoutTree NOTIFY changed)
    Q_PROPERTY(QVariantMap controlValues READ controlValues NOTIFY changed)
    Q_PROPERTY(int activeImage READ activeImage WRITE selectImage NOTIFY changed)
    Q_PROPERTY(int activeControl READ activeControl WRITE selectControl NOTIFY changed)
    Q_PROPERTY(QString activeImageSource READ activeImageSource NOTIFY changed)
    Q_PROPERTY(QString validationError READ validationError NOTIFY changed)
public:
    explicit AnnotationDocument(QObject *parent = nullptr);
    Q_INVOKABLE bool configure(const QString &xml);
    Q_INVOKABLE bool load(const QJsonArray &results, int annotationId = -1);
    bool loadAnnotation(const QJsonObject &annotation);
    QJsonArray results() const;
    QVariantList rectangles() const;
    QVariantList labels() const;
    bool dirty() const;
    bool editable() const;
    bool canUndo() const;
    bool canRedo() const;
    QString error() const;
    QString imageKey() const;
    QString shapeType() const { return m_shapeType; }
    int annotationId() const;
    Q_INVOKABLE void setImageSize(int width, int height);
    Q_INVOKABLE void setTaskData(const QVariantMap &data);
    Q_INVOKABLE void selectImage(int index);
    Q_INVOKABLE void selectControl(int index);
    Q_INVOKABLE bool validate();
    Q_INVOKABLE void setControlValue(const QString &name, const QVariant &value);
    QVariantList controls() const;
    QVariantList imageFrames() const { return m_frames; }
    QVariantList layoutTree() const { return m_layout; }
    QVariantMap controlValues() const;
    int activeImage() const { return m_activeImage; }
    int activeControl() const { return m_activeControl; }
    QString activeImageSource() const;
    QString validationError() const { return m_validationError; }
    void markSaved(int id);
    Q_INVOKABLE void addRectangle(double x1, double y1, double x2, double y2, const QString &label);
    Q_INVOKABLE void addPolygon(const QVariantList &points, const QString &label);
    Q_INVOKABLE void updatePolygon(int index, const QVariantList &points);
    Q_INVOKABLE void updateRectangle(int index, double x, double y, double width, double height);
    Q_INVOKABLE void setLabel(int index, const QString &label);
    Q_INVOKABLE void removeRectangle(int index);
    Q_INVOKABLE void undo();
    Q_INVOKABLE void redo();
signals:
    void changed();
    void activeImageChanged();
private:
    bool fail(const QString &message);
    bool knownLabel(const QString &name) const;
    void commit(const QJsonArray &next);
    void refreshActive();
    QVariantMap control(const QString &name) const;
    QVariantList labelsFor(const QVariantMap &control) const;
    QString resultLabel(const QVariantMap &control, const QString &name) const;
    int resultIndex(int visibleIndex) const;
    bool visibleResult(const QJsonObject &region) const;
    QVariant resolve(const QString &value) const;
    bool validControlValue(const QVariantMap &control, const QJsonObject &value) const;
    QJsonObject regionBase(const QString &type) const;
    QString m_from, m_to, m_imageKey, m_error, m_shapeType;
    QVariantList m_labels;
    QJsonArray m_results, m_saved;
    QList<QJsonArray> m_history;
    int m_historyIndex = 0, m_id = -1, m_width = 0, m_height = 0;
    bool m_configured = false, m_editable = false;
    QVariantList m_controls, m_images, m_frames, m_layout;
    QVariantMap m_taskData;
    int m_activeImage = -1, m_activeControl = -1;
    QString m_validationError;
};
