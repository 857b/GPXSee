#ifndef VSPEEDGRAPH_H
#define VSPEEDGRAPH_H

#include <QList>
#include "graphtab.h"

class VSpeedGraphItem: public GraphItem2
{
	Q_OBJECT

public:
	VSpeedGraphItem(GraphSet* s, int c, GraphType t,
						const Style& y, GraphTab1* g)
		: GraphItem2(s, c, t, y, g) {}

protected:
	virtual void makeTooltip(ToolTip& tt) const;
};

class VSpeedGraph : public GraphTab2
{
	Q_OBJECT

public:
	VSpeedGraph(QWidget *parent = 0);

protected:
	void updateTracksInfos();
	GraphItem1* makeTrackItem(GraphSet* set, int chId,
									const GraphItem1::Style& st);
};

#endif // VSPEEDGRAPH_H
