#ifndef QSSTVSETTINGS_H
#define QSSTVSETTINGS_H

#include <QSettings>

// Call before any settings are read, after setting the application version.
// Linux upgrades to qsstv_<applicationVersion>.conf; existing targets win.
bool initializeQsstvSettings(QString *errorMessage);

class QsstvSettings : public QSettings
{
public:
  QsstvSettings();
};

#endif
