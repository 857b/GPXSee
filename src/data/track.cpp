#include "track.h"

#include <cmath>
#include <algorithm>
#include <stdexcept>

#include "compute.h"
#include "trackpoint.h"
#include "dem.h"

bool Track::_useSegments = true;

QString chanTyName(ChanTy ty)
{
	switch(ty) {
		case CTdistance:
			return Track::tr("Distance");
		case CTelevation:
			return Track::tr("Elevation");
		case CTspeed:
			return Track::tr("Speed");
		case CTaccel:
			return Track::tr("Acceleration");
		case CTvSpeed:
			return Track::tr("Vertical Speed");
		case CTheartRate:
			return Track::tr("Heart rate");
		case CTtemperature:
			return Track::tr("Temperature");
		case CTcadence:
			return Track::tr("Cadence");
		case CTpower:
			return Track::tr("Power");
		case CTratio:
			return Track::tr("Gear ratio");
	}
	qWarning("unknown ty: %d", (int)ty);
	return Track::tr("Unknown");
}


struct TrackpointFields {
	ChanTy ty;
	qreal (Trackpoint::*get)() const;
	bool  (Trackpoint::*has)() const;
};

static const unsigned NbTrackpointFields = 6;
static  const TrackpointFields TrackpointFields[NbTrackpointFields] = {
	{.ty  = CTelevation,
	 .get = &Trackpoint::elevation,   .has = &Trackpoint::hasElevation},
	{.ty  = CTspeed,
	 .get = &Trackpoint::speed,       .has = &Trackpoint::hasSpeed},
	{.ty  = CTheartRate,
	 .get = &Trackpoint::heartRate,   .has = &Trackpoint::hasHeartRate},
	{.ty  = CTtemperature,
	 .get = &Trackpoint::temperature, .has = &Trackpoint::hasTemperature},
	{.ty  = CTcadence,
	 .get = &Trackpoint::cadence,     .has = &Trackpoint::hasCadence},
	{.ty  = CTratio,
	 .get = &Trackpoint::power,       .has = &Trackpoint::hasPower}
};

static bool segmentHas(bool (Trackpoint::*has)() const, const SegmentData& s)
{
	for (int i = 0; i < s.size(); ++i)
		if ((s[i].*has)())
			return true;
	return false;
}

template<typename R, typename T>
static void copyField(R (Trackpoint::*get)() const, const SegmentData& s,
				QVector<T>& d)
{
	d.reserve(s.size());
	for (int i = 0; i < s.size(); ++i)
		d.append((s[i].*get)());
}



static QString nameComputedFrom(QString src) {
	return src.isEmpty()
		? Track::tr("computed")
		: Track::tr("computed from %1").arg(src);
}

Track::Track() {}

Track::Track(const TrackData& data) : TrackInfos(data)
{
	int fieldChan[NbTrackpointFields];
	std::fill(fieldChan, fieldChan + NbTrackpointFields, ~0);

	for (int i_s = 0; i_s < data.size(); ++i_s) {
		_segments.append(Segment());
		Segment& sg(_segments.last());
		const SegmentData& sd(data[i_s]);

		// Coord
		copyField(&Trackpoint::coordinates, sd, sg.coord);

		// Time
		if ((sg.timePres = segmentHas(&Trackpoint::hasTimestamp, sd)))
			copyField(&Trackpoint::timestamp, sd, sg.time);

		// trackpoint fields
		for (unsigned f = 0; f < NbTrackpointFields; ++f)
			if (segmentHas(TrackpointFields[f].has, sd)) {
				if (!~fieldChan[f])
					fieldChan[f] = newChannel(
							ChannelDescr(TrackpointFields[f].ty, CSbase));
				Channel& ch(sg.append(Channel(fieldChan[f])));
				copyField(TrackpointFields[f].get, sd, ch);
			}
	}

	finalize();
}

