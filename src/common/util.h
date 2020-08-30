#ifndef UTIL_H
#define UTIL_H

#include <QtGlobal>
#include <QString>
#if QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
#include <QTextDocument>
#endif

int    str2int(const char *str, int len);
bool   str2sint(const char *str, int len, int& rt);
double niceNum(double x, bool round);

static inline QString qstring_toHtmlEscaped(const QString& p)
{
#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
	return p.toHtmlEscaped();
#else
	return Qt::escape(p);
#endif
}

#endif // UTIL_H
