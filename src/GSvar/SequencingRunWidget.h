#ifndef SEQUENCINGRUNWIDGET_H
#define SEQUENCINGRUNWIDGET_H

#include <QWidget>
#include <QAction>
#include <QTableWidgetItem>
#include "NGSD.h"

namespace Ui {
class SequencingRunWidget;
}

class SequencingRunWidget : public QWidget
{
	Q_OBJECT

public:
	SequencingRunWidget(QWidget* parent, QString run_id);
	~SequencingRunWidget();

signals:
	void openProcessedSampleTab(QString ps_name);

protected slots:
	void updateGUI();
	void openSelectedSampleTabs();
	void openSampleTab(int row);
	void updateReadQualityTable();
	void updateRunSampleTable();
	void setQuality();
	void edit();
	void sendStatusEmail();
	void checkMids();

private:
	Ui::SequencingRunWidget* ui_;
	QString run_id_;

	static void highlightItem(QTableWidgetItem* item);
};

#endif // SEQUENCINGRUNWIDGET_H
