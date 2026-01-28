#ifndef FUSIONWIDGET_H
#define FUSIONWIDGET_H

#include <QWidget>
#include <QTableWidget>
#include "ArribaFile.h"
#include "RnaReportConfiguration.h"
#include "NGSD.h"

namespace Ui {
class FusionWidget;
}

class FusionWidget : public QWidget
{
	Q_OBJECT

public:
    explicit FusionWidget(const QString& filename, const QString& rna_ps_name, QSharedPointer<RnaReportConfiguration> rna_rep_conf,  QWidget *parent = 0);
	~FusionWidget();

signals:
    void storeRnaReportConfiguration();

private slots:
	void displayFusionImage();
	void applyFilters(bool debug_time=false);
	void updateStatus(int shown);
	void editRnaReportConfiguration(int row);
	void editRnaReportConfiguration(const QList<int>& rows);
	void updateReportConfigHeaderIcon(int row);
	void updateGUI();
	void showContextMenu(QPoint p);
	//TODO
	//void fusionDoubleClicked(QTableWidgetItem* item);
	//void importPhenotypesFromNGSD();
	//void copyToClipboard();


	//void fusionHeaderDoubleClicked(int row);
	//void fusionHeaderContextMenu(QPoint pos);



private:
	QList<QImage> imagesFromFiles(const QStringList& files);
    QSharedPointer<RnaReportConfiguration> rna_report_config_;

	QString filename_;
	ArribaFile fusions_;
	QString rna_ps_name_;
	QList<QImage> images_;
	Ui::FusionWidget *ui_;
};

#endif // FUSIONWIDGET_H
