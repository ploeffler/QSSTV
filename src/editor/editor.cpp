/***************************************************************************
 *   Copyright (C) 2000-2019 by Johan Maes                                 *
 *   on4qz@telenet.be                                                      *
 *   https://www.qsl.net/o/on4qz                                           *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/
#include "qsstvsettings.h"
#include "editor.h"

#include <QtGui>
#include "appglobal.h"
#include "dirdialog.h"
#include "editorview.h"
#include "configparams.h"
#include <QFileDialog>
#include <QDialogButtonBox>
#include <QVBoxLayout>
#include <QLabel>
#include "mmsstvimport.h"
#include <QTemporaryDir>
#include <QCheckBox>
#include <QSpinBox>
#include <QFormLayout>
#include <QColorDialog>
#include <QTimer>
#include <QMouseEvent>
#include <QSignalBlocker>
#include <functional>

#ifndef STANDALONE
#include "dispatcher.h"
#endif

namespace {
class TemplateSampleView : public QGraphicsView
{
public:
  TemplateSampleView(QGraphicsScene *scene,QWidget *parent) : QGraphicsView(scene,parent) {}
  std::function<bool(const QPoint &)> samplePicked;
protected:
  void mousePressEvent(QMouseEvent *event) override
  {
    const QPointF point=mapToScene(event->pos());
    const QRectF bounds=sceneRect();
    if(event->button()==Qt::LeftButton && samplePicked &&
       point.x()>=bounds.left() && point.y()>=bounds.top() &&
       point.x()<bounds.right() && point.y()<bounds.bottom() &&
       samplePicked(QPoint(qFloor(point.x()-bounds.left()),qFloor(point.y()-bounds.top())))) {
      event->accept();return;
    }
    QGraphicsView::mousePressEvent(event);
  }
};
}

/*! 
  constructor
*/
editor::editor(QWidget *parent): QMainWindow(parent)
{
  ev=new editorView(this);
  setCentralWidget (ev);
  initActions();
  initMenubar();
  statusBar()->showMessage("Select a tool");
  setMinimumSize(640,480);
  resize(640,480);
  addToLog (QString(" editor create: %1")
            .arg(QString::number((ulong)this,16)),LOGEDIT);
  setWindowTitle("Template Editor");
  readSettings();
}

/*! 
  destructor (saves settings on deletion)
*/

editor::~editor()
{
  writeSettings();
}

/*! 
  reads the settings (saved images for tx,rx,templates)
*/

void editor::readSettings()
{
  QsstvSettings qSettings;
  qSettings.beginGroup ("Editor" );
  int windowWidth = qSettings.value( "windowWidth", 640 ).toInt();
  int windowHeight = qSettings.value( "windowHeight", 480 ).toInt();
  int windowX = qSettings.value( "windowX", -1 ).toInt();
  int windowY = qSettings.value( "windowY", -1 ).toInt();
  if ( windowX != -1 || windowY != -1 ) 	move ( windowX, windowY );
  resize ( windowWidth, windowHeight );
  qSettings.endGroup();
}

/*! 
  writes the settings (saved images for tx,rx,templates)
*/
void editor::writeSettings()
{
  QsstvSettings qSettings;
  qSettings.beginGroup ("Editor" );
  qSettings.setValue ( "windowWidth", width() );
  qSettings.setValue ( "windowHeight", height() );
  qSettings.setValue ( "windowX", x() );
  qSettings.setValue ( "windowY", y() );
  qSettings.endGroup();
}


