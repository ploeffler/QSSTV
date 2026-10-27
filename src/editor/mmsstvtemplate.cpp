// Independent reader of the on-disk format described by CDraw/Draw.cpp:
// https://github.com/n5ac/mmsstv (CDrawGroup, CDrawText, CDrawPic).
#include "mmsstvtemplate.h"
#include <QFile>
#include <QTextCodec>
#include <QPainter>
#include <QPainterPath>
#include <QPolygonF>
#include <QtEndian>
#include <cmath>
#include <climits>
#include <stdexcept>

namespace Mmsstv {
namespace {
void warn(Document &d, const QString &message) {
  if (!d.warnings.contains(message)) d.warnings.append(message);
}
class Reader {
public:
  QByteArray data;
  int pos = 0, count = 0, vertices = 0;
  qint64 pixels = 0;
  Document result;
  explicit Reader(const QByteArray &bytes) : data(bytes) {}
  void require(int n) {
    if (n<0 || n>data.size()-pos) throw std::runtime_error("Truncated MMSSTV template.");
  }
  quint8 byte() { require(1); return quint8(data[pos++]); }
  qint32 integer() {
    require(4); qint32 v=qFromLittleEndian<qint32>(reinterpret_cast<const uchar *>(data.constData()+pos));pos+=4;return v;
  }
  QByteArray string() {
    int n=integer();if(n<0 || n>65536) throw std::runtime_error("Invalid text length.");
    require(n);QByteArray v=data.mid(pos,n);pos+=n;return v;
  }
  QColor color() {
    quint32 c=quint32(integer());
    if(c&0xff000000) {
      warn(result,"Windows system colors were replaced with black.");return Qt::black;
    }
    return QColor(c&255,(c>>8)&255,(c>>16)&255);
  }
  QImage bitmap() {
    int w=integer(),h=integer();
    if(w==0 && h==0) return QImage();
    if(w<=0 || h<=0 || w>4096 || h>4096 || (pixels+=qint64(w)*h)>16777216)
      throw std::runtime_error("Unsupported or oversized bitmap dimensions.");
    require(26);
    if(data.mid(pos,2)!="BM") throw std::runtime_error("Unsupported embedded bitmap format.");
    const uchar *p=reinterpret_cast<const uchar *>(data.constData()+pos);
    quint32 n=qFromLittleEndian<quint32>(p+2);
    if(n<26 || n>quint32(data.size()-pos)) throw std::runtime_error("Invalid embedded bitmap size.");
    // Inspect dimensions before invoking the image decoder.
    quint32 header=qFromLittleEndian<quint32>(p+14);
    int bw,bh;
    if(header==12) {bw=qFromLittleEndian<quint16>(p+18);bh=qFromLittleEndian<quint16>(p+20);}
    else if(header>=40 && n>=54) {bw=qFromLittleEndian<qint32>(p+18);qint32 raw=qFromLittleEndian<qint32>(p+22);if(raw==INT_MIN)throw std::runtime_error("Invalid bitmap height.");bh=qAbs(raw);}
    else throw std::runtime_error("Unsupported BMP header.");
    if(bw!=w || bh!=h)throw std::runtime_error("Bitmap dimensions do not match the template.");
    QImage image=QImage::fromData(data.mid(pos,int(n)),"BMP");pos+=int(n);
    if(image.isNull()) throw std::runtime_error("Cannot decode embedded bitmap.");
    return image;
  }
  QPolygonF polygon() {
    int n=integer(),w=256,h=200;
    const bool legacy=n!=0x55aa2233;
    if(!legacy){n=integer();w=integer();h=integer();}
    if(n<3 || n>4096 || (vertices+=n)>16384 || w<=0 || h<=0 || w>16384 || h>16384)
      throw std::runtime_error("Invalid or oversized polygon mask.");
    require(n*8);
    QPolygonF points;
    for(int i=0;i<n;++i) {
      int x=integer(),y=integer();
      if(qAbs(qint64(x))>qint64(w)*4 || qAbs(qint64(y))>qint64(h)*4)
        throw std::runtime_error("Polygon coordinates exceed the import limit.");
      points.append(QPointF(double(x)/w,double(y)/h));
    }
    if(legacy)warn(result,"Legacy polygon coordinates were normalized to the image; check the preview.");
    return points;
  }
  void maskImage(QImage &image,int shape,const QPolygonF &polygon) {
    const QRectF bounds(0,0,image.width(),image.height());
    QPainterPath path;
    if(shape>=1 && shape<=3) {
      const double factor=shape==1?1.0/6:shape==2?1.0/4:3.0/8;
      path.addRoundedRect(bounds,bounds.width()*factor,bounds.height()*factor);
    } else if(shape==4)path.addEllipse(bounds);
    else {
      QPolygonF scaled;
      for(const QPointF &point:polygon)scaled.append(QPointF(point.x()*bounds.width(),point.y()*bounds.height()));
      path.setFillRule(Qt::OddEvenFill);path.addPolygon(scaled);path.closeSubpath();
    }
    QImage mask(image.size(),QImage::Format_ARGB32_Premultiplied);mask.fill(0);
    {QPainter painter(&mask);painter.setRenderHint(QPainter::Antialiasing);painter.fillPath(path,Qt::white);}
    image=image.convertToFormat(QImage::Format_ARGB32_Premultiplied);
    {QPainter painter(&image);painter.setCompositionMode(QPainter::CompositionMode_DestinationIn);painter.drawImage(0,0,mask);}
    warn(result,"Picture shape was baked into image transparency; mask vertices are not editable and edge rendering may differ.");
  }
  void roundedFrame(Item &item,int shape) {
    const int pad=item.penStyle==5?0:(item.penWidth+1)/2+1;
    const int w=int(item.rect.width())+2*pad,h=int(item.rect.height())+2*pad;
    if(w<1 || h<1 || w>4096 || h>4096 || (pixels+=qint64(w)*h)>16777216)
      throw std::runtime_error("Rounded frame exceeds the image import limit.");
    QImage image(w,h,QImage::Format_ARGB32_Premultiplied);image.fill(0);
    const Qt::PenStyle styles[]={Qt::SolidLine,Qt::DashLine,Qt::DotLine,Qt::DashDotLine,Qt::DashDotDotLine,Qt::NoPen};
    {QPainter painter(&image);painter.setRenderHint(QPainter::Antialiasing);
      painter.setPen(QPen(item.color,item.penWidth,styles[item.penStyle]));painter.setBrush(item.fill);
      const double factor=shape==1?1.0/6:shape==2?1.0/4:3.0/8;
      const QRectF bounds(pad,pad,item.rect.width()-1,item.rect.height()-1);
      painter.drawRoundedRect(bounds,bounds.width()*factor,bounds.height()*factor);
    }
    item.kind=Item::Image;item.image=image;
    item.rect.adjust(-pad,-pad,pad,pad);item.penStyle=5;item.penWidth=0;
    warn(result,"Rounded frame was rasterized; its curve is not separately editable and edge rendering may differ.");
  }
  void record(int depth=0, bool root=false) {
    if(depth>16 || ++count>4096) throw std::runtime_error("Too many or deeply nested template objects.");
    int command=integer(),version=integer();
    if(version<0 || version>7)throw std::runtime_error("Unsupported object version.");
    if(root && command!=1)throw std::runtime_error("Not an MMSSTV group template.");
    int x1=integer(),y1=integer(),x2=integer(),y2=integer();
    if(qAbs(qint64(x1))>16384 || qAbs(qint64(y1))>16384 || qAbs(qint64(x2))>16384 || qAbs(qint64(y2))>16384)
      throw std::runtime_error("Object coordinates exceed the import limit.");
    Item item;item.rect=QRectF(QPointF(x1,y1),QPointF(x2+1,y2+1)).normalized();
    item.line=QLineF(x1,y1,x2,y2);item.color=color();item.penStyle=byte();
    const int sourcePen=item.penStyle;
    int boxStyle=0;
    quint32 width=quint32(integer());
    if((width&0xffff0000)==0x55aa0000) {
      boxStyle=integer();width=quint32(integer());
    }
    if(width>256 || (sourcePen>6 && sourcePen!=255))throw std::runtime_error("Invalid pen style or width.");
    if(boxStyle<0 || boxStyle>5)throw std::runtime_error("Unsupported rectangle style.");
    if(sourcePen==255)item.penStyle=5;
    if((command==2 || command==3 || command==6) && (sourcePen==5 || sourcePen==6)) {
      item.penStyle=0;
      warn(result,"Beveled line/frame effects were simplified to visible solid outlines.");
    }
    item.penWidth=int(width);
    if(command==1) {
      if(version>2)throw std::runtime_error("Unsupported group version.");
      QPoint sample(319,255);QColor background(248,248,248);
      if(version>=1) {int x=integer(),y=integer();sample=QPoint(x,y);background=color();}
      int w=320,h=256;
      if(version>=2) {w=integer();h=integer();}
      if(w<=0 || h<=0 || w>4096 || h>4096)throw std::runtime_error("Invalid template canvas size.");
      if(root) {
        if(sample.x()<0 || sample.y()<0 || sample.x()>=w || sample.y()>=h)
          throw std::runtime_error("Template transparency point is outside the canvas.");
        result.size=QSize(w,h);result.colorKey=true;result.background=background;
        result.transparencyPoint=sample;
        warn(result,"Whole-template transparency is sampled after rendering. Saving this behavior requires the new QSSTV template reader.");
      }
      int n=integer();if(n<0 || n>4096-count)throw std::runtime_error("Invalid group item count.");
      for(int i=0;i<n;++i)record(depth+1);
      return;
    }
    if(command==2) item.kind=Item::Line;
    else if(command==3 || command==6) {
      item.kind=boxStyle>=4?Item::Ellipse:Item::Rectangle;
      if(command==6) {
        item.fill=item.color;
        if(sourcePen!=255)item.color=Qt::white;
      }
      if(boxStyle>=1 && boxStyle<=3)roundedFrame(item,boxStyle);
    }
    else if(command==4) {
      item.kind=Item::Text;
      int grade=integer(),shadow=integer(),zero=0,right=0,stack=0,perspective=0,vertical=0;
      if(version>=1)zero=integer();
      if(version>=2){item.rotation=integer()%360;integer();integer();}
      if(version>=4)right=integer();
      if(version>=5){stack=integer();integer();perspective=integer();if(perspective){require(80);pos+=80;}}
      if(version>=6){vertical=integer();integer();}
      QByteArray text=string();
      QVector<QColor> colors;for(int i=0;i<4;++i)colors.append(color());
      QColor contour=color();
      if(version>=3)color();
      QByteArray family=string();int charset=integer(),height=integer(),style=integer();integer();
      if(height==INT_MIN || qAbs(height)>1024 || height==0 || style<0 || style>15)
        throw std::runtime_error("Invalid font attributes.");
      const char *encoding="Windows-1252";
      if(charset==128)encoding="Shift-JIS";
      else if(charset==134)encoding="GBK";
      else if(charset==136)encoding="Big5";
      else if(charset==204)encoding="Windows-1251";
      else if(charset!=0 && charset!=1)warn(result,"An unsupported Windows character set was interpreted as Windows-1252.");
      QTextCodec *codec=QTextCodec::codecForName(encoding);
      if(!codec)throw std::runtime_error("Required text codec is unavailable.");
      item.text=convertMacros(codec->toUnicode(text),&result.warnings);
      item.font.setFamily(codec->toUnicode(family));
      if(height<0)item.font.setPixelSize(-height);else item.font.setPointSize(height);
      item.font.setBold(style&1);item.font.setItalic(style&2);item.font.setUnderline(style&4);item.font.setStrikeOut(style&8);
      item.fill=colors[0];
      const int outlineWidth=item.penWidth;
      item.penStyle=5;item.penWidth=0;
      if(grade==1 || grade==2) {
        item.gradientDirection=grade==1?0:270;
        item.gradientColors=colors;
        warn(result,"Text gradients use editable QSSTV color stops; interpolation may differ slightly.");
      }
      if(shadow==1 && outlineWidth>0) {
        item.color=contour;item.penStyle=0;item.penWidth=2*outlineWidth;
        warn(result,"Text contours use editable QSSTV vector outlines; edge rendering may differ.");
      }
      if(grade==3) {
        item.image=bitmap();
        if(item.image.isNull())throw std::runtime_error("Text bitmap fill has no embedded image.");
        warn(result,"Text bitmap fill was preserved as a tiled texture; tile alignment may differ.");
      }
      if((grade!=0 && grade!=1 && grade!=2 && grade!=3) || (shadow!=0 && shadow!=1) || zero || right || stack || perspective || vertical || item.rotation)
        warn(result,"Text effects, rotation placement, alignment, stacking and vertical text were simplified; check the preview.");
    }
    else if(command==5) {
      if(version<1 || version>5)throw std::runtime_error("Unsupported picture version.");
      int type=integer(),shape=version>=2?integer():0,adjust=version>=4?integer():0;
      quint32 transparent=version>=5?quint32(integer()):0;
      if(shape<0 || shape>5)throw std::runtime_error("Unsupported picture shape.");
      if(type<0 || type>2)throw std::runtime_error("Unsupported picture type.");
      if(type==0) {
        item.kind=Item::Replay;
        warn(result,"MMSSTV history-image placeholders use the last received QSSTV image.");
      } else {
        item.kind=Item::Image;item.image=bitmap();
        if(item.image.isNull())throw std::runtime_error("Picture has no embedded bitmap.");
        if(type==2) {
          int x=transparent&65535,y=transparent>>16;
          if(x>=item.image.width() || y>=item.image.height())throw std::runtime_error("Invalid picture transparency point.");
          QRgb key=item.image.pixel(x,y)&0xffffff;
          item.image=item.image.convertToFormat(QImage::Format_ARGB32);
          for(int row=0;row<item.image.height();++row) {
            QRgb *line=reinterpret_cast<QRgb *>(item.image.scanLine(row));
            for(int col=0;col<item.image.width();++col)if((line[col]&0xffffff)==key)line[col]=0;
          }
          warn(result,"Transparent picture overlays use QSSTV scaling; check their size in the preview.");
        }
      }
      QPolygonF points;
      if(shape==5)points=polygon();
      if(shape && type==1)maskImage(item.image,shape,points);
      else if(shape && type==0)warn(result,"History-image shape masks are not preserved; the placeholder remains rectangular.");
      else if(shape && type==2)warn(result,"MMSSTV transparent overlays ignore shape masks; only the color key is applied.");
      if(adjust)warn(result,"Picture cropping adjustments were simplified; check the preview.");
      item.penStyle=5;item.penWidth=0;
    }
    else if(command==7) {
      if(version<1 || version>3)throw std::runtime_error("Unsupported color-bar version.");
      int type=integer(),vertical=version>=2?integer():0;
      QColor colors[4];for(int i=0;i<4;++i)colors[i]=color();
      if(type==4)string();
      if(type==3) {item.kind=Item::Image;item.image=bitmap();if(item.image.isNull())throw std::runtime_error("Color bar has no bitmap.");}
      else if(type==0) {item.kind=Item::Rectangle;item.fill=colors[0];}
      else if(type==1 || type==2) {
        item.kind=Item::Image;
        int w=int(item.rect.width()),h=int(item.rect.height());
        if(w<1 || h<1 || w>4096 || h>4096 || (pixels+=qint64(w)*h)>16777216)throw std::runtime_error("Oversized color bar.");
        item.image=QImage(w,h,QImage::Format_ARGB32);
        for(int y=0;y<h;++y)for(int x=0;x<w;++x){
          double t=double(vertical?y:x)/qMax(1,(vertical?h:w)-1);
          int parts=type==1?1:3,segment=qMin(parts-1,int(t*parts));double f=t*parts-segment;
          QColor a=colors[segment],b=colors[segment+1];
          item.image.setPixelColor(x,y,QColor(qRound(a.red()+(b.red()-a.red())*f),qRound(a.green()+(b.green()-a.green())*f),qRound(a.blue()+(b.blue()-a.blue())*f)));
        }
        warn(result,"Color-bar gradients were rasterized; interpolation may differ. Check the preview.");
      }
      else throw std::runtime_error("Sound/custom color bars are not supported.");
      item.penStyle=5;item.penWidth=0;
    }
    else throw std::runtime_error("Unsupported MMSSTV object type (OLE/custom objects are not activated).");
    result.items.append(item);
  }
};
}
QString convertMacros(const QString &text, QStringList *warnings) {
  QString out;
  for(int i=0;i<text.size();++i) {
    if(text[i]!='%') {out+=text[i];continue;}
    if(i+1==text.size()){out+="%%";continue;}
    QChar tag=text[++i];
    if(tag=='m' || tag=='c' || tag=='C' || tag=='r' || tag=='v' || tag=='U' || tag=='u' || tag=='%')out+='%'+QString(tag);
    else if(tag=='R' || tag=='N') {
      out+='%'+QString(tag);
      const QString message="Report macros use the TX RSV field; short or empty reports are not replaced with a default 595.";
      if(warnings && !warnings->contains(message))warnings->append(message);
    }
    else if(tag=='n')out+="%o";
    else if(tag=='T')out+="%t";
    else if(tag=='t')out+="%T";
    else if(tag=='D' || tag=='L') {
      out+=tag=='D'?"%d":"%D";
      const QString message="Date format uses QSSTV yyyy/MM/dd.";
      if(warnings && !warnings->contains(message))warnings->append(message);
    }
    else {out+="%%"+QString(tag);QString message=QString("Unmapped MMSSTV macro '%1' was preserved as literal text; edit it before transmission.").arg("%"+QString(tag));if(warnings && !warnings->contains(message))warnings->append(message);}
  }
  return out;
}
bool readTemplate(const QString &fileName, Document *document, QString *error) {
  if(error)error->clear();
  QFile file(fileName);
  if(!document || !file.open(QIODevice::ReadOnly)){if(error)*error="Cannot open MMSSTV template.";return false;}
  if(file.size()>32*1024*1024){if(error)*error="Template exceeds the 32 MiB import limit.";return false;}
  try {
    Reader reader(file.readAll());reader.record(0,true);
    if(reader.pos!=reader.data.size())throw std::runtime_error("Unexpected trailing template data.");
    *document=reader.result;return true;
  } catch(const std::exception &e) {if(error)*error=QString::fromUtf8(e.what());return false;}
}
}