void Track::finalize()
{
	_chanDist = newChannel(ChannelDescr(CTdistance, CSgpsDist));

	int speedChan = ~0,
		accelChan = ~0;

	timeLength = 0;
	distLength = 0;

	for (int i_s = 0; i_s < _segments.size(); ++i_s) {
		Segment& sg(_segments[i_s]);
		
		sg.time0 = timeLength;
		sg.dist0 = distLength;

		if (sg.hasTime()) {
			
			QDateTime tMin = sg.time.first(),
					  tMax = sg.time.first();

			for (int i = 0; i < sg.time.size(); ++i) {
				if (!sg.time[i].isValid()) {
					sg.timePres = false;
					qWarning("%s segment %d / %d: missing timestamp(s)",
							  qPrintable(name()),
							  i_s + 1, _segments.size());
				} else if (sg.time[i] < tMax) {
					qWarning("%s: %s: time skew detected",
						qPrintable(name()),
						qPrintable(sg.time[i].toString(Qt::ISODate)));
					tMin = qMin(tMin, sg.time[i]);
				} else
					tMax = sg.time[i];
			}

			sg.tms0 = tMin;
			timeLength  += sg.tms0.msecsTo(tMax) / 1000.;

			// Acceleration
			if (~accelChan)
				accelChan = newChannel(ChannelDescr(CTaccel, CSgpsAcc));
			Channel& acc_chan = sg.append(
					Compute::acceleration(sg.time, sg.coord,
						Compute::derivOptions, Compute::useSpeedDirection),
					accelChan);

			// Outliers
			if (Compute::outlierEliminate)
				sg.outliers = Compute::eliminate(acc_chan);
		}
		
		// Distance
		Channel& dist_chan(sg.append(Channel(_chanDist)));
		{
			qreal d = 0.;
			int i0 = 0;
			for (; i0 < sg.size() && sg.outliers.contains(i0); ++i0)
				dist_chan.append(NAN);
			for (int i1 = i0; i1 < sg.coord.size(); ) {
				d += sg.coord[i0].distanceTo(sg.coord[i1]);
				dist_chan.append(d);
				i0 = i1;
				for (++i1;
					i1 < sg.coord.size() && sg.outliers.contains(i1); ++i1)
					dist_chan.append(NAN);
			}
			distLength += d;
		}
		
		if (sg.hasTime()) {
			// Speed
			if (!~speedChan)
				speedChan = newChannel(ChannelDescr(CTspeed, CSgpsVit));
			Channel& spd_chan = sg.append(
					Compute::useSpeedDirection
					? Compute::speed(sg.coord, sg.time,
						sg.outliers, Compute::derivOptions)
					: Compute::derivateChannel(dist_chan, sg.time,
						sg.outliers, Compute::derivOptions),
					speedChan);

			// Stop-points
			Compute::stopPoints(sg, spd_chan);
		}
	}

	if (hasTime()) computeVSpeed();
	computeDEMelevation();
}

void Track::computeVSpeed()
{
	for (int cid = 0; cid < _chanDescr.size(); ++cid)
		if (_chanDescr[cid]._ty == CTelevation) {
			int vs_cid = newChannel(ChannelDescr(
						CTvSpeed, CSderiv, cid));

			for (int i_s = 0; i_s < _segments.size(); ++i_s) {
				Segment& sg(_segments[i_s]);
				const Channel* elv;
				if (sg.hasTime() && (elv = sg.findChannel(cid)))
					sg.append(Compute::derivateChannel(*elv, sg.time,
								sg.outliers, Compute::derivOptions),
							vs_cid);
			}
		}
}

void Track::computeDEMelevation()
{
	int dem_cid = newChannel(ChannelDescr(CTelevation, CSdem));
	for (int i_s = 0; i_s < _segments.size(); ++i_s) {
		Segment& sg(_segments[i_s]);
		Channel  ev(dem_cid);
		for (int i_p = 0; i_p < sg.coord.size(); ++i_p)
			ev.append(DEM::elevation(sg.coord[i_p]));
		sg.append(ev);
	}
}

Path Track::path() const
{
	Path ret;

	for (int i = 0; i < _segments.size(); ++i) {
		const Segment &sg = _segments.at(i);
		if (sg.coord.size() < 2)
			continue;

		ret.append(PathSegment());
		PathSegment &ps = ret.last();
		const Channel &dist(sg[_chanDist]);

		for (int j = 0; j < sg.coord.size(); ++j)
			if (!sg.outliers.contains(j) && !sg.discardStopPoint(j))
				ps.append(PathPoint(j,
							sg.coord[j],
							sg.dist0 + dist[j]));
	}

	return ret;
}

