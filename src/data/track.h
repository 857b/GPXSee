#ifndef TRACK_H
#define TRACK_H

#include <QObject>
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

QString chanTyName(ChanTy ty);

enum ChanSrc
{
	CSbase, // from file
	CSgpsDist,
	CSgpsVit,
	CSgpsAcc,
	CSdem,
	CSderiv
};

class TrackBuilder; // parser.h

class Track : public QObject, public TrackInfos {
	Q_OBJECT

	friend TrackBuilder;
public:

	class Channel : public QVector<qreal> 
	{
		public:
			int     _id;     // in Track::_chanDescr

			Channel() {}
			Channel(int id): _id(id) {}
			Channel(int id, int sz): QVector(sz), _id(id) {}

			// [a, b[
			qreal sum(int a, int b) const;
			qreal avg(int a, int b) const;
			Channel filter(int window) const;
			bool  hasData() const;
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

		QString name(const Track& t, bool full = false) const;

		// s must have an id-channel
		qreal sum(const Track& t, const Segment& s, int id,
					int a, int b) const;
		qreal sum(const Track& t, int id) const;
		qreal tsum(const Track& t, int id) const;
	};

	struct Segment
	{
		QVector<Coordinates> coord;
		// may be empty if !hasTime()
		QVector<QDateTime>   time;
		bool                 timePres;
		QVector<Channel>     chan;

		QSet<int>            outliers;
		QSet<int>            stop;

		// offset of the segment
		qreal                dist0;
		QDateTime            tms0;
		qreal                time0;

		qreal                pauseTime;


		Segment();


		void           computeStopPoints(const Channel& speed,
								qreal pauseInterval, qreal pauseSpeed);
		bool           discardStopPoint(int i) const;

		// return index
		int            addChannel(int chanId);
		// adding channels invalidates references
		Channel&       append(const Channel& ch);
		Channel&       append(const Channel& ch, int id);
		Channel&       channel(int id);
		const Channel& channel(int id) const;
		const Channel* findChannel(int id) const;
		
		int            firstValid() const;
		int            lastValid() const;
		
		bool           hasTime() const;
		// need hasTime()
		qreal          timeAt(int i) const;
		qreal          totalTime() const;
		qreal          movingTime() const;

		int size() const {return coord.size();}
		const Channel& operator[](int id) const {return channel(id);}
		Channel& operator[](int id) {return channel(id);}
	};
	
	struct ChannelPoint {
		int   seg_num;
		int   pt0, pt1;
		qreal t;

		ChannelPoint(int sg = -1, int p0 = 0, int p1 = 0)
			: seg_num(sg), pt0(p0), pt1(p1), t(0) {}
		operator bool() const {return ~seg_num;}
		qreal interpol(qreal v0, qreal v1) const {
			return (1 - t) * v0 + t * v1;
		}
		QDateTime interpol(const QDateTime& v0, const QDateTime& v1) const {
			return v0.addMSecs(t * v0.msecsTo(v1));
		}
		template<typename T>
		T interpol(const QVector<T>& ch) const {
			return interpol(ch.at(pt0), ch.at(pt1));
		}
	};


	Track(QObject* parent, const TrackData &data);

	Path path() const;

	const QList<Segment>& segments()  const {return _segments;}
	QVector<ChannelDescr> chanDescr() const {return _chanDescr;}
	int          distChan() const {return _chanDist;}

	QList<int>   findChannels(ChanTy ct)  const;

	qreal        distance()   const;
	qreal        time()       const;
	qreal        movingTime() const;
	QDateTime    date()   const;

	ChannelPoint pointAtTime(qreal t)     const;
	ChannelPoint pointAtDistance(qreal d) const;
	qreal        distanceAtTime(qreal t)  const;

	/* valid iff has a segment of length >= 2 */
	bool isValid() const;
	bool hasTime() const;
	bool hasData(int chanId) const;

	static int  filterWindow(ChanTy ct);
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

private:
	Track(QObject* parent);

	int newChannel(const ChannelDescr& ch);
	int findChannel(int ct, int cs = -1) const;

	void finalize();
	void computeVSpeed();
	void computeDEMelevation();


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
