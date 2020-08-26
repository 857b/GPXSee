#ifndef POWERGRAPH_H
#define POWERGRAPH_H

#include "graphtab.h"

class PowerGraphItem : public GraphItem2
{
	Q_OBJECT

public:
	PowerGraphItem(GraphSet* s, int c, GraphType t,
						const Style& y, GraphTab1* g)
		: GraphItem2(s, c, t, y, g) {}

protected:
	virtual void makeTooltip(ToolTip& tt) const;
};

class PowerGraph : public GraphTab2
{
	Q_OBJECT

public:
	PowerGraph(QWidget *parent = 0);

protected:
	void updateTracksInfos();
	GraphItem1* makeTrackItem(GraphSet* set, int chId,
									const GraphItem1::Style& st);
};

#endif // POWERGRAPH_H
