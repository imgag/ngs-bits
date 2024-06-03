#include "ReportVariantSelectionDialog.h"
#include "ui_ReportVariantSelectionDialog.h"
#include "LoginManager.h"
#include "Exceptions.h"
#include "GlobalServiceProvider.h"
#include "GSvarHelper.h"

#include <GUIHelper.h>
#include <NGSD.h>
#include <QMessageBox>

ReportVariantSelectionDialog::ReportVariantSelectionDialog(QString ps_id, int ignored_rcv_id,  QWidget* parent)
	: QDialog(parent)
	, ui_(new Ui::ReportVariantSelectionDialog)
	, ps_id_(ps_id)
	, variants_(GlobalServiceProvider::getSmallVariantList())
	, cnvs_(GlobalServiceProvider::getCnvList())
	, svs_(GlobalServiceProvider::getSvList())
	, res_(GlobalServiceProvider::getReList())
{
	if (!LoginManager::active())
	{
		INFO(DatabaseException, "ClinVar Upload requires access to the NGSD!");
	}
	ui_->setupUi(this);



	initTable(ignored_rcv_id);
}

SelectedReportVariant ReportVariantSelectionDialog::getSelectedReportVariant()
{
	NGSD db;
	SelectedReportVariant report_variant;
	report_variant.rvc_id = selected_rvc_id_;
	report_variant.report_variant_configuration = selected_report_variant_;
	if (selected_report_variant_.variant_type == VariantType::SNVS_INDELS)
	{
		report_variant.small_variant = variants_[selected_report_variant_.variant_index];
		report_variant.variant_id = db.variantId(report_variant.small_variant).toInt();
	}
	else if (selected_report_variant_.variant_type == VariantType::CNVS)
	{
		SampleData sample_data = db.getSampleData(db.sampleId(db.processedSampleName(ps_id_)));
		report_variant.cnv = cnvs_[selected_report_variant_.variant_index];
		report_variant.cn = report_variant.cnv.copyNumber(cnvs_.annotationHeaders());
		report_variant.ref_cn = CnvList::determineReferenceCopyNumber(report_variant.cnv, sample_data.gender, GSvarHelper::build());
		report_variant.variant_id = db.cnvId(report_variant.cnv, cnv_callset_id_).toInt();
	}
	else if (selected_report_variant_.variant_type == VariantType::SVS)
	{
		report_variant.sv = svs_[selected_report_variant_.variant_index];
		report_variant.variant_id = db.svId(report_variant.sv, sv_callset_id_, svs_).toInt();
	}
	else if (selected_report_variant_.variant_type == VariantType::RES)
	{
		report_variant.re = res_[selected_report_variant_.variant_index];
		report_variant.variant_id = db.repeatExpansionId(report_variant.re.region(), report_variant.re.unit());
	}
	else
	{
		THROW(ArgumentException, "Invalid variant type")
	}

	return report_variant;
}

ReportVariantSelectionDialog::~ReportVariantSelectionDialog()
{
	delete ui_;
}

