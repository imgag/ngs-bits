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
	SequencingRunWidget(QWidget* parent, const QStringList& run_ids);
	~SequencingRunWidget();

signals:
	void addModelessDialog(QSharedPointer<QDialog> dlg, bool maximize);

protected slots:
	void initBatchView();
	void updateGUI();
	void openSelectedSampleTabs();
	void openSampleTab(int row);
	void updateReadQualityTable();
	void updateRunSampleTable();
	void setQuality();
	void scheduleForResequencing();
	void showPlot();
	void edit();
	//edit slot for batch view
	void edit(int run_id);
	void sendStatusEmail();
	void checkMids();
	void exportSampleSheet();

private:
	Ui::SequencingRunWidget* ui_;
	QStringList run_ids_;
	bool is_batch_view_;
	QStringList qc_metric_accessions_;
	void setQCMetricAccessions(const QSet<QString>& sample_types);

	static void highlightItem(QTableWidgetItem* item);
};

#endif // SEQUENCINGRUNWIDGET_H
