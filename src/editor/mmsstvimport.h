#ifndef MMSSTVIMPORT_H
#define MMSSTVIMPORT_H
#include "mmsstvtemplate.h"
namespace Mmsstv {
struct ImportResult {
  QString source, target, error;
  QStringList warnings;
};
QStringList importWarnings(const Document &document);
// Existing destination entries and source files are never replaced.
QVector<ImportResult> importTemplates(const QStringList &sources, const QString &directory);
}
#endif
