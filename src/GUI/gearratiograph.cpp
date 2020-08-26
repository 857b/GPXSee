#include "gearratiograph.h"

static const Unit::Fmt fmt = Unit::Fmt(2);

GearRatioGraph::GearRatioGraph(QWidget *parent)
	: GraphTab1(CTratio, parent)
{
	setYUnit(Unit());
	setSliderFmt(fmt);
}

void GearRatioGraph::updateTracksInfos()
{
	const Unit& u(yUnit());
	addInfo(tr("Most used"), top(), u, fmt);
	addInfo(tr("Minimum"),   min(), u, fmt);
	addInfo(tr("Maximum"),   max(), u, fmt);
}

GraphItem1* GearRatioGraph::makeTrackItem(GraphSet* set, int chId,
								const GraphItem1::Style& st)
{
	return new GearRatioGraphItem(set, chId, graphType(), st, this);
}

static qreal topOfMap(const QMap<qreal, qreal>& m)
{
	qreal key = NAN, val = NAN;
	for (QMap<qreal, qreal>::const_iterator it = m.constBegin();
	  it != m.constEnd(); ++it) {
		if (std::isnan(val) || it.value() > val) {
			val = it.value();
			key = it.key();
		}
	}

	return key;
}

struct usage_f {
	QMap<qreal, qreal> u;
	void operator() (const GraphItem1& it1) {
		const QMap<qreal, qreal>& s(
			static_cast<const GearRatioGraphItem&>(it1).usageDist());
		for (QMap<qreal, qreal>::const_iterator it = s.constBegin();
				it != s.constEnd(); ++it)
			u.insert(it.key(), u.value(it.key(), 0.) + it.value());
	}
};


qreal GearRatioGraph::top() const
{
	usage_f f;
	iterTrackItems(f);
	return topOfMap(f.u);
}
	
GearRatioGraphItem::GearRatioGraphItem(GraphSet* s, int c, GraphType t,
						const Style& y, GraphTab1* g)
		: GraphItem1(s, c, t, y, g)
{
	const QList<Track::Segment>& sgs(track().segments());
	int distId = track().distChan();
	for (int i_s = 0; i_s < sgs.size(); ++i_s) {
		const Track::Segment& sg(sgs.at(i_s));
		const Track::Channel* ch(sg.findChannel(chanId()));
		if (!ch) continue;
		const Track::Channel& dst(sg[distId]);

		for (int i = 1; i < ch->size(); i++) {
			qreal  r = ch->at(i - 1);
			qreal ds = dst.at(i) - dst.at(i - 1);
			_usageDist.insert(r, _usageDist.value(r, 0.) + ds);
		}
	}

	_top = topOfMap(_usageDist);
}

void GearRatioGraphItem::makeTooltip(ToolTip& tt) const
{
	const Unit&  u = graph().yUnit();
	tt.insert(tr("Minimum"),   min(), u, fmt);
	tt.insert(tr("Maximum"),   max(), u, fmt);
	tt.insert(tr("Most used"), top(), u, fmt);
}
