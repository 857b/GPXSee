#include "track.h"

#include <QCoreApplication>

#include <cmath>
#include <algorithm>
#include <stdexcept>

#include "trackpoint.h"
#include "dem.h"

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

static const qreal min_dt = 1e-3;//s
static const qreal min_dt_ms = 1e-3 * 1000.;//ms

static inline qreal deriv3pt(qreal dx1, qreal dy1, qreal dx2, qreal dy2)
{
	qreal r1 = dy1 / dx1,
		  r2 = dy2 / dx2;
	return r1 + (r2 - r1) * (-dx1) / (dx2 - dx1);
}

static inline qreal accel3pt(qreal dx1, qreal dy1, qreal dx2, qreal dy2)
{
	qreal r1 = dy1 / dx1,
		  r2 = dy2 / dx2;
	return (r2 - r1) / (2 * (dx2 - dx1));
}


// eliminate outliers
/*
   Modified Z-score (Iglewicz and Hoaglin)
   The acceleration data distribution has usualy a (much) higher kurtosis than
   the normal distribution thus a higher comparsion value than the usual 3.5 is
   required.
*/
static qreal median(QVector<qreal> &v)
{
	qSort(v.begin(), v.end());
	return v.at(v.size() / 2);
}
static qreal MAD(QVector<qreal> &v, qreal m)
{
	for (int i = 0; i < v.size(); i++)
		v[i] = qAbs(v.at(i) - m);
	return median(v);
}
static QSet<int> eliminate(const QVector<qreal> &v)
{
	QSet<int> rm;

	QVector<qreal> w(v);
	qreal m = median(w);
	qreal M = MAD(w, m);

	for (int i = 0; i < v.size(); i++)
		if (qAbs((0.6745 * (v.at(i) - m)) / M) > 5.0)
			rm.insert(i);

	return rm;
}


// df / dt
static inline int succ(int sz, const QSet<int>& outliers, int i)
{
	int j = i + 1;
	while (j < sz && outliers.contains(j)) ++j;
	return j;
}
template<typename F>
static Track::Channel derivatePoints(F& f,
		const QVector<QDateTime>& t, const QSet<int>& outliers)
{
	Track::Channel rt;
	rt.reserve(t.size());
	int i1 = 0, i2 = 0;//[i1, i2]
	int i1s = succ(t.size(), outliers, 0), i2s = i1s;

	for (int i = 0; i < t.size(); ++i) {
		while (i1s < i && t[i1s].msecsTo(t[i]) >= min_dt_ms)
			i1s = succ(t.size(), outliers, i1 = i1s);
		while (i2s < t.size() && t[i].msecsTo(t[i2]) < min_dt_ms)
			i2s = succ(t.size(), outliers, i2 = i2s);

		qreal dt1 = t[i].msecsTo(t[i1]),
			  dt2 = t[i].msecsTo(t[i2]);
		bool  i1v = dt1 <= -min_dt_ms,
			  i2v = dt2 >=  min_dt_ms;

		if (i1v && i2v)
			rt.append(f(i, dt1, i1, dt2, i2));
		else if (i1v)
			rt.append(f(i, dt1, i1));
		else if (i2v)
			rt.append(f(i, dt2, i2));
		else
			rt.append(NAN);
	}

	return rt;
}

struct derivateChannelF {
	const Track::Channel& f;
	derivateChannelF(const Track::Channel& f): f(f) {}

	qreal operator()(int i, qreal dt1, int i1) {
		return 1000. * (f[i1] - f[i]) / dt1;
	}
	
	qreal operator()(int i, qreal dt1, int i1, qreal dt2, int i2) {
		return 1000. * deriv3pt(dt1, f[i1] - f[i], dt2, f[i2] - f[i]);
	}
};

static Track::Channel derivateChannel(
		const Track::Channel& f, const QVector<QDateTime>& t,
		const QSet<int>& outliers)
{
	derivateChannelF d(f);
	return derivatePoints(d, t, outliers);
}

struct derivateCoordF {
	const QVector<Coordinates>& f;
	derivateCoordF(const QVector<Coordinates>& f): f(f) {}

	qreal operator()(int i, qreal dt1, int i1) {
		return 1000. * (f[i].distanceTo(f[i1])) / dt1;
	}
	
	qreal operator()(int i, qreal dt1, int i1, qreal dt2, int i2) {
		Coordinates::delta d1 = f[i].deltaTo(f[i1]),
		                   d2 = f[i].deltaTo(f[i2]);
		Coordinates::delta d(
				1000. * deriv3pt(dt1, d1.dLon, dt2, d2.dLon),
				1000. * deriv3pt(dt1, d1.dLat, dt2, d2.dLat));
		return f[i].deltaDistance(d);
	}
};

static Track::Channel derivateCoord(
		const QVector<Coordinates>& f, const QVector<QDateTime>& t,
		const QSet<int>& outliers)
{
	derivateCoordF d(f);
	return derivatePoints(d, t, outliers);
}

