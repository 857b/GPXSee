#include "gdata.h"

#include "trackitem.h"
#include "graphtab.h"

void GTrack::childEvent(QChildEvent* event)
{
	if (event->removed()) {
		QObject* c = event->child();
		if (_item == (TrackItem*)c) _item = NULL;
		_gsets.remove((GraphSet*)c);
	}
	QObject::childEvent(event);
}

void GTrack::setItem(TrackItem* item)
{
	if ((_item = item))
		connect(_item, SIGNAL(selected(bool)),
				this,  SLOT(setSelected(bool)));
}

void GTrack::addGraphSet(GraphSet* gset)
{
	_gsets.insert(gset);
	// TODO: signal from gset ?
	const QObjectList& gs = gset->children();
	for (int i = 0; i < gs.size(); ++i) {
		connect(gs[i], SIGNAL(selected(bool)),
				this,  SLOT(setPSelected(bool)));
		connect(gs[i], SIGNAL(sliderPositionChanged(qreal)),
				this,  SLOT(moveMarker(qreal)));
	}
}

int GTrack::displayedChannel()
{
	return _item ? _item->displayedChannel() : ~0;
}

void GTrack::setDispChannel(int chId)
{
	if (_item) _item->setDispChannel(chId);
}

void GTrack::moveSlider(const SliderPos& pos)
{
	qreal dist;
	switch (pos.type) {
		case Distance:
			dist = pos.value;
			break;
		case Time:
			if (hasTime()) {
				dist = distanceAtTime(pos.value);
				break;
			}
			// FALLTHRU
		default:
			return;
	}
	moveMarker(dist);
}

void GTrack::moveMarker(qreal distance)
{
	if (_item) _item->moveMarker(distance);
}

void GTrack::setSelected(bool sel)
{
	if (_item) _item->hover(sel);
	for (QSet<GraphSet*>::iterator it = _gsets.begin();
			it != _gsets.end(); ++it) {
		const QObjectList& gs = (**it).children();
		for (int i = 0; i < gs.size(); ++i)
			static_cast<GraphItem&>(*gs[i]).hover(sel);
	}
}

void GTrack::setPSelected(bool sel)
{
	if (_item) _item->hover(sel);
}

GData::GData(const Data& data, QObject* parent)
	: QObject(parent), _data(data)
{
	_tracks.reserve(_data.tracks().count());
	for (int i = 0; i < _data.tracks().count(); ++i)
		_tracks.append(new GTrack(_data.tracks().at(i), this));
}