void ReportVariantSelectionDialog::initTable(int ignored_rcv_id)
{
	NGSD db;
	int rc_id = db.reportConfigId(ps_id_);
	QStringList messages;
	QSharedPointer<ReportConfiguration> report_config = db.reportConfig(rc_id, variants_, cnvs_, svs_, res_);
	cnv_callset_id_ = db.getValue("SELECT id FROM cnv_callset WHERE processed_sample_id=:0", false, ps_id_).toInt();
	sv_callset_id_ = db.getValue("SELECT id FROM sv_callset WHERE processed_sample_id=:0", false, ps_id_).toInt();

	// init table
	ui_->tw_report_variants->setRowCount(report_config->count());
	ui_->tw_report_variants->setColumnCount(4);
	ui_->tw_report_variants->setSortingEnabled(false);

	//create header
	int col_idx = 0;
	ui_->tw_report_variants->setHorizontalHeaderItem(col_idx++, GUIHelper::createTableItem("type"));
	ui_->tw_report_variants->setHorizontalHeaderItem(col_idx++, GUIHelper::createTableItem("description"));
	ui_->tw_report_variants->setHorizontalHeaderItem(col_idx++, GUIHelper::createTableItem("classification"));
	ui_->tw_report_variants->setHorizontalHeaderItem(col_idx++, GUIHelper::createTableItem("exclude"));

	//fill table
	int row_idx = 0;
	foreach (const ReportVariantConfiguration& rvc, report_config->variantConfig())
	{
		col_idx = 0;
		if (rvc.variant_type == VariantType::SNVS_INDELS)
		{
			Variant var = variants_[rvc.variant_index];
			QString var_id = db.variantId(var);
			if (var_id == "")
			{
				messages << "(SNV/InDel) " + var.toString();
				continue;
			}
			int rvc_id = db.getValue("SELECT id FROM report_configuration_variant WHERE report_configuration_id=" + QString::number(rc_id) + " AND variant_id=" + var_id, false).toInt();
			// skip already selected variant
			if (ignored_rcv_id == rvc_id) continue;
			ui_->tw_report_variants->setItem(row_idx, col_idx++, GUIHelper::createTableItem(variantTypeToString(rvc.variant_type)));
			report_variants_.insert(QPair<int, VariantType>(rvc_id, rvc.variant_type), rvc);
			QTableWidgetItem* variant_item = GUIHelper::createTableItem(var.toString(QChar(), -1, true));
			variant_item->setData(Qt::UserRole, rvc_id);
			ui_->tw_report_variants->setItem(row_idx, col_idx++, variant_item);
		}
		else if(rvc.variant_type == VariantType::CNVS)
		{
			CopyNumberVariant cnv = cnvs_[rvc.variant_index];
			QString cnv_id = db.cnvId(cnv, cnv_callset_id_);
			if (cnv_id == "")
			{
				messages << "(CNV) " + cnv.toString();
				continue;
			}
			int rvc_id = db.getValue("SELECT id FROM report_configuration_cnv WHERE report_configuration_id=" + QString::number(rc_id) + " AND cnv_id=" + cnv_id, false).toInt();
			// skip already selected variant
			if (ignored_rcv_id == rvc_id) continue;
			ui_->tw_report_variants->setItem(row_idx, col_idx++, GUIHelper::createTableItem(variantTypeToString(rvc.variant_type)));
			report_variants_.insert(QPair<int, VariantType>(rvc_id, rvc.variant_type), rvc);
			QTableWidgetItem* variant_item = GUIHelper::createTableItem(cnv.toStringWithMetaData());
			variant_item->setData(Qt::UserRole, rvc_id);
			ui_->tw_report_variants->setItem(row_idx, col_idx++, variant_item);
		}
		else if(rvc.variant_type == VariantType::SVS)
		{
			BedpeLine sv = svs_[rvc.variant_index];
			QString sv_id = db.svId(sv, sv_callset_id_, svs_);
			if (sv_id == "")
			{
				messages << "(SV) " + sv.toString();
				continue;
			}
			int rvc_id = db.getValue("SELECT id FROM report_configuration_sv WHERE report_configuration_id=" + QString::number(rc_id) + " AND "  + db.svTableName(sv.type()) + "_id=" + sv_id, false).toInt();
			// skip already selected variant
			if (ignored_rcv_id == rvc_id) continue;
			ui_->tw_report_variants->setItem(row_idx, col_idx++, GUIHelper::createTableItem(variantTypeToString(rvc.variant_type)));
			report_variants_.insert(QPair<int, VariantType>(rvc_id, rvc.variant_type), rvc);
			QTableWidgetItem* variant_item = GUIHelper::createTableItem(sv.toString());
			variant_item->setData(Qt::UserRole, rvc_id);
			ui_->tw_report_variants->setItem(row_idx, col_idx++, variant_item);
		}
		else
		{
			THROW(ArgumentException, "Invalid variant type provided!");
		}

		ui_->tw_report_variants->setItem(row_idx, col_idx++, GUIHelper::createTableItem(rvc.classification));
		ui_->tw_report_variants->setItem(row_idx, col_idx++, GUIHelper::createTableItem((rvc.showInReport())? "No": "Yes"));
		row_idx++;
	}

	//resize table
	ui_->tw_report_variants->setRowCount(row_idx);
	GUIHelper::resizeTableCells(ui_->tw_report_variants);

	//sort table
	ui_->tw_report_variants->setSortingEnabled(true);
	ui_->tw_report_variants->sortByColumn(1, Qt::AscendingOrder);

	//define selection model
	ui_->tw_report_variants->setSelectionBehavior(QAbstractItemView::SelectRows);
	ui_->tw_report_variants->setSelectionMode(QAbstractItemView::SingleSelection);

	//signals and slots
	connect(ui_->tw_report_variants, SIGNAL(itemSelectionChanged()), this, SLOT(updateSelection()));

	//disable buttons by default (no selection yet)
	updateSelection();

	if(messages.size() > 0)
	{
		QMessageBox::warning(this, "Missing variant(s)", "The following variants are not in the NGSD. A variant has to be in NGSD to be published on ClinVar.\n\n" + messages.join("\n"));
	}

}

void ReportVariantSelectionDialog::updateSelection()
{
	QList<int> selected_rows  = GUIHelper::selectedTableRows(ui_->tw_report_variants);
	if(selected_rows.size() > 1) THROW(ArgumentException, "Multiple lines selected");
	if(selected_rows.size() == 0)
	{
		//delete previous selection
		selected_report_variant_ = ReportVariantConfiguration();
		selected_rvc_id_ = -1;
		ui_->buttonBox->setEnabled(false);
	}
	else
	{
		//update selection
		selected_rvc_id_ = ui_->tw_report_variants->item(selected_rows.at(0), 1)->data(Qt::UserRole).toInt();
		VariantType variant_type = stringToVariantType(ui_->tw_report_variants->item(selected_rows.at(0), 0)->text());
		selected_report_variant_ = report_variants_.value(QPair<int, VariantType>(selected_rvc_id_, variant_type));
		ui_->buttonBox->setEnabled(true);

	}

}