int Track::newChannel(const ChannelDescr& ch)
{
	_chanDescr.append(ch);
	return _chanDescr.size() - 1;
}

int Track::findChannel(int ct, int cs) const
{
	for (int ch = 0; ch < _chanDescr.size(); ++ch)
		if ((!~ct || _chanDescr[ch]._ty  == ct)
		 && (!~cs || _chanDescr[ch]._src == cs))
			return ch;
	return ~0;
}

QList<int> Track::findChannels(ChanTy ct) const
{
	QList<int> rt;
	for (int i = 0; i < _chanDescr.size(); ++i)
		if (_chanDescr[i]._ty == ct)
			rt.append(i);
	return rt;
}

qreal Track::distance() const
{
	return distLength;
}

qreal Track::time() const
{
	return timeLength;
}

qreal Track::movingTime() const
{
	qreal pause = 0;
	for (int i = 0; i < _segments.size(); ++i)
		pause += _segments[i].pauseTime;
	return time() - pause;
}

QDateTime Track::date() const
{
	for (int i = 0; i < _segments.size(); ++i)
		if (!_segments[i].time.isEmpty())
			return _segments[i].time.first();
	return QDateTime();
}

bool Track::isValid() const
{
	for (int i = 0; i < _segments.size(); i++)
		if (_segments[i].size() >= 2)
			return true;
	return false;
}

bool Track::hasTime() const
{
	for (int i = 0; i < _segments.size(); i++)
		if (_segments[i].size() >= 2 && _segments[i].hasTime())
			return true;
	return false;
}

bool Track::hasData(int chanId) const
{
	for (int i = 0; i < _segments.size(); ++i) {
		const Channel* c = _segments.at(i).findChannel(chanId);
		if (c && c->hasData())
			return true;
	}
	return false;
}

template<typename F>
Track::ChannelPoint pointAt(const F& f, int i_s,
		const Track::Segment& s, qreal x)
{
	Track::ChannelPoint p(i_s, s.firstValid(), s.lastValid());
	if (!~p.pt0 || !~p.pt1 || f(p.pt0) > x || f(p.pt1) < x)
		return Track::ChannelPoint(~0);
	// f(p.pt0) <= x <= f(p.pt1)
	while (p.pt1 > p.pt0 + 1) {
		int pt = (p.pt0 + p.pt1) / 2;
		qreal px = f(pt);
		if (px > x)
			p.pt1 = pt;
		else
			p.pt0 = pt;
	}
	// end because firstValid & lastValid are not outliers
	while (s.outliers.contains(p.pt0)) --p.pt0;
	while (s.outliers.contains(p.pt1)) ++p.pt1;

	qreal x0 = f(p.pt0), x1 = f(p.pt1);
	p.t = x1 > x0
		? (x - x0) / (x1 - x0)
		: 0;
	return p;
}

struct TimeGetter {
	const Track::Segment& sg;
	TimeGetter(const Track::Segment& sg) : sg(sg) {}
	qreal operator()(int i) const {return sg.timeAt(i);}
};
Track::ChannelPoint Track::pointAtTime(qreal t) const
{
	for (int i_s = 0; i_s < _segments.count(); ++i_s) {
		const Segment& s(_segments.at(i_s));
		if (!s.hasTime()) continue;
		ChannelPoint p(pointAt(TimeGetter(s), i_s, s, t));
		if (p) return p;
	}
	return ChannelPoint(~0);
}

struct DistGetter {
	const Track::Channel& ch;
	DistGetter(const Track::Channel& ch) : ch(ch) {}
	qreal operator()(int i) const {return ch.at(i);}
};
Track::ChannelPoint Track::pointAtDistance(qreal d) const
{
	for (int i_s = 0; i_s < _segments.count(); ++i_s) {
		const Segment& s(_segments.at(i_s));
		ChannelPoint p(pointAt(DistGetter(s[_chanDist]),
						i_s, s, d - s.dist0));
		if (p) return p;
	}
	return ChannelPoint(~0);
}

qreal Track::distanceAtTime(qreal t) const
{
	ChannelPoint p(pointAtTime(t));
	return p ? p.interpol(_segments[p.seg_num][_chanDist])
	         : distance();
}

// Channel

qreal Track::Channel::sum(int a, int b) const
{
	qreal sum = 0;
	for (int i = a; i < b; ++i)
		sum += at(i);
	return sum;
}

