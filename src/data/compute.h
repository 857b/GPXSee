#ifndef _COMPUTE_H
#define _COMPUTE_H

#include <QSet>
#include "track.h"
#include "GUI/optionsdialog.h"
#include "common/range.h"

namespace Compute {

Track::Channel filter(const Track::Channel& ch,
					  int window, QSet<int> outliers);

// eliminate outliers
QSet<int> eliminate(const QVector<qreal> &acc);

// stop points
void stopPoints(Track::Segment& sg, const Track::Channel& speed);

// df / dt
Track::Channel derivateChannel(const Track::Channel& f,
		const QVector<QDateTime>& t, const QSet<int>& outliers,
		const DerivOptions& opt);
Track::Channel speed(const QVector<Coordinates>& f,
		const QVector<QDateTime>& t, const QSet<int>& outliers,
		const DerivOptions& opt);

// d²f / dt²
Track::Channel acceleration(const QVector<QDateTime>& t,
		const QVector<Coordinates>& c, const DerivOptions& opt,
		bool accDir);

template <typename F>
void iterate_y(F& f, const Track& tk, int cid, int filtrWindow = 1)
{
	for (int i_s = 0; i_s < tk.segments().count(); ++i_s) {
		const Track::Segment& s(tk.segments().at(i_s));
		const Track::Channel* c0(s.findChannel(cid));
		if (!c0) continue;
		const Track::Channel c(
				Compute::filter(*c0, filtrWindow, s.outliers));

		bool beginSeg = true;
		for (int i_p = 0; i_p < c.size(); ++i_p) {
			if (s.outliers.contains(i_p)) continue;
			qreal v(c.at(i_p));
			if (std::isnan(v)) continue;
			f(v, beginSeg);
			beginSeg = false;
		}
	}
}

RangeF bounds(const Track& tk, int cid, int filterWindow = 1);

// options
extern bool  outlierEliminate;
extern int   elevationWindow;
extern int   speedWindow;
extern int   heartRateWindow;
extern int   cadenceWindow;
extern int   powerWindow;
extern bool  automaticPause;
extern qreal pauseSpeed;
extern int   pauseInterval;
extern DerivOptions derivOptions;
extern bool  useSpeedDirection;

int filterWindow(const Track::ChannelDescr& d);
inline void setElevationFilter(int window)        {elevationWindow = window;}
inline void setSpeedFilter(int window)            {speedWindow = window;}
inline void setHeartRateFilter(int window)        {heartRateWindow = window;}
inline void setCadenceFilter(int window)          {cadenceWindow = window;}
inline void setPowerFilter(int window)            {powerWindow = window;}

inline void setAutomaticPause(bool set)           {automaticPause = set;}
inline void setPauseSpeed(qreal speed)            {pauseSpeed = speed;}
inline void setPauseInterval(int interval)        {pauseInterval = interval;}
inline void setOutlierElimination(bool eliminate)
		{outlierEliminate = eliminate;}
inline void setDerivOptions(const DerivOptions& opt) {derivOptions = opt;}
inline void setUseSpeedDirection(bool use) {useSpeedDirection = use;}
}

#endif
