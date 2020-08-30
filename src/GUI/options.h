#ifndef _OPTIONS_H
#define _OPTIONS_H

#include "common/config.h"
#include "settings.h"
#include "palette.h"
#include "units.h"
#ifdef ENABLE_TIMEZONES
#include "timezoneinfo.h"
#endif // ENABLE_TIMEZONES

struct DerivOptions {
	qreal min, max, opt; // ms

	DerivOptions(qreal min = DERIV_DMIN_DEFAULT,
			     qreal max = DERIV_DMAX_DEFAULT, 
			     qreal opt = DERIV_DOPT_DEFAULT)
		: min(min), max(max), opt(opt) {}

	bool operator!=(const DerivOptions& o) const {
		return min != o.min || max != o.max || opt != o.opt;
	}
};

struct Options {
	// Appearance
	Palette palette;
	int trackWidth;
	int routeWidth;
	int areaWidth;
	Qt::PenStyle trackStyle;
	Qt::PenStyle routeStyle;
	Qt::PenStyle areaStyle;
	int areaOpacity;
	QColor waypointColor;
	QColor poiColor;
	int waypointSize;
	int poiSize;
	int graphWidth;
	QColor sliderColor;
	bool pathAntiAliasing;
	bool graphAntiAliasing;
	int mapOpacity;
	QColor backgroundColor;
	// Map
	int projection;
#ifdef ENABLE_HIDPI
	bool hidpiMap;
#endif // ENABLE_HIDPI
	// Data
	int elevationFilter;
	int speedFilter;
	int heartRateFilter;
	int cadenceFilter;
	int powerFilter;
	bool outlierEliminate;
	bool automaticPause;
	qreal pauseSpeed;
	int pauseInterval;
	DerivOptions deriv;
	bool speedDirection;
#ifdef ENABLE_TIMEZONES
	TimeZoneInfo timeZone;
#endif // ENABLE_TIMEZONES
	// POI
	int poiRadius;
	// System
	bool useOpenGL;
#ifdef ENABLE_HTTP2
	bool enableHTTP2;
#endif // ENABLE_HTTP2
	int pixmapCache;
	int connectionTimeout;
	// Print/Export
	bool hiresPrint;
	bool printName;
	bool printDate;
	bool printDistance;
	bool printTime;
	bool printMovingTime;
	bool printItemCount;
	bool separateGraphPage;

	Units units;
};


#endif
