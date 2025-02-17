#ifndef DBQCWIDGET_H
#define DBQCWIDGET_H

#include "ui_DBQCWidget.h"
#include "NGSD.h"

#include <QWidget>
#include <QChart>
#include <QScatterSeries>

///Quality statistics widget.
class DBQCWidget
	: public QWidget
{
	Q_OBJECT

	public:
		DBQCWidget(QWidget* parent = 0);

		//Set QC term database ID (required)
		void setTermId(QString id);

		//Set QC term database ID (optional - for scatterplot of two QC metrics)
		void setSecondTermId(QString id);

		//Set processing system database ID (optional filter)
		void setSystemId(QString id);

		//Adds a processed sample to be highlighted (black). If no name is given, it is looked up in the database.
		void addHighlightedProcessedSampleById(QString id, QString name=QString(), bool update_plot=true);

	protected slots:

		//Update highlighted samples (convert to IDs)
		void checkHighlightedSamples();

		//Updates the statistics and plot
		void updatePlot();

		//Reset zoom
		void resetZoom();

		//Swap metrics
		void swapMetrics();

		void clearHighlighting();
		void addHighlightSample();
		void addHighlightRun();
		void addHighlightProject();

		void copyQcMetricsToClipboard();

	private:
		Ui::DBQCWidget ui_;
		NGSD db_;
		QSet<QString> highlight_;

		static QScatterSeries* getSeries(QString name, QColor color, bool square = false);
		static void addSeries(QChart* chart, QAbstractAxis* x_axis, QAbstractAxis* y_axis, QAbstractSeries* series);
};

#endif // DBQCWIDGET_H
