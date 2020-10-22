#include <QPainter>
#include <QBrush>
#include <QLinearGradient>
#include <QColor>

#include "map/map.h"
#include "format.h"
#include "tooltip.h"
#include "trackitem.h"
#include "data/compute.h"


TrackItem::TrackItem(GTrack &track, Map *map, QGraphicsItem *parent)
  : PathItem(track.path(), map, &track, parent),
	_dispChannel(~0)
{
	_name = track.name();
	_desc = track.description();
	_comment = track.comment();
	_links = track.links();
	_date = track.date();
	_time = track.time();
	_movingTime = track.movingTime();

	_pen.setCapStyle(Qt::RoundCap);
	_dispChannel = ~0;
}

TrackItem::~TrackItem()
{
	ColorScales* cs = colorScales();
	if (cs) cs->removeEntry(_csEntry);
}

int TrackItem::displayedChannel()
{
	return _dispChannel;
}

void TrackItem::setDispChannel(int chId)
{
	ColorScales* cs = colorScales();
	if (cs) cs->removeEntry(_csEntry);
	_dispChannel   = chId;
	if (~chId) {
		_csEntry.ty    = track().chanDescr().at(chId)._ty;
		_csEntry.range = Compute::bounds(track(), chId);
		if (cs) cs->addEntry(_csEntry);
	} else
		_csEntry = ColorScales::entry::None();
	update();
}

QString TrackItem::info() const
{
	ToolTip tt;

	if (!_name.isEmpty())
		tt.insert(tr("Name"), _name);
	if (!_desc.isEmpty())
		tt.insert(tr("Description"), _desc);
	if (!_comment.isEmpty() && _comment != _desc)
		tt.insert(tr("Comment"), _comment);
	tt.insert(tr("Distance"), Format::distance(path().last().last().distance(),
	  _units));
	if  (_time > 0)
		tt.insert(tr("Total time"), Format::timeSpan(_time));
	if  (_movingTime > 0)
		tt.insert(tr("Moving time"), Format::timeSpan(_movingTime));
	if (!_date.isNull())
		tt.insert(tr("Date"),
#ifdef ENABLE_TIMEZONES
		  _date.toTimeZone(_timeZone)
#else // ENABLE_TIMEZONES
		  _date
#endif // ENABLE_TIMEZONES
		  .toString(Qt::SystemLocaleShortDate));
	if (!_links.isEmpty()) {
		QString links;
		for (int i = 0; i < _links.size(); i++) {
			const Link &link = _links.at(i);
			links.append(QString("<a href=\"%0\">%1</a>").arg(link.URL(),
			  link.text().isEmpty() ? link.URL() : link.text()));
			if (i != _links.size() - 1)
				links.append("<br/>");
		}
		tt.insert(tr("Links"), links);
	}

	return tt.toString();
}

struct paint_nchan_d {
	QPainter& painter;

	QPointF prev;

	paint_nchan_d(QPainter& painter) : painter(painter) {}

	void move(const QPointF& p, int n1, int n2, qreal t) {
		Q_UNUSED(n1) Q_UNUSED(n2) Q_UNUSED(t)
		prev = p;
	}
	
	void line(const QPointF& p, int n1, int n2, qreal t) {
		Q_UNUSED(n1) Q_UNUSED(n2) Q_UNUSED(t)
		painter.drawLine(prev, p);
		prev = p;
	}
};
struct paint_chan_d : public paint_nchan_d {
	ColorScales::scale    cs;
	const Track::Channel& ch;
	QColor base;
	QPen   pen;
	QLinearGradient color;
	qreal  prev_v;

	paint_chan_d(ColorScales::scale cs, const Track::Channel& ch,
			const QPen& pen0, QPainter& painter)
		: paint_nchan_d(painter), cs(cs), ch(ch), base(pen0.color()),
		  pen(pen0) {
		color.setSpread(QGradient::PadSpread);
	}

	qreal valAt(int n1, int n2, qreal t) {
		return (1 - t) * ch.at(n1) + t * ch.at(n2);
	}

	void move(const QPointF& p, int n1, int n2, qreal t) {
		prev   = p;
		prev_v = valAt(n1, n2, t);
	}
	
	void line(const QPointF& p, int n1, int n2, qreal t) {
		qreal v = valAt(n1, n2, t);

		color.setStart(prev);
		color.setFinalStop(p);
		cs.gradient(color, base, prev_v, v);
		pen.setBrush(QBrush(color));
		painter.setPen(pen);
		painter.drawLine(prev, p);
		
		prev   = p;
		prev_v = v;
	}
};
void TrackItem::paint(QPainter *painter,
		const QStyleOptionGraphicsItem *option, QWidget *widget)
{
	if (!~_dispChannel) {
		PathItem::paint(painter, option, widget);
		return;
	}

	ColorScales::scale cs = _csEntry.color(colorScales());

	for (int i = 0; i < _path.size(); i++) {
		const PathSegment &segment = _path.at(i);
		const Track::Channel* ch
			= track().segments().at(i).findChannel(_dispChannel);

		if (ch) {
			//TODO: filter ?
			paint_chan_d d(cs, *ch, _pen, *painter);
			drawSegment(d, *_map, segment);
		} else {
			painter->setPen(_pen);
			paint_nchan_d d(*painter);
			drawSegment(d, *_map, segment);
		}
	}
}
