#ifndef DBQCWIDGET_H
#define DBQCWIDGET_H

#include "cppNGSD_global.h"
#include "ui_DBQCWidget.h"
#include "NGSD.h"

#include <QWidget>


///Quality statistics widget.
class CPPNGSDSHARED_EXPORT DBQCWidget
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

		void updateGUI();

	private:
		Ui::DBQCWidget ui_;
		NGSD db_;
		QString id_term_;
		QString id_sys_;
};

#endif // DBQCWIDGET_H
