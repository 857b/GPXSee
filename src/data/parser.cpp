#include "parser.h"

bool Parser::parse(QFile *file,
		QList<Track>      &tracks,
		QList<Route>      &routes,
		QList<Area>       &polygons,
		QVector<Waypoint> &waypoints)
{ 
	QList<TrackData> td;
	QList<RouteData> rd;

	if(!parse(file, td, rd, polygons, waypoints))
		return false;

	for (int i = 0; i < td.count(); ++i)
		tracks.append(Track(td.at(i)));
	for (int i = 0; i < rd.count(); ++i)
		routes.append(Route(rd.at(i)));

	return true;
}
	

TrackInfos& TrackBuilder::infos()
{
	return _track;
}

int TrackBuilder::newChannel(const Track::ChannelDescr& ch)
{
	return _track.newChannel(ch);
}

Track::Segment& TrackBuilder::beginSegment(bool timePres)
{
	_track._segments.append(Track::Segment());
	Track::Segment& rt(_track._segments.last());
	rt.timePres = timePres;
	return rt;
}

const Track& TrackBuilder::finalize()
{
	_track.finalize();
	return _track;
}
