#ifndef PROCESSEDSAMPLEWIDGET_H
#define PROCESSEDSAMPLEWIDGET_H

#include "NGSD.h"

#include <QWidget>
#include <QLabel>

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

	static void styleQualityLabel(QLabel* label, const QString& quality);

signals:
	void openProcessedSampleTab(QString ps_name);
	void openRunTab(QString run_name);

protected slots:
	void updateGUI();
	void updateQCMetrics();
	void showPlot();
	void openSampleInNGSD();
	void openSampleTab();

private:
	Ui::ProcessedSampleWidget* ui_;
	QString ps_id_;
	NGSD db_;
};

#endif // PROCESSEDSAMPLEWIDGET_H
