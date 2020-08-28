#include "parser.h"

bool Parser::parse(QObject* parent, QFile *file,
		QList<Track*>     &tracks,
		QList<Route>      &routes,
		QList<Area>       &polygons,
		QVector<Waypoint> &waypoints)
{
	QList<TrackData> td;
	QList<RouteData> rd;

	if(!parse(file, td, rd, polygons, waypoints))
		return false;

	for (int i = 0; i < td.count(); ++i)
		tracks.append(new Track(parent, td.at(i)));
	for (int i = 0; i < rd.count(); ++i)
		routes.append(Route(rd.at(i)));

	return true;
}
	

void TrackBuilder::begin(QObject* parent)
{
	_track = new Track(parent);
}

TrackInfos& TrackBuilder::infos()
{
	return *_track;
}

int TrackBuilder::newChannel(const Track::ChannelDescr& ch)
{
	return _track->newChannel(ch);
}

Track::Segment& TrackBuilder::beginSegment(bool timePres)
{
	_track->_segments.append(Track::Segment());
	Track::Segment& rt(_track->_segments.last());
	rt.timePres = timePres;
	return rt;
}

Track* TrackBuilder::finalize()
{
	_track->finalize();
	Track* rt = _track;
	_track = NULL;
	return rt;
}

void TrackBuilder::abort()
{
	delete _track;
	_track = NULL;
}