void editor::initActions()
{
  fileNew = new QAction(QIcon(":/icons/filenew.png"),tr("&New"),this);
  fileNew->setShortcut(tr("Ctrl+N"));
  fileNew->setStatusTip(tr("Create a new image"));
  connect(fileNew, SIGNAL(triggered()), this, SLOT(slotFileNew()));

  fileOpen = new QAction(QIcon(":/icons/fileopen.png"),tr("&Open"),this);
  fileOpen->setShortcut(tr("Ctrl+O"));
  fileOpen->setStatusTip(tr("Open an image file"));
  connect(fileOpen, SIGNAL(triggered()), this, SLOT(slotFileOpen()));

  fileImportMmsstvBatch = new QAction(tr("Batch import MMSSTV templates..."),this);
  connect(fileImportMmsstvBatch, SIGNAL(triggered()), this, SLOT(slotImportMmsstvBatch()));
  fileImportMmsstv = new QAction(tr("Import MMSSTV template..."),this);
  connect(fileImportMmsstv, SIGNAL(triggered()), this, SLOT(slotImportMmsstv()));

  templateTransparency = new QAction(tr("Template transparency..."),this);
  connect(templateTransparency,SIGNAL(triggered()),this,SLOT(slotTemplateTransparency()));

  fileSave = new QAction(QIcon(":/icons/filesave.png"),tr("&Save file"),this);
  fileSave->setStatusTip(tr("Save the file under the same name and format"));
  connect(fileSave, SIGNAL(triggered()), this, SLOT(slotFileSave()));

  fileSaveImage = new QAction(tr("Save as &Image"),this);
  fileSaveImage->setStatusTip(tr("Save the file in PNG format"));
  connect(fileSaveImage, SIGNAL(triggered()), this, SLOT(slotFileSaveImage()));

  fileSaveTemplate = new QAction(("Save as &Template"),this);
  fileSaveTemplate->setStatusTip(tr("Save template file "));
  connect(fileSaveTemplate, SIGNAL(triggered()), this, SLOT(slotFileSaveTemplate()));

  fileQuit = new QAction(tr("Quit"),this);
  fileQuit->setShortcut(tr("Ctrl+Q"));
  fileQuit->setStatusTip(tr("Quits the editor"));
  connect(fileQuit, SIGNAL(triggered()), this, SLOT(slotFileQuit()));

  clearAll= new QAction(QIcon(":/icons/eraser.png"),tr("Clear &All"),this);
  clearAll->setShortcut(tr("Ctrl+A"));
  clearAll->setStatusTip(tr("Delete all objects and fill the background with the background color"));
  connect(clearAll, SIGNAL(triggered()), ev, SLOT(slotClearAll()));

  copy= new QAction(tr("Copy"),this);
  copy->setShortcut(tr("Ctrl+C"));
  connect(copy, SIGNAL(triggered()), ev->getScene(), SLOT(slotCopy()));

  paste= new QAction(tr("Paste"),this);
  paste->setShortcut(tr("Ctrl+V"));
  connect(paste, SIGNAL(triggered()), ev->getScene(), SLOT(slotPaste()));

  deleteAction=new QAction(tr("&Delete"),this);
  deleteAction->setShortcut(tr("Del"));
  connect(deleteAction, SIGNAL(triggered()), ev->getScene(), SLOT(slotDeleteItem()));
}



void editor::initMenubar()
{
  fileMenu=menuBar()->addMenu(tr("&File"));
  editMenu=menuBar()->addMenu(tr("&Edit"));
  fileMenu->addAction(fileNew);
  fileMenu->addAction(fileOpen);
  fileMenu->addAction(fileImportMmsstv);
  fileMenu->addAction(fileImportMmsstvBatch);
  fileMenu->addAction(fileSave);
  fileMenu->addAction(fileSaveImage);
  fileMenu->addAction(fileSaveTemplate);
  fileMenu->addAction(fileQuit);
  editMenu->addAction(deleteAction);
  editMenu->addAction(copy);
  editMenu->addAction(paste);
  editMenu->addAction(clearAll);
  editMenu->addAction(templateTransparency);
}


void editor::slotFileNew()
{
  if(ev->isModified())
    {
      switch( QMessageBox::information( this, "Editor",
                                        "The document has not been saved as a template.\n"
                                        "Discard changes, or cancel \"New file\" action?",
                                        QMessageBox::Cancel | QMessageBox::Discard,
                                        QMessageBox::Cancel) )
        {
        case QMessageBox::Discard:
          break;
        case QMessageBox::Cancel:
          return;
        default: // cannot happen ;-)
          return;
        }
    }
  ev->slotClearAll();
  localFile.close();
  localFile.setFileName("Untitled.templ");
  localFileTemplate=true;
  setWindowTitle(QString("Template Editor: %1").arg(localFile.fileName()));
}

void editor::slotFileOpen()
{
  /*	QFileDialog *fd = new QFileDialog(this,0,true);
  fd->show();*/
  dirDialog d(this,0);
  QString s=d.openFileName(templatesPath,"*.png *.gif *.jpg *.templ");
  if (s.isNull()) return ;
  if (s.isEmpty()) return ;
  localFile.setFileName(s);
  if(ev->open(localFile))
    {
      localFileTemplate=ev->getScene()->getImageType()==editorScene::TEMPLATE;
      setWindowTitle(QString("Template Editor: %1").arg(s));
      addToLog("localfile after open = " + localFile.fileName(),LOGEDIT);
    }
}

