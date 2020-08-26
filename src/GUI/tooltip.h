#ifndef TOOLTIP_H
#define TOOLTIP_H

#include <QString>
#include <QList>
#include <QVector>
#include "common/kv.h"
#include "data/imageinfo.h"
#include "units.h"

class ToolTip
{
public:
	void setTitle(const QString& title);
	void insert(const QString &key, const QString &value);
	void insert(const QString &key, qreal val, const Unit& u,
					const Unit::Fmt& fmt = Unit::Fmt());
	void setImages(const QVector<ImageInfo> &images) {_images = images;}
	QString toString() const;

private:
	QString _title;
	QList<KV<QString, QString> > _list;
	QVector<ImageInfo> _images;
};

#endif // TOOLTIP_H
