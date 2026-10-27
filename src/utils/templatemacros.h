#ifndef TEMPLATEMACROS_H
#define TEMPLATEMACROS_H
#include <QDateTime>
#include <QTimeZone>
#include "macroexpansion.h"

// Start a fresh TX-template context, using one instant for all time fields.
void configureTemplateContext(macroExpansion &macros, const QString &contact,
                              const QDateTime &instant,
                              const QTimeZone &localZone = QTimeZone::systemTimeZone());
// RSV stays free text. R is its first three characters; N its exact suffix.
void addTemplateReportConversions(macroExpansion &macros, const QString &report);
#endif
