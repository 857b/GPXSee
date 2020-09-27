#ifndef OPTIONSDIALOG_H
#define OPTIONSDIALOG_H

#include <QDialog>

#include "options.h"

//TODO #define CONTROL_SEGMENTS

class ColorBox;
class StyleComboBox;
class OddSpinBox;
class QSpinBox;
class QDoubleSpinBox;
class QComboBox;
class QCheckBox;
class QRadioButton;
class PercentSlider;
class LimitedComboBox;

class OptionsDialog : public QDialog
{
	Q_OBJECT

public slots:
	void accept();

public:
	OptionsDialog(Options &options, Units units, QWidget *parent = 0);

private slots:
	void automaticPauseDetectionSet(bool set);

private:
	QWidget *createMapPage();
	QWidget *createAppearancePage();
	QWidget *createDataPage();
	QWidget *createPOIPage();
	QWidget *createSystemPage();
	QWidget *createExportPage();

	Options &_options;

	Units _units;
	// Appearance
	ColorBox *_baseColor;
	PercentSlider *_colorOffset;
	PercentSlider *_mapOpacity;
	ColorBox *_backgroundColor;
	QSpinBox *_trackWidth;
	StyleComboBox *_trackStyle;
	QSpinBox *_routeWidth;
	StyleComboBox *_routeStyle;
	QSpinBox *_areaWidth;
	StyleComboBox *_areaStyle;
	PercentSlider *_areaOpacity;
	QCheckBox *_pathAA;
	QSpinBox *_waypointSize;
	ColorBox *_waypointColor;
	QSpinBox *_poiSize;
	ColorBox *_poiColor;
	QSpinBox *_graphWidth;
	ColorBox *_sliderColor;
	QCheckBox *_graphAA;
	// Map
	LimitedComboBox *_projection;
#ifdef ENABLE_HIDPI
	QRadioButton *_hidpi;
	QRadioButton *_lodpi;
#endif // ENABLE_HIDPI
	// Data
	OddSpinBox     *_elevationFilter;
	OddSpinBox     *_speedFilter;
	OddSpinBox     *_heartRateFilter;
	OddSpinBox     *_cadenceFilter;
	OddSpinBox     *_powerFilter;
	QCheckBox      *_outlierEliminate;
	QRadioButton   *_automaticPause;
	QRadioButton   *_manualPause;
	QDoubleSpinBox *_pauseSpeed;
	QSpinBox       *_pauseInterval;
	QDoubleSpinBox *_derivDeltas[3];//min, max, opt
	QCheckBox      *_speedDirection;
#ifdef ENABLE_TIMEZONES
	QRadioButton *_utcZone;
	QRadioButton *_systemZone;
	QRadioButton *_customZone;
	QComboBox *_timeZone;
#endif // ENABLE_TIMEZONES
#ifdef CONTROL_SEGMENTS
	QCheckBox *_useSegments;
#endif
	// POI
	QDoubleSpinBox *_poiRadius;
	// System
	QSpinBox *_pixmapCache;
	QSpinBox *_connectionTimeout;
	QCheckBox *_useOpenGL;
#ifdef ENABLE_HTTP2
	QCheckBox *_enableHTTP2;
#endif // ENABLE_HTTP2
	// Print/Export
	QRadioButton *_wysiwyg;
	QRadioButton *_hires;
	QCheckBox *_name;
	QCheckBox *_date;
	QCheckBox *_distance;
	QCheckBox *_time;
	QCheckBox *_movingTime;
	QCheckBox *_itemCount;
	QCheckBox *_separateGraphPage;
};

#endif // OPTIONSDIALOG_H
