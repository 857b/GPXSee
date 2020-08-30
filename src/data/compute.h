#ifndef _COMPUTE_H
#define _COMPUTE_H

#include <QSet>
#include "track.h"
#include "GUI/optionsdialog.h"

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
