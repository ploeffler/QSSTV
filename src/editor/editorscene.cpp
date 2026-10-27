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
#include <QtGui>
#include <QSaveFile>

#include "editorscene.h"
#include "gradientdialog.h"
#include "appglobal.h"
#include "ui_textform.h"
#include "imageviewer.h"

#ifndef STANDALONE
#include "gallerywidget.h"
#include "txwidget.h"
#endif


// Optional native 9.6 record; legacy templates without it keep their format.
static const int TEMPLATE_COLORKEY_RECORD=0x4d4d434b; // MMCK

#undef ENABLETEMPLATEVIEWER

#ifdef ENABLETEMPLATEVIEWER
#include "templateviewer.h"
#endif

editorScene::editorScene(QGraphicsView *parent)  : QGraphicsScene(parent)
{
  contextMenu=new QMenu();
  arrange = new QMenu( "Arrange");
  arrange->setTearOffEnabled(true);
  arrange->addAction("Forward",this,SLOT(slotSendForward()));
  arrange->addAction("Backward",this,SLOT(slotSendBackward()));
  arrange->addAction("Bring to front",this,SLOT(slotBringToFront()));
  arrange->addAction("Send to back",this,SLOT(slotSendToBack()));
  contextMenu->addMenu(arrange);
  contextMenu->addSeparator();
  contextMenu->addAction("Change Text",this,SLOT(slotChangeText()));
  contextMenu->addSeparator();
  contextMenu->addAction("Delete",this,SLOT(slotDeleteItem()));
  contextMenu->addAction("Expand",this,SLOT(slotExpand()));
  contextMenu->addAction("Lock",this,SLOT(slotLock()));
  contextMenu->addAction("Unlock",this,SLOT(slotUnlock()));
#ifndef QT_NO_DEBUG
  contextMenu->addAction("Properties",this,SLOT(slotProperties()));
#endif

  zMax=0;
  pasted=false;
  copyItem=NULL;
  mode=MOVE;
  imageType=NONE;
  localImage=NULL;
  rotate=0;
  vShear=0;
  hShear=0;
  border=QRectF(0,0,0,0);
  borderItemPtr=NULL;
  penWidth=1;
}

editorScene::~editorScene()
{
  if(localImage!=NULL) delete localImage;
  if((!pasted) &&(copyItem!=NULL)) delete copyItem;
  delete arrange;
  delete contextMenu;
}


