#ifndef TRACKITEM_H
#define TRACKITEM_H

#include <QDateTime>
#include <QPen>
#include "gdata.h"
#include "pathitem.h"
#include "units.h"
#include "graphicsscene.h"
#include "colorscales.h"

class Map;

class TrackItem : public PathItem
{
	Q_OBJECT

public:
	TrackItem(GTrack &track, Map *map, QGraphicsItem *parent = 0);
	~TrackItem();

	GTrack& track() const {return (GTrack&)*parent();}
	ColorScales* colorScales() const
		{return dynamic_cast<ColorScales*>(track().data().parent());}

	int displayedChannel();
	void setDispChannel(int chId);
	QString info() const;

	void paint(QPainter *painter, const QStyleOptionGraphicsItem *option,
				QWidget *widget);

private:
	QString _name;
	QString _desc;
	QString _comment;
	QVector<Link> _links;
	QDateTime _date;
	qreal _time;
	qreal _movingTime;

	int _dispChannel;

	ColorScales::entry _csEntry;
};

#endif // TRACKITEM_H
