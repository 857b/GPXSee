#ifndef TRACK_H
#define TRACK_H

#include <QString>
#include <QVector>
#include <QList>
#include <QDateTime>

#include "common/coordinates.h"
#include "link.h"

#include "trackdata.h"
#include "graph.h"
#include "path.h"

enum ChanTy
{
	CTdistance,
	CTelevation,
	CTspeed,
	CTaccel,
	CTvSpeed,
	CTheartRate,
	CTtemperature,
	CTcadence,
	CTpower,
	CTratio
};

enum ChanSrc
{
	CSbase, // from file
	CSgpsDist,
	CSgpsVit,
	CSgpsAcc,
	CSdem,
	CSderiv
};

class Track : public TrackInfos {
public:

	class Channel : public QVector<qreal> 
	{
		public:
			int     _id;     // in Track::_chanDescr

			Channel() {}
			Channel(int id): _id(id) {}

			// [a, b[
			qreal sum(int a, int b) const;
			qreal avg(int a, int b) const;
	};

	struct Segment;

	struct ChannelDescr
	{
		ChanTy  _ty;
		ChanSrc _src;
		int     _srcArg; // CSderiv

		QString _name;

		ChannelDescr() {}
		ChannelDescr(ChanTy ty, ChanSrc src, int srcArg = ~0,
					 QString name=QString())
			: _ty(ty), _src(src), _srcArg(srcArg), _name(name) {}
		ChannelDescr(ChanTy ty, ChanSrc src, QString name)
			: _ty(ty), _src(src), _name(name) {}

		QString fullName() const;

		qreal sum(const Track& t, const Segment& s, int id,
					int a, int b) const;
	};

	struct Segment
	{
		QVector<Coordinates> coord;
		// may be empty if !hasTime()
		QVector<QDateTime>   time;
		QVector<Channel>     chan;
		QSet<int>            outliers;
		QSet<int>            stop;

		// offset of the segment
		qreal                dist0;
		QDateTime            tms0;
		qreal                time0;

		bool                 timePres;
		qreal                pauseTime;


		Segment();

		void           computeStopPoints(const Channel& speed,
								qreal pauseInterval, qreal pauseSpeed);
		bool           discardStopPoint(int i) const;
		
		// adding channels invalidates references
		Channel&       append(const Channel& ch);
		Channel&       append(const Channel& ch, int id);
		Channel&       channel(int id);
		const Channel& channel(int id) const;
		const Channel* findChannel(int id) const;
		
		int            firstValid() const;
		int            lastValid() const;
		
		bool           hasTime() const;
		qreal          timeAt(int i) const;

		int size() const {return coord.size();}
		const Channel& operator[](int id) const {return channel(id);}
		Channel& operator[](int id) {return channel(id);}
	};


	Track(const TrackData &data);

	void computeChannels();

	Path path() const;

	GraphPair elevation() const;
	GraphPair speed() const;
	Graph vspeed() const;
	Graph heartRate() const;
	Graph temperature() const;
	Graph cadence() const;
	Graph power() const;
	Graph ratio() const;

	qreal distance() const;
	qreal time() const;
	qreal movingTime() const;
	QDateTime date() const;

	/* valid iff has a segment of length >= 2 */
	bool isValid() const;

	static void setElevationFilter(int window) {_elevationWindow = window;}
	static void setSpeedFilter(int window) {_speedWindow = window;}
	static void setHeartRateFilter(int window) {_heartRateWindow = window;}
	static void setCadenceFilter(int window) {_cadenceWindow = window;}
	static void setPowerFilter(int window) {_powerWindow = window;}
	static void setAutomaticPause(bool set) {_automaticPause = set;}
	static void setPauseSpeed(qreal speed) {_pauseSpeed = speed;}
	static void setPauseInterval(int interval) {_pauseInterval = interval;}
	static void setOutlierElimination(bool eliminate)
	  {_outlierEliminate = eliminate;}
	static void useReportedSpeed(bool use) {_useReportedSpeed = use;}
	static void useDEM(bool use) {_useDEM = use;}
	static void showSecondaryElevation(bool show)
	  {_show2ndElevation = show;}
	static void showSecondarySpeed(bool show)
	  {_show2ndSpeed = show;}
	
	static QString tr(const char *sourceText, const char *disambiguation = 0);

private:
	int newChannel(const ChannelDescr& ch);
	int findChannel(int ct, int cs = -1) const;
	Graph graphOfChannel(int cid, int filtrWindow = 1) const;
	Graph graphOfType(ChanTy ty, int filtrWindow = 1) const;


	QVector<ChannelDescr>  _chanDescr;
	QList<Segment>         _segments;

	int _chanDist; // distance since the beginning of the segment

	qreal timeLength, distLength;

	static bool  _outlierEliminate;
	static int   _elevationWindow;
	static int   _speedWindow;
	static int   _heartRateWindow;
	static int   _cadenceWindow;
	static int   _powerWindow;
	static bool  _automaticPause;
	static qreal _pauseSpeed;
	static int   _pauseInterval;
	static bool  _useReportedSpeed;
	static bool  _useDEM;
	static bool  _show2ndElevation;
	static bool  _show2ndSpeed;
};

#endif
