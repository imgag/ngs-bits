#ifndef SAMPLESEARCHWIDGET_H
#define SAMPLESEARCHWIDGET_H

#include <QWidget>
#include "ui_SampleSearchWidget.h"
#include "NGSD.h"

class SampleSearchWidget
	: public QWidget
{
	Q_OBJECT

public:
	SampleSearchWidget(QWidget *parent = 0);

signals:
	void openProcessedSampleTab(QString ps_name);
	void openProcessedSample(QString ps_name);

protected slots:
	void search();
	void openProcessedSampleTab();
	void openProcessedSample();
	void deleteSampleData();
	void queueAnalysis();

private:
	Ui::SampleSearchWidget ui_;
	NGSD db_;
};

#endif // SAMPLESEARCHWIDGET_H
