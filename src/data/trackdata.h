#ifndef TRACKDATA_H
#define TRACKDATA_H

#include <QList>
#include <QVector>
#include <QString>
#include <QFile>
#include <QFileInfo>
#include "trackpoint.h"
#include "link.h"

typedef QVector<Trackpoint> SegmentData;

class TrackInfos
{
public:
	const QString &name() const {return _name;}
	const QString &description() const {return _desc;}
	const QString &comment() const {return _comment;}
	const QVector<Link> &links() const {return _links;}

	void setName(const QString &name) {_name = name;}
	void setName(const QFile &file)
		{_name = QFileInfo(file.fileName()).fileName();}
	void setDescription(const QString &desc) {_desc = desc;}
	void setComment(const QString &comment) {_comment = comment;}
	void addLink(const Link &link) {_links.append(link);}

private:
	QString _name;
	QString _desc;
	QString _comment;
	QVector<Link> _links;
};

class TrackData : public QList<SegmentData>, public TrackInfos {};

#endif // TRACKDATA_H
