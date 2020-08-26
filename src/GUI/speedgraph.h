#ifndef SPEEDGRAPH_H
#define SPEEDGRAPH_H

#include "graphtab.h"

class SpeedGraphItem: public GraphItem2
{
	Q_OBJECT

public:
	SpeedGraphItem(GraphSet* s, int c, GraphType t,
						const Style& y, GraphTab1* g)
		: GraphItem2(s, c, t, y, g) {}

protected:
	virtual void makeTooltip(ToolTip& tt) const;
};

class SpeedGraph : public GraphTab2
{
	Q_OBJECT

public:
	SpeedGraph(QWidget *parent = 0);

	void setUnits(Units units);
	const Unit& paceUnit() const {return _paceUnit;};

protected:
	void updateTracksInfos();
	GraphItem1* makeTrackItem(GraphSet* set, int chId,
									const GraphItem1::Style& st);

private:
	void setYUnits(Units units);

	Unit _paceUnit;
};

#endif // SPEEDGRAPH_H
