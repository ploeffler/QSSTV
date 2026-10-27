#include "mmsstvimport.h"
#include "appglobal.h"
// Integration test; link with application objects except main.o (Qt Widgets).
#include "mmsstvtemplate.h"
#include "templatemacros.h"
#include "editorscene.h"
#include "editor.h"
#include "editorview.h"
#include <QFileDialog>
#include <QMessageBox>
#include <QTimer>
#include <QLineEdit>
#include <QTextEdit>
#include <QLabel>
#include <QCheckBox>
#include <QSpinBox>
#include <QMouseEvent>
#include <functional>
#include <QApplication>
#include <QBuffer>
#include <QDataStream>
#include <QFile>
#include <QDir>
#include <QFileInfo>
#include <QPainter>
#include <QTemporaryDir>
#include <QTextStream>
#include <QtEndian>
#include <stdexcept>

#define CHECK(c) do { if(!(c)) throw std::runtime_error(#c); } while(false)
static void integer(QByteArray &b,qint32 value) {
  char bytes[4];qToLittleEndian<qint32>(value,reinterpret_cast<uchar *>(bytes));b.append(bytes,4);
}
static void text(QByteArray &b,const QByteArray &value){integer(b,value.size());b+=value;}
static QByteArray base(int command,int version) {
  QByteArray b;
  for(int value:{command,version,10,20,100,50,0x000000ff})integer(b,value);
  b.append(char(0));integer(b,1);return b;
}
static QByteArray group(const QList<QByteArray> &records) {
  QByteArray b=base(1,2);
  for(int value:{0,0,0x00f8f8f8,320,256,records.size()})integer(b,value);
  for(const QByteArray &r:records)b+=r;
  return b;
}
static QByteArray shapeRecord(int command,int shape,int pen) {
  QByteArray b=base(command,0);b[28]=char(pen);b.chop(4);
  integer(b,0x55aa0000);integer(b,shape);integer(b,1);return b;
}
static QByteArray textRecord(int grade=0,int shadow=0) {
  QByteArray b=base(4,7);
  for(int v:{grade,shadow,0,0,10,20,0,0,0,0,0,-6})integer(b,v);
  text(b,"%m %n %q");for(int value:{0x000000ff,0x0000ff00,0x00ff0000,0x00ffffff,0x00223344,0})integer(b,value);
  text(b,"Sans");for(int v:{0,-20,1,0})integer(b,v);
  if(grade==3) {
    integer(b,4);integer(b,4);
    QImage tile(4,4,QImage::Format_RGB32);tile.fill(Qt::red);
    for(int y=0;y<4;++y)for(int x=0;x<4;++x)if((x+y)%2)tile.setPixelColor(x,y,Qt::blue);
    QBuffer buffer(&b);buffer.open(QIODevice::Append);CHECK(tile.save(&buffer,"BMP"));
  }
  return b;
}
static QByteArray picture(int shape=0,int type=1,bool legacy=false) {
  QByteArray b=base(5,5);
  for(int v:{type,shape,0,0})integer(b,v);
  if(type) {
    int size=shape?32:2;
    integer(b,size);integer(b,size);
    QImage image(size,size,QImage::Format_RGB32);image.fill(Qt::green);
    QBuffer buffer(&b);buffer.open(QIODevice::Append);CHECK(image.save(&buffer,"BMP"));
  }
  if(shape==5) {
    if(!legacy)for(int v:{0x55aa2233,3,320,256})integer(b,v);
    else integer(b,3);
    for(int v:{0,0,legacy?256:320,0,0,legacy?200:256})integer(b,v);
  }
  return b;
}

static QString store(const QString &root,const QByteArray &bytes) {
  QString name=root+"/input.mtm";QFile f(name);CHECK(f.open(QIODevice::WriteOnly));CHECK(f.write(bytes)==bytes.size());return name;
}
static QImage render(editorScene &scene) {
  return scene.renderImage(320,256)->copy();
}

