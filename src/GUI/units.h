#ifndef UNITS_H
#define UNITS_H

#include <QtGlobal>
#include <QCoreApplication>
#include <QLocale>

enum Units
{
	Metric,
	Imperial,
	Nautical

};

#define M2KM    0.001000000000  // m -> km
#define M2MI    0.000621371192  // m -> mi
#define M2NMI   0.000539956803  // m -> nmi
#define M2FT    3.280839900000  // m -> ft
#define MS2KMH  3.600000000000  // m/s -> km/h
#define MS2MIH  2.236936290000  // m/s -> mi/h
#define MS2KN   1.943844490000  // m/s -> kn
#define FT2MI   0.000189393939  // ft -> mi
#define MM2IN   0.039370100000  // mm -> in
#define MM2CM   0.100000000000  // mm -> cm
#define H2S     0.000277777778  // h -> s
#define MIN2S   0.016666666667  // min -> s

#define KMINM   1000.0      // 1 km in m
#define MIINFT  5280.0      // 1 mi in ft
#define NMIINFT 6076.11549  // 1 nm in ft
#define MIINM   1609.344    // 1 mi in m
#define NMIINM  1852.0      // 1 nmi in m
#define MININS  60.0        // 1 min in s
#define HINS    3600.0      // 1 hins

#define C2FS    1.8  // Celsius to Farenheit - scale
#define C2FO    32.0 // Celsius to Farenheit - offset

#define UNIT_SPACE     QString::fromUtf8("\xE2\x80\x89")

template<typename T>
inline T toUnit(const T& x, qreal scale, qreal ofs = 0.) {
	return ofs + scale * x;
}

template<typename T>
inline T fromUnit(const T& x, qreal scale, qreal ofs = 0.) {
	return (-ofs + x) / scale;
}

class Unit
{
	Q_DECLARE_TR_FUNCTIONS(Unit)

public:
	QString name;
	qreal   scale, offset;

	Unit() : scale(1.), offset(0.) {}

	Unit(const QString& name, qreal scale = 1., qreal offset = 0.)
		: name(name), scale(scale), offset(offset) {}

	template<typename T>
	T toUnit(const T& x) const {
		return ::toUnit(x, scale, offset);	
	}

	template<typename T>
	T fromUnit(const T& x) const {
		return ::fromUnit(x, scale, offset);	
	}

	QString suffix() const;

	struct Fmt {
		int  prec;
		bool force_sign;
		Fmt(int prec = 2, bool force_sign = false)
			: prec(prec), force_sign(force_sign) {}
	};
	QString format(QString s) const;
	QString format(qreal x, int prec = 2, bool force_sign = false) const;
	QString format(qreal x, const Fmt& fmt) const;

	// speed: m/s
	static Unit mPs;
	static Unit kn;
	static Unit miPh;
	static Unit kmPh;

	// pace: s/m
	static Unit sPnmi;
	static Unit sPmi;
	static Unit sPkm;

	// distance: m
	static Unit m;
	static Unit km;
	static Unit ft;
	static Unit mi;
	static Unit nmi;

	// time: s
	static Unit s;
	static Unit min;
	static Unit h;

	// heart rate
	static Unit bpm;

	// temperature: Celsius
	static Unit cls;
	static Unit fhr;

	// cadence: rpm
	static Unit rpm;

	// power: Watt
	static Unit w;

	static const Unit& distance(Units u, qreal span);
	static const Unit& time(qreal span);

	static void init();
private:
	static QLocale local;
};

#endif // UNITS_H