bool editorScene::load(QFile &f)
{
  bool borderSet=false;
  QImage im;
  graphItemBase *item;
  quint32 magic;
  QString version;
  quint16 streamVersion;
  int type;
  if(f.fileName().isEmpty()) return false;
  if(!f.open(QIODevice::ReadOnly)) return false;
  QDataStream str(&f);
  str >> magic;
  if (magic != MAGICNUMBER)
    {
      //       qDebug() << QChar((MAGICNUMBER>>24)&0xFF)
      //                << QChar((MAGICNUMBER>>16)&0xFF)
      //                << QChar((MAGICNUMBER>>8)&0xFF)
      //                << QChar((MAGICNUMBER)&0xFF);
      //       qDebug() << QChar((magic>>24)&0xFF)
      //                << QChar((magic>>16)&0xFF)
      //                << QChar((magic>>8)&0xFF)
      //                << QChar((magic)&0xFF);

      //try to load an image
      f.reset();
      if(im.load(&f,0))
        {
          addToLog("image loaded",LOGEDIT);
          imageType=FLATIMAGE;
          setImage(&im);
          border=QRect(0,0,im.width(),im.height());
          f.close();
          return true;
        }
      else
        {
          addToLog("image failed to load",LOGEDIT);
          f.close();
          return false;
        }
    }
  imageType=TEMPLATE;
  bool keyRecordSeen=false;
  str >> version;  // optional records determine the required reader capabilities
  str >> streamVersion;
  str.setVersion(streamVersion);
  while (!str.atEnd())
    {
      str >> type;
      switch (type)
        {
        case TEMPLATE_COLORKEY_RECORD:
          {
            QColor background;QPoint point;QSize canvas;
            str >> background >> point >> canvas;
            if(keyRecordSeen || str.status()!=QDataStream::Ok || !background.isValid() || background.alpha()!=255 ||
               canvas.width()<1 || canvas.height()<1 || canvas.width()>4096 || canvas.height()>4096 ||
               point.x()<0 || point.y()<0 || point.x()>=canvas.width() || point.y()>=canvas.height()) {
              f.close();return false;
            }
            keyRecordSeen=true;
            templateColorKey=true;templateBackground=background;
            templateKeyPoint=point;templateKeyCanvas=canvas;
            setBackgroundBrush(background);
            continue;
          }
        case graphItemBase::RECTANGLE:
          item=new itemRectangle(contextMenu);
          break;
        case graphItemBase::ELLIPSE:
          item=new itemEllipse(contextMenu);
          break;
        case graphItemBase::LINE:
          item=new itemLine(contextMenu);
          break;
        case graphItemBase::TEXT:
          item=new itemText(contextMenu);
          break;
        case graphItemBase::IMAGE:
          item=new itemImage(contextMenu);
          break;
        case graphItemBase::REPLAY:
          item=new itemReplayImage(contextMenu);
          break;
        case graphItemBase::SBORDER:
          borderSet=true;
          item=new itemImage(contextMenu);
          item->load(str);
          border=item->rect();
          delete item;
          continue;
          break;
        default:
          addToLog("Error in datastream",LOGEDIT);
          f.close();
          return false;
          break;
        }
      item->load(str);
      addItem(item);
    }
  optimizeDepth();
  if(!borderSet) border=QRectF(0,0,320,256);
  addToLog(QString("border position %1,%2 size: %3 x %4 border set=%5")
           .arg(border.topLeft().x()).arg(border.topLeft().y())
           .arg(border.width()).arg(border.height()).arg(borderSet),LOGEDIT);
  f.close();
  setSceneRect(border);
  return true;
}

QImage *editorScene::renderImage(int w,int h)
{
  clearSelection();
  if (localImage!=NULL) delete localImage;
  if(w==0)
    {
      localImage=new QImage(border.width(),border.height(),QImage::Format_ARGB32_Premultiplied);
    }
  else
    {
      localImage=new QImage(w,h,QImage::Format_ARGB32_Premultiplied);
    }
  addToLog(QString("editorScene: pre-render size: %1 x %2").arg(localImage->size().width()).arg(localImage->size().height()),LOGEDIT);
  renderTemplate(*localImage,Qt::KeepAspectRatio);
  addToLog(QString("editor: post-render size: %1 x %2").arg(localImage->size().width()).arg(localImage->size().height()),LOGEDIT);
#ifdef ENABLETEMPLATEVIEWER
  templateViewer tv;
  qDebug() << "items" << items().count()<<sceneRect();
  dumpItems();
  tv.setImage(localImage);
  tv.exec();
#endif
  return localImage;
}


void editorScene::overlay(QImage *ima)
{
  clearSelection();
  setSceneRect(border);
  if (localImage!=NULL) delete localImage;
  localImage=new QImage(ima->copy());
  addToLog(QString("localImageSize %1,%2").arg(localImage->width()).arg(localImage->height()),LOGEDIT);
  convertText();
  convertReplayImage();
  if(templateColorKey) {
    QImage layer(localImage->size(),QImage::Format_ARGB32_Premultiplied);
    renderTemplate(layer);
    QPainter painter(localImage);painter.drawImage(0,0,layer);
  } else {
    QPainter painter(localImage);
    painter.setRenderHint(QPainter::Antialiasing);
    render(&painter,QRectF(),QRectF(),Qt::IgnoreAspectRatio);
  }
#ifdef ENABLETEMPLATEVIEWER
  templateViewer tv;
  qDebug() << "items" << items().count()<<sceneRect();
  tv.setImage(localImage);
  tv.exec();
#endif
}

