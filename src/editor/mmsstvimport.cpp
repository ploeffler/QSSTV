#include "mmsstvimport.h"
#include "editorscene.h"
#include <QDir>
#include <QFileInfo>
#include <QFontDatabase>
#include <QTemporaryDir>
#include <QCoreApplication>

QStringList Mmsstv::importWarnings(const Document &document)
{
  QStringList warnings=document.warnings;
  const QStringList families=QFontDatabase().families();
  for(const Item &item:document.items) {
    if(item.kind!=Item::Text || families.contains(item.font.family(),Qt::CaseInsensitive)) continue;
    const QString message=QCoreApplication::translate("editor",
      "Font '%1' is unavailable; Qt will substitute a font.").arg(item.font.family());
    if(!warnings.contains(message)) warnings.append(message);
  }
  return warnings;
}

QVector<Mmsstv::ImportResult> Mmsstv::importTemplates(const QStringList &sources,const QString &directory)
{
  QVector<ImportResult> results;
  const QDir destination(directory);
  for(const QString &source:sources) {
    ImportResult result;result.source=source;
    Document document;
    if(!QFileInfo(directory).isDir()) result.error=QStringLiteral("Destination directory does not exist.");
    else if(readTemplate(source,&document,&result.error)) {
      result.warnings=importWarnings(document);
      QTemporaryDir staging(destination.absoluteFilePath(".qsstv-import-XXXXXX"));
      if(!staging.isValid()) result.error=QStringLiteral("Cannot create a temporary file in the destination directory.");
      else {
        editorScene scene;scene.importMmsstv(document);
        QFile staged(staging.filePath("template.templ"));
        if(!scene.save(staged,true)) result.error=QStringLiteral("Cannot write the converted template.");
        else {
          QString base=QFileInfo(source).completeBaseName();
          if(base.isEmpty() || base=="." || base=="..") base="template";
          for(int suffix=1;suffix<=100000;++suffix) {
            const QString name=base+(suffix==1?QString():QString("_%1").arg(suffix))+".templ";
            const QString target=destination.absoluteFilePath(name);
            const QFileInfo entry(target);
            if(entry.exists() || entry.isSymLink()) continue;
            // QFile::rename refuses to overwrite an entry created concurrently.
            if(QFile::rename(staged.fileName(),target)) {result.target=target;break;}
            const QFileInfo raced(target);
            if(raced.exists() || raced.isSymLink()) continue;
            result.error=QStringLiteral("Cannot move the converted template into the destination directory.");
            break;
          }
          if(result.target.isEmpty() && result.error.isEmpty()) result.error=QStringLiteral("No available output filename.");
        }
      }
    }
    results.append(result);
  }
  return results;
}
