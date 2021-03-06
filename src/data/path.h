#ifndef PATH_H
#define PATH_H

#include <QVector>
#include <QRectF>
#include "common/coordinates.h"
#include "common/rectc.h"

class PathPoint
{
public:
	PathPoint() :
	  _num(~0), _coordinates(Coordinates()), _distance(NAN) {}
	PathPoint(int num, const Coordinates &coordinates, qreal distance)
	  : _num(num), _coordinates(coordinates), _distance(distance) {}

	int num() const {return _num;}
	const Coordinates &coordinates() const {return _coordinates;}
	qreal distance() const {return _distance;}

private:
	int         _num;
	Coordinates _coordinates;
	qreal       _distance;
};

Q_DECLARE_TYPEINFO(PathPoint, Q_PRIMITIVE_TYPE);
#ifndef QT_NO_DEBUG
QDebug operator<<(QDebug dbg, const PathPoint &point);
#endif // QT_NO_DEBUG

typedef QVector<PathPoint> PathSegment;

class Path : public QList<PathSegment>
{
public:
	bool isValid() const;
	RectC boundingRect() const;
};

#endif // PATH_H