bool editorScene::save(QFile &f,bool templ)
{
  QImage im(border.width(),border.height(),QImage::Format_ARGB32_Premultiplied);
  im.fill(0);
  setSceneRect(border);
#ifndef QT_NO_DEBUG
  qDebug()<<"dumping";
  dumpItems();
#endif
  clearSelection();

  QList<QGraphicsItem *> itemList = items(border, Qt::IntersectsItemBoundingRect);
//  qDebug() << "itemList.count()" << itemList.count();


  addToLog(QString("editorscene:save %1x%2 %3,%4")
           .arg(sceneRect().width()).arg(sceneRect().height())
           .arg(sceneRect().x()).arg(sceneRect().y())
           ,LOGEDIT);
  if(!templ)
    {
      renderTemplate(im);
#ifdef ENABLETEMPLATEVIEWER
      templateViewer tv;
      qDebug() << "items" << items().count()<<sceneRect();
      tv.setImage(&im);
      tv.exec();
#endif
      QSaveFile output(f.fileName());
      if (!output.open(QIODevice::WriteOnly)) return false;
      return im.save(&output,"PNG") && output.commit();
    }
  QSaveFile output(f.fileName());
  if (!output.open(QIODevice::WriteOnly)) return false;
  QDataStream str(&output);
  str.setVersion(QDataStream::Qt_4_4);
  // Header with a "magic number" and a version
  str << (quint32) MAGICNUMBER;
  str << (templateColorKey?QString("9.6"):CONFIGVERSION);
  str << (quint16) QDataStream::Qt_4_4;
  graphItemBase *it;
  foreach(QGraphicsItem *t,items())
    {
      it=qgraphicsitem_cast<graphItemBase *>(t);
      if(t->type()>graphItemBase::BASE)
        {
          it->save(str);
        }
    }
  if(templateColorKey)str << TEMPLATE_COLORKEY_RECORD << templateBackground << templateKeyPoint << templateKeyCanvas;
  return str.status()==QDataStream::Ok && output.commit();
}

void editorScene::flattenImage(int w,int h)
{
  if (localImage!=NULL) delete localImage;
  setSceneRect(border);
  localImage=new QImage(w,h,QImage::Format_ARGB32_Premultiplied);
  convertText();
  convertReplayImage();
  renderTemplate(*localImage,Qt::KeepAspectRatio);
}

void editorScene::convertReplayImage()
{
#ifndef STANDALONE
  if(!txWidgetPtr)return;
  QString fn;
  imageViewer imv;

  fn=txWidgetPtr->getPreviewFilename();
  if(fn.isEmpty()) return;
  if(imv.openImage(fn,false,false,false,false))
    {
      foreach(QGraphicsItem *t,items())
        {
          if(t->type()==graphItemBase::REPLAY)
            {
              itemReplayImage *itt=qgraphicsitem_cast<itemReplayImage *>(t);
              itt->setImage(*imv.getImagePtr());
            }
        }
    }
#endif

}




void editorScene::convertText()
{
  foreach(QGraphicsItem *t,items())
    {
      if(t->type()==graphItemBase::TEXT)
        {
          itemText *itt=qgraphicsitem_cast<itemText *>(t);
          itt->setText(mexp.convert(itt->text()));
        }
    }
}





void editorScene::setMode(eMode m)
{
  mode = m;
  if(mode==INSERT) clearSelection () ;
}

void editorScene::setItemType(graphItemBase::egraphType tp)
{
  itemType = tp;
}