/*!
    \fn editor::slotFileSave()
    \brief save file under same name and same type
*/

void editor::slotFileSave()
{
  if(localFile.fileName().isEmpty())
    {
      slotFileSaveTemplate();
      return;
    }
  if(!localFileTemplate)
    {
      addToLog("localfile to save = " + localFile.fileName(),LOGEDIT);
      saveToFile(localFile.fileName(),false);
    }
  else
    {
      saveToFile(localFile.fileName(),true);
    }
}

void editor::slotFileSaveImage()
{
  dirDialog d((QWidget *)this,"Editor");
  QString s(localFile.fileName());
  if(s.isEmpty())
    {
      s=txStockImagesPath;
    }
  s=d.saveFileName(s,"*.png","png");
  if (s.isNull()) return ;
  if (s.isEmpty()) return ;
  saveToFile(s,false);
}

void editor::slotFileSaveTemplate()
{
  dirDialog d((QWidget *)this,"Browse");
  QString s(localFile.fileName());
  if(s.isEmpty())
    {
      s=templatesPath;
    }
  s=d.saveFileName(s,"*.templ","templ");
  if (s.isNull()) return ;
  if (s.isEmpty()) return ;
  saveToFile(s,true);
}


void editor::slotFileQuit()
{
  ev->writeSettings();
  close();
}



void editor::closeEvent(QCloseEvent *e)
{

  if(ev->isModified())
    {
      QMessageBox msgBox;
      msgBox.setText("The document has been modified.");
      msgBox.setInformativeText("Do you want to save your changes?");
      msgBox.setStandardButtons(QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);
      msgBox.setDefaultButton(QMessageBox::Save);
      int ret = msgBox.exec();
      switch (ret)
        {
        case QMessageBox::Save:
          slotFileSave();
          break;
        case QMessageBox::Discard:
          // Don't Save was clicked
          break;
        case QMessageBox::Cancel:
          return;
          break;
        default:
          // should never be reached
          break;
        }
    }
#ifndef STANDALONE
  editorFinishedEvent *ce = new editorFinishedEvent(true,localFile.fileName());
  QApplication::postEvent(dispatcherPtr, ce );  // Qt will delete it when done	emit imageAvailable(ev->getImage());
#endif
  writeSettings();
  e->accept();
}



bool editor::setImage(QImage *im)
{
  ev->setImage(im);
  return true;
}

bool editor::openFile(QString fn)
{
  QFile f(fn);
  localFile.setFileName(fn);
  if (!ev->open(f)) return false;
  localFileTemplate=ev->getScene()->getImageType()==editorScene::TEMPLATE;
  return true;
}


void editor::slotImportMmsstv()
{
  const QString path=QFileDialog::getOpenFileName(this,tr("Import MMSSTV template"),
                                               templatesPath,tr("MMSSTV templates (*.mtm *.MTM)"));
  if (path.isEmpty()) return;
  Mmsstv::Document document;
  QString error;
  if (!Mmsstv::readTemplate(path,&document,&error)) {
    QMessageBox::warning(this,tr("MMSSTV import failed"),error);
    return;
  }
  document.warnings=Mmsstv::importWarnings(document);
  QDialog preview(this);
  preview.setWindowTitle(tr("MMSSTV import preview"));
  QVBoxLayout *layout=new QVBoxLayout(&preview);
  QLabel *description=new QLabel(tr("Check the converted layout. Import replaces the current canvas.\n"
                                    "Dynamic fields are shown as QSSTV macros; save the result as a .templ file."));
  description->setWordWrap(true);
  layout->addWidget(description);
  editorScene scene;
  scene.importMmsstv(document);
  QGraphicsScene renderedPreview;
  renderedPreview.setSceneRect(scene.border);
  renderedPreview.addPixmap(QPixmap::fromImage(*scene.renderImage(0,0)));
  QPixmap checker(16,16);checker.fill(QColor(240,240,240));
  {QPainter painter(&checker);painter.fillRect(0,0,8,8,QColor(210,210,210));painter.fillRect(8,8,8,8,QColor(210,210,210));}
  renderedPreview.setBackgroundBrush(QBrush(checker));
  QGraphicsView *view=new QGraphicsView(&renderedPreview,&preview);
  view->setMinimumSize(480,360);
  view->setSceneRect(scene.border);
  layout->addWidget(view);
  QTextEdit *warnings=new QTextEdit(&preview);
  warnings->setReadOnly(true);
  warnings->setPlainText(document.warnings.isEmpty()?tr("No conversion warnings."):document.warnings.join("\n"));
  warnings->setMaximumHeight(150);
  layout->addWidget(warnings);
  QDialogButtonBox *buttons=new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel,&preview);
  buttons->button(QDialogButtonBox::Ok)->setText(tr("Import"));
  connect(buttons,SIGNAL(accepted()),&preview,SLOT(accept()));
  connect(buttons,SIGNAL(rejected()),&preview,SLOT(reject()));
  layout->addWidget(buttons);
  preview.resize(640,640);
  if (preview.exec()!=QDialog::Accepted) return;
  if (ev->isModified() && QMessageBox::question(this,tr("Replace template"),
          tr("Discard the unsaved changes and import this template?"),
          QMessageBox::Discard|QMessageBox::Cancel,QMessageBox::Cancel)!=QMessageBox::Discard) return;
  ev->importMmsstv(document);
  // Never save native data back into the original .mtm file.
  localFile.close();
  localFile.setFileName(QString());
  localFileTemplate=true;
  setWindowTitle(tr("Template Editor: imported %1 (unsaved)").arg(QFileInfo(path).fileName()));
}

