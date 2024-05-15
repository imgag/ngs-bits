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

protected slots:
	void search();
	void openProcessedSampleTab();
	void openProcessedSampleTab(int row);
	void openVariantList();
	void deleteSampleData();
	void amendSampleComments();
	void queueAnalysis();
	void phenotypeSelection();

private:
	Ui::SampleSearchWidget ui_;
	NGSD db_;
	PhenotypeList phenotypes_;
};

#endif // SAMPLESEARCHWIDGET_H
