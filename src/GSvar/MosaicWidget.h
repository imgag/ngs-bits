#ifndef MOSAICWIDGET_H
#define MOSAICWIDGET_H

#include "ui_MosaicWidget.h"
#include <QWidget>
#include <QTableWidgetItem>
#include <QMenu>
#include "CnvList.h"
#include "GeneSet.h"
#include "FilterWidget.h"
#include "VariantTable.h"
#include "Settings.h"

namespace Ui {
class MosaicWidget;
}

///Widget for visualization and filtering of CNVs.
class MosaicWidget
	: public QWidget
{
	Q_OBJECT

public:

	MosaicWidget(const VariantList& variants, QString ps_id, FilterWidget* filter_widget, ReportSettings rep_settings, QHash<QByteArray, BedFile>& cache, QWidget* parent = 0);

	~MosaicWidget();

protected:

signals:

private slots:
	void applyFilters(bool debug_time=false);
	void copyToClipboard();
	void copyVariantsToClipboard();
	void updateStatus(int num);
	void updateGUI(bool keep_widths =  false);
	void variantDoubleClicked(QTableWidgetItem *item);


private:
	void initGUI();
	void disableGUI();

	Ui::MosaicWidget* ui_;
	QString ps_id_; //processed sample database ID. '' if unknown of NGSD is disabled.
	const VariantList& variants_;
	FilterResult filter_result_;
	ReportSettings report_settings_;

	QHash<QByteArray, BedFile>& gene2region_cache_;
	bool ngsd_enabled_;
};

#endif // MOSAICWIDGET_H