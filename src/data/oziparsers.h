#ifndef OZIPARSERS_H
#define OZIPARSERS_H

#include "parser.h"

class PLTParser : public Parser1
{
protected:
	bool parse(QFile *file, QList<TrackData> &tracks, QList<RouteData> &routes,
	  QList<Area> &polygons, QVector<Waypoint> &waypoints);
};

class RTEParser : public Parser1
{
protected:
	bool parse(QFile *file, QList<TrackData> &tracks, QList<RouteData> &routes,
	  QList<Area> &polygons, QVector<Waypoint> &waypoints);
};

class WPTParser : public Parser1
{
protected:
	bool parse(QFile *file, QList<TrackData> &tracks, QList<RouteData> &routes,
	  QList<Area> &polygons, QVector<Waypoint> &waypoints);
};

#endif // OZIPARSERS_H
