#include "units.h"

#include <QCoreApplication>
#include <cmath>

QLocale Unit::local = QLocale();

Unit Unit::mPs   = Unit();
Unit Unit::kn    = Unit();
Unit Unit::miPh  = Unit();
Unit Unit::kmPh  = Unit();
Unit Unit::sPnmi = Unit();
Unit Unit::sPmi  = Unit();
Unit Unit::sPkm  = Unit();
Unit Unit::m     = Unit();
Unit Unit::km    = Unit();
Unit Unit::ft    = Unit();
Unit Unit::mi    = Unit();
Unit Unit::nmi   = Unit();
Unit Unit::s     = Unit();
Unit Unit::min   = Unit();
Unit Unit::h     = Unit();
Unit Unit::bpm   = Unit();
Unit Unit::cls   = Unit();
Unit Unit::fhr   = Unit();
Unit Unit::rpm   = Unit();
Unit Unit::w     = Unit();

void Unit::init()
{
	local = QLocale::system();
	mPs   = Unit(tr("m/s"));
	kn    = Unit(tr("kn"), MS2KN);
	miPh  = Unit(tr("mi/h"), MS2MIH);
	kmPh  = Unit(tr("km/h"), MS2KMH);

	sPnmi = Unit(tr("/nmi"), HINS / MS2KN);
	sPmi  = Unit(tr("/mi"),  HINS / MS2MIH);
	sPkm  = Unit(tr("/km"),  HINS / MS2KMH);

	m     = Unit(tr("m"));
	km    = Unit(tr("km"),  M2KM);
	ft    = Unit(tr("ft"),  M2FT);
	mi    = Unit(tr("mi"),  M2MI);
	nmi   = Unit(tr("nmi"), M2NMI);
	
	s     = Unit(tr("s"));
	min   = Unit(tr("min"), 1. / MININS);
	h     = Unit(tr("h"),   1. / HINS);
	
	bpm   = Unit(tr("bpm"));

	cls   = Unit(tr("deg C"));
	fhr   = Unit(tr("deg F"), C2FS, C2FO);

	rpm   = Unit(tr("rpm"));

	w     = Unit(tr("W"));
} 

QString Unit::suffix() const
{
	return UNIT_SPACE + name;
}

QString Unit::format(QString s) const
{
	return s + UNIT_SPACE + name;
}

QString Unit::format(qreal x, int prec, bool force_sign) const
{
	qreal    v = toUnit(x);
	QString sv;
	if (force_sign && !std::isnan(v))
		sv = (v >= 0 ? QString("+") : QString("-"))
				+ local.toString(std::abs(v), 'f', prec);
	else
		sv = local.toString(v, 'f', prec);

	return sv + UNIT_SPACE + name;
}

QString Unit::format(qreal x, const Fmt& fmt) const
{
	return format(x, fmt.prec, fmt.force_sign);
}

const Unit& Unit::distance(Units u, qreal span)
{
	switch(u) {
		case Imperial: return span < MIINM  ? ft : mi;
		case Nautical: return span < NMIINM ? ft : nmi;
		default:       return span < KMINM  ? m  : km;
	}
}

const Unit& Unit::time(qreal span)
{
	return span < MININS ? s : span < HINS ? min : h;
}
