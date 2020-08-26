#ifndef HEARTRATEGRAPH_H
#define HEARTRATEGRAPH_H

#include "graphtab.h"

class HeartRateGraphItem : public GraphItem2
{
	Q_OBJECT

public:
	HeartRateGraphItem(GraphSet* s, int c, GraphType t,
						const Style& y, GraphTab1* g)
		: GraphItem2(s, c, t, y, g) {}

protected:
	virtual void makeTooltip(ToolTip& tt) const;
};

class HeartRateGraph : public GraphTab2
{
	Q_OBJECT

public:
	HeartRateGraph(QWidget *parent = 0);

protected:
	void updateTracksInfos();
	GraphItem1* makeTrackItem(GraphSet* set, int chId,
									const GraphItem1::Style& st);
};

#endif // HEARTRATEGRAPH_H
