#include <cstring>
#include "common/util.h"
#include "igcparser.h"


static bool readLat(const char *data, qreal &lat)
{
	int d = str2int(data, 2);
	int mi = str2int(data + 2, 2);
	int mf = str2int(data + 4, 3);
	if (d < 0 || mi < 0 || mf < 0)
		return false;

	if (!(data[7] == 'N' || data[7] == 'S'))
		return false;

	lat = d + (((qreal)mi + (qreal)mf/1000) / 60);
	if (lat > 90)
		return false;

	if (data[7] == 'S')
		lat = -lat;

	return true;
}

static bool readLon(const char *data, qreal &lon)
{
	int d = str2int(data, 3);
	int mi = str2int(data + 3, 2);
	int mf = str2int(data + 5, 3);
	if (d < 0 || mi < 0 || mf < 0)
		return false;

	if (!(data[8] == 'E' || data[8] == 'W'))
		return false;

	lon = d + (((qreal)mi + (qreal)mf/1000) / 60);
	if (lon > 180)
		return false;

	if (data[8] == 'W')
		lon = -lon;

	return true;
}

static inline bool readAltitude1(const char *data, int& a)
{
	if (data[0] == '-') {
		if ((a = str2int(data + 1, 4)) < 0)
			return false;
		a = -a;
	} else {
		if ((a = str2int(data, 5)) < 0)
			return false;
	}
	return true;
}

static bool readAltitude(const char *data,
				qreal& prs_alt, qreal& gps_alt)
{
	int pa, ga;

	if (!(data[0] == 'A' || data[0] == 'V'))
		return false;
	if (!readAltitude1(data + 1, pa))
		return false;
	if (!readAltitude1(data + 6, ga))
		return false;

	prs_alt = pa;
	gps_alt = data[0] == 'A' ? (qreal)ga : NAN;

	return true;
}

static bool readTimestamp(const IGCParser::CTX& ctx,
						  const char *data, QTime &time)
{
	int h = str2int(data, 2) - ctx.tz_offset;
	int m = str2int(data + 2, 2);
	int s = str2int(data + 4, 2);

	if (h < 0 || m < 0 || s < 0)
		return false;

	time = QTime(h, m, s);
	if (!time.isValid())
		return false;

	return true;
}

static bool readARecord(const char *line, qint64 len)
{
	/* The minimal A record length should be 7 according to the specification,
	   but records with length of 6 exist in the wild */
	if (len < 6 || line[0] != 'A')
		return false;

	for (int i = 1; i < 6; i++)
		if (!::isprint(line[i]))
			return false;
	return true;
}

bool IGCParser::readHRecord(CTX &ctx, const char *line, int len)
{
	if (!::strncmp(line, "HFTZNTIMEZONE", 13)) {
		if (len < 16) {
			_errorString = "Missing timezone";
			return false;
		}
		if (!str2sint(line + 14, len - 16, ctx.tz_offset)) {
			_errorString = "Invalid timezone";
			return false;
		}

	} else if (!::strncmp(line, "HFDTE", 5)) {
		if (len < 11) return true;

		int offset = len < 16 || ::strncmp(line + 5, "DATE:", 5)
			       ? 5 : 10;

		int d = str2int(line + offset, 2);
		int m = str2int(line + offset + 2, 2);
		int y = str2int(line + offset + 4, 2);

		if (y < 0 || m < 0 || d < 0) {
			_errorString = "Invalid date header format";
			return false;
		}

		ctx.date = QDate(y + 2000 <= QDate::currentDate().year()
		  ? 2000 + y : 1900 + y, m, d);
		if (!ctx.date.isValid()) {
			_errorString = "Invalid date";
			return false;
		}
	}
	return true;
}

