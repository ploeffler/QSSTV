#include "templatemacros.h"

void configureTemplateContext(macroExpansion &macros, const QString &contact,
                              const QDateTime &instant, const QTimeZone &localZone)
{
  const QDateTime utc=instant.toUTC();
  const QDateTime local=instant.toTimeZone(localZone);
  macros.clear();
  macros.addConversion('c',contact);
  macros.addConversion('C',contact.toLower());
  macros.addConversion('t',utc.toString("hh:mm"));
  macros.addConversion('T',utc.toString("hhmm"));
  macros.addConversion('d',utc.toString("yyyy/MM/dd"));
  macros.addConversion('D',local.toString("yyyy/MM/dd"));
  macros.addConversion('U',local.toString("hh:mm"));
  macros.addConversion('u',local.toString("hhmm"));
}

void addTemplateReportConversions(macroExpansion &macros, const QString &report)
{
  macros.addConversion('r',report);
  macros.addConversion('R',report.left(3));
  macros.addConversion('N',report.mid(3));
}
