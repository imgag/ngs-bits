#include "CfDNAPanelBatchImport.h"
#include "ui_CfDNAPanelBatchImport.h"
#include "Exceptions.h"
#include "NGSD.h"
#include "LoginManager.h"
#include "GUIHelper.h"
#include "GlobalServiceProvider.h"
#include "Settings.h"
#include "GSvarHelper.h"
#include <QMessageBox>

CfDNAPanelBatchImport::CfDNAPanelBatchImport(QWidget *parent) :
	QDialog(parent),
	ui_(new Ui::CfDNAPanelBatchImport)
{
	// abort if no connection to NGSD
	if (!LoginManager::active())
	{
		GUIHelper::showMessage("No connection to the NGSD!", "You need access to the NGSD to import cfDNA panels!");
		this->close();
	}

	ui_->setupUi(this);
	initGUI();
}

CfDNAPanelBatchImport::~CfDNAPanelBatchImport()
{
	delete ui_;
}

void CfDNAPanelBatchImport::initGUI()
{
	//create table headers
	ui_->tw_import_table->setColumnCount(3);
	ui_->tw_import_table->setHorizontalHeaderItem(0, new QTableWidgetItem("Processed sample (tumor)"));
	ui_->tw_import_table->setHorizontalHeaderItem(1, new QTableWidgetItem("Processing system"));
	ui_->tw_import_table->setHorizontalHeaderItem(2, new QTableWidgetItem("File path to cfDNA panel (VCF)"));

	//Signals and slots
	connect(ui_->b_validate, SIGNAL(clicked(bool)), this, SLOT(importTextInput()));
	connect(ui_->b_back, SIGNAL(clicked(bool)), this, SLOT(showRawInputView()));

}

void CfDNAPanelBatchImport::fillTable()
{
	ui_->tw_import_table->setColumnCount(3);
	ui_->tw_import_table->setRowCount(input_table_.size());
	for (int row_idx = 0; row_idx < input_table_.size(); ++row_idx)
	{
		ui_->tw_import_table->setItem(row_idx, 0, new QTableWidgetItem(QString(input_table_.at(row_idx).processed_sample)));
		ui_->tw_import_table->setItem(row_idx, 1, new QTableWidgetItem(QString(input_table_.at(row_idx).processing_system)));
		ui_->tw_import_table->setItem(row_idx, 2, new QTableWidgetItem(QString(input_table_.at(row_idx).vcf_file_path)));
	}
}

void CfDNAPanelBatchImport::writeToDbImportLog(const QString& text)
{
	ui_->te_import_result->setText(ui_->te_import_result->toPlainText() + text + "\n");
}

void CfDNAPanelBatchImport::validateTable()
{
	ui_->b_import->setEnabled(false);
	bool valid = true;
	for (int row_idx = 0; row_idx < ui_->tw_import_table->rowCount(); ++row_idx)
	{
		//check processed sample
		QString ps_name = ui_->tw_import_table->item(row_idx, 0)->text().trimmed();
		QString ps_id = NGSD().processedSampleId(ps_name, false);
		if(ps_id == "")
		{
			valid = false;
			ui_->tw_import_table->item(row_idx, 0)->setBackgroundColor(Qt::red);
			ui_->tw_import_table->item(row_idx, 0)->setToolTip("Processed sample not found in NGSD!");
		}
		SampleData sample_info = NGSD().getSampleData(NGSD().sampleId(ps_name));
		if (!sample_info.is_tumor)
		{
			valid = false;
			ui_->tw_import_table->item(row_idx, 0)->setBackgroundColor(Qt::red);
			ui_->tw_import_table->item(row_idx, 0)->setToolTip("Processed sample '" + ps_name + "' is not a tumor sample!");
		}

		//check processing system
		int sys_id = NGSD().processingSystemId(ui_->tw_import_table->item(row_idx, 1)->text(), false);
		if(sys_id == -1)
		{
			valid = false;
			ui_->tw_import_table->item(row_idx, 1)->setBackgroundColor(Qt::red);
			ui_->tw_import_table->item(row_idx, 1)->setToolTip("Processing system not found in NGSD!");
		}

		//check if file exists
		if (!QFile::exists(ui_->tw_import_table->item(row_idx, 2)->text()))
		{
			valid = false;
			ui_->tw_import_table->item(row_idx, 2)->setBackgroundColor(Qt::red);
			ui_->tw_import_table->item(row_idx, 2)->setToolTip("File does not exist!");
		}
	}

	if(valid)
	{
		ui_->b_import->setEnabled(true);
		ui_->l_validation_result->setText("Validation successful.");
	}
	else
	{
		ui_->l_validation_result->setText("Validation failed!");
	}
}

