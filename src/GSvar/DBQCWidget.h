#ifndef DBQCWIDGET_H
#define DBQCWIDGET_H

#include "ui_DBQCWidget.h"
#include "NGSD.h"

#include <QWidget>


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

	protected slots:

		//Updates the statistics and plot
		void updateGUI();

	private:
		Ui::DBQCWidget ui_;
		NGSD db_;

		QScatterSeries* getSeries(QColor color);
};

#endif // DBQCWIDGET_H
