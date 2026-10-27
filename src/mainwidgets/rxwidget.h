#ifndef RXWIDGET_H
#define RXWIDGET_H
#include "ftpfunctions.h"
#include <QWidget>
#include <QFrame>
#include "spectrumwidget.h"
#include "ui_rxwidget.h"


class rxFunctions;
class imageViewer;
class spectrumWidget;
class vuMeter;
class QComboBox;
class QPlainTextEdit;
class QLabel;
class QCheckBox;


namespace Ui {
class rxWidget;
}

class rxWidget : public QWidget
{
  Q_OBJECT
  
public:
  explicit rxWidget(QWidget *parent = 0);
  ~rxWidget();
  void readSettings();
  void writeSettings();
  void startRX(bool st);
  rxFunctions *functionsPtr() {return rxFunctionsPtr;}
  imageViewer *getImageViewerPtr(){ return imageViewerPtr;}
  //  spectrumWidget *fftDisplayPtr() ;
  vuMeter *vMeterPtr();
  vuMeter *sMeterPtr();
  drmConstellationFrame *mscWdg() {return ui->drmMSCWidget;}
  drmConstellationFrame *facWdg() {return ui->drmFACWidget;}
  void setDRMStatusText(QString txt)
  {
    ui->drmStatusLineEdit->clear();
    ui->drmStatusLineEdit->appendPlainText(txt);
  }
  void setOnlineStatus(bool online, QString info="");
  drmStatusFrame *statusWdg() {return ui->drmStatusWidget;}
  //  int getFilterIndex();
  void init();
  void setSSTVStatusText(QString txt);
  void setSettingsTab();
  void changeTransmissionMode(int rxtxMode);
  bool rxBusy();
  void setKgstvBusy(bool busy);

private slots:
  void slotKgstvImage(QImage image,int received,bool complete,quint64 epoch);
  void slotKgstvText(QString sender,QString text);
  void slotKgstvStatus(QString status);
  void slotKgstvLocalText(QString text,bool sent);
  void slotStart();
  void slotStop();
  void slotResync();
  void slotGetParams();
  void slotTransmissionMode(int rxtxMode);
  void slotNewCall(QString);
  void slotResetCall();
  void slotLogCall();
  void slotErase();
  void slotSave();
  void slotWho();
  void slotWhoResult(bool err);

signals:
  void modeSwitch(int);


private:
  Ui::rxWidget *ui;
  rxFunctions *rxFunctionsPtr;
  imageViewer *imageViewerPtr;
  void getParams();
  void setParams();
  ftpFunctions ff;
  bool doRemove;
  QComboBox *kgstvModulation;
  QCheckBox *kgstvAutoSave;
  QPlainTextEdit *kgstvHistory;
  QLabel *kgstvStatus;
  bool kgstvSaved=false;
};
#endif // RXWIDGET_H
