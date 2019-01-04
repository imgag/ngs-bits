#ifndef PROCESSEDSAMPLEWIDGET_H
#define PROCESSEDSAMPLEWIDGET_H

#include "NGSD.h"

#include <QWidget>

namespace Ui {
class ProcessedSampleWidget;
}

class ProcessedSampleWidget
	: public QWidget
{
	Q_OBJECT


public:
	ProcessedSampleWidget(QWidget* parent, QString ps_id);
	~ProcessedSampleWidget();

protected slots:
	void updateGUI();
	void updateQCMetrics();
	void showPlot();
	void openSampleInNGSD();

private:
	Ui::ProcessedSampleWidget* ui_;
	QString id_;
	NGSD db_;
};

#endif // PROCESSEDSAMPLEWIDGET_H