void editorScene::apply(changeFlags cf)
{
  QPen p;
  graphItemBase *it;
  if(selectedItems().isEmpty()) return; // nothing to do
  foreach(QGraphicsItem *t,selectedItems())
    {
      it=qgraphicsitem_cast<graphItemBase *>(t);
      if(cf & DFILLCOLOR)
        {
          if(t->type()==graphItemBase::TEXT)it->setImage(QImage());
          it->setBrush(fillColor);
        }
      if(cf & DLINECOLOR)
        {
          p=it->pen();
          p.setColor(lineColor);
          it->setPen(p);
        }
      if(cf & DPEN)
        {
          p=it->pen();
          p.setWidth(penWidth);
          it->setPen(p);
        }
      if(cf & DGRADIENT)
        {
          gradientDialog gd;
          sgradientParam tmp;
          tmp=gd.param();
          if(t->type()==graphItemBase::TEXT)it->setImage(QImage());
          it->setGradient(tmp);
          it->update();
        }
      if(cf & DTRANSFORM)
        {
          it->setTransform(rotate,hShear,vShear);
        }
      if(t->type()==graphItemBase::TEXT)
        {
          itemText *itt=qgraphicsitem_cast<itemText *>(t);
          if(cf & DFONT) itt->setFont(font);
          if(cf & DTEXT) itt->setText(text);
        }
      it->update();
    }
}

void editorScene::clearAll()
{
  templateColorKey=false;
  setBackgroundBrush(Qt::NoBrush);
  graphItemBase *r;
  foreach(QGraphicsItem *t,items())
    {
      r=qgraphicsitem_cast<graphItemBase *>(t);
      if((r->getParamPtr()->type>graphItemBase::BASE) && (r->getParamPtr()->type!=graphItemBase::SBORDER))
        {
          // remove the correct graphics item
          r->markedForDeletion=true;
          removeItem(r);
//          qDebug() << "after remove";
          delete r;
        }
    }
}


void editorScene::itemSetup(graphItemBase *item)
{
  QPen p;
  gradientDialog gd;
  sgradientParam tmp;
  tmp=gd.param();
  item->setGradient(tmp);
  item->setTransform(rotate,hShear,vShear);
  p=item->pen();
  p.setColor(lineColor);
  p.setWidth(penWidth);
  item->setPen(p);
  item->setBrush(fillColor);
  item->setZValue(zMax);
  zMax+=1;
  addItem(item);
}

void editorScene::setImage(QImage *im)
{
  graphItemBase *item;
  item=new itemImage(contextMenu);
  item->setImage(*im);
  item->setRect(0,0,im->width(),im->height());
  itemSetup(item);
  item->setPos(QPointF(0,0));
  item->setSelected(true);
  emit changeSize(im->width(),im->height());
}

void editorScene::mousePressEvent(QGraphicsSceneMouseEvent *mouseEvent)
{
  graphItemBase *item;
  QImage im;
  if (mouseEvent->button() == Qt::LeftButton)
    {
      switch(mode)
        {
        case INSERT:
          switch(itemType)
            {
            case graphItemBase::RECTANGLE:
              item=new itemRectangle(contextMenu);
              itemSetup(item);
              item->setPos(mouseEvent->scenePos());
              break;
            case graphItemBase::LINE:
              item=new itemLine(contextMenu);
              itemSetup(item);
              item->setPos(mouseEvent->scenePos());
              break;
            case graphItemBase::ELLIPSE:
              item=new itemEllipse(contextMenu);
              itemSetup(item);
              item->setPos(mouseEvent->scenePos());
              break;
            case graphItemBase::TEXT:
              if (!text.isEmpty())
                {
                  item=new itemText(contextMenu);
                  item->setFont(font);
                  item->setText(text);
                  itemSetup(item);
                  item->setPos(mouseEvent->scenePos());
                }
              break;
            case graphItemBase::IMAGE:
              if (fl.isEmpty()) break;
              if(im.load(fl))
                {
                  item=new itemImage(contextMenu);
                  item->setImage(im);
                  itemSetup(item);
                  item->setPos(mouseEvent->scenePos());
                }
              break;
            case graphItemBase::REPLAY:
              item=new itemReplayImage(contextMenu);
              itemSetup(item);
              item->setPos(mouseEvent->scenePos());
              break;
            case graphItemBase::SBORDER:
            case graphItemBase::BASE:
              break;
            }
          break;
        case MOVE:
          //          if(!selectedItems().isEmpty())
          //            {
          //              item=qgraphicsitem_cast<graphItemBase *>(selectedItems().first());
          //            }
          break;

        }
    }
  QGraphicsScene::mousePressEvent(mouseEvent);
  update();
}

