#ifndef _GTRACK_H
#define _GTRACK_H

#include <QObject>
#include <QList>
#include <QSet>
#include <QChildEvent>

#include "data/track.h"
#include "data/data.h"
#include "slider.h"

class TrackItem;
class GraphSet;
class GData;

class GTrack : public QObject, public Track
{
	Q_OBJECT

public:
	explicit GTrack(const Track& track, QObject* parent = NULL)
		: QObject(parent), Track(track), _item(NULL) {}

	void setItem(TrackItem* item);
	void addGraphSet(GraphSet* gset);
	
	int displayedChannel();
	void setDispChannel(int chId);

	GData& data() const {return (GData&)*parent();}

public slots:
	void moveSlider(const SliderPos& pos);
	void moveMarker(qreal distance);
	void setSelected(bool sel);
	void setPSelected(bool sel);

protected:
	void childEvent(QChildEvent* event);

private:
	TrackItem*      _item;
	QSet<GraphSet*> _gsets;
};

class GData : public QObject
{
	Q_OBJECT

public:
	explicit GData(const Data& data, QObject* parent = NULL);

	const Data& data() {return _data;}
	QList<GTrack*>          &tracks() {return _tracks;};
	const QList<Route>      &routes() const {return _data.routes();}
	const QVector<Waypoint> &waypoints() const {return _data.waypoints();}
	const QList<Area>       &areas() const {return _data.areas();}

private:
	Data           _data;
	QList<GTrack*> _tracks;
};

#endif
