#ifndef PATHITEM_H
#define PATHITEM_H

#include "common/config.h"
#include <QGraphicsObject>
#include <QPen>
#ifdef ENABLE_TIMEZONES
#include <QTimeZone>
#endif // ENABLE_TIMEZONES
#include "data/path.h"
#include "markeritem.h"
#include "units.h"
#include "graphicsscene.h"

class Map;
class PathTickItem;

class PathItem : public QObject, public GraphicsItem
{
	Q_OBJECT

public:
	PathItem(const Path &path, Map *map,
			QObject* oparent = 0,
			QGraphicsItem *gparent = 0);
	virtual ~PathItem() {}

	QPainterPath shape() const {return _shape;}
	QRectF boundingRect() const {return _shape.boundingRect();}
	void paint(QPainter *painter, const QStyleOptionGraphicsItem *option,
	  QWidget *widget);

	const Path &path() const {return _path;}

	void setMap(Map *map);

	void setColor(const QColor &color);
	void setWidth(qreal width);
	void setStyle(Qt::PenStyle style);
	void setDigitalZoom(int zoom);
	void setMarkerColor(const QColor &color);
	void showMarker(bool show);
	void showTicks(bool show);

	void updateTicks();

	static void setUnits(Units units) {_units = units;}
#ifdef ENABLE_TIMEZONES
	static void setTimeZone(const QTimeZone &zone) {_timeZone = zone;}
#endif // ENABLE_TIMEZONES

	static const qreal GEOGRAPHICAL_MILE;

	static inline unsigned segments(qreal distance);
	template <typename D>
	static inline void drawSegment1(D& d, Map& map,
			const Coordinates &c1, const Coordinates &c2,
			int n1, int n2, qreal t1, qreal t2);
	template <typename D>
	static inline void drawSegment(D& d, Map& map,
			const PathSegment &segment);

public slots:
	void moveMarker(qreal distance);
	void hover(bool hover);

signals:
	void selected(bool);

protected:
	void hoverEnterEvent(QGraphicsSceneHoverEvent *event);
	void hoverLeaveEvent(QGraphicsSceneHoverEvent *event);
	void mousePressEvent(QGraphicsSceneMouseEvent *event);

	static Units _units;
#ifdef ENABLE_TIMEZONES
	static QTimeZone _timeZone;
#endif // ENABLE_TIMEZONES

	Path _path;
	Map *_map;
	qreal _width;
	QPen _pen;
private:
	const PathSegment *segment(qreal x) const;
	QPointF position(qreal distance) const;
	void updatePainterPath();
	void updateShape();

	qreal xInM() const;
	unsigned tickSize() const;

	qreal _markerDistance;
	int _digitalZoom;

	QPainterPath _shape;
	QPainterPath _painterPath;
	bool _showMarker;
	bool _showTicks;

	MarkerItem *_marker;
	QVector<PathTickItem*> _ticks;
};

// ---
#include "common/greatcircle.h"
#include "map/map.h"

inline unsigned PathItem::segments(qreal distance)
{
	return ceil(distance / GEOGRAPHICAL_MILE);
}

template <typename D>
inline void PathItem::drawSegment1(D& d, Map& map,
		const Coordinates &c1, const Coordinates &c2,
		int n1, int n2, qreal t1, qreal t2)
{
	if (fabs(c1.lon() - c2.lon()) > 180.0) {
		// Split segment on date line crossing
		QPointF p;

		if (c2.lon() < 0) {
			QLineF l(QPointF(c1.lon(), c1.lat()), QPointF(c2.lon() + 360,
			  c2.lat()));
			QLineF dl(QPointF(180, -90), QPointF(180, 90));
			l.intersect(dl, &p);
			qreal ti = t1 + (180 - c1.lon())
				            * (t2 - t1) / (c2.lon() + 360 - c1.lon());
			d.line(map.ll2xy(Coordinates(+180, p.y())), n1, n2, ti);
			d.move(map.ll2xy(Coordinates(-180, p.y())), n1, n2, ti);

		} else {
			QLineF l(QPointF(c1.lon(), c1.lat()), QPointF(c2.lon() - 360,
			  c2.lat()));
			QLineF dl(QPointF(-180, -90), QPointF(-180, 90));
			l.intersect(dl, &p);
			qreal ti = t1 + (-180 - c1.lon())
				            * (t2 - t1) / (c2.lon() - 360 - c1.lon());
			d.line(map.ll2xy(Coordinates(-180, p.y())), n1, n2, ti);
			d.move(map.ll2xy(Coordinates(+180, p.y())), n1, n2, ti);
 		}
 	}

	d.line(map.ll2xy(c2), n1, n2, t2);
}

template <typename D>
inline void PathItem::drawSegment(D& d, Map& map, const PathSegment &segment)
{
	if (segment.isEmpty()) return;

	d.move(map.ll2xy(segment.first().coordinates()),
			segment.first().num(), segment.first().num(), 0.);

	for (int j = 1; j < segment.size(); j++) {
		const PathPoint &p1 = segment.at(j-1);
		const PathPoint &p2 = segment.at(j);
		unsigned n = segments(p2.distance() - p1.distance());

		if (n > 1) {
			GreatCircle gc(p1.coordinates(), p2.coordinates());
			qreal last_t = 0.;
			Coordinates last = p1.coordinates();

			for (unsigned k = 1; k <= n; k++) {
				qreal t = k / (double)n;
				Coordinates c(gc.pointAt(t));
				drawSegment1(d, map, last, c,
						p1.num(), p2.num(), last_t, t);
				last_t = t;
				last   = c;
			}
		} else
			drawSegment1(d, map, p1.coordinates(), p2.coordinates(),
					p1.num(), p2.num(), 0., 1.);
	}
}

#endif // PATHITEM_H
