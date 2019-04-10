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
	void openProcessedSampleTab(QString);

protected slots:
	void search();
	void openProcessedSample();

private:
	Ui::SampleSearchWidget ui_;
	NGSD db_;
};

#endif // SAMPLESEARCHWIDGET_H