void editorScene::mouseMoveEvent(QGraphicsSceneMouseEvent *mouseEvent)
{
  QGraphicsScene::mouseMoveEvent(mouseEvent);
}

void editorScene::mouseReleaseEvent(QGraphicsSceneMouseEvent *mouseEvent)
{
  graphItemBase *item;
  if(mode==MOVE)
    {
      if(!selectedItems().isEmpty())
        {
          item=qgraphicsitem_cast<graphItemBase *>(selectedItems().first());
          emit itemSelected(item);
        }
    }
  mode=MOVE;
  QGraphicsScene::mouseReleaseEvent(mouseEvent);
}


void editorScene::slotCopy()
{
  graphItemBase *item;
  if((!pasted) &&(copyItem!=NULL)) delete copyItem;
  if(selectedItems().isEmpty()) return; // nothing to do
  item=qgraphicsitem_cast<graphItemBase *>(selectedItems().first());
  makeCopy(item);
}

void editorScene::makeCopy(graphItemBase *it)
{
  graphItemBase *item=it;
  graphItemBase::egraphType type=(graphItemBase::egraphType)item->type();
  switch(type)
    {
    case graphItemBase::RECTANGLE:
      copyItem=new itemRectangle(item->getParam().menu);
      break;
    case graphItemBase::LINE:
      copyItem=new itemLine(item->getParam().menu);
      break;
    case graphItemBase::ELLIPSE:
      copyItem=new itemEllipse(item->getParam().menu);
      break;
    case graphItemBase::TEXT:
      copyItem=new itemText(item->getParam().menu);
      break;
    case graphItemBase::IMAGE:
      copyItem=new itemImage(item->getParam().menu);
      break;
    case graphItemBase::REPLAY:
      copyItem=new itemReplayImage(item->getParam().menu);
      break;
    default:
      return;
    }
  copyItem->setParam(item->getParam());
  copyItem->setPos(item->pos()+QPointF(10,10));
  pasted=false;
}

void editorScene::slotPaste()
{
  clearSelection();
  copyItem->setZValue(zMax+1);
  zMax+=1;
  addItem(copyItem);
  pasted=true,
      makeCopy(copyItem);
  clearSelection();
}

void editorScene::slotExpand()
{
  graphItemBase *it;
  if(selectedItems().isEmpty()) return; // nothing to do
  foreach(QGraphicsItem *t,selectedItems())
    {
      it=qgraphicsitem_cast<graphItemBase *>(t);
      if(it->type()!=graphItemBase::TEXT)
        {
          it->setRect(border);
          it->setPos(0,0);
        }
    }
}

void editorScene::slotChangeText()
{
  if(selectedItems().isEmpty()) return; // nothing to do
  itemText *item=qgraphicsitem_cast<itemText *>(selectedItems().first());
  if(!item)
    {
      return;
    }
  if(item->type()!=graphItemBase::TEXT) return;
  QDialog d(0);
  Ui::textForm t;
  t.setupUi(&d);
  t.plainTextEdit->setPlainText(item->text());
  if(d.exec()==QDialog::Accepted)
    {
      item->setText(t.plainTextEdit->toPlainText());
    }
}

