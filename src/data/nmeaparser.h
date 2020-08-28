#ifndef NMEAPARSER_H
#define NMEAPARSER_H

#include <QDate>
#include "parser.h"


class NMEAParser : public Parser1
{
protected:
	bool parse(QFile *file, QList<TrackData> &tracks, QList<RouteData> &routes,
	  QList<Area> &polygons, QVector<Waypoint> &waypoints);

private:
	struct CTX {
		CTX() : GGA(false) {}

		QDate date;
		QTime time;
		bool GGA;
	};

	bool readEW(const char *data, int len, qreal &lon);
	bool readLon(const char *data, int len, qreal &lon);
	bool readNS(const char *data, int len, qreal &lat);
	bool readLat(const char *data, int len, qreal &lat);
	bool readDate(const char *data, int len, QDate &date);
	bool readTime(const char *data, int len, QTime &time);
	bool readAltitude(const char *data, int len, qreal &ele);
	bool readGeoidHeight(const char *data, int len, qreal &gh);

	bool readRMC(CTX &ctx, const char *line, int len, SegmentData &segment);
	bool readGGA(CTX &ctx, const char *line, int len, SegmentData &segment);
	bool readWPL(const char *line, int len, QVector<Waypoint> &waypoints);
	bool readZDA(CTX &ctx, const char *line, int len);
};

#endif // NMEAPARSER_H
