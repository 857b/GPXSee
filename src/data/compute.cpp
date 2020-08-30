#include "compute.h"

namespace Compute {

int   elevationWindow  = 3;
int   speedWindow      = 5;
int   heartRateWindow  = 3;
int   cadenceWindow    = 3;
int   powerWindow      = 3;

bool  automaticPause   = true;
qreal pauseSpeed       = 0.5;
int   pauseInterval    = 10;

bool  outlierEliminate = true;

DerivOptions derivOptions = DerivOptions();

bool  useSpeedDirection = false;


Track::Channel filter(const Track::Channel& ch,
					  int window, QSet<int> outliers)
{ 
	if (ch.size() < window || window < 2)
		return ch;

	Track::Channel ret(ch._id, ch.size());

	int w2 = window / 2;
	qreal acc   = 0;
	int   count = 0;
	int i1 = 0, i2 = 0;//[i1, i2[

	for (int i = 0; i  < ch.size(); ++i) {
		for (; i2 - i <= w2 && i2 < ch.size(); ++i2)
			if (!outliers.contains(i2)) {
				acc += ch.at(i2);
				++count;
			}
		for (; i - i1 > w2; ++i1)
			if (!outliers.contains(i1)) {
				acc -= ch.at(i1);
				--count;
			}
		ret[i] = acc / count;
	}

	return ret;
}


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
QSet<int> eliminate(const QVector<qreal> &acc)
{
	QSet<int> rm;

	QVector<qreal> w(acc);
	qreal m = median(w);
	qreal M = MAD(w, m);

	for (int i = 0; i < acc.size(); i++)
		if (qAbs((0.6745 * (acc.at(i) - m)) / M) > 5.0)
			rm.insert(i);

	return rm;
}

void stopPoints(Track::Segment& sg, const Track::Channel& speed)
{
	qreal pauseSpeed;
	int   pauseInterval;

	if (automaticPause) {
		pauseSpeed    = (speed.avg(0, speed.size()) > 2.8) 
					  ? 0.40 : 0.15;
		pauseInterval = 10;
	} else {
		pauseSpeed    = Compute::pauseSpeed;
		pauseInterval = Compute::pauseInterval;
	}
	
	qreal pauseTimeMs = 0;
	int ss = 0, la = 0;
	for (int j = 1; j < speed.size(); ++j) {
		if (speed[j] > pauseSpeed)
			ss = -1;
		else if (ss < 0)
			ss = j-1;

		if (ss >= 0
		 && sg.time[ss].msecsTo(sg.time[j]) > 1000. * pauseInterval) {
			int l = qMax(ss, la);
			pauseTimeMs += sg.time[l].msecsTo(sg.time[j]);
			for (int k = l; k <= j; k++)
				sg.stop.insert(k);
			la = j;
		}
	}
	sg.pauseTime = pauseTimeMs / 1000.;
}

static inline int succ(int sz, const QSet<int>& outliers, int i)
{
	int j = i + 1;
	while (j < sz && outliers.contains(j)) ++j;
	return j;
}
// dt1 >= dt0
static inline bool better_inf(qreal dt0, qreal dt1, const DerivOptions& opt)
{
	if (dt0 <= 0) return false;
	if (dt1 > opt.max) return true;
	if (dt0 < opt.min) return false;
	return std::abs(dt0 - opt.opt) < std::abs(dt1 - opt.opt);
}
// dt0 >= dt1
static inline bool better_sup(qreal dt0, qreal dt1, const DerivOptions& opt)
{
	if (dt1 <= 0 || dt1 < opt.min) return true;
	if (dt0 > opt.max) return false;
	return std::abs(dt0 - opt.opt) < std::abs(dt1 - opt.opt);
}
template<typename F>
static Track::Channel derivatePoints(F& f, const QVector<QDateTime>& t,
		const QSet<int>& outliers, const DerivOptions& opt)
{
	Track::Channel rt;
	rt.reserve(t.size());
	int i1 = 0, i2 = 0;//[i1, i2]
	int i1s = succ(t.size(), outliers, 0), i2s = i1s;

	for (int i = 0; i < t.size(); ++i) {
		while (i1s < i
		  && better_inf(t[i1s].msecsTo(t[i]), t[i1].msecsTo(t[i]), opt))
			i1s = succ(t.size(), outliers, i1 = i1s);
		while (i2s < t.size()
		  && better_sup(t[i].msecsTo(t[i2s]), t[i].msecsTo(t[i2]), opt))
			i2s = succ(t.size(), outliers, i2 = i2s);

		qreal dt1 = t[i].msecsTo(t[i1]),
			  dt2 = t[i].msecsTo(t[i2]);
		bool  i1v = -opt.max <= dt1 && dt1 <= -opt.min && dt1 < 0,
			  i2v =  opt.min <= dt2 && dt2 <=  opt.max && dt2 > 0;

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

Track::Channel derivateChannel(const Track::Channel& f,
		const QVector<QDateTime>& t, const QSet<int>& outliers,
		const DerivOptions& opt)
{
	derivateChannelF d(f);
	return derivatePoints(d, t, outliers, opt);
}

struct derivateCoordF {
	const QVector<Coordinates>& f;
	derivateCoordF(const QVector<Coordinates>& f): f(f) {}

	qreal operator()(int i, qreal dt1, int i1) {
		return 1000. * (f[i].distanceTo(f[i1])) / std::abs(dt1);
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

Track::Channel speed(const QVector<Coordinates>& f,
		const QVector<QDateTime>& t, const QSet<int>& outliers,
		const DerivOptions& opt)
{
	derivateCoordF d(f);
	return derivatePoints(d, t, outliers, opt);
}

// d²f / dt²
Track::Channel acceleration(const QVector<QDateTime>& t,
		const QVector<Coordinates>& c, const DerivOptions& opt,
		bool accDir)
{
	Track::Channel rt;
	rt.reserve(t.size());
	int i1 = 0, i2 = 0;//[i1, i2]
	for (int i = 0; i < t.size(); ++i) {
		while (i1 < i - 1
		  && better_inf(t[i1+1].msecsTo(t[i]), t[i1].msecsTo(t[i]), opt))
			++i1;
		while (i2 < t.size() - 1
		  && better_sup(t[i].msecsTo(t[i2+1]), t[i].msecsTo(t[i2]), opt))
			++i2;

		qreal dt1 = t[i].msecsTo(t[i1]),
			  dt2 = t[i].msecsTo(t[i2]);
		
		if (-opt.max <= dt1 && dt1 <= -opt.min && dt1 < 0
		  && opt.min <= dt2 && dt2 <=  opt.max && dt2 > 0) {
			qreal a;
			if (accDir) {
				Coordinates::delta d1 = c[i].deltaTo(c[i1]),
								   d2 = c[i].deltaTo(c[i2]);
				Coordinates::delta d(
						1000. * accel3pt(dt1, d1.dLon, dt2, d2.dLon),
						1000. * accel3pt(dt1, d1.dLat, dt2, d2.dLat));
				a = c[i].deltaDistance(d);
			} else {
				qreal d1 = c[i].distanceTo(c[i1]),
					  d2 = c[i].distanceTo(c[i2]);
				a = 1000. * accel3pt(dt1, d1, dt2, d2);
			}

			if (rt.isEmpty())
				for (int j = 0; j <= i; ++j)
					rt.append(a);
			else
				rt.append(a);

		} else if (!rt.isEmpty())
			rt.append(rt.last());
	}
	if (rt.isEmpty())
		for (int i = 0; i < t.size(); ++i)
			rt.append(NAN);
	return rt;
}

int filterWindow(const Track::ChannelDescr& d)
{
	switch (d._src) {
		case CSgpsVit: case CSgpsAcc: case CSderiv: case CSgpsDist:
			return 1;
		case CSbase: case CSdem: break;
	}
	switch(d._ty) {
		case CTelevation:
			return elevationWindow;
		case CTspeed: case CTvSpeed:
			return speedWindow;
		case CTheartRate:
			return heartRateWindow;
		case CTcadence:
			return cadenceWindow;
		case CTpower:
			return powerWindow;
		default:
			return 1;
	}
}

}