VcfFile CfDNAPanelBatchImport::createVcf(const QString& ps_name, const QString& vcf_file_path)
{
	// parse VCF
	QMap<QString, bool> selected_variants;
	VcfFile input_file;
	input_file.load(vcf_file_path, false);
	for (int i = 0; i < input_file.count(); ++i)
	{
		VcfLine vcf_line = input_file.vcfLine(i);
		// create vcf pos string
		QString vcf_pos = vcf_line.chr().strNormalized(true) + ":" + QString::number(vcf_line.start()) + " " + vcf_line.ref() + ">" + vcf_line.altString();
		selected_variants.insert(vcf_pos, false);
	}

	// load reference
	FastaFileIndex genome_reference(Settings::string("reference_genome", false));

	// load processed sample GSvar file
	QString ps_id = NGSD().processedSampleId(ps_name);
	FileLocation sample_gsvar_file = GlobalServiceProvider::database().processedSamplePath(ps_id, PathType::GSVAR);
	if (!sample_gsvar_file.exists) THROW(FileAccessException, "GSvar file '" +  sample_gsvar_file.filename + "' not found!");
	VariantList gsvar;
	gsvar.load(sample_gsvar_file.filename);


	// extract selected variants
	VariantList cfdna_panel;
	cfdna_panel.copyMetaData(gsvar);

	for (int i = 0; i < gsvar.count(); ++i)
	{
		const Variant& var = gsvar[i];
		VariantVcfRepresentation vcf_line = var.toVCF(genome_reference);
		// create vcf pos string
		QString vcf_pos = vcf_line.chr.strNormalized(true) + ":" + QString::number(vcf_line.pos) + " " + vcf_line.ref + ">" + vcf_line.alt;
		if (selected_variants.contains(vcf_pos))
		{
			cfdna_panel.append(var);
			selected_variants[vcf_pos] = true;
		}
	}

	// check if all varaints were found
	QStringList missing_variants;
	foreach (const QString& pos, selected_variants.keys())
	{
		if (!selected_variants.value(pos))
		{
			missing_variants.append(pos);
		}
	}

	if (missing_variants.size() > 0)
	{
		THROW(FileParseException, "The following variants were not found in GSvar file of sample '" + ps_name + "'" + missing_variants.join('\n'));
	}

	return VcfFile::convertGSvarToVcf(cfdna_panel, Settings::string("reference_genome", false));
}

void CfDNAPanelBatchImport::showRawInputView()
{
	ui_->sw_import_panels->setCurrentIndex(0);
}

void CfDNAPanelBatchImport::importTextInput()
{
	try
	{
		parseInput();
	}
	catch (FileParseException e)
	{
		QMessageBox::warning(this, "Error parsing table input", e.message());
		return;
	}

	// switch to next widget
	ui_->sw_import_panels->setCurrentIndex(1);


	// fill and validate table
	fillTable();
	validateTable();
}

void CfDNAPanelBatchImport::parseInput()
{
	input_table_.clear();
	foreach (const QString& line, ui_->te_tsv_input->toPlainText().split('\n'))
	{
		//ignore comments, headers and empty lines
		if (line.trimmed().isEmpty() || line.startsWith("#")) continue;

		QByteArrayList columns = line.toUtf8().split('\t');

		if (columns.size() < 3) THROW(FileParseException, "Table line has to contain at least 3 columms, got " + QString::number(columns.size()) + "!");

		cfDNATableLine table_line;
		table_line.processed_sample = columns.at(0).trimmed();
		table_line.processing_system = columns.at(1).trimmed();
		table_line.vcf_file_path = columns.at(2).trimmed();
		input_table_.append(table_line);
	}
}

