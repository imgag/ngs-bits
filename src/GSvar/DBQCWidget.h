#ifndef DBQCWIDGET_H
#define DBQCWIDGET_H

#include "ui_DBQCWidget.h"
#include "NGSD.h"

#include <QWidget>
#include <QChart>


///Quality statistics widget.
class DBQCWidget
	: public QWidget
{
	Q_OBJECT

	public:
		DBQCWidget(QWidget* parent = 0);

		//Set QC term database ID (required)
		void setTermId(QString id);

		//Set processing system database ID (optional filter)
		void setSystemId(QString id);

		//Adds a processed sample ID to be highlighted (black)
		void addHighLightProcessedSampleId(QString id);

	protected slots:

		//Update highlighted samples (convert to IDs)
		void updateHighlightedSamples();

		//Updates the statistics and plot
		void updateGUI();

		//Reset zoom
		void resetZoom();

	private:
		Ui::DBQCWidget ui_;
		NGSD db_;
		QSet<QString> highlight_;

		static QScatterSeries* getSeries(QString name, QColor color, bool square = false);
		static void addSeries(QChart* chart, QAbstractAxis* x_axis, QAbstractAxis* y_axis, QAbstractSeries* series);
};

#endif // DBQCWIDGET_H