// d²f / dt²
static Track::Channel computeAcceleration(
		const QVector<QDateTime>& t, QVector<Coordinates>& c)
{
	Track::Channel rt;
	rt.reserve(t.size());
	int i1 = 0, i2 = 0;//[i1, i2]
	for (int i = 0; i < t.size(); ++i) {
		while (i1 < i && t[i1 + 1].msecsTo(t[i]) >= min_dt_ms)
			++i1;
		while (i2 < t.size()-1 && t[i].msecsTo(t[i2]) < min_dt_ms)
			++i2;

		qreal dt1 = t[i].msecsTo(t[i1]),
			  dt2 = t[i].msecsTo(t[i2]);

		if (dt1 <= -min_dt_ms && dt2 >= min_dt_ms) {
			Coordinates::delta d1 = c[i].deltaTo(c[i1]),
							   d2 = c[i].deltaTo(c[i2]);
			Coordinates::delta d(
					1000. * accel3pt(dt1, d1.dLon, dt2, d2.dLon),
					1000. * accel3pt(dt1, d1.dLat, dt2, d2.dLat));
			qreal a = c[i].deltaDistance(d);

			if (rt.isEmpty())
				for (int j = 0; j <= i; ++j)
					rt.append(a);
			else
				rt.append(a);

		} else if (!rt.isEmpty())
			rt.append(rt.last());
	}
	return rt;
}


static QString nameComputedFrom(QString src) {
	return src.isEmpty()
		? Track::tr("computed")
		: Track::tr("computed from %1").arg(src);
}

int Track::_elevationWindow = 3;
int Track::_speedWindow = 5;
int Track::_heartRateWindow = 3;
int Track::_cadenceWindow = 3;
int Track::_powerWindow = 3;

bool Track::_automaticPause = true;
qreal Track::_pauseSpeed = 0.5;
int Track::_pauseInterval = 10;

bool Track::_outlierEliminate = true;
bool Track::_useReportedSpeed = false;
bool Track::_useDEM = false;
bool Track::_show2ndElevation = false;
bool Track::_show2ndSpeed = false;

Track::Track(QObject* parent) : QObject(parent) {}

Track::Track(QObject* parent, const TrackData& data)
	: QObject(parent), TrackInfos(data)
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
					computeAcceleration(sg.time, sg.coord), accelChan);

			// Outliers
			if (_outlierEliminate)
				sg.outliers = eliminate(acc_chan);

			// Speed
			if (!~speedChan)
				speedChan = newChannel(ChannelDescr(CTspeed, CSgpsVit));
			Channel& spd_chan = sg.append(
					derivateCoord(sg.coord, sg.time, sg.outliers),
					speedChan);

			// Stop-points
			int pauseInterval;
			qreal pauseSpeed;

			if (_automaticPause) {
				pauseSpeed    = (spd_chan.avg(0, spd_chan.size()) > 2.8) 
							  ? 0.40 : 0.15;
				pauseInterval = 10;
			} else {
				pauseSpeed    = _pauseSpeed;
				pauseInterval = _pauseInterval;
			}

			sg.computeStopPoints(spd_chan, pauseInterval, pauseSpeed);

		}
		
		// Distance
		{
			Channel& ch(sg.append(Channel(_chanDist)));
			qreal d = 0.;
			int i0 = 0;
			for (; i0 < sg.size() && sg.outliers.contains(i0); ++i0)
				ch.append(NAN);
			for (int i1 = i0; i1 < sg.coord.size(); ) {
				d += sg.coord[i0].distanceTo(sg.coord[i1]);
				ch.append(d);
				i0 = i1;
				for (++i1;
					i1 < sg.coord.size() && sg.outliers.contains(i1); ++i1)
					ch.append(NAN);
			}
			distLength += d;
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
				if (!sg.hasTime() || !(elv = sg.findChannel(cid)))
					continue;
				sg.append(derivateChannel(*elv, sg.time, sg.outliers), vs_cid);
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
				ps.append(PathPoint(
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

int Track::filterWindow(ChanTy ct)
{
	switch(ct) {
		case CTelevation:
			return _elevationWindow;
		case CTspeed: case CTvSpeed:
			return _speedWindow;
		case CTheartRate:
			return _heartRateWindow;
		case CTcadence:
			return _cadenceWindow;
		case CTpower:
			return _powerWindow;
		default:
			return 1;
	}
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

Track::Channel Track::Channel::filter(int window) const
{ 
	if (size() < window || window < 2)
		return *this;

	Channel ret(_id, size());

	int w2 = window / 2;
	qreal acc = 0;
	int i1 = 0, i2 = 0;//[i1, i2[

	for (int i = 0; i  < size(); ++i) {
		for (; i2 - i <= w2 && i2 < size(); ++i2)
			acc += at(i2);
		for (; i - i1 > w2; ++i1)
			acc -= at(i1);
		ret[i] = acc / (i2 - i1);
	}

	return ret;
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

void Track::Segment::computeStopPoints(const Channel& speed,
								qreal pauseInterval, qreal pauseSpeed)
{
	qreal pauseTimeMs = 0;
	int ss = 0, la = 0;
	for (int j = 1; j < size(); ++j) {
		if (speed[j] > pauseSpeed)
			ss = -1;
		else if (ss < 0)
			ss = j-1;

		if (ss >= 0 && time[ss].msecsTo(time[j]) > 1000. * pauseInterval) {
			int l = qMax(ss, la);
			pauseTimeMs += time[l].msecsTo(time[j]);
			for (int k = l; k <= j; k++)
				stop.insert(k);
			la = j;
		}
	}
	pauseTime += pauseTimeMs / 1000.;
}

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
