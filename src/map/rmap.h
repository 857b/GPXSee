#ifndef RMAP_H
#define RMAP_H

#include <QFile>
#include "map.h"
#include "transform.h"
#include "projection.h"

class RMap : public Map
{
	Q_OBJECT

public:
	RMap(const QString &fileName, QObject *parent = 0);

	QString name() const;

	QRectF bounds();

	int zoom() const {return _zoom;}
	void setZoom(int zoom) {_zoom = zoom;}
	int zoomFit(const QSize &size, const RectC &rect);
	int zoomIn();
	int zoomOut();

	QPointF ll2xy(const Coordinates &c);
	Coordinates xy2ll(const QPointF &p);

	void load();
	void unload();

	void draw(QPainter *painter, const QRectF &rect, Flags flags);

	bool isValid() const {return _valid;}
	QString errorString() const {return _errorString;}

private:
	struct Zoom {
		QSize size;
		QSize dim;
		QPointF scale;
		QVector<quint64> tiles;
	};

	bool ok(const QDataStream &stream);
	bool seek(QFile &file, quint64 offset);
	bool parseIMP(const QByteArray &data);
	QPixmap tile(int x, int y);

	QList<Zoom> _zooms;
	Projection _projection;
	Transform _transform;
	QSize _tileSize;
	QFile _file;

	QString _fileName;
	int _zoom;

	bool _valid;
	QString _errorString;
};

#endif // RMAP_H