#ifndef QT_NO_DEBUG
void editorScene::slotProperties()
{
  sitemParam *paramPtr;
  if(selectedItems().isEmpty()) return; // nothing to do
  itemText *item=qgraphicsitem_cast<itemText *>(selectedItems().first());
  if(!item)
    {
      return;
    }
  paramPtr=item->getParamPtr();
  QFont font=paramPtr->font;
  QString txt=paramPtr->txt;
  sgradientParam gr=paramPtr->gradient;
//  qDebug() << "font" << font.family() << "size" << font.pointSize();
  QString gradientType;
  switch (gr.type)
    {
    case sgradientParam::LINEAR :
      gradientType="LinearGradient";
      break;
    case sgradientParam::RADIAL:
      gradientType="RadialGradient";
      break;
    case sgradientParam::CONICAL :
      gradientType="ConicalGradient";
      break;
    case sgradientParam::NONE:
      gradientType="NoGradient";
      break;
    }

//  qDebug() << "gradient" << gradientType;
//  qDebug() << "rotation" << gr.direction;
//  qDebug() << gr.color1 << gr.pos1;
//  qDebug() << gr.color2 << gr.pos2;
//  qDebug() << gr.color3 << gr.pos3;
//  qDebug() << gr.color4 << gr.pos4;
}
#endif

void editorScene::slotDeleteItem()
{
  graphItemBase *r;
  if(selectedItems().isEmpty()) return; // nothing to do
  foreach(QGraphicsItem *t,selectedItems())
    {

      r=qgraphicsitem_cast<graphItemBase *>(t);
      if((r->getParamPtr()->type>graphItemBase::BASE) && (r->getParamPtr()->type!=graphItemBase::SBORDER))
        {
          // remove the correct graphics item
          r->markedForDeletion=true;
          removeItem(r);
          delete r;
        }

    }
}


void editorScene::slotLock()
{
  graphItemBase *it;
  if(selectedItems().isEmpty()) return; // nothing to do
  foreach(QGraphicsItem *t,selectedItems())
    {
      it=qgraphicsitem_cast<graphItemBase *>(t);
      it->setLocked(true);
    }
}

void editorScene::slotUnlock()
{

  graphItemBase *it;
  if(selectedItems().isEmpty()) return; // nothing to do
  foreach(QGraphicsItem *t,selectedItems())
    {
      it=qgraphicsitem_cast<graphItemBase *>(t);
      it->setLocked(false);
    }
}


void editorScene::slotBringToFront()
{
  if(selectedItems().isEmpty()) return; // nothing to do
  foreach(QGraphicsItem *t,selectedItems())
    {
      zMax+=1;
      t->setZValue(zMax);
    }
  optimizeDepth();
}

void editorScene::slotSendToBack()
{
  if(selectedItems().isEmpty()) return; // nothing to do
  foreach(QGraphicsItem *t,selectedItems())
    {
      t->setZValue(0.5);
    }
  optimizeDepth();
}


void editorScene::slotSendBackward()
{
  if(selectedItems().isEmpty()) return; // nothing to do
  foreach(QGraphicsItem *t,selectedItems())
    {
      t->setZValue(t->zValue()-1.5);
    }
  optimizeDepth();
}

void editorScene::slotSendForward()
{
  if(selectedItems().isEmpty()) return; // nothing to do
  foreach(QGraphicsItem *t,selectedItems())
    {
      t->setZValue(t->zValue()+1.5);
    }
  optimizeDepth();
}

void editorScene::optimizeDepth()
{
  graphItemBase *it;
  zMax=items().count();
  qreal i=0;
  //  foreach(QGraphicsItem *t,items(itemsBoundingRect ()))
  foreach(QGraphicsItem *t,items())
    {
      it=qgraphicsitem_cast<graphItemBase *>(t);
      if(it->type()==graphItemBase::SBORDER)
        {
          it->setZValue(0.1);
        }
      else if(it->type()>graphItemBase::BASE)
        {
          it->setZValue(zMax-i);
          i+=1;
        }
    }
}

void editorScene::addBorder(int w,int h)
{
  if (borderItemPtr==NULL)
    {
      borderItemPtr=new itemBorder(contextMenu);
      itemSetup(borderItemPtr);
    }
  setSceneRect(0,0,w,h);
  borderItemPtr->setPos(0,0);
  borderItemPtr->setRect(0,0,w,h);
  border=QRectF(0,0,w,h);
  slotSendToBack();
}




