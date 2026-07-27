#ifndef SEQUENCINGRUNWIDGET_H
#define SEQUENCINGRUNWIDGET_H

#include <QWidget>
#include <QTableWidgetItem>
#include "NGSD.h"
#include "TabBaseClass.h"

namespace Ui {
class SequencingRunWidget;
}

class SequencingRunWidget
	: public TabBaseClass
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
	void setQualityManually();
	void setQualityAutomatically();
	void toggleScheduleForResequencing();
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
	void setQCMetricAccessions(const QSet<QString>& sample_types, const QSet<QString>& system_types);

	static void highlightItem(QTableWidgetItem* item);

	//cache long names to avoid repeated NGSD queries
	QHash<QString, QString> sys_long_to_short_;
	QString systemShortName(NGSD &db, const QString& long_name);

	//cache QT term names to avoid repeated NGSD queries
	QHash<QString, QString> qc_accession_to_name_;
	QString qcTermName(NGSD &db, const QString& accession);
};

#endif // SEQUENCINGRUNWIDGET_H
