#include <QFileDialog>
#include "CfDNAPanelWidget.h"
#include "ui_CfDNAPanelWidget.h"
#include "GUIHelper.h"

CfDNAPanelWidget::CfDNAPanelWidget(const CfdnaPanelInfo& panel_info, QWidget *parent) :
	QWidget(parent),
	ui_(new Ui::cfDNAPanelWidget),
	panel_info_(panel_info)
{
	ui_->setupUi(this);

	//connect signal and slot
	connect(ui_->btn_copy_clipboard, SIGNAL(clicked(bool)), this, SLOT(copyToClipboard()));
	connect(ui_->btn_export_vars, SIGNAL(clicked(bool)), this, SLOT(exportBed()));

	loadBedFile();
}

CfDNAPanelWidget::~CfDNAPanelWidget()
{
	delete ui_;
}

void CfDNAPanelWidget::copyToClipboard()
{
	GUIHelper::copyToClipboard(ui_->vars);
}

void CfDNAPanelWidget::exportBed()
{
	// get all hotspot regions, somatic variants and sample identifier
	BedFile patient_specific_variants;
	for (int i = 0; i < bed_file_.count(); ++i)
	{
		const BedLine& bed_line = bed_file_[i];
		if (bed_line.annotations().at(0).startsWith("hotspot_region:")
			|| bed_line.annotations().at(0).startsWith("SNP_for_sample_identification:")
			|| bed_line.annotations().at(0).startsWith("patient_specific_somatic_variant:"))
		{
			patient_specific_variants.append(bed_line);
		}
	}

	//open file save dialog
	QString output_file_path = QFileDialog::getSaveFileName(this, "Export patient-specific BED file", NGSD().processedSampleName(QString::number(panel_info_.tumor_id))
															+ "_patient-specific.bed", "BED files (*.bed);;All Files (*)");

	if (output_file_path != "")
	{
		patient_specific_variants.store(output_file_path);
	}
}

void CfDNAPanelWidget::loadBedFile()
{
	// set file name
	ui_->l_file_name->setText("cfDNA panel for " + panel_info_.processing_system  + " (" + panel_info_.created_date.toString("dd.MM.yyyy") + " by " + panel_info_.created_by + ")");
	// load BED file
	bed_file_= NGSD().cfdnaPanelRegions(panel_info_.id);

	// create table view
	ui_->vars->setRowCount(bed_file_.count());
	ui_->vars->setColumnCount(5);
	ui_->vars->setHorizontalHeaderItem(0, GUIHelper::createTableItem("chr"));
	ui_->vars->setHorizontalHeaderItem(1, GUIHelper::createTableItem("start"));
	ui_->vars->setHorizontalHeaderItem(2, GUIHelper::createTableItem("end"));
	ui_->vars->setHorizontalHeaderItem(3, GUIHelper::createTableItem("type"));
	ui_->vars->setHorizontalHeaderItem(4, GUIHelper::createTableItem("details"));

	for (int i=0; i < bed_file_.count(); i++)
	{
		const BedLine& line = bed_file_[i];
		ui_->vars->setItem(i, 0, GUIHelper::createTableItem(line.chr().strNormalized(true)));
		ui_->vars->setItem(i, 1, GUIHelper::createTableItem(QByteArray::number(line.start()-1)));
		ui_->vars->setItem(i, 2, GUIHelper::createTableItem(QByteArray::number(line.end())));
		ui_->vars->setItem(i, 3, GUIHelper::createTableItem(line.annotations()[0].split(':')[0].replace("_", " ")));
		ui_->vars->setItem(i, 4, GUIHelper::createTableItem(line.annotations()[0].split(':')[1]));
	}
	GUIHelper::resizeTableCells(ui_->vars, 250);
	ui_->vars->setSizeAdjustPolicy(QAbstractScrollArea::AdjustToContentsOnFirstShow);

}