void editor::slotImportMmsstvBatch()
{
  const QStringList paths=QFileDialog::getOpenFileNames(this,tr("Batch import MMSSTV templates"),
      templatesPath,tr("MMSSTV templates (*.mtm *.MTM)"));
  if(paths.isEmpty()) return;
  const QString destination=QFileDialog::getExistingDirectory(this,tr("Destination for converted templates"),templatesPath);
  if(destination.isEmpty()) return;
  const auto results=Mmsstv::importTemplates(paths,destination);
  QStringList report;int succeeded=0;
  for(const auto &result:results) {
    if(result.error.isEmpty()) {
      ++succeeded;
      report.append(tr("%1\nSaved: %2").arg(result.source,result.target));
    } else report.append(tr("%1\nError: %2").arg(result.source,result.error));
    for(const QString &warning:result.warnings) report.append(tr("Warning: %1").arg(warning));
    report.append(QString());
  }
  QDialog dialog(this);dialog.setWindowTitle(tr("MMSSTV batch import report"));
  QVBoxLayout *layout=new QVBoxLayout(&dialog);
  layout->addWidget(new QLabel(tr("Converted %1 of %2 templates.").arg(succeeded).arg(results.size()),&dialog));
  QTextEdit *text=new QTextEdit(&dialog);text->setReadOnly(true);text->setPlainText(report.join("\n"));
  layout->addWidget(text);
  QDialogButtonBox *buttons=new QDialogButtonBox(QDialogButtonBox::Close,&dialog);
  connect(buttons,SIGNAL(rejected()),&dialog,SLOT(reject()));layout->addWidget(buttons);
  dialog.resize(700,500);dialog.exec();
}

bool editor::saveToFile(const QString &path, bool templ)
{
  QFile destination(path);
  if (!ev->save(destination,templ)) {
    QMessageBox::warning(this,tr("Cannot save template"),
                         tr("Could not save '%1'. Your changes are still unsaved.").arg(path));
    return false;
  }
  localFile.setFileName(path);
  localFileTemplate=templ;
  setWindowTitle(QString("Template Editor: %1").arg(path));
  return true;
}

