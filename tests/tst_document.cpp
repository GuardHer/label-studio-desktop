#include <QtTest>
#include <QJsonObject>
#include "annotationdocument.h"

static const QString config = QStringLiteral(R"(<View><RectangleLabels name="bbox" toName="img"><Label value="Car" background="#3977ef"/><Label value="Person" background="#f1963c"/></RectangleLabels><Image name="img" value="$photo"/></View>)");

class DocumentTest : public QObject {
    Q_OBJECT
private slots:
    void mixedControlsFramesAliasesAndValidation() {
        AnnotationDocument d;
        QVERIFY(d.configure("<View><Header value='Review'/><Image name='front' valueList='$photos'/><RectangleLabels name='box' toName='front' required='true'><Label value='Car' alias='C' showAlias='true'/></RectangleLabels><PolygonLabels name='outline' toName='front'><Label value='Road'/></PolygonLabels><Image name='rear' value='$rear'/><RectangleLabels name='rearBox' toName='rear'><Label value='Wheel'/></RectangleLabels></View>"));
        d.setTaskData({{"photos",QStringList{"a.png","b.png"}},{"rear","rear.png"}});
        QCOMPARE(d.imageFrames().size(),3); QVERIFY(!d.validate());
        d.setImageSize(100,200); d.addRectangle(1,2,20,30,"Car");
        QCOMPARE(d.results()[0].toObject()["value"].toObject()["rectanglelabels"].toArray()[0].toString(),"C");
        QCOMPARE(d.results()[0].toObject()["item_index"].toInt(),0);
        QCOMPARE(d.labels()[0].toMap()["display"].toString(),"Car C");
        d.selectControl(1); d.addPolygon({QVariantList{1.,2.},QVariantList{30.,2.},QVariantList{20.,40.}},"Road");
        QCOMPARE(d.rectangles().size(),2);
        d.selectImage(1); QCOMPARE(d.rectangles().size(),0); d.setImageSize(50,70);
        d.selectControl(0); d.addRectangle(10,10,30,30,"Car");
        QCOMPARE(d.results()[2].toObject()["item_index"].toInt(),1);
        const auto all=d.results(); QVERIFY(d.load(all,32)); QCOMPARE(d.results(),all); QVERIFY(d.validate());
        d.selectImage(0); d.updatePolygon(1,{QVariantList{2.,2.},QVariantList{30.,2.},QVariantList{20.,40.}});
        QCOMPARE(d.results()[2],all[2]); d.undo(); QCOMPARE(d.results(),all);
    }
    void dynamicLabelsAndBasicControls() {
        AnnotationDocument d;
        QVERIFY(d.configure("<View><Text name='text' value='$text'/><Choices name='class' toName='text' required='true'><Choice value='A'/><Choice value='B'/></Choices><Number name='count' toName='text' min='1' max='3'/><Image name='img' value='$image'/><RectangleLabels name='box' toName='img' value='$labels'/></View>"));
        d.setTaskData({{"text","Example"},{"image","a.png"},{"labels",QVariantList{QVariantMap{{"value","Car"},{"alias","C"},{"showAlias",true},{"hotkey","q"}}}}});
        QCOMPARE(d.labels()[0].toMap()["name"].toString(),"Car"); QVERIFY(!d.validate());
        d.setControlValue("class",QStringList{"A"}); d.setControlValue("count",2); QVERIFY(d.validate());
        const auto saved=d.results(); d.setControlValue("count",4); QCOMPARE(d.results(),saved);
        QVERIFY(d.load(saved,2)); QVERIFY(d.validate());
        QVERIFY(!d.configure("<View visibleWhen='choice-selected'><Image name='img' value='$image'/><RectangleLabels name='box' toName='img'><Label value='Car'/></RectangleLabels></View>"));
    }
    void nativeConstraintsAndUnsupportedSemanticsRemainProtected() {
        AnnotationDocument d;
        QVERIFY(!d.configure("<View><Text name='text' value='$text'/><TextArea name='notes' toName='text' min='3'/></View>"));
        QVERIFY(!d.configure("<View><Text name='text' value='$text'/><Choices name='class' toName='text'><Choice value='A' selected='true'/></Choices></View>"));
        QVERIFY(d.configure("<View><Image name='img' valueList='$images'/><RectangleLabels name='box' toName='img'><Label value='Car' maxUsages='1'/></RectangleLabels></View>"));
        d.setTaskData({{"images",QStringList{"a.png","b.png"}}});d.setImageSize(100,100);
        d.addRectangle(1,2,20,30,"Car"); QVERIFY(d.validate());
        const auto first=d.results();d.selectImage(1);d.addRectangle(1,2,20,30,"Car");QCOMPARE(d.results(),first);
        d.setImageSize(50,70);d.addRectangle(1,2,20,30,"Car");QVERIFY(!d.validate());
        auto bad=first;auto r=bad[0].toObject();r["readonly"]=true;bad[0]=r;QVERIFY(!d.load(bad));QVERIFY(!d.editable());
    }
    void ratingAndSignedNumericRangesMatchConfiguration() {
        AnnotationDocument d;
        QVERIFY(d.configure("<View><Text name='text' value='$text'/><Rating name='stars' toName='text' maxRating='10' required='true'/><Number name='temperature' toName='text' min='-20' max='-1' step='0.5'/></View>"));
        d.setControlValue("stars",8);d.setControlValue("temperature",-5.5);QVERIFY(d.validate());
        const auto saved=d.results();d.setControlValue("stars",11);QCOMPARE(d.results(),saved);
        d.setControlValue("stars",0);QVERIFY(!d.validate());
    }
    void choiceAndShapeAliasesUseBackendResultValues() {
        AnnotationDocument d;
        QVERIFY(d.configure("<View><Image name='img' value='$image'/><Choices name='quality' toName='img'><Choice value='Accept' alias='accepted'/><Choice value='Reject' alias='rejected'/></Choices><RectangleLabels name='box' toName='img'><Label value='Car' alias='vehicle' maxUsages='1'/><Label value='Person' alias='human'/></RectangleLabels></View>"));
        QCOMPARE(d.controls()[0].toMap()["labels"].toList()[0].toMap()["resultValue"].toString(),"accepted");
        d.setControlValue("quality",QStringList{"accepted"});
        QCOMPARE(d.results()[0].toObject()["value"].toObject()["choices"].toArray(),QJsonArray{"accepted"});
        d.setImageSize(100,100);d.addRectangle(1,2,20,30,"Car");
        QCOMPARE(d.results()[1].toObject()["value"].toObject()["rectanglelabels"].toArray(),QJsonArray{"vehicle"});
        QCOMPARE(d.rectangles()[0].toMap()["label"].toString(),"Car");
        const auto saved=d.results();QVERIFY(d.load(saved,44));QCOMPARE(d.results(),saved);
        d.setLabel(0,"Person");QCOMPARE(d.results()[1].toObject()["value"].toObject()["rectanglelabels"].toArray(),QJsonArray{"human"});
        d.undo();QCOMPARE(d.results(),saved);d.addRectangle(30,30,40,40,"Car");QVERIFY(!d.validate());
    }
    void globalControlsRejectUnsupportedValueKeys() {
        AnnotationDocument d;
        QVERIFY(d.configure("<View><Text name='text' value='$text'/><TextArea name='notes' toName='text' maxSubmissions='3'/></View>"));
        d.setControlValue("notes",QStringList{"First","Second"});const auto saved=d.results();QVERIFY(d.load(saved,18));
        QCOMPARE(d.controlValues().value("notes").toStringList(),QStringList({"First","Second"}));
        auto unsupported=saved;auto r=unsupported[0].toObject();auto v=r["value"].toObject();v["unsupported_per_region"]="region-id";r["value"]=v;unsupported[0]=r;
        QVERIFY(!d.load(unsupported,18));QVERIFY(!d.editable());
    }
    void polygonCoordinatesAndHistory() {
        AnnotationDocument d;
        QVERIFY(d.configure("<View><Image name='img' value='$photo'/><PolygonLabels name='outline' toName='img'><Label value='Car'/><Label value='Person'/></PolygonLabels></View>"));
        QCOMPARE(d.property("shapeType").toString(), "polygonlabels");
        d.setImageSize(640,480);
        const QVariantList points{QVariantList{10.,20.},QVariantList{70.,20.},QVariantList{40.,80.}};
        QVERIFY(QMetaObject::invokeMethod(&d,"addPolygon",Q_ARG(QVariantList,points),Q_ARG(QString,QStringLiteral("Car"))));
        QCOMPARE(d.results().size(),1); QVERIFY(d.dirty());
        const auto original = d.results(); const auto region = original[0].toObject();
        QCOMPARE(region["type"].toString(),"polygonlabels");
        QCOMPARE(region["from_name"].toString(),"outline");
        QCOMPARE(region["original_width"].toInt(),640);
        QCOMPARE(region["value"].toObject()["points"].toArray(),QJsonArray::fromVariantList(points));
        d.markSaved(84); QVERIFY(d.load(original,84));
        d.setLabel(0,"Person");
        QCOMPARE(d.results()[0].toObject()["value"].toObject()["polygonlabels"].toArray()[0].toString(),"Person");
        d.undo(); QCOMPARE(d.results(),original);
        auto changed = points; changed[0] = QVariantList{15.,25.};
        QVERIFY(QMetaObject::invokeMethod(&d,"updatePolygon",Q_ARG(int,0),Q_ARG(QVariantList,changed)));
        QVERIFY(d.dirty()); QCOMPARE(d.results()[0].toObject()["id"],region["id"]);
        d.undo(); QCOMPARE(d.results(),original); d.redo(); QVERIFY(d.dirty());
        d.removeRectangle(0); QVERIFY(d.results().isEmpty());
    }
    void malformedPolygonsCannotBeEditedOrSaved() {
        AnnotationDocument d;
        QVERIFY(d.configure("<View><Image name='img' value='$photo'/><PolygonLabels name='outline' toName='img'><Label value='Car'/></PolygonLabels></View>"));
        d.setImageSize(640,480);
        const QVariantList invalid{QVariantList{0.,0.},QVariantList{10.,10.},QVariantList{20.,20.}};
        QVERIFY(QMetaObject::invokeMethod(&d,"addPolygon",Q_ARG(QVariantList,invalid),Q_ARG(QString,QStringLiteral("Car"))));
        QVERIFY(d.results().isEmpty());
        const QVariantList points{QVariantList{10.,20.},QVariantList{70.,20.},QVariantList{40.,80.}};
        QVERIFY(QMetaObject::invokeMethod(&d,"addPolygon",Q_ARG(QVariantList,points),Q_ARG(QString,QStringLiteral("Car"))));
        auto items = d.results(); auto r = items[0].toObject(); auto v = r["value"].toObject();
        v["points"] = QJsonArray{QJsonArray{10,20},QJsonArray{120,20},QJsonArray{40,80}};
        r["value"] = v; items[0] = r;
        QVERIFY(!d.load(items,84)); QVERIFY(!d.editable()); QVERIFY(d.results().isEmpty());
    }
    void parsesConfig() {
        AnnotationDocument d;
        QVERIFY(d.configure(config));
        QCOMPARE(d.imageKey(), "photo");
        QCOMPARE(d.labels().size(), 2);
        QCOMPARE(d.labels()[0].toMap()["name"].toString(), "Car");
        QVERIFY(d.editable());
    }
    void percentCoordinatesAndHistory() {
        AnnotationDocument d;
        QVERIFY(d.configure(config));
        QVERIFY(d.load({}));
        d.setImageSize(1920, 1080);
        d.addRectangle(80, 70, -5, 10, "Car");
        QCOMPARE(d.rectangles().size(), 1);
        auto r = d.results()[0].toObject();
        auto v = r["value"].toObject();
        QCOMPARE(v["x"].toDouble(), 0.0);
        QCOMPARE(v["y"].toDouble(), 10.0);
        QCOMPARE(v["width"].toDouble(), 80.0);
        QCOMPARE(v["height"].toDouble(), 60.0);
        QCOMPARE(r["original_width"].toInt(), 1920);
        QCOMPARE(r["from_name"].toString(), "bbox");
        QCOMPARE(r["to_name"].toString(), "img");
        const auto id = r["id"].toString();
        QVERIFY(!id.isEmpty());
        QVERIFY(d.dirty());
        d.markSaved(72);
        QVERIFY(!d.dirty());
        d.updateRectangle(0, 95, 98, 80, 60);
        v = d.results()[0].toObject()["value"].toObject();
        QCOMPARE(v["x"].toDouble(), 20.0);
        QCOMPARE(v["y"].toDouble(), 40.0);
        d.undo();
        QVERIFY(!d.dirty());
        QCOMPARE(d.results()[0].toObject()["id"].toString(), id);
        d.redo();
        QVERIFY(d.dirty());
        d.removeRectangle(0);
        QVERIFY(d.rectangles().isEmpty());
        d.undo();
        QCOMPARE(d.rectangles().size(), 1);
    }
    void preservesResultRoundTrip() {
        AnnotationDocument d;
        QVERIFY(d.configure(config));
        d.setImageSize(100, 200);
        d.addRectangle(10, 20, 30, 50, "Person");
        auto result = d.results();
        AnnotationDocument copy;
        QVERIFY(copy.configure(config));
        QVERIFY(copy.load(result, 41));
        QCOMPARE(copy.results(), result);
        QCOMPARE(copy.annotationId(), 41);
        QVERIFY(!copy.dirty());
        copy.setLabel(0, "Car");
        QVERIFY(copy.dirty());
        QCOMPARE(copy.results()[0].toObject()["value"].toObject()["rectanglelabels"].toArray()[0].toString(), "Car");
    }
    void refusesUnsupportedWithoutPartialLoad() {
        AnnotationDocument d;
        QVERIFY(d.configure(config));
        d.setImageSize(100, 100);
        d.addRectangle(10, 20, 30, 50, "Car");
        auto items = d.results();
        auto rotated = items[0].toObject();
        auto v = rotated["value"].toObject();
        v["rotation"] = 45;
        rotated["value"] = v;
        items.append(rotated);
        QVERIFY(!d.load(items, 3));
        QVERIFY(!d.editable());
        QVERIFY(!d.error().isEmpty());
        QVERIFY(d.rectangles().isEmpty());
        d.addRectangle(0, 0, 10, 10, "Car");
        QVERIFY(d.rectangles().isEmpty());
    }
    void refusesOtherControlTypes() {
        AnnotationDocument d;
        QVERIFY(!d.configure("<View><Image name='img' value='$image'/><BrushLabels name='c' toName='img'><Label value='A'/></BrushLabels></View>"));
        QVERIFY(!d.editable());
        QVERIFY(!d.error().isEmpty());
        QVERIFY(!d.configure("<View>"));
    }
    void invalidInputDoesNotChangeDocument() {
        AnnotationDocument d;
        QVERIFY(d.configure(config));
        d.setImageSize(100, 100);
        d.addRectangle(10, 10, 10, 20, "Car");
        d.addRectangle(0, 0, 20, 20, "Unknown");
        d.addRectangle(qQNaN(), 0, 20, 20, "Car");
        QVERIFY(d.results().isEmpty());
        QVERIFY(!d.dirty());
    }
};
QTEST_GUILESS_MAIN(DocumentTest)
#include "tst_document.moc"
