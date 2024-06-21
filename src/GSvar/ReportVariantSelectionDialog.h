#ifndef REPORTVARIANTSELECTIONDIALOG_H
#define REPORTVARIANTSELECTIONDIALOG_H

#include <BedpeFile.h>
#include <CnvList.h>
#include <QDialog>
#include <ReportConfiguration.h>
#include <VariantList.h>

namespace Ui {
class ReportVariantSelectionDialog;
}

struct SelectedReportVariant
{
	int rvc_id = -1;
	int variant_id = -1;
	ReportVariantConfiguration report_variant_configuration;
	Variant small_variant;
	CopyNumberVariant cnv;
	RepeatLocus re;
	int cn = -1;
	int ref_cn = 2;
	BedpeLine sv;
};

class ReportVariantSelectionDialog : public QDialog
{
	Q_OBJECT

public:
	explicit ReportVariantSelectionDialog(QString ps_id, int ignored_rcv_id, QWidget *parent = 0);
	SelectedReportVariant getSelectedReportVariant();
	~ReportVariantSelectionDialog();

private:
	Ui::ReportVariantSelectionDialog *ui_;
	QString ps_id_;
	const VariantList& variants_;
	const CnvList& cnvs_;
	const BedpeFile& svs_;
	const RepeatLocusList& res_;
	int cnv_callset_id_ = -1;
	int sv_callset_id_ = -1;
	ReportVariantConfiguration selected_report_variant_;
	int selected_rvc_id_;
	QMap<QPair<int, VariantType>, ReportVariantConfiguration> report_variants_;
	void initTable(int ignored_rcv_id);

private slots:
	void updateSelection();
};

#endif // REPORTVARIANTSELECTIONDIALOG_H