void CfDNAPanelBatchImport::importPanels()
{
	ui_->te_import_result->setText("Starting import...\n");

	// add sample identifier on demand
	bool add_sample_identifier = ui_->cb_add_sample_identifier->isChecked();
	QMap<int,VcfFile> proc_sys_sample_ids = QMap<int,VcfFile>();
	VcfFile general_sample_ids;
	if (add_sample_identifier)
	{
		// get KASP SNPs
		QTextStream vcf_content(new QFile("://Resources/" + buildToString(GSvarHelper::build()) + "_KASP_set2.vcf"));
		general_sample_ids.fromText(vcf_content.readAll().toUtf8());
	}

	//overwrite panels on demand
	bool overwrite_existing = ui_->cb_overwrite_existing->isChecked();

	// start mysql transaction
	NGSD().transaction();

	try
	{
		//iterate over table
		for (int row_idx = 0; row_idx < ui_->tw_import_table->rowCount(); ++row_idx)
		{
			// read table line
			QString ps_name = ui_->tw_import_table->item(row_idx, 0)->text();
			int processing_system_id = NGSD().processingSystemId(ui_->tw_import_table->item(row_idx, 1)->text());
			QString vcf_file_path = ui_->tw_import_table->item(row_idx, 2)->text();

			writeToDbImportLog("Importing cfDNA panel for processed sample " + ps_name + "...\n");

			//create panel info
			CfdnaPanelInfo panel_info;
			if (overwrite_existing)
			{
				// get previous cfDNA panel
				QList<CfdnaPanelInfo> panel_list = NGSD().cfdnaPanelInfo(NGSD().processedSampleId(ps_name), QString::number(processing_system_id));
				if (panel_list.size() > 0)
				{
					panel_info = panel_list.at(0);
				}
			}

			panel_info.tumor_id = Helper::toInt(NGSD().processedSampleId(ps_name));
			panel_info.created_by = LoginManager::userId();
			panel_info.created_date = QDate::currentDate();
			panel_info.processing_system_id = processing_system_id;


			// parse VCF and generate cfDNA panel
			VcfFile cfdna_panel = createVcf(ps_name, vcf_file_path);

			// append sample ids
			if (add_sample_identifier)
			{
				// add KASP SNPs
				foreach (VcfLinePtr vcf_line_ptr, general_sample_ids.vcfLines())
				{
					cfdna_panel.vcfLines() << vcf_line_ptr;
				}

				// add ID SNPs from processing system
				if (!proc_sys_sample_ids.contains(processing_system_id))
				{
					proc_sys_sample_ids.insert(processing_system_id, NGSD().getIdSnpsFromProcessingSystem(processing_system_id, true));
				}
				foreach (VcfLinePtr vcf_line_ptr, proc_sys_sample_ids.value(processing_system_id).vcfLines())
				{
					cfdna_panel.vcfLines() << vcf_line_ptr;
				}

				cfdna_panel.sort();
			}

			// generate bed file
			BedFile cfdna_panel_region;
			for (int i=0; i<cfdna_panel.count(); i++)
			{
				QByteArray annotation = (cfdna_panel[i].id().contains("M"))?"patient_specific_somatic_variant:" : "SNP_for_sample_identification:";
				cfdna_panel_region.append(BedLine(cfdna_panel[i].chr(), cfdna_panel[i].start(), cfdna_panel[i].end(),
												  QByteArrayList() << (annotation + cfdna_panel[i].ref() + ">" + cfdna_panel[i].altString())));
			}

			// import into NGSD
			NGSD().storeCfdnaPanel(panel_info, cfdna_panel_region.toText().toUtf8(), cfdna_panel.toText());
		}

		// if all panels were imported successfully -> apply changes to the database
		NGSD().commit();
	}
	catch (Exception e)
	{
		writeToDbImportLog("<font color='red'>Import of cfDNA panels failed!</font><br>ERROR:" + e.message());
		// perform db rollback
		NGSD().rollback();
	}


}
