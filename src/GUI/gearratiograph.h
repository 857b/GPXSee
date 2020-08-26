#ifndef GEARRATIOGRAPH_H
#define GEARRATIOGRAPH_H

#include <QMap>
#include "graphtab.h"

class GearRatioGraphItem : public GraphItem1
{
	Q_OBJECT

public:
	GearRatioGraphItem(GraphSet* s, int c, GraphType t,
						const Style& y, GraphTab1* g);

	qreal top() const {return _top;}
	const QMap<qreal, qreal>& usageDist() const {return _usageDist;}

protected:
	virtual void makeTooltip(ToolTip& tt) const;

private:
	QMap<qreal, qreal> _usageDist; // ratio -> usage distance
	qreal              _top;       // most used ratio (by distance)
};

class GearRatioGraph : public GraphTab1
{
	Q_OBJECT

public:
	GearRatioGraph(QWidget *parent = 0);

protected:
	void updateTracksInfos();
	GraphItem1* makeTrackItem(GraphSet* set, int chId,
									const GraphItem1::Style& st);

private:
	qreal top() const;
};

#endif // GEARRATIOGRAPH_H
