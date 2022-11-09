#include "ReportVariantSelectionDialog.h"
#include "ui_ReportVariantSelectionDialog.h"
#include "LoginManager.h"
#include "Exceptions.h"
#include "GlobalServiceProvider.h"

#include <GUIHelper.h>
#include <NGSD.h>

ReportVariantSelectionDialog::ReportVariantSelectionDialog(QString ps_id, QWidget *parent) :
	QDialog(parent),
	ui_(new Ui::ReportVariantSelectionDialog),
	ps_id_(ps_id)
{
	if (!LoginManager::active())
	{
		INFO(DatabaseException, "ClinVar Upload requires access to the NGSD!");
	}
	ui_->setupUi(this);

	initTable();
}

ReportVariantSelectionDialog::~ReportVariantSelectionDialog()
{
	delete ui_;
}

void ReportVariantSelectionDialog::initTable()
{
	NGSD db;
	int rc_id = db.reportConfigId(ps_id_);
	QStringList messages;
	const VariantList& variants = GlobalServiceProvider::getSmallVariantList();
	const CnvList& cnvs = GlobalServiceProvider::getCnvList();
	const BedpeFile& svs = GlobalServiceProvider::getSvList();
	report_config_ = db.reportConfig(rc_id, variants, cnvs, svs, messages);

	// init table
	ui_->tw_report_variants->setRowCount(report_config_->count());
	ui_->tw_report_variants->setColumnCount(4);

	//create header
	int col_idx = 0;
	ui_->tw_report_variants->setHorizontalHeaderItem(col_idx++, GUIHelper::createTableItem("type"));
	ui_->tw_report_variants->setHorizontalHeaderItem(col_idx++, GUIHelper::createTableItem("description"));
	ui_->tw_report_variants->setHorizontalHeaderItem(col_idx++, GUIHelper::createTableItem("classification"));
	ui_->tw_report_variants->setHorizontalHeaderItem(col_idx++, GUIHelper::createTableItem("exclude"));

	//fill table
	int row_idx = 0;
	foreach (const ReportVariantConfiguration& rvc, report_config_->variantConfig())
	{
		col_idx = 0;
		ui_->tw_report_variants->setItem(row_idx, col_idx++, GUIHelper::createTableItem(variantTypeToString(rvc.variant_type)));
		if (rvc.variant_type == VariantType::SNVS_INDELS)
		{
			Variant var = variants[rvc.variant_index];
			ui_->tw_report_variants->setItem(row_idx, col_idx++, GUIHelper::createTableItem(var.toString(false, -1, true)));
		}
		else if(rvc.variant_type == VariantType::CNVS)
		{
			CopyNumberVariant cnv = cnvs[rvc.variant_index];
			ui_->tw_report_variants->setItem(row_idx, col_idx++, GUIHelper::createTableItem(cnv.toStringWithMetaData()));
		}
		else if(rvc.variant_type == VariantType::SVS)
		{
			BedpeLine sv = svs[rvc.variant_index];
			ui_->tw_report_variants->setItem(row_idx, col_idx++, GUIHelper::createTableItem(sv.toString()));
		}
		else
		{
			THROW(ArgumentException, "Invalid variant type provided!");
		}

		ui_->tw_report_variants->setItem(row_idx, col_idx++, GUIHelper::createTableItem(rvc.classification));
		ui_->tw_report_variants->setItem(row_idx, col_idx++, GUIHelper::createTableItem((rvc.showInReport())? "No": "Yes"));
		row_idx++;
	}
	ui_->tw_report_variants->setSelectionBehavior(QAbstractItemView::SelectRows);
	ui_->tw_report_variants->setSelectionMode(QAbstractItemView::SingleSelection);

}