bool IGCParser::readBRecord(CTX &ctx, const char *line, int len)
{
	qreal lat, lon, prs_alt, gps_alt;
	QTime time;

	if (len < 35)
		return false;
	if (line[24] != 'A')
		return true;

	if (!readTimestamp(ctx, line + 1, time)) {
		_errorString = "Invalid timestamp";
		return false;
	}

	if (!readLat(line + 7, lat)) {
		_errorString = "Invalid latitude";
		return false;
	}
	if (!readLon(line + 15, lon)) {
		_errorString = "Invalid longitude";
		return false;
	}

	if (!readAltitude(line + 24, prs_alt, gps_alt)) {
		_errorString = "Invalid altitude";
		return false;
	}

	if (time < ctx.time && !ctx.sg->time.isEmpty()
			&& ctx.date == ctx.sg->time.last().date())
		ctx.date = ctx.date.addDays(1);
	ctx.time = time;

	ctx.sg->coord.append(Coordinates(lon, lat));
	ctx.sg->time.append(QDateTime(ctx.date, ctx.time, Qt::UTC));
	ctx.prsAlt->append(prs_alt);
	ctx.gpsAlt->append(gps_alt);

	return true;
}

bool IGCParser::readCRecord(const char *line, int len, RouteData &route)
{
	qreal lat, lon;

	if (len < 18)
		return false;

	if (!readLat(line + 1, lat)) {
		_errorString = "Invalid latitude";
		return false;
	}
	if (!readLon(line + 9, lon)) {
		_errorString = "Invalid longitude";
		return false;
	}

	if (!(lat == 0 && lon == 0)) {
		QByteArray ba(line + 18, len - 19);

		Waypoint w(Coordinates(lon, lat));
		w.setName(QString(ba.trimmed()));
		route.append(w);
	}

	return true;
}

IGCParser::CTX::CTX()
	: tz_offset(0) {}

static const unsigned max_line_len = 76;

bool IGCParser::parse(QFile *file,
		QList<Track>      &tracks,
		QList<Route>      &routes,
		QList<Area>       &polygons,
		QVector<Waypoint> &waypoints)
{
	Q_UNUSED(waypoints);
	Q_UNUSED(polygons);

	qint64 len;
	// limit len + CRLF + \0 + 1 to check limit
	char line[max_line_len + 2 + 1 + 1];
	bool route = false, track = false;
	RouteData rdt;
	CTX ctx;
	TrackBuilder bld;


	_errorLine = 1;
	_errorString.clear();

	while (!file->atEnd()) {
		len = file->readLine(line, sizeof(line));

		if (len < 0) {
			_errorString = "I/O error";
			return false;
		} else if (len >= (qint64)sizeof(line) - 1) {
			_errorString = "Line limit exceeded";
			return false;
		}

		if (_errorLine == 1) {
			if (!readARecord(line, len)) {
				_errorString = "Invalid/missing A record";
				return false;
			}
		} else {
			if (line[0] == 'H') {
				if (!readHRecord(ctx, line, len))
					return false;
			} else if (line[0] == 'C') {
				route = true;
				if (!readCRecord(line, len, rdt))
					return false;
			} else if (line[0] == 'B') {
				if (ctx.date.isNull()) {
					_errorString = "Missing date header";
					return false;
				}
				if (!track) {
					int gpsAltId = bld.newChannel(Track::ChannelDescr(
							CTelevation, CSbase, tr("GPS"))),
					    prsAltId = bld.newChannel(Track::ChannelDescr(
							CTelevation, CSbase, tr("pressure")));

					ctx.sg = &bld.beginSegment(true);
					int gpsAltIdx = ctx.sg->addChannel(gpsAltId),
					    prsAltIdx = ctx.sg->addChannel(prsAltId);
					ctx.prsAlt = &ctx.sg->chan[prsAltIdx];
					ctx.gpsAlt = &ctx.sg->chan[gpsAltIdx];

					ctx.time = QTime(0, 0);

					track = true;
				}
				if (!readBRecord(ctx, line, len))
					return false;
			}
		}

		_errorLine++;
	}

	if (track) {
		bld.infos().setName(*file);
		tracks.append(bld.finalize());
	}
	if (route)
		routes.append(Route(rdt));

	return true;
}
