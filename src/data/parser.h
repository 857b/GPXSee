#ifndef PARSER_H
#define PARSER_H

#include <QString>
#include <QList>
#include <QVector>
#include <QFile>
#include "track.h"
#include "trackdata.h"
#include "route.h"
#include "routedata.h"
#include "waypoint.h"
#include "area.h"


class Parser
{
public:
	virtual ~Parser() {}

	virtual bool parse(QFile *file,
			QList<Track>      &tracks,
			QList<Route>      &routes,
			QList<Area>       &polygons,
			QVector<Waypoint> &waypoints);

	virtual QString errorString() const = 0;
	virtual int errorLine() const = 0;

protected:
	virtual bool parse(QFile *file, QList<TrackData> &tracks,
			QList<RouteData> &routes, QList<Area> &polygons,
			QVector<Waypoint> &waypoints) {
		Q_UNUSED(file)
		Q_UNUSED(tracks)
		Q_UNUSED(routes)
		Q_UNUSED(polygons)
		Q_UNUSED(waypoints)
		return false;
	};
};


class Parser1 : public Parser
{
public:
	Parser1() : _errorLine(0) {}
	virtual QString errorString() const {return _errorString;}
	virtual int errorLine() const {return _errorLine;};

protected:
	QString _errorString;
	int _errorLine;
};


class TrackBuilder
{
public:
	TrackBuilder() : _track() {}
	~TrackBuilder() {}

	TrackInfos&     infos();
	int             newChannel(const Track::ChannelDescr& ch);
	Track::Segment& beginSegment(bool timePres);
	const Track&    finalize();

private:
	Track _track;
};

#endif // PARSER_H