static void roundTrip(const Mmsstv::Document &doc,const QString &root) {
  editorScene source;source.importMmsstv(doc);
  CHECK(source.getImageType()==editorScene::TEMPLATE);
  QImage before=render(source);
  QFile saved(root+"/converted.templ");CHECK(source.save(saved,true));
  editorScene restored;CHECK(restored.load(saved));
  CHECK(restored.border.size()==source.border.size());
  int sourceCount=0,restoredCount=0;
  for(QGraphicsItem *item:source.items())if(item->type()!=graphItemBase::SBORDER)++sourceCount;
  for(QGraphicsItem *item:restored.items())if(item->type()!=graphItemBase::SBORDER)++restoredCount;
  CHECK(sourceCount==restoredCount);
  CHECK(render(restored)==before);
}
struct UiResult { bool preview=false, replacement=false, warning=false, timeout=false; };
static UiResult driveUi(const std::function<void()> &action,const QString &path,
                        bool cancelFile=false,bool acceptPreview=true,bool discard=true)
{
  UiResult result;
  QTimer poll;
  int ticks=0;
  QObject::connect(&poll,&QTimer::timeout,[&]() {
    QWidget *modal=QApplication::activeModalWidget();
    if(!modal)for(QWidget *widget:QApplication::topLevelWidgets())
      if(widget->isVisible() && qobject_cast<QDialog *>(widget))modal=widget;

    if (++ticks>250) {
      result.timeout=true;
      if (QDialog *dialog=qobject_cast<QDialog *>(modal))dialog->reject();
      return;
    }
    if (QFileDialog *dialog=qobject_cast<QFileDialog *>(modal)) {
      if(cancelFile)dialog->reject();
      else {
        dialog->selectFile(path);
        if(QLineEdit *name=dialog->findChild<QLineEdit *>("fileNameEdit"))name->setText(path);
        QMetaObject::invokeMethod(dialog,"accept");
      }
    } else if (QMessageBox *message=qobject_cast<QMessageBox *>(modal)) {
      if(message->windowTitle()=="Replace template") {
        result.replacement=true;
        message->button(discard?QMessageBox::Discard:QMessageBox::Cancel)->click();
      } else {result.warning=true;message->button(QMessageBox::Ok)->click();}
    } else if (QDialog *dialog=qobject_cast<QDialog *>(modal)) {
      if(dialog->windowTitle()=="MMSSTV import preview") {
        result.preview=true;
        const QByteArray artifacts=qgetenv("QSSTV_TEST_ARTIFACTS");
        if(!artifacts.isEmpty())dialog->grab().save(QString::fromUtf8(artifacts)+"/import-preview.png");
        dialog->done(acceptPreview?QDialog::Accepted:QDialog::Rejected);
      } else dialog->reject();
    }
  });
  poll.start(20);action();poll.stop();
  CHECK(!result.timeout);
  return result;
}
static void driveTransparency(const std::function<void()> &action,bool enabled,const QPoint &point,bool accept,bool byClick=false)
{
  QTimer poll;int ticks=0;bool seen=false,failed=false;
  QObject::connect(&poll,&QTimer::timeout,[&]() {
    QDialog *dialog=qobject_cast<QDialog *>(QApplication::activeModalWidget());
    if(++ticks>250){failed=true;if(dialog)dialog->reject();return;}
    if(!dialog)return;
    if(QMessageBox *message=qobject_cast<QMessageBox *>(dialog)) {
      failed=true;message->button(QMessageBox::Ok)->click();return;
    }
    QCheckBox *toggle=dialog->findChild<QCheckBox *>("enableSampledTransparency");
    QSpinBox *x=dialog->findChild<QSpinBox *>("transparencyX"),*y=dialog->findChild<QSpinBox *>("transparencyY");
    if(!toggle || !x || !y){failed=true;dialog->reject();return;}
    seen=true;toggle->setChecked(enabled);
    if(byClick) {
      QGraphicsView *view=dialog->findChild<QGraphicsView *>("transparencyPreview");
      if(!view){failed=true;dialog->reject();return;}
      auto click=[&](const QPoint &position,Qt::MouseButton button=Qt::LeftButton) {
        QMouseEvent press(QEvent::MouseButtonPress,QPointF(position),button,button,Qt::NoModifier);
        QApplication::sendEvent(view->viewport(),&press);
        QMouseEvent release(QEvent::MouseButtonRelease,QPointF(position),button,Qt::NoButton,Qt::NoModifier);
        QApplication::sendEvent(view->viewport(),&release);
      };
      const QPoint initial(x->value(),y->value());
      const QPoint margin=view->mapFromScene(QPointF(view->sceneRect().left()-1,view->sceneRect().center().y()));
      if(!view->viewport()->rect().contains(margin)){failed=true;dialog->reject();return;}
      click(margin);
      if(QPoint(x->value(),y->value())!=initial){failed=true;dialog->reject();return;}
      const QPoint target=view->mapFromScene(QPointF(point)+QPointF(0.5,0.5));
      click(target,Qt::RightButton);
      if(QPoint(x->value(),y->value())!=initial){failed=true;dialog->reject();return;}
      click(target);
      if(QPoint(x->value(),y->value())!=(enabled?point:initial)){failed=true;dialog->reject();return;}
    } else {x->setValue(point.x());y->setValue(point.y());}
    const QByteArray artifacts=qgetenv("QSSTV_TEST_ARTIFACTS");
    if(!artifacts.isEmpty())dialog->grab().save(QString::fromUtf8(artifacts)+"/transparency-editor.png");
    if(accept)dialog->accept();else dialog->reject();
  });
  poll.start(20);action();poll.stop();CHECK(seen && !failed);
}
struct BatchUiResult {
  bool files=false, destination=false, report=false, failed=false;
  QString text, summary;
};
static BatchUiResult driveBatchUi(const std::function<void()> &action,const QStringList &paths,
                                  const QString &destination,int cancelStage=0)
{
  BatchUiResult result;QTimer poll;int ticks=0;
  QObject::connect(&poll,&QTimer::timeout,[&]() {
    QDialog *modal=qobject_cast<QDialog *>(QApplication::activeModalWidget());
    if(++ticks>250) {QTextStream(stderr)<<"Batch timeout: "<<(modal?modal->windowTitle():QString("no modal"))<<"\n";result.failed=true;if(modal)modal->reject();return;}
    if(!modal)return;
    if(QFileDialog *dialog=qobject_cast<QFileDialog *>(modal)) {
      if(dialog->fileMode()==QFileDialog::ExistingFiles) {
        result.files=true;
        if(cancelStage==1){dialog->reject();return;}
        dialog->setDirectory(QFileInfo(paths.first()).absolutePath());
        QStringList quoted;
        for(const QString &path:paths)quoted.append("\""+path+"\"");
        QLineEdit *name=dialog->findChild<QLineEdit *>("fileNameEdit");
        if(!name){result.failed=true;dialog->reject();return;}
        name->setText(quoted.join(" "));
      } else if(dialog->fileMode()==QFileDialog::Directory || dialog->fileMode()==QFileDialog::DirectoryOnly) {
        result.destination=true;
        if(cancelStage==2){dialog->reject();return;}
        dialog->setDirectory(destination);
      } else {result.failed=true;dialog->reject();return;}
      QMetaObject::invokeMethod(dialog,"accept");
    } else if(modal->windowTitle()=="MMSSTV batch import report") {
      result.report=true;
      if(QTextEdit *text=modal->findChild<QTextEdit *>())result.text=text->toPlainText();
      for(QLabel *label:modal->findChildren<QLabel *>())result.summary+=label->text();
      const QByteArray artifacts=qgetenv("QSSTV_TEST_ARTIFACTS");
      if(!artifacts.isEmpty())modal->grab().save(QString::fromUtf8(artifacts)+"/batch-import-report.png");
      modal->reject();
    } else {QTextStream(stderr)<<"Unexpected batch modal: "<<modal->windowTitle()<<"\n";result.failed=true;modal->reject();}
  });
  poll.start(20);action();poll.stop();CHECK(!result.failed);return result;
}
static void batchEditorWorkflow(editor &window,editorView &view,const QByteArray &bytes,const QString &root)
{
  const QString inputs=root+"/batch UI inputs",outputs=root+"/batch UI outputs";
  CHECK(QDir().mkpath(inputs) && QDir().mkpath(outputs));
  const QString valid=inputs+QString::fromUtf8("/gültige Vorlage.mtm"),invalid=inputs+"/broken.mtm";
  QFile source(valid);CHECK(source.open(QIODevice::WriteOnly));CHECK(source.write(bytes)==bytes.size());source.close();
  QFile corrupt(invalid);CHECK(corrupt.open(QIODevice::WriteOnly));corrupt.write("broken");corrupt.close();
  const QString occupied=outputs+QString::fromUtf8("/gültige Vorlage.templ");
  QFile sentinel(occupied);CHECK(sentinel.open(QIODevice::WriteOnly));sentinel.write("existing");sentinel.close();
  const QStringList initialEntries=QDir(outputs).entryList(QDir::Files);
  const QImage before=render(*view.getScene());const QString title=window.windowTitle();
  CHECK(view.isModified());
  QAction *batchAction=nullptr;
  for(QAction *action:window.findChildren<QAction *>())
    if(action->text()=="Batch import MMSSTV templates...")batchAction=action;
  CHECK(batchAction);
  auto batch=[&](){batchAction->trigger();};
  const QStringList paths={valid,invalid};
  auto canceled=driveBatchUi(batch,paths,outputs,1);
  CHECK(canceled.files && !canceled.destination && !canceled.report);
  CHECK(QDir(outputs).entryList(QDir::Files)==initialEntries);
  canceled=driveBatchUi(batch,paths,outputs,2);
  CHECK(canceled.files && canceled.destination && !canceled.report);
  CHECK(QDir(outputs).entryList(QDir::Files)==initialEntries);
  const auto result=driveBatchUi(batch,paths,outputs);
  CHECK(result.files && result.destination && result.report);
  CHECK(result.summary=="Converted 1 of 2 templates.");
  const QString converted=outputs+QString::fromUtf8("/gültige Vorlage_2.templ");
  CHECK(result.text.contains(valid) && result.text.contains(invalid) && result.text.contains(converted));
  CHECK(result.text.contains("Saved:") && result.text.contains("Error:") && result.text.contains("Warning:"));
  CHECK(QDir(outputs).entryList(QDir::Files).size()==2);
  QFile saved(converted);editorScene restored;CHECK(restored.load(saved));
  Mmsstv::Document document;QString error;CHECK(Mmsstv::readTemplate(valid,&document,&error));
  editorScene expected;expected.importMmsstv(document);CHECK(render(restored)==render(expected));
  CHECK(source.open(QIODevice::ReadOnly));CHECK(source.readAll()==bytes);source.close();
  CHECK(sentinel.open(QIODevice::ReadOnly));CHECK(sentinel.readAll()=="existing");sentinel.close();
  CHECK(view.isModified() && render(*view.getScene())==before && window.windowTitle()==title);
}
static void editorWorkflow(const QByteArray &bytes,const QString &root)
{
  QString input=store(root,bytes),error;
  Mmsstv::Document original;
  original.items.append(Mmsstv::Item());
  original.items[0].rect=QRectF(20,20,10,10);
  editor window;
  editorView *view=qobject_cast<editorView *>(window.centralWidget());CHECK(view);
  view->importMmsstv(original);
  batchEditorWorkflow(window,*view,bytes,root);
  const QImage before=render(*view->getScene());
  QAction *importAction=nullptr;
  for(QAction *a:window.findChildren<QAction *>())
    if(a->text()=="Import MMSSTV template...")importAction=a;
  CHECK(importAction);
  auto import=[&](){importAction->trigger();};
  UiResult r=driveUi(import,input,true);
  CHECK(!r.preview && render(*view->getScene())==before);
  r=driveUi(import,input,false,false);
  CHECK(r.preview && !r.replacement && render(*view->getScene())==before);
  r=driveUi(import,input,false,true,false);
  CHECK(r.preview && r.replacement && render(*view->getScene())==before);
  r=driveUi(import,input);
  CHECK(r.preview && r.replacement && !r.warning);
  CHECK(view->isModified());
  CHECK(window.windowTitle().contains("unsaved"));
  QFile unchanged(input);CHECK(unchanged.open(QIODevice::ReadOnly));CHECK(unchanged.readAll()==bytes);unchanged.close();
  const QString output=root+"/ui-converted.templ";
  r=driveUi([&](){window.slotFileSave();},output);
  CHECK(!r.warning && QFileInfo::exists(output) && !view->isModified());
  QFile saved(output);editorScene reloaded;CHECK(reloaded.load(saved));
  CHECK(render(reloaded)==render(*view->getScene()));
  QAction *transparencyAction=nullptr;
  for(QAction *a:window.findChildren<QAction *>())if(a->text()=="Template transparency...")transparencyAction=a;
  CHECK(transparencyAction);
  auto transparency=[&](){transparencyAction->trigger();};
  const QImage beforeSettings=render(*view->getScene());
  driveTransparency(transparency,false,QPoint(15,25),false,true);
  CHECK(!view->isModified() && render(*view->getScene())==beforeSettings);
  driveTransparency(transparency,true,QPoint(15,25),true,true);
  CHECK(view->isModified() && view->getScene()->hasTemplateTransparency());
  CHECK(view->getScene()->templateTransparencyPoint()==QPoint(15,25));
  CHECK(render(*view->getScene()).pixelColor(15,25).alpha()==0);
  CHECK(view->setTemplateTransparency(true,QColor(12,34,56),QPoint(15,25),&error));
  CHECK(render(*view->getScene()).pixelColor(200,200)==QColor(12,34,56));
  QFile settingsSaved(root+"/edited-transparency.templ");CHECK(view->save(settingsSaved,true));
  editorScene settingsRestored;CHECK(settingsRestored.load(settingsSaved));
  CHECK(settingsRestored.hasTemplateTransparency() && settingsRestored.templateTransparencyPoint()==QPoint(15,25));
  CHECK(settingsRestored.templateTransparencyBackground()==QColor(12,34,56));
  driveTransparency(transparency,false,QPoint(0,0),false);
  CHECK(!view->isModified() && view->getScene()->hasTemplateTransparency());
  driveTransparency(transparency,false,QPoint(15,25),true);
  CHECK(view->isModified() && !view->getScene()->hasTemplateTransparency());
  CHECK(render(*view->getScene()).pixelColor(15,25).alpha()==255);
  QFile settingsOff(root+"/disabled-transparency.templ");CHECK(view->save(settingsOff,true));
  CHECK(settingsOff.open(QIODevice::ReadOnly));
  {QDataStream header(&settingsOff);header.setVersion(QDataStream::Qt_4_4);
    quint32 magic;QString version;header>>magic>>version;CHECK(version=="9.0");}
  settingsOff.close();
  driveTransparency(transparency,false,QPoint(15,25),true);CHECK(!view->isModified());
  QString settingsError;
  CHECK(!view->setTemplateTransparency(true,Qt::white,QPoint(9999,0),&settingsError));
  CHECK(!settingsError.isEmpty() && !view->isModified() && !view->getScene()->hasTemplateTransparency());
  // A failed save must preserve dirty state and not change the source MTM.
  view->importMmsstv(original);
  CHECK(QFile::remove(output));CHECK(QDir().mkpath(output));
  r=driveUi([&](){window.slotFileSave();},QString());
  CHECK(r.warning && view->isModified());
  // Exporting an imported template as PNG must not turn subsequent Save
  // into native template bytes written under a .png filename.
  const QString exported=root+"/ui-export.png";
  r=driveUi([&](){window.slotFileSaveImage();},exported);
  CHECK(!r.warning && !QImage(exported).isNull());
  view->importMmsstv(original);
  r=driveUi([&](){window.slotFileSave();},QString());
  CHECK(!r.warning && !view->isModified() && !QImage(exported).isNull());
  // PNG uses the same atomic path and reports encoding/write failures.
  QFile png(root+"/flat.png");CHECK(view->save(png,false));
  CHECK(!QImage(png.fileName()).isNull());
  view->importMmsstv(original);
  QFile impossible(root+"/missing/directory/flat.png");
  CHECK(!view->save(impossible,false) && view->isModified());
}
int main(int argc,char **argv) {
  QApplication::setAttribute(Qt::AA_DontUseNativeDialogs);
  QApplication app(argc,argv);QTemporaryDir temp;
  QCoreApplication::setApplicationVersion(MAJORVERSION+MINORVERSION);
  CHECK(temp.isValid());qputenv("XDG_CONFIG_HOME",temp.path().toUtf8());
  QCoreApplication::setOrganizationName("QSSTV-test");QCoreApplication::setApplicationName("import-test");
  try {
    CHECK(qsstvVersion=="QSSTV 9.6.0");
    CHECK(CONFIGVERSION=="9.0" && APPLICATION=="qsstv_9.0");
    QByteArray bytes=group({base(2,0),base(3,0),base(6,0),textRecord(),picture(),group({base(3,0)})});
    Mmsstv::Document doc;QString error;
    CHECK(Mmsstv::readTemplate(store(temp.path(),bytes),&doc,&error));
    CHECK(doc.items.size()==6);CHECK(doc.items[3].text=="%m %o %%q");
    CHECK(doc.items[4].image.pixelColor(0,0)==QColor(Qt::green));
    roundTrip(doc,temp.path());
    for(int length=0;length<bytes.size();++length) {
      Mmsstv::Document unchanged;unchanged.size=QSize(123,456);
      CHECK(!Mmsstv::readTemplate(store(temp.path(),bytes.left(length)),&unchanged,&error));
      CHECK(unchanged.size==QSize(123,456));
    }
    CHECK(!Mmsstv::readTemplate(store(temp.path(),bytes+"extra"),&doc,&error));
    CHECK(!Mmsstv::readTemplate(store(temp.path(),group({base(8,1)})),&doc,&error));
    QByteArray invalid=bytes;char huge[4];qToLittleEndian<qint32>(0x7fffffff,reinterpret_cast<uchar *>(huge));invalid.replace(53,4,huge,4);
    CHECK(!Mmsstv::readTemplate(store(temp.path(),invalid),&doc,&error));
    QStringList warnings;
    CHECK(Mmsstv::convertMacros("%m %c %n %r %T %D %q %s %%",&warnings)=="%m %c %o %r %t %d %%q %%s %%");
    Mmsstv::Document styled;
    CHECK(Mmsstv::readTemplate(store(temp.path(),group({textRecord(1,1),textRecord(2,0)})),&styled,&error));
    CHECK(styled.items[0].gradientDirection==0 && styled.items[1].gradientDirection==270);
    CHECK(styled.items[0].gradientColors==QVector<QColor>({Qt::red,Qt::green,Qt::blue,Qt::white}));
    CHECK(styled.items[0].color==QColor(0x44,0x33,0x22));
    CHECK(styled.items[0].penStyle==0 && styled.items[0].penWidth==2);
    editorScene styledScene;styledScene.importMmsstv(styled);
    bool horizontal=false,vertical=false;
    for(QGraphicsItem *graphic:styledScene.items()) {
      if(graphic->type()!=graphItemBase::TEXT)continue;
      graphItemBase *item=static_cast<graphItemBase *>(graphic);
      sitemParam param=item->getParam();
      CHECK(param.gradient.type==sgradientParam::LINEAR);
      CHECK(param.gradient.color1==QColor(Qt::red) && param.gradient.color4==QColor(Qt::white));
      if(param.gradient.direction==0){horizontal=true;CHECK(item->pen().width()==2);}
      if(param.gradient.direction==270)vertical=true;
    }
    CHECK(horizontal && vertical);
    roundTrip(styled,temp.path());
    const QByteArray artifacts=qgetenv("QSSTV_TEST_ARTIFACTS");
    if(!artifacts.isEmpty()) {
      Mmsstv::Document preview=styled;
      for(int i=0;i<preview.items.size();++i) {
        preview.items[i].text="Gradient";
        preview.items[i].font.setFamily("Sans");
        preview.items[i].font.setPixelSize(42);
        preview.items[i].rect=QRectF(20,40+i*90,280,60);
      }
      editorScene scene;scene.importMmsstv(preview);
      CHECK(render(scene).save(QString::fromUtf8(artifacts)+"/text-style-preview.png"));
    }
    Mmsstv::Document masks;
    CHECK(Mmsstv::readTemplate(store(temp.path(),group({picture(1),picture(2),picture(3),picture(4),picture(5),picture(5,1,true)})),&masks,&error));
    CHECK(masks.items.size()==6);
    for(int i=0;i<4;++i) {
      CHECK(masks.items[i].image.pixelColor(0,0).alpha()==0);
      CHECK(masks.items[i].image.pixelColor(16,16)==QColor(Qt::green));
    }
    for(int i=4;i<6;++i) {
      CHECK(masks.items[i].image.pixelColor(4,4)==QColor(Qt::green));
      CHECK(masks.items[i].image.pixelColor(30,30).alpha()==0);
    }
    roundTrip(masks,temp.path());
    if(!artifacts.isEmpty()) {
      Mmsstv::Document preview=masks;
      for(int i=0;i<preview.items.size();++i)
        preview.items[i].rect=QRectF(10+(i%3)*100,20+(i/3)*110,90,90);
      editorScene scene;scene.importMmsstv(preview);
      CHECK(render(scene).save(QString::fromUtf8(artifacts)+"/image-mask-preview.png"));
    }
    QByteArray brokenPolygon=picture(5);
    qToLittleEndian<qint32>(0x7fffffff,reinterpret_cast<uchar *>(brokenPolygon.data()+brokenPolygon.size()-36));
    CHECK(!Mmsstv::readTemplate(store(temp.path(),group({brokenPolygon})),&masks,&error));
    QByteArray truncatedMask=group({picture(5)});
    CHECK(!Mmsstv::readTemplate(store(temp.path(),truncatedMask.left(truncatedMask.size()-1)),&masks,&error));
    Mmsstv::Document textured;
    CHECK(Mmsstv::readTemplate(store(temp.path(),group({textRecord(3)})),&textured,&error));
    CHECK(textured.items.size()==1 && textured.items[0].image.size()==QSize(4,4));
    CHECK(textured.items[0].image.pixelColor(0,0)==QColor(Qt::red));
    CHECK(textured.items[0].image.pixelColor(1,0)==QColor(Qt::blue));
    roundTrip(textured,temp.path());
    editorScene textureScene;textureScene.importMmsstv(textured);
    QImage painted=render(textureScene);int red=0,blue=0;
    for(int y=0;y<painted.height();++y)for(int x=0;x<painted.width();++x) {
      QColor color=painted.pixelColor(x,y);
      if(color.alpha()==0)continue;
      if(color.red()>200 && color.blue()<30)++red;
      if(color.blue()>200 && color.red()<30)++blue;
    }
    CHECK(red>10 && blue>10);
    graphItemBase *textItem=nullptr;
    for(QGraphicsItem *graphic:textureScene.items())if(graphic->type()==graphItemBase::TEXT)textItem=static_cast<graphItemBase *>(graphic);
    CHECK(textItem && !textItem->getParam().im.isNull());
    textItem->setSelected(true);textureScene.fillColor=Qt::yellow;
    textureScene.apply(editorScene::DFILLCOLOR);
    CHECK(textItem->getParam().im.isNull() && textItem->getParam().fillColor==QColor(Qt::yellow));
    textItem->setImage(textured.items[0].image);
    textureScene.apply(editorScene::DGRADIENT);
    CHECK(textItem->getParam().im.isNull());
    QByteArray truncatedTexture=group({textRecord(3)});
    CHECK(!Mmsstv::readTemplate(store(temp.path(),truncatedTexture.left(truncatedTexture.size()-1)),&textured,&error));
    macroExpansion context;
    QStringList clockWarnings;
    QString clock=Mmsstv::convertMacros("%C|%T|%t|%D|%L|%U|%u",&clockWarnings);
    CHECK(clock=="%C|%t|%T|%d|%D|%U|%u");
    QTimeZone vienna("Europe/Vienna");CHECK(vienna.isValid());
    configureTemplateContext(context,"OE6PLD",QDateTime(QDate(2026,7,1),QTime(22,15),Qt::UTC),vienna);
    CHECK(context.convert(clock)=="oe6pld|22:15|2215|2026/07/01|2026/07/02|00:15|0015");
    configureTemplateContext(context,"OE6PLD",QDateTime(QDate(2026,1,1),QTime(22,15),Qt::UTC),vienna);
    CHECK(context.convert(clock)=="oe6pld|22:15|2215|2026/01/01|2026/01/01|23:15|2315");
    CHECK(context.convert("%c|%%C|%%T")=="OE6PLD|%C|%T");
    QStringList reportWarnings;
    QString reportMacros=Mmsstv::convertMacros("%r|%R|%N|%s|%M",&reportWarnings);
    CHECK(reportMacros=="%r|%R|%N|%%s|%%M");
    addTemplateReportConversions(context,"595#007");
    CHECK(context.convert(reportMacros)=="595#007|595|#007|%s|%M");
    const QList<QPair<QString,QString>> reports={
      {"","||"},{"59","59|59|"},{"595","595|595|"},
      {"595 012","595 012|595| 012"},{"Good Copy","Good Copy|Goo|d Copy"}
    };
    for(const auto &test:reports) {
      configureTemplateContext(context,"OE6PLD",QDateTime(QDate(2026,1,1),QTime(22,15),Qt::UTC),vienna);
      addTemplateReportConversions(context,test.first);
      CHECK(context.convert("%r|%R|%N")==test.second);
    }
    Mmsstv::Document forms;
    CHECK(Mmsstv::readTemplate(store(temp.path(),group({shapeRecord(3,4,0),shapeRecord(6,5,255),
       shapeRecord(3,1,0),shapeRecord(3,2,0),shapeRecord(6,3,255),shapeRecord(6,0,255),
       shapeRecord(2,0,5),shapeRecord(3,0,6)})),&forms,&error));
    CHECK(forms.items[0].kind==Mmsstv::Item::Ellipse && forms.items[1].kind==Mmsstv::Item::Ellipse);
    CHECK(forms.items[0].fill.alpha()==0);
    CHECK(forms.items[1].fill==QColor(Qt::red) && forms.items[1].penStyle==5);
    for(int i=2;i<=4;++i)CHECK(forms.items[i].kind==Mmsstv::Item::Image && !forms.items[i].image.isNull());
    CHECK(forms.items[5].kind==Mmsstv::Item::Rectangle && forms.items[5].penStyle==5);
    CHECK(forms.items[6].penStyle==0 && forms.items[7].penStyle==0);
    roundTrip(forms,temp.path());
    Mmsstv::Document filled;filled.items.append(forms.items[1]);
    editorScene ellipse;ellipse.importMmsstv(filled);QImage ellipsePixels=render(ellipse);
    CHECK(ellipsePixels.pixelColor(55,35)==QColor(Qt::red));
    CHECK(ellipsePixels.pixelColor(10,20).alpha()==0);
    bool editableEllipse=false;
    for(QGraphicsItem *graphic:ellipse.items())if(graphic->type()==graphItemBase::ELLIPSE)editableEllipse=true;
    CHECK(editableEllipse);
    CHECK(!Mmsstv::readTemplate(store(temp.path(),group({shapeRecord(3,0,254)})),&forms,&error));
    CHECK(!Mmsstv::readTemplate(store(temp.path(),group({shapeRecord(3,6,0)})),&forms,&error));
    Mmsstv::Document keyed;
    keyed.colorKey=true;keyed.background=QColor(248,248,248);keyed.size=QSize(320,256);
    keyed.transparencyPoint=QPoint(30,30);
    Mmsstv::Item under;under.kind=Mmsstv::Item::Rectangle;under.rect=QRectF(10,10,80,80);
    under.fill=Qt::red;under.penStyle=5;under.penWidth=0;
    Mmsstv::Item hole=under;hole.rect=QRectF(20,20,40,40);hole.fill=Qt::blue;
    keyed.items={under,hole};
    editorScene keyScene;keyScene.importMmsstv(keyed);QImage keyedPixels=render(keyScene);
    CHECK(keyedPixels.pixelColor(30,30).alpha()==0);
    CHECK(keyedPixels.pixelColor(15,15)==QColor(Qt::red));
    CHECK(keyedPixels.pixelColor(200,200)==QColor(248,248,248));
    QImage baseImage(320,256,QImage::Format_RGB32);baseImage.fill(Qt::green);
    keyScene.overlay(&baseImage);
    CHECK(keyScene.getImagePtr()->pixelColor(30,30)==QColor(Qt::green));
    CHECK(keyScene.getImagePtr()->pixelColor(15,15)==QColor(Qt::red));
    roundTrip(keyed,temp.path());
    QFile keyedNative(temp.path()+"/keyed.templ");CHECK(keyScene.save(keyedNative,true));
    CHECK(keyedNative.open(QIODevice::ReadOnly));
    {QDataStream header(&keyedNative);header.setVersion(QDataStream::Qt_4_4);
      quint32 magic;QString version;header>>magic>>version;CHECK(version=="9.6");}
    keyedNative.close();
    Mmsstv::Document legacy;legacy.items.append(under);
    editorScene legacyScene;legacyScene.importMmsstv(legacy);
    QFile legacyNative(temp.path()+"/legacy.templ");CHECK(legacyScene.save(legacyNative,true));
    CHECK(legacyNative.open(QIODevice::ReadOnly));
    {QDataStream header(&legacyNative);header.setVersion(QDataStream::Qt_4_4);
      quint32 magic;QString version;header>>magic>>version;CHECK(version=="9.0");}
    legacyNative.close();
    QFile invalidNative(temp.path()+"/invalid-key.templ");CHECK(invalidNative.open(QIODevice::WriteOnly));
    {QDataStream stream(&invalidNative);stream.setVersion(QDataStream::Qt_4_4);
      stream<<quint32(0x34515a53)<<QString("9.6")<<quint16(QDataStream::Qt_4_4)
            <<qint32(0x4d4d434b)<<QColor(Qt::white)<<QPoint(9999,0)<<QSize(320,256);}
    invalidNative.close();editorScene invalidScene;CHECK(!invalidScene.load(invalidNative));
    QFile keyedPng(temp.path()+"/keyed.png");CHECK(keyScene.save(keyedPng,false));
    CHECK(QImage(keyedPng.fileName()).pixelColor(30,30).alpha()==0);
    QByteArray invalidKey=group({base(3,0)});
    qToLittleEndian<qint32>(9999,reinterpret_cast<uchar *>(invalidKey.data()+33));
    CHECK(!Mmsstv::readTemplate(store(temp.path(),invalidKey),&keyed,&error));
    const QString batchDir=temp.path()+"/batch";CHECK(QDir().mkpath(batchDir));
    const QString batchSource=store(temp.path(),bytes);
    QFile original(batchSource);CHECK(original.open(QIODevice::ReadOnly));const QByteArray originalBytes=original.readAll();original.close();
    const QString occupied=batchDir+"/"+QFileInfo(batchSource).completeBaseName()+".templ";
    QFile sentinel(occupied);CHECK(sentinel.open(QIODevice::WriteOnly));sentinel.write("keep");sentinel.close();
    const auto batch=Mmsstv::importTemplates({batchSource,temp.path()+"/missing.mtm",batchSource},batchDir);
    CHECK(batch.size()==3 && batch[0].error.isEmpty() && !batch[1].error.isEmpty() && batch[2].error.isEmpty());
    CHECK(batch[0].target!=occupied && batch[0].target!=batch[2].target);
    CHECK(sentinel.open(QIODevice::ReadOnly));CHECK(sentinel.readAll()=="keep");sentinel.close();
    CHECK(original.open(QIODevice::ReadOnly));CHECK(original.readAll()==originalBytes);original.close();
    editorScene batchExpected;Mmsstv::Document batchDoc;CHECK(Mmsstv::readTemplate(batchSource,&batchDoc,&error));
    batchExpected.importMmsstv(batchDoc);
    for(int index:{0,2}) {editorScene converted;QFile file(batch[index].target);CHECK(converted.load(file));
      CHECK(render(converted)==render(batchExpected));CHECK(converted.hasTemplateTransparency()==batchDoc.colorKey);}
    CHECK(!Mmsstv::importTemplates({batchSource},temp.path()+"/absent")[0].error.isEmpty());
#ifdef Q_OS_UNIX
    const QString linkDir=temp.path()+"/links";CHECK(QDir().mkpath(linkDir));
    const QString dangling=linkDir+"/"+QFileInfo(batchSource).completeBaseName()+".templ";
    CHECK(QFile::link(temp.path()+"/missing-link-target",dangling));
    const auto linked=Mmsstv::importTemplates({batchSource},linkDir);
    CHECK(linked[0].error.isEmpty() && linked[0].target!=dangling && QFileInfo(dangling).isSymLink());
#endif

    CHECK(QDir(batchDir).entryList({".qsstv-import-*"},QDir::Dirs|QDir::Hidden|QDir::NoDotAndDotDot).isEmpty());
    editorWorkflow(bytes,temp.path());
    if(argc>1)for(int i=1;i<=5;++i) {
      CHECK(Mmsstv::readTemplate(QString(argv[1])+QString("/def%1.mtm").arg(i),&doc,&error));
      roundTrip(doc,temp.path());
    }
    QTextStream(stdout)<<"PASS: real batch dialogs/multiple selection/unicode paths/cancel/report/dirty canvas preservation, batch import mixed failures/collisions/source preservation/native rendering, objects, macros, embedded BMP, nested groups, every truncated prefix, unsupported objects, length limits, native save/reload and identical rendering, editable text gradients/contours, rounded/ellipse/current-and-legacy polygon masks, textured text rendering/editing, UTC/local/case macros across date and DST changes, report/contest suffix macros without fabricated defaults, editable ellipses/no-contour and rounded/beveled frame compatibility, whole-template sampled transparency/overlap/TX/PNG/native persistence, editable transparency dialog/click/disabled/outside/right-click/cancel/dirty/disable-to-legacy behavior, editor import/cancel/replace/save dialogs and failed-save dirty state"<<(argc>1?", all five original MMSSTV templates":"")<<"\n";
  } catch(const std::exception &e) {QTextStream(stderr)<<"FAIL: "<<e.what()<<"\n";return 1;}
}
