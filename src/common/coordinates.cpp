#include "wgs84.h"
#include "coordinates.h"

double Coordinates::distanceTo(const Coordinates &c) const
{
	double dLat = deg2rad(c.lat() - _lat);
	double dLon = deg2rad(c.lon() - _lon);
	double a = pow(sin(dLat / 2.0), 2.0)
	  + cos(deg2rad(_lat)) * cos(deg2rad(c.lat())) * pow(sin(dLon / 2.0), 2.0);

	return (WGS84_RADIUS * (2.0 * atan2(sqrt(a), sqrt(1.0 - a))));
}

Coordinates::delta Coordinates::deltaTo(const Coordinates& c) const
{
	return delta(deg2rad(c.lon() - _lon),
				 deg2rad(c.lat() - _lat));
}

double Coordinates::deltaDistance(const delta& d) const
{
	double aLat = sin(d.dLat / 2.),
		   aLon = cos(deg2rad(_lat)) * sin(d.dLon / 2.);
	double a = pow(aLat, 2.) + pow(aLon, 2.);
	return (WGS84_RADIUS * (2. * atan2(sqrt(a), sqrt(1. - a))));
}

#ifndef QT_NO_DEBUG
QDebug operator<<(QDebug dbg, const Coordinates &c)
{
	dbg.nospace() << qSetRealNumberPrecision(10) << "Coordinates(" << c.lon()
	  << ", " << c.lat() << ")";
	return dbg.space();
}
#endif // QT_NO_DEBUG
