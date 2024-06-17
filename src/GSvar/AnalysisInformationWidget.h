#ifndef ANALYSISINFORMATIONWIDGET_H
#define ANALYSISINFORMATIONWIDGET_H

#include <QWidget>
#include "DelayedInitializationTimer.h"
#include "NGSD.h"
#include "ui_AnalysisInformationWidget.h"

class AnalysisInformationWidget
	: public QWidget
{
	Q_OBJECT

public:
	AnalysisInformationWidget(QString ps_id, QWidget* parent = 0);

protected slots:
	void delayedInitialization();

private slots:
	void updateGUI();
	void copyTableToClipboard();

private:
	Ui::AnalysisInformationWidget ui_;
	DelayedInitializationTimer init_timer_;
	QString ps_id_;

	QString rcData(NGSD& db, QString table, QString rc_id);
};

#endif // ANALYSISINFORMATIONWIDGET_H
