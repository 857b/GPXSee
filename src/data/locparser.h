#ifndef LOCPARSER_H
#define LOCPARSER_H

#include <QXmlStreamReader>
#include "parser.h"

class LOCParser : public Parser
{
public:
	QString errorString() const {return _reader.errorString();}
	int errorLine() const {return _reader.lineNumber();}

protected:
	bool parse(QFile *file, QList<TrackData> &tracks, QList<RouteData> &routes,
	  QList<Area> &polygons, QVector<Waypoint> &waypoints);

private:
	void loc(QVector<Waypoint> &waypoints);
	void waypoint(Waypoint &waypoint);
	Coordinates coordinates();

	QXmlStreamReader _reader;
};

#endif // LOCPARSER_H
