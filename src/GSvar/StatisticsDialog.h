#ifndef STATISTICSDIALOG_H
#define STATISTICSDIALOG_H

#include <QDialog>
#include "ui_StatisticsDialog.h"

class StatisticsDialog
		: public QDialog
{
	Q_OBJECT
	
public:
	StatisticsDialog(QWidget* parent = 0);
	void setStatistics(QMap<QString, QString> data);

private:
	Ui::StatisticsDialog ui_;
};

#endif // STATISTICSDIALOG_H
