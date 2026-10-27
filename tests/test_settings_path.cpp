// Build with Qt Core and src/utils/qsstvsettings.cpp; no GUI or radio required.
#include "qsstvsettings.h"
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QTemporaryDir>
#include <QTextStream>
#include <QLockFile>
#include <QProcess>

#define CHECK(condition) do { if (!(condition)) { \
  QTextStream(stderr) << "FAILED line " << __LINE__ << ": " << #condition << '\n'; \
  return 1; } } while (false)
static bool writeFile(const QString &path,const QByteArray &contents)
{
  if(!QDir().mkpath(QFileInfo(path).absolutePath()))return false;
  QFile file(path);return file.open(QIODevice::WriteOnly) && file.write(contents)==contents.size();
}
static QByteArray readFile(const QString &path)
{
  QFile file(path);if(!file.open(QIODevice::ReadOnly))return QByteArray();return file.readAll();
}
int main(int argc,char **argv)
{
  QCoreApplication app(argc,argv);
  QCoreApplication::setOrganizationName("ON4QZ");
  QCoreApplication::setApplicationName("qsstv_9.0"); // Native/template identity stays independent.
  QCoreApplication::setApplicationVersion("9.6.0");
  QString error;
  if(app.arguments().contains("--worker"))return initializeQsstvSettings(&error)?0:2;
  QTemporaryDir temporary;CHECK(temporary.isValid());
  const QString root=temporary.path();
  const QByteArray original("; preserved comment\n[Operator]\ncallsign=OE6PLD\n[RX]\nautoSave=true\n[Private]\nunknown=keep\nblob=@ByteArray(abc)\n");
  auto use=[&](const QString &name) {const QString dir=root+"/"+name;qputenv("XDG_CONFIG_HOME",dir.toUtf8());return dir;};
  auto clean=[&](const QString &directory) {return QDir(directory).entryList({".qsstv-config-*"},QDir::AllEntries|QDir::Hidden|QDir::NoDotAndDotDot).isEmpty();};

  qputenv("HOME",(root+"/home").toUtf8());qunsetenv("XDG_CONFIG_HOME");
  CHECK(initializeQsstvSettings(&error));
  {QsstvSettings settings;CHECK(settings.fileName()==root+"/home/.config/qsstv_9.6.0.conf");}
  const QString fresh=use("fresh");CHECK(initializeQsstvSettings(&error));
  CHECK(QFileInfo::exists(fresh+"/qsstv_9.6.0.conf"));
  CHECK(!QFileInfo::exists(fresh+"/ON4QZ"));
  {QsstvSettings settings;settings.setValue("RX/autoSave",true);settings.sync();CHECK(settings.status()==QSettings::NoError);}

  // Both historical paths are recognized; direct files win at equal versions.
  for(const QString &location:{QString(),QString("ON4QZ/")}) {
    const QString dir=use(location.isEmpty()?"direct":"legacy");
    const QString source=dir+"/"+location+"qsstv_9.0.conf";
    CHECK(writeFile(source,original));CHECK(initializeQsstvSettings(&error));
    CHECK(readFile(dir+"/qsstv_9.6.0.conf")==original && readFile(source)==original);
    {QsstvSettings settings;CHECK(settings.value("Operator/callsign").toString()=="OE6PLD");
      CHECK(settings.value("RX/autoSave").toBool());CHECK(settings.value("Private/blob").toByteArray()=="abc");
      settings.setValue("Operator/callsign","NEW");settings.sync();CHECK(settings.status()==QSettings::NoError);}
    CHECK(writeFile(source,"[Operator]\ncallsign=OLD\n"));
    CHECK(initializeQsstvSettings(&error));
    {QsstvSettings settings;CHECK(settings.value("Operator/callsign").toString()=="NEW");}
    CHECK(clean(dir));
  }
  const QString equal=use("equal");
  CHECK(writeFile(equal+"/qsstv_9.0.conf",original));
  CHECK(writeFile(equal+"/ON4QZ/qsstv_9.0.conf","[Operator]\ncallsign=WRONG\n"));
  CHECK(initializeQsstvSettings(&error));CHECK(readFile(equal+"/qsstv_9.6.0.conf")==original);

  // Compare numeric versions, not filenames or timestamps; never downgrade a future file.
  const QString versions=use("versions");
  CHECK(writeFile(versions+"/qsstv_9.5.9.conf","[Operator]\ncallsign=OLD\n"));
  CHECK(writeFile(versions+"/qsstv_9.5.12.conf",original));
  CHECK(writeFile(versions+"/ON4QZ/qsstv_9.5.10.conf","[Operator]\ncallsign=OLDER\n"));
  CHECK(writeFile(versions+"/qsstv_9.10.0.conf","[Operator]\ncallsign=FUTURE\n"));
  CHECK(writeFile(versions+"/qsstv_9.9999999999999999.0.conf","[broken"));
  CHECK(writeFile(versions+"/qsstv_9.6-beta.conf","[broken"));
  CHECK(initializeQsstvSettings(&error));CHECK(readFile(versions+"/qsstv_9.6.0.conf")==original);
  CHECK(readFile(versions+"/qsstv_9.10.0.conf").contains("FUTURE"));
  const QString nested=use("newer-nested");
  CHECK(writeFile(nested+"/qsstv_9.5.9.conf","[Operator]\ncallsign=OLD\n"));
  CHECK(writeFile(nested+"/ON4QZ/qsstv_9.5.12.conf",original));
  CHECK(initializeQsstvSettings(&error));CHECK(readFile(nested+"/qsstv_9.6.0.conf")==original);
  // A prior same-version organization file can be moved to the direct location.
  const QString relocated=use("same-version");
  CHECK(writeFile(relocated+"/ON4QZ/qsstv_9.6.0.conf",original));
  CHECK(initializeQsstvSettings(&error));CHECK(readFile(relocated+"/qsstv_9.6.0.conf")==original);

  // Future application versions select the prior full-version file automatically.
  use("versions");QCoreApplication::setApplicationVersion("9.7.0");
  {QCoreApplication::setApplicationVersion("9.6.0");QsstvSettings settings;
    settings.setValue("Operator/callsign","UPGRADE");settings.sync();CHECK(settings.status()==QSettings::NoError);}
  const QByteArray prior=readFile(versions+"/qsstv_9.6.0.conf");
  QCoreApplication::setApplicationVersion("9.7.0");CHECK(initializeQsstvSettings(&error));
  CHECK(readFile(versions+"/qsstv_9.7.0.conf")==prior && readFile(versions+"/qsstv_9.6.0.conf")==prior);
  {QsstvSettings settings;CHECK(settings.fileName()==versions+"/qsstv_9.7.0.conf");CHECK(settings.value("Operator/callsign").toString()=="UPGRADE");}
  QCoreApplication::setApplicationVersion("9.6.0");

  const QString futureOnly=use("future-only");
  CHECK(writeFile(futureOnly+"/qsstv_10.0.0.conf",original));
  CHECK(initializeQsstvSettings(&error));CHECK(readFile(futureOnly+"/qsstv_9.6.0.conf").isEmpty());
  CHECK(readFile(futureOnly+"/qsstv_10.0.0.conf")==original);
  const QString corrupt=use("corrupt");
  CHECK(writeFile(corrupt+"/qsstv_9.5.12.conf","[broken\nkey=value\n"));
  CHECK(writeFile(corrupt+"/qsstv_9.0.conf",original));
  CHECK(!initializeQsstvSettings(&error));CHECK(error.contains("Invalid configuration"));
  CHECK(!QFileInfo::exists(corrupt+"/qsstv_9.6.0.conf") && clean(corrupt));
  const QString corruptTarget=use("corrupt-target");
  CHECK(writeFile(corruptTarget+"/qsstv_9.6.0.conf","[broken\nkey=value\n"));
  CHECK(writeFile(corruptTarget+"/qsstv_9.0.conf",original));
  CHECK(!initializeQsstvSettings(&error));CHECK(readFile(corruptTarget+"/qsstv_9.6.0.conf").startsWith("[broken"));

  const QString invalid=use("invalid");CHECK(QDir().mkpath(invalid+"/qsstv_9.6.0.conf"));
  CHECK(!initializeQsstvSettings(&error) && !error.isEmpty());
  const QString badLegacy=use("badlegacy");CHECK(QDir().mkpath(badLegacy+"/ON4QZ/qsstv_9.0.conf"));
  CHECK(!initializeQsstvSettings(&error));CHECK(!QFileInfo::exists(badLegacy+"/qsstv_9.6.0.conf"));
#ifdef Q_OS_UNIX
  const QString dangling=use("dangling");CHECK(QDir().mkpath(dangling));
  CHECK(QFile::link(dangling+"/absent",dangling+"/qsstv_9.6.0.conf"));
  CHECK(!initializeQsstvSettings(&error));CHECK(QFileInfo(dangling+"/qsstv_9.6.0.conf").isSymLink());
  const auto permissions=QFileInfo(versions+"/qsstv_9.7.0.conf").permissions();
  CHECK(!(permissions & (QFile::ReadGroup|QFile::WriteGroup|QFile::ReadOther|QFile::WriteOther)));
#endif
  CHECK(writeFile(root+"/blocked","file"));qputenv("XDG_CONFIG_HOME",(root+"/blocked/config").toUtf8());
  CHECK(!initializeQsstvSettings(&error));
  const QString badVersion=use("bad-version");
  for(const QString &version:{QString(),QString("../unsafe"),QString("9.6-beta"),QString("9.9999999999999999.0")}) {
    QCoreApplication::setApplicationVersion(version);CHECK(!initializeQsstvSettings(&error));
    CHECK(!QFileInfo::exists(badVersion));
  }
  QCoreApplication::setApplicationVersion("9.6.0");
  // Two fresh startup processes must not expose a partial file or overwrite each other.
  const QString concurrent=use("concurrent");CHECK(writeFile(concurrent+"/qsstv_9.0.conf",original));
  QProcess first,second;first.start(app.applicationFilePath(),{"--worker"});second.start(app.applicationFilePath(),{"--worker"});
  CHECK(first.waitForFinished(10000) && second.waitForFinished(10000));
  CHECK(first.exitCode()==0 && second.exitCode()==0 && first.exitStatus()==QProcess::NormalExit && second.exitStatus()==QProcess::NormalExit);
  CHECK(readFile(concurrent+"/qsstv_9.6.0.conf")==original && clean(concurrent));
  error="stale error";CHECK(initializeQsstvSettings(&error));CHECK(error.isEmpty());
  QTextStream(stdout)<<"PASS: versioned config paths, HOME/XDG, legacy paths, numeric latest-version selection, future exclusion, direct precedence, full-version upgrade, byte-preserved originals/unknown keys, persistence, malformed/invalid paths and versions, private permissions, concurrent migration and cleanup\n";
  return 0;
}