void editor::slotTemplateTransparency()
{
  QTemporaryDir temporary;
  QFile snapshot(temporary.path()+"/preview.templ");
  const QList<QGraphicsItem *> selection=ev->getScene()->selectedItems();
  const bool saved=temporary.isValid() && ev->getScene()->save(snapshot,true);
  for(QGraphicsItem *item:selection)item->setSelected(true);
  editorScene previewScene;
  if(!saved || !previewScene.load(snapshot)) {
    QMessageBox::warning(this,tr("Template transparency"),tr("Cannot create a preview. Your template is unchanged."));
    return;
  }
  QDialog dialog(this);dialog.setWindowTitle(tr("Template transparency"));
  QVBoxLayout *layout=new QVBoxLayout(&dialog);
  QCheckBox *enabled=new QCheckBox(tr("Make the sampled color transparent"),&dialog);
  enabled->setObjectName("enableSampledTransparency");
  enabled->setChecked(ev->getScene()->hasTemplateTransparency());layout->addWidget(enabled);
  QFormLayout *form=new QFormLayout();
  QSpinBox *x=new QSpinBox(&dialog),*y=new QSpinBox(&dialog);
  x->setObjectName("transparencyX");y->setObjectName("transparencyY");
  x->setRange(0,qMax(0,int(ev->getScene()->border.width())-1));
  y->setRange(0,qMax(0,int(ev->getScene()->border.height())-1));
  const QPoint current=ev->getScene()->templateTransparencyPoint();x->setValue(current.x());y->setValue(current.y());
  form->addRow(tr("Sample X (pixels)"),x);form->addRow(tr("Sample Y (pixels)"),y);
  QColor background=ev->getScene()->templateTransparencyBackground();
  QPushButton *color=new QPushButton(background.name(),&dialog);color->setObjectName("transparencyBackground");
  form->addRow(tr("Template background"),color);layout->addLayout(form);
  QLabel *note=new QLabel(tr("Click the preview or use X/Y to choose the point. Its color is sampled after all objects are drawn.\n"
      "Saving as .templ with this enabled requires the new QSSTV reader."),&dialog);
  note->setWordWrap(true);layout->addWidget(note);
  QGraphicsScene renderedPreview;
  QPixmap checker(16,16);checker.fill(QColor(240,240,240));
  {QPainter painter(&checker);painter.fillRect(0,0,8,8,QColor(210,210,210));painter.fillRect(8,8,8,8,QColor(210,210,210));}
  renderedPreview.setBackgroundBrush(QBrush(checker));
  TemplateSampleView *view=new TemplateSampleView(&renderedPreview,&dialog);view->setObjectName("transparencyPreview");
  view->setMinimumSize(480,320);layout->addWidget(view);
  QLabel *errorLabel=new QLabel(&dialog);errorLabel->setWordWrap(true);layout->addWidget(errorLabel);
  QDialogButtonBox *buttons=new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel,&dialog);
  layout->addWidget(buttons);
  auto refresh=[&]() {
    x->setEnabled(enabled->isChecked());y->setEnabled(enabled->isChecked());color->setEnabled(enabled->isChecked());
    view->viewport()->setCursor(enabled->isChecked()?Qt::CrossCursor:Qt::ArrowCursor);
    QString error;
    const bool valid=previewScene.setTemplateTransparency(enabled->isChecked(),background,QPoint(x->value(),y->value()),&error);
    errorLabel->setText(error);buttons->button(QDialogButtonBox::Ok)->setEnabled(valid);
    if(!valid)return;
    renderedPreview.clear();
    renderedPreview.setSceneRect(previewScene.border);
    renderedPreview.addPixmap(QPixmap::fromImage(*previewScene.renderImage(0,0)));
    renderedPreview.addRect(previewScene.border,QPen(Qt::gray),Qt::NoBrush);
    if(enabled->isChecked()) {
      for(const QPen &pen:QList<QPen>({QPen(Qt::black,3),QPen(Qt::yellow,1)})) {
        renderedPreview.addLine(x->value()-5,y->value(),x->value()+5,y->value(),pen);
        renderedPreview.addLine(x->value(),y->value()-5,x->value(),y->value()+5,pen);
      }
    }
    view->fitInView(renderedPreview.sceneRect(),Qt::KeepAspectRatio);
  };
  view->samplePicked=[&](const QPoint &point) {
    if(!enabled->isChecked())return false;
    const QSignalBlocker blockX(x),blockY(y);
    x->setValue(point.x());y->setValue(point.y());refresh();return true;
  };
  connect(enabled,&QCheckBox::toggled,&dialog,[&](){refresh();});
  connect(x,static_cast<void(QSpinBox::*)(int)>(&QSpinBox::valueChanged),&dialog,[&](){refresh();});
  connect(y,static_cast<void(QSpinBox::*)(int)>(&QSpinBox::valueChanged),&dialog,[&](){refresh();});
  connect(color,&QPushButton::clicked,&dialog,[&]() {
    QColor chosen=QColorDialog::getColor(background,&dialog,tr("Template background"));
    if(chosen.isValid()){background=chosen;color->setText(background.name());refresh();}
  });
  connect(buttons,SIGNAL(accepted()),&dialog,SLOT(accept()));
  connect(buttons,SIGNAL(rejected()),&dialog,SLOT(reject()));
  dialog.resize(640,650);refresh();
  QTimer::singleShot(0,&dialog,[&](){refresh();});
  if(dialog.exec()!=QDialog::Accepted)return;
  QString error;
  if(!ev->setTemplateTransparency(enabled->isChecked(),background,QPoint(x->value(),y->value()),&error))
    QMessageBox::warning(this,tr("Template transparency"),error);
}
