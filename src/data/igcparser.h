#ifndef IGCPARSER_H
#define IGCPARSER_H

#include <QCoreApplication>
#include <QDate>
#include <QTime>
#include "parser.h"


class IGCParser : public Parser1
{
	Q_DECLARE_TR_FUNCTIONS(IGCParser)

public:
	bool parse(QObject* parent, QFile *file,
			QList<Track*>     &tracks,
			QList<Route>      &routes,
			QList<Area>       &polygons,
			QVector<Waypoint> &waypoints);

	struct CTX {
		QDate date;
		QTime time;
		int   tz_offset;

		Track::Segment* sg;
		Track::Channel *prsAlt, *gpsAlt;

		CTX();
	};

	bool readHRecord(CTX &ctx, const char *line, int len);
	bool readBRecord(CTX &ctx, const char *line, int len);
	bool readCRecord(const char *line, int len, RouteData &route);
};

#endif // IGCPARSER_H
