#include <QCoreApplication>
#include "common/coordinates.h"
#include "format.h"


static QString deg2DMS(double val)
{
	int deg = val;
	double r1 = val - deg;

	int min = r1 * 60.0;
	double r2 = r1 - (min / 60.0);

	double sec = r2 * 3600.0;

	return QString("%1°%2'%3\"").arg(deg).arg(min, 2, 10, QChar('0'))
	  .arg(sec, 4, 'f', 1, QChar('0'));
}

static QString deg2DMM(double val)
{
	int deg = val;
	double r1 = val - deg;

	double min = r1 * 60.0;

	return QString("%1°%2'").arg(deg).arg(min, 6, 'f', 3, QChar('0'));
}


QString Format::timeSpan(qreal time, bool full, bool units)
{
	unsigned v[3];
	v[0] = time / 3600;
	v[1] = (time - (v[0] * 3600)) / 60;
	v[2] = time - (v[0] * 3600) - (v[1] * 60);

	if (units) {
		unsigned n = full || v[0] ? 0 : v[1] ? 1 : 2;
		QString rt = QCoreApplication::translate(
						"Format", "%1%2%3", "time with units");
		QString units[3] = {
			QCoreApplication::translate("Format", "%1h"),
			QCoreApplication::translate("Format", "%1m"),
			QCoreApplication::translate("Format", "%1s")
		};
		for (unsigned i = 0; i < n; ++i)
			rt = rt.arg(QString());
		rt = rt.arg(units[n].arg(v[n]));
		for (unsigned i = n + 1; i < 3; ++i)
			rt = rt.arg(units[i].arg(v[i], 2, 10, QChar('0')));
		return rt;
	}
	if (full || v[0])
		return QString("%1:%2:%3")
				.arg(v[0], 2, 10, QChar('0'))
				.arg(v[1], 2, 10, QChar('0'))
				.arg(v[2], 2, 10, QChar('0'));
	else
		return QString("%1:%2")
			.arg(v[1], 2, 10, QChar('0'))
			.arg(v[2], 2, 10, QChar('0'));
}

QString Format::distance(qreal value, Units units)
{
	switch (units) {
		case Imperial:
			return value < MIINM
				? Unit::ft.format(value, 0)
				: Unit::mi.format(value, 1);
		case Nautical:
			return value < NMIINM
				? Unit::ft.format(value, 0)
				: Unit::nmi.format(value, 1);
		default:
			return value < KMINM
				? Unit::m.format(value, 0)
				: Unit::km.format(value, 1);
	}
}

QString Format::elevation(qreal value, Units units)
{
	return units == Metric
		? Unit::m.format(value, 0)
		: Unit::ft.format(value, 0);
}

QString Format::coordinates(const Coordinates &value, CoordinatesFormat type)
{
	QChar yH = (value.lat() < 0) ? 'S' : 'N';
	QChar xH = (value.lon() < 0) ? 'W' : 'E';

	switch (type) {
		case DegreesMinutes:
			return deg2DMM(qAbs(value.lat())) + yH + "," + QChar(0x00A0)
			  + deg2DMM(qAbs(value.lon())) + xH;
			break;
		case DMS:
			return deg2DMS(qAbs(value.lat())) + yH + "," + QChar(0x00A0)
			  + deg2DMS(qAbs(value.lon())) + xH;
			break;
		default:
			QLocale l(QLocale::system());
			return l.toString(qAbs(value.lat()), 'f', 5) + yH + ","
			+ QChar(0x00A0) + l.toString(qAbs(value.lon()), 'f', 5) + xH;
	}
}
