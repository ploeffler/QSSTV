#include "qsstvsettings.h"
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QStandardPaths>
#include <QLockFile>
#include <QRegularExpression>
#include <QTemporaryFile>
#include <QVersionNumber>

#ifdef Q_OS_LINUX
namespace {
const QRegularExpression versionPattern(QStringLiteral("^[0-9]+(?:\\.[0-9]+){1,3}$"));
const QRegularExpression filePattern(QStringLiteral("^qsstv_([0-9]+(?:\\.[0-9]+){1,3})\\.conf$"));

QVersionNumber numericVersion(const QString &text)
{
  int suffix=0;
  const QVersionNumber version=QVersionNumber::fromString(text,&suffix);
  return suffix==text.size()?version.normalized():QVersionNumber();
}

QString configDirectory()
{
  return QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation);
}
QString settingsFileName()
{
  return QDir(configDirectory()).filePath("qsstv_"+QCoreApplication::applicationVersion()+".conf");
}
bool fail(QString *error,const QString &message)
{
  if(error)*error=message;
  return false;
}
bool validateSettings(const QString &path,QString *error)
{
  QFile file(path);
  if(!QFileInfo(path).isFile() || !file.open(QIODevice::ReadOnly))
    return fail(error,QString("Cannot read configuration file: %1").arg(path));
  QSettings settings(path,QSettings::IniFormat);
  settings.setFallbacksEnabled(false);
  settings.allKeys(); // Force INI parsing, including unknown groups.
  if(settings.status()!=QSettings::NoError)
    return fail(error,QString("Invalid configuration file: %1").arg(path));
  return true;
}
bool entryExists(const QString &path)
{
  const QFileInfo entry(path);
  return entry.exists() || entry.isSymLink();
}
}
#endif

QsstvSettings::QsstvSettings()
#ifdef Q_OS_LINUX
  : QSettings(settingsFileName(), QSettings::IniFormat)
#else
  : QSettings()
#endif
{
#ifdef Q_OS_LINUX
  setFallbacksEnabled(false);
#endif
}

bool initializeQsstvSettings(QString *errorMessage)
{
  if(errorMessage)errorMessage->clear();
#ifdef Q_OS_LINUX
  const QString version=QCoreApplication::applicationVersion();
  if(!versionPattern.match(version).hasMatch())
    return fail(errorMessage,QString("Invalid application version for configuration: %1").arg(version));
  const QVersionNumber current=numericVersion(version);
  if(current.isNull())return fail(errorMessage,"Invalid numeric application version.");
  const QString directory=configDirectory();
  if(directory.isEmpty() || !QDir().mkpath(directory))
    return fail(errorMessage,QString("Cannot create the configuration directory: %1").arg(directory));
  // Serialize migration by cooperating QSSTV processes, then check again under the lock.
  QLockFile lock(QDir(directory).filePath(".qsstv-config-upgrade.lock"));
  if(!lock.tryLock(5000))
    return fail(errorMessage,QString("Cannot lock the configuration directory for upgrade: %1").arg(directory));
  const QString destination=settingsFileName();
  if(entryExists(destination))return validateSettings(destination,errorMessage);

  QString source;
  QVersionNumber selected;
  // Direct files have precedence over organization subdirectories at equal versions.
  QStringList roots={directory};
  const QString organization=QCoreApplication::organizationName();
  if(!organization.isEmpty() && organization!="." && organization!=".." &&
     !organization.contains('/') && !organization.contains('\\'))
    roots.append(QDir(directory).filePath(organization));
  if(organization!="ON4QZ")roots.append(QDir(directory).filePath("ON4QZ"));
  for(const QString &root:roots) {
    const QDir folder(root);
    for(const QString &name:folder.entryList({"qsstv_*.conf"},QDir::AllEntries|QDir::System|QDir::NoDotAndDotDot,QDir::Name)) {
      const auto match=filePattern.match(name);
      if(!match.hasMatch())continue;
      const QVersionNumber candidate=numericVersion(match.captured(1));
      if(candidate.isNull() || QVersionNumber::compare(candidate,current)>0)continue;
      const QString path=folder.filePath(name);
      if(path==destination)continue;
      if(source.isEmpty() || QVersionNumber::compare(candidate,selected)>0) {
        source=path;selected=candidate;
      }
    }
  }
  if(!source.isEmpty() && !validateSettings(source,errorMessage))return false;

  // Stage in the destination filesystem. Rename refuses to replace an existing entry.
  // Preserve every setting, unknown key, comment and Qt-encoded value byte-for-byte.
  QTemporaryFile staged(QDir(directory).filePath(".qsstv-config-XXXXXX"));
  if(!staged.open())return fail(errorMessage,"Cannot create the upgraded configuration file.");
  if(!source.isEmpty()) {
    QFile input(source);
    if(!input.open(QIODevice::ReadOnly))return fail(errorMessage,QString("Cannot read configuration: %1").arg(source));
    while(!input.atEnd()) {
      const QByteArray chunk=input.read(65536);
      if(input.error()!=QFile::NoError || staged.write(chunk)!=chunk.size())
        return fail(errorMessage,QString("Cannot copy configuration from %1 to %2").arg(source,destination));
    }
  }
  if(!staged.flush())return fail(errorMessage,"Cannot flush the upgraded configuration file.");
  staged.close();
  if(!QFile::rename(staged.fileName(),destination)) {
    if(entryExists(destination))return validateSettings(destination,errorMessage);
    return fail(errorMessage,QString("Cannot install the upgraded configuration: %1").arg(destination));
  }
  staged.setAutoRemove(false);
#else
  Q_UNUSED(errorMessage);
#endif
  return true;
}