qreal Track::Channel::avg(int a, int b) const
{
	return sum(a, b) / (qreal)size();
}

bool Track::Channel::hasData() const
{
	for (int i = 0; i < size(); ++i)
		if (std::isfinite(at(i)))
			return true;
	return false;
}

// ChannelDescr

QString Track::ChannelDescr::name(const Track& t, bool full) const
{
	if (!_name.isEmpty()) return _name;
	switch (_src) { 
		case CSbase:
			return QString();
		case CSgpsDist:
			return Track::tr("GPS distance");
		case CSgpsVit:
			return Track::tr("GPS speed");
		case CSgpsAcc:
			return Track::tr("GPS acceleration");
		case CSdem:
			return Track::tr("DEM");
		case CSderiv: {
			const QString& pn = t.chanDescr()[_srcArg].name(t, false);
			return full ? nameComputedFrom(pn) : pn;
		}
	}
	return QString();
}

qreal Track::ChannelDescr::sum(const Track& t, const Segment& s, int id,
								int a, int b) const
{
	switch (_src)
	{
		case CSgpsVit:
			return b > a ? s[t._chanDist][b-1] - s[t._chanDist][a] : 0;
			break;
		case CSderiv:
			return b > a ? s[_srcArg][b-1] - s[_srcArg][a] : 0;
			break;
		default:
			return s[id].sum(a, b);
	}
}

qreal Track::ChannelDescr::sum(const Track& t, int id) const
{
	qreal s = 0;
	const QList<Segment>& sgs(t.segments());
	for (int i = 0; i < sgs.count(); ++i) {
		const Segment& sg(sgs.at(i));
		if (sg.findChannel(id))
			s += sum(t, sg, id, 0, sg.size());
	}
	return s;
}

qreal Track::ChannelDescr::tsum(const Track& t, int id) const
{
	qreal s = 0;
	const QList<Segment>& sgs(t.segments());
	for (int i = 0; i < sgs.count(); ++i) {
		const Segment& sg(sgs.at(i));
		if (sg.hasTime() && sg.findChannel(id))
			s += sum(t, sg, id, 0, sg.size());
	}
	return s;
}

// Segment

Track::Segment::Segment(): timePres(false), pauseTime(0) {}

bool Track::Segment::discardStopPoint(int i) const
{
	return (stop.contains(i) && stop.contains(i-1)
	  && stop.contains(i+1) && i > 0 && i < size() - 1);
}

int Track::Segment::addChannel(int chanId)
{
	chan.append(Channel(chanId));
	return chan.size() - 1;
}

Track::Channel& Track::Segment::append(const Channel& ch)
{
	chan.append(ch);
	return chan.last();
}

Track::Channel& Track::Segment::append(const Channel& ch0, int id)
{
	chan.append(ch0);
	Channel& ch(chan.last());
	ch._id = id;
	return ch;
}

Track::Channel& Track::Segment::channel(int id)
{
	for (int i = 0; i < chan.size(); ++i)
		if (chan[i]._id == id)
			return chan[i];
	throw std::invalid_argument("channel id");
}

const Track::Channel* Track::Segment::findChannel(int id) const
{
	for (int i = 0; i < chan.size(); ++i)
		if (chan[i]._id == id)
			return &chan[i];
	return NULL;
}

const Track::Channel& Track::Segment::channel(int id) const
{
	const Channel* ch = findChannel(id);
	if (!ch) throw std::invalid_argument("channel id");
	return *ch;
}

int Track::Segment::firstValid() const
{
	for (int i = 0; i < coord.size(); ++i)
		if (!outliers.contains(i))
			return i;
	return ~0;
}

int Track::Segment::lastValid() const
{
	for (int i = coord.size(); i--;)
		if (!outliers.contains(i))
			return i;
	return ~0;
}

bool Track::Segment::hasTime() const
{
	return timePres;
} 

qreal Track::Segment::timeAt(int i) const
{
	return time0 + tms0.msecsTo(time[i]) / 1000.;
}

qreal Track::Segment::totalTime() const
{
	int lv = lastValid();
	if (!~lv) return 0;
	return tms0.msecsTo(time[lv]) / 1000.;
}

qreal Track::Segment::movingTime() const
{
	return totalTime() - pauseTime;
}
