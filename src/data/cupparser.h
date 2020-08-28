#ifndef CUPPARSER_H
#define CUPPARSER_H

#include "parser.h"

class CUPParser : public Parser1
{
protected:
	bool parse(QFile *file, QList<TrackData> &tracks, QList<RouteData> &routes,
	  QList<Area> &polygons, QVector<Waypoint> &waypoints);

private:
	bool waypoint(const QStringList &entry, QVector<Waypoint> &waypoints);
	bool task(const QStringList &entry, const QVector<Waypoint> &waypoints,
	  QList<RouteData> &routes);
};

#endif // CUPPARSER_H
