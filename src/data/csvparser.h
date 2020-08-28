#ifndef CSVPARSER_H
#define CSVPARSER_H

#include "parser.h"

class CSVParser : public Parser1
{
protected:
	bool parse(QFile *file, QList<TrackData> &tracks, QList<RouteData> &routes,
	  QList<Area> &polygons, QVector<Waypoint> &waypoints);
};

#endif // CSVPARSER_H
