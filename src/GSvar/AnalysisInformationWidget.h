#ifndef ANALYSISINFORMATIONWIDGET_H
#define ANALYSISINFORMATIONWIDGET_H

#include <QWidget>
#include "ui_AnalysisInformationWidget.h"

class AnalysisInformationWidget
	: public QWidget
{
	Q_OBJECT

public:
	AnalysisInformationWidget(QString ps_id, QWidget* parent = 0);

private slots:
	void updateGUI();
	void copyTableToClipboard();

private:
	Ui::AnalysisInformationWidget ui_;
	QString ps_id_;
};

#endif // ANALYSISINFORMATIONWIDGET_H
