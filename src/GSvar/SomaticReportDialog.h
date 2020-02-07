#ifndef SOMATICREPORTDIALOG_H
#define SOMATICREPORTDIALOG_H

#include <QDialog>
#include "SomaticReportSettings.h"
#include "NGSD.h"
#include "ui_SomaticReportDialog.h"

class SomaticReportDialog
		: public QDialog
{
	Q_OBJECT

public:
	SomaticReportDialog(SomaticReportSettings& settings, const VariantList& variants, const CnvList& cnvs, QWidget* parent = 0);

	enum report_type
	{
		DNA,
		RNA
	};

	///Returns whether report type shall be DNA or RNA
	report_type getReportType();

	///enable option to choose between DNA or RNA report
	void enableChoiceReportType(bool enabled);

	///enable option to include gap statistics
	void enableGapStatistics(bool enabled);

private:
	Ui::SomaticReportDialog ui_;
	NGSD db_;
	SomaticReportSettings& settings_;
	const VariantList& variants_;
	const CnvList& cnvs_;
	QString target_region_;

	double tum_cont_snps_;
	double tum_cont_max_clonality_;
	double tum_cont_histological_;


public slots:
	void writeBackSettings();

private slots:
	void updateGUI();
};

#endif // SOMATICREPORTDIALOG_H