void editorScene::addConversion(QChar tag,QString value,bool clear)
{
  if(clear) mexp.clear();
  mexp.addConversion(tag,value);
}

#ifndef QT_NO_DEBUG
void editorScene::dumpItems()
{
  QString t;
  int i;
  QList<QGraphicsItem *> l=items();
  graphItemBase *b;
  addToLog(QString("dump editorView of items: %1").arg(l.count()),LOGEDIT); //exclude border
  for(i=0;i<l.count();i++)
    {
      //      if(l.at(i)->type()>=BASE)
      {
        b=qgraphicsitem_cast<graphItemBase *>(l.at(i));
        int index=b->type();
        if((index>graphItemBase::SBORDER) ||(index<graphItemBase::BASE))
          {
            t=QString("Illegal item type: %1").arg(l.at(i)->type());
          }
        t=itemTypeStr[index-graphItemBase::BASE];
        addToLog(QString("editorViewItems %1 pos=%2,%3 rectxy=%4,%5 size=%6x%7 depth=%8 txt=%9")
                      .arg(t)
                      .arg(b->pos().x()).arg(b->pos().y())
                      .arg(b->rect().x()).arg(b->rect().y())
                      .arg(b->rect().width()).arg(b->rect().height())
                      .arg(b->zValue())
                      .arg(b->text())
                      ,LOGEDIT);
      }
    }

}
#endif






// The parser validates the complete file before this replaces the scene.
void editorScene::importMmsstv(const Mmsstv::Document &document)
{
  clearAll();
  templateColorKey=document.colorKey;
  templateBackground=document.background;
  templateKeyPoint=document.transparencyPoint;
  templateKeyCanvas=document.size;
  if(templateColorKey)setBackgroundBrush(templateBackground);
  imageType=TEMPLATE;
  const Qt::PenStyle styles[] = {Qt::SolidLine, Qt::DashLine, Qt::DotLine,
                                Qt::DashDotLine, Qt::DashDotDotLine, Qt::NoPen, Qt::SolidLine};
  int depth=1;
  for (const Mmsstv::Item &source : document.items) {
    graphItemBase *item=nullptr;
    switch (source.kind) {
      case Mmsstv::Item::Line: item=new itemLine(contextMenu); break;
      case Mmsstv::Item::Rectangle: item=new itemRectangle(contextMenu); break;
      case Mmsstv::Item::Ellipse: item=new itemEllipse(contextMenu); break;
      case Mmsstv::Item::Text: item=new itemText(contextMenu); break;
      case Mmsstv::Item::Image: item=new itemImage(contextMenu); break;
      case Mmsstv::Item::Replay: item=new itemReplayImage(contextMenu); break;
    }
    sitemParam *param=item->getParamPtr();
    param->rotation=0;
    param->hShear=0;
    param->vShear=0;
    param->gradient=sgradientParam();
    param->gradient.pos1=param->gradient.pos2=param->gradient.pos3=param->gradient.pos4=0;
    param->gradient.direction=0;
    if (source.gradientDirection>=0 && source.gradientColors.size()==4) {
      param->gradient.type=sgradientParam::LINEAR;
      param->gradient.direction=source.gradientDirection;
      param->gradient.color1=source.gradientColors[0];
      param->gradient.color2=source.gradientColors[1];
      param->gradient.color3=source.gradientColors[2];
      param->gradient.color4=source.gradientColors[3];
      param->gradient.pos1=0;
      param->gradient.pos2=33;
      param->gradient.pos3=67;
      param->gradient.pos4=100;
    }
    item->setRect(0,0,source.rect.width(),source.rect.height());
    item->setPos(source.rect.topLeft());
    item->setPen(QPen(source.color,source.penWidth,styles[source.penStyle]));
    item->setBrush(source.fill);
    item->setFont(source.font);
    item->setText(source.text);
    item->setImage(source.image);
    if (source.kind==Mmsstv::Item::Line) {
      const qreal length=source.line.length();
      item->setRect(0,0,length,2);
      item->setPos(source.line.pointAt(0.5)-QPointF(length/2,1));
      item->setTransform(qRound(-source.line.angle()),0,0);
    } else if (source.kind==Mmsstv::Item::Text) {
      item->setPos(source.rect.topLeft()+QPointF(0,QFontMetricsF(source.font).ascent()));
      item->setTransform(-source.rotation,0,0);
    }
    item->setZValue(depth++);
    addItem(item);
  }
  zMax=depth;
  addBorder(document.size.width(),document.size.height());
  if (borderItemPtr) borderItemPtr->setZValue(0);
  clearSelection();
}

void editorScene::renderTemplate(QImage &image, Qt::AspectRatioMode aspect)
{
  image.fill(0);
  const bool visible=borderItemPtr && borderItemPtr->isVisible();
  if(borderItemPtr)borderItemPtr->setVisible(false);
  {QPainter painter(&image);painter.setRenderHint(QPainter::Antialiasing);
    render(&painter,QRectF(),border,aspect);}
  if(borderItemPtr)borderItemPtr->setVisible(visible);
  QSize content=image.size();
  if(aspect==Qt::KeepAspectRatio && border.width()>0 && border.height()>0) {
    const double scale=qMin(image.width()/border.width(),image.height()/border.height());
    content=QSize(qRound(border.width()*scale),qRound(border.height()*scale));
  }
  applyTemplateColorKey(image,content);
}

void editorScene::applyTemplateColorKey(QImage &image, const QSize &contentSize)
{
  if(!templateColorKey || image.isNull() || templateKeyCanvas.isEmpty())return;
  const int x=qBound(0,int(qint64(templateKeyPoint.x())*contentSize.width()/templateKeyCanvas.width()),image.width()-1);
  const int y=qBound(0,int(qint64(templateKeyPoint.y())*contentSize.height()/templateKeyCanvas.height()),image.height()-1);
  const QRgb key=image.pixel(x,y)&0xffffff;
  for(int row=0;row<image.height();++row) {
    QRgb *pixels=reinterpret_cast<QRgb *>(image.scanLine(row));
    for(int col=0;col<image.width();++col)if((pixels[col]&0xffffff)==key)pixels[col]=0;
  }
}

QColor editorScene::templateTransparencyBackground() const
{
  return templateBackground.isValid()?templateBackground:QColor(248,248,248);
}

QPoint editorScene::templateTransparencyPoint() const
{
  const QSize canvas=border.size().toSize();
  if(templateKeyCanvas.isEmpty())return QPoint(qMax(0,canvas.width()-1),qMax(0,canvas.height()-1));
  return QPoint(qBound(0,int(qint64(templateKeyPoint.x())*canvas.width()/templateKeyCanvas.width()),qMax(0,canvas.width()-1)),
                qBound(0,int(qint64(templateKeyPoint.y())*canvas.height()/templateKeyCanvas.height()),qMax(0,canvas.height()-1)));
}

bool editorScene::setTemplateTransparency(bool enabled,const QColor &background,const QPoint &point,QString *error)
{
  if(error)error->clear();
  const QSize canvas=border.size().toSize();
  if(enabled && (canvas.width()<1 || canvas.height()<1 || canvas.width()>4096 || canvas.height()>4096 ||
                 !background.isValid() || background.alpha()!=255 ||
                 point.x()<0 || point.y()<0 || point.x()>=canvas.width() || point.y()>=canvas.height())) {
    if(error)*error=tr("Choose an opaque background and a point inside a canvas of at most 4096 by 4096 pixels.");
    return false;
  }
  templateColorKey=enabled;
  if(enabled) {
    templateBackground=background;templateKeyPoint=point;templateKeyCanvas=canvas;
    setBackgroundBrush(background);
  } else setBackgroundBrush(Qt::NoBrush);
  update();return true;
}
