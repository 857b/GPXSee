#ifndef CADENCEGRAPH_H
#define CADENCEGRAPH_H

#include "graphtab.h"

class CadenceGraphItem: public GraphItem2
{
	Q_OBJECT

public:
	CadenceGraphItem(GraphSet* s, int c, GraphType t,
						const Style& y, GraphTab1* g)
		: GraphItem2(s, c, t, y, g) {}

protected:
	virtual void makeTooltip(ToolTip& tt) const;
};

class CadenceGraph : public GraphTab2
{
	Q_OBJECT

public:
	CadenceGraph(QWidget *parent = 0);

protected:
	void updateTracksInfos();
	GraphItem1* makeTrackItem(GraphSet* set, int chId,
									const GraphItem1::Style& st);
};

#endif // CADENCEGRAPH_H
