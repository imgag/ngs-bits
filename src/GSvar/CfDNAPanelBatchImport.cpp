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
#include <QInputDialog>

CfDNAPanelBatchImport::CfDNAPanelBatchImport(QWidget *parent) :
	QDialog(parent),
	ui_(new Ui::CfDNAPanelBatchImport)
{
	// abort if no connection to NGSD
	if (!LoginManager::active())
	{
		GUIHelper::showMessage("No connection to the NGSD!", "You need access to the NGSD to import cfDNA panels!");
		close();
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

	//make log output read-only
	ui_->te_import_result->setReadOnly(true);

	//Signals and slots
	connect(ui_->b_validate, SIGNAL(clicked(bool)), this, SLOT(importTextInput()));
	connect(ui_->b_back, SIGNAL(clicked(bool)), this, SLOT(showRawInputView()));
	connect(ui_->b_import, SIGNAL(clicked(bool)), this, SLOT(importPanels()));
	connect(ui_->b_back2, SIGNAL(clicked(bool)), this, SLOT(showValidationTableView()));
	connect(ui_->cb_overwrite_existing, SIGNAL(stateChanged(int)), this, SLOT(validateTable()));
	connect(ui_->b_close, SIGNAL(clicked(bool)), this, SLOT(close()));
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

	GUIHelper::resizeTableCellWidths(ui_->tw_import_table, 300);
	GUIHelper::resizeTableCellHeightsToFirst(ui_->tw_import_table);
}

void CfDNAPanelBatchImport::writeToDbImportLog(const QString& text, bool critical)
{
	QString font_color;
	if (critical)
	{
		font_color = " color=\"Red\"";
	}
	ui_->te_import_result->insertHtml("<font" + font_color + ">" + text + "</font><br>");
}

void CfDNAPanelBatchImport::validateTable()
{
	ui_->b_import->setEnabled(false);
	QApplication::setOverrideCursor(Qt::BusyCursor);
	bool valid = true;

	// define table backround colors
	QColor bg_red = QColor(255, 0, 0, 128);
//	QColor bg_green = QColor(0, 99, 37, 128);
	QColor bg_orange = QColor(255, 135, 60, 128);
	QColor bg_white = QColor(255, 255, 255, 255);


	for (int row_idx = 0; row_idx < ui_->tw_import_table->rowCount(); ++row_idx)
	{
		bool db_entries_valid = true;
		//check processed sample
		QString ps_name = ui_->tw_import_table->item(row_idx, 0)->text().trimmed();
		QString ps_id = db_.processedSampleId(ps_name, false);
		if(ps_id == "")
		{
			valid = false;
			db_entries_valid = false;
			ui_->tw_import_table->item(row_idx, 0)->setBackgroundColor(bg_red);
			ui_->tw_import_table->item(row_idx, 0)->setToolTip("Processed sample not found in NGSD!");
		}
		else
		{
			ui_->tw_import_table->item(row_idx, 0)->setBackgroundColor(bg_white);
			ui_->tw_import_table->item(row_idx, 0)->setToolTip("");
		}

		SampleData sample_info = db_.getSampleData(db_.sampleId(ps_name));
		if (!sample_info.is_tumor)
		{
			valid = false;
			db_entries_valid = false;
			ui_->tw_import_table->item(row_idx, 0)->setBackgroundColor(bg_red);
			ui_->tw_import_table->item(row_idx, 0)->setToolTip("Processed sample '" + ps_name + "' is not a tumor sample!");
		}
		else
		{
			ui_->tw_import_table->item(row_idx, 0)->setBackgroundColor(bg_white);
			ui_->tw_import_table->item(row_idx, 0)->setToolTip("");
		}

		//check processing system
		int sys_id = db_.processingSystemId(ui_->tw_import_table->item(row_idx, 1)->text(), false);
		if(sys_id == -1)
		{
			valid = false;
			db_entries_valid = false;
			ui_->tw_import_table->item(row_idx, 1)->setBackgroundColor(bg_red);
			ui_->tw_import_table->item(row_idx, 1)->setToolTip("Processing system not found in NGSD!");
		}
		else
		{
			ui_->tw_import_table->item(row_idx, 1)->setBackgroundColor(bg_white);
			ui_->tw_import_table->item(row_idx, 1)->setToolTip("");
		}

		//check if file exists
		if (!QFile::exists(ui_->tw_import_table->item(row_idx, 2)->text()))
		{
			valid = false;
			ui_->tw_import_table->item(row_idx, 2)->setBackgroundColor(bg_red);
			ui_->tw_import_table->item(row_idx, 2)->setToolTip("File does not exist!");
		}
		else
		{
			ui_->tw_import_table->item(row_idx, 2)->setToolTip(ui_->tw_import_table->item(row_idx, 2)->text());
			ui_->tw_import_table->item(row_idx, 2)->setBackgroundColor(bg_white);
		}

		//check for already existing panels
		if (db_entries_valid)
		{
			QList<CfdnaPanelInfo> existing_panel = db_.cfdnaPanelInfo(ps_id, sys_id);
			if (existing_panel.size() > 0)
			{
				ui_->tw_import_table->item(row_idx, 0)->setBackgroundColor(bg_orange);
				ui_->tw_import_table->item(row_idx, 0)->setToolTip("A cfDNA panel with this sample - processing system combination already exists!");
				ui_->tw_import_table->item(row_idx, 1)->setBackgroundColor(bg_orange);
				ui_->tw_import_table->item(row_idx, 1)->setToolTip("A cfDNA panel with this sample - processing system combination already exists!");

				// invalid if overwrite is not checked
				if (!ui_->cb_overwrite_existing->isChecked()) valid = false;
			}
			else
			{
				ui_->tw_import_table->item(row_idx, 0)->setBackgroundColor(bg_white);
				ui_->tw_import_table->item(row_idx, 0)->setToolTip("");
				ui_->tw_import_table->item(row_idx, 1)->setBackgroundColor(bg_white);
				ui_->tw_import_table->item(row_idx, 1)->setToolTip("");
			}
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
	QApplication::restoreOverrideCursor();
}

VcfFile CfDNAPanelBatchImport::createCfdnaPanelVcf(const QString& ps_name, const QString& vcf_file_path)
{
	// parse VCF
	QMap<QString, bool> selected_variants;
	VcfFile input_file;
	input_file.load(vcf_file_path, false);
	for (int i = 0; i < input_file.count(); ++i)
	{
		const VcfLine& vcf_line = input_file[i];

		// skip ID SNPs
		if (vcf_line.id().contains("ID")) continue;

		// create vcf pos string
		QString vcf_pos = vcf_line.chr().strNormalized(true) + ":" + QString::number(vcf_line.start()) + " " + vcf_line.ref() + ">" + vcf_line.altString();
		selected_variants.insert(vcf_pos, false);
	}

	// load reference
	FastaFileIndex genome_reference(Settings::string("reference_genome", false));

	// load processed sample GSvar file
	QString ps_id = db_.processedSampleId(ps_name);
	QStringList analyses;

	//single sample
	FileLocation single_sample_gsvar_file = GlobalServiceProvider::database().processedSamplePath(ps_id, PathType::GSVAR);
	if (single_sample_gsvar_file.exists) analyses << single_sample_gsvar_file.filename;

	//tumor-normal pair
	QString normal_sample = NGSD().normalSample(ps_id);
	if (normal_sample!="")
	{
		analyses << GlobalServiceProvider::database().secondaryAnalyses(ps_name + "-" + normal_sample, "somatic");
	}

	QString gsvar_filename;
	if (analyses.size() == 0)
	{
		THROW(FileAccessException, "No GSvar file found for processed sample '" +  ps_name + "'!");
	}
	else if (analyses.size() == 1)
	{
		gsvar_filename = analyses.at(0);
	}
	else //several analyses > let the user decide
	{
		//create list of anaylsis names
		QStringList names;
		foreach(QString gsvar, analyses)
		{
			VariantList vl;
			vl.loadHeaderOnly(gsvar);
			names << vl.analysisName();
		}

		//show selection dialog (analysis name instead of file name)
		bool ok = false;
		QString name = QInputDialog::getItem(this, "Several analyses of the sample present", "select analysis:", names, 0, false, &ok);
		if (!ok) THROW(FileAccessException, "No GSvar file selected, cancel import!");

		int index = names.indexOf(name);
		gsvar_filename = analyses[index];
	}

	VariantList gsvar;
	gsvar.load(gsvar_filename);


	// extract selected variants
	VariantList cfdna_panel;
	cfdna_panel.copyMetaData(gsvar);

	for (int i = 0; i < gsvar.count(); ++i)
	{
		const Variant& var = gsvar[i];
		VcfLine vcf_line = var.toVCF(genome_reference);
		// create vcf pos string
		QString vcf_pos = vcf_line.chr().strNormalized(true) + ":" + QString::number(vcf_line.start()) + " " + vcf_line.ref() + ">" + vcf_line.altString();
		if (selected_variants.contains(vcf_pos))
		{
			cfdna_panel.append(var);
			selected_variants[vcf_pos] = true;
		}
	}

	// check if all variants were found
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
		THROW(FileParseException, "The following variants were not found in GSvar file of sample '" + ps_name + "'\n\t" + missing_variants.join("\n\t"));
	}

	//mark all selected variants as monitoring
	VcfFile vcf_file = VcfFile::fromGSvar(cfdna_panel, Settings::string("reference_genome"));
	for (int i = 0; i < vcf_file.count(); ++i)
	{
		vcf_file[i].setId(QByteArrayList() << "M");
	}

	return vcf_file;
}

void CfDNAPanelBatchImport::showRawInputView()
{
	ui_->sw_import_panels->setCurrentIndex(0);
}

void CfDNAPanelBatchImport::showValidationTableView()
{
	fillTable();
	validateTable();
	ui_->sw_import_panels->setCurrentIndex(1);
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
		//remove quotes
		if ((table_line.vcf_file_path.startsWith("\"") && table_line.vcf_file_path.endsWith("\"")) || (table_line.vcf_file_path.startsWith("\'") && table_line.vcf_file_path.endsWith("\'")))
		{
			table_line.vcf_file_path = table_line.vcf_file_path.mid(1, table_line.vcf_file_path.length() - 2);
		}
		input_table_.append(table_line);
	}
}

void CfDNAPanelBatchImport::importPanels()
{
	// switch to import widget
	ui_->sw_import_panels->setCurrentIndex(2);

	// clear output log
	ui_->te_import_result->clear();
	writeToDbImportLog("Starting import...\n");

	// add sample identifier on demand
	bool add_sample_identifier = ui_->cb_add_sample_identifier->isChecked();
	QMap<int,VcfFile> proc_sys_sample_ids = QMap<int,VcfFile>();
	VcfFile general_sample_ids;
	if (add_sample_identifier)
	{
		// get KASP SNPs
		QStringList vcf_content = Helper::loadTextFile("://Resources/" + buildToString(GSvarHelper::build()) + "_KASP_set2.vcf", false,QChar::Null, false);
		general_sample_ids.fromText(vcf_content.join("\n").toUtf8());
	}

	//overwrite panels on demand
	bool overwrite_existing = ui_->cb_overwrite_existing->isChecked();

	try
	{
		// start mysql transaction
		db_.transaction();
		int n_imported_panels = 0;

		//iterate over table
		for (int row_idx = 0; row_idx < ui_->tw_import_table->rowCount(); ++row_idx)
		{
			// read table line
			QString ps_name = ui_->tw_import_table->item(row_idx, 0)->text();
			int processing_system_id = db_.processingSystemId(ui_->tw_import_table->item(row_idx, 1)->text());
			QString vcf_file_path = ui_->tw_import_table->item(row_idx, 2)->text();

			writeToDbImportLog("<p style=\"text-indent: 20px\">Importing cfDNA panel for processed sample " + ps_name + "... </p>");

			//create panel info
			CfdnaPanelInfo panel_info;
			if (overwrite_existing)
			{
				// get previous cfDNA panel
				QList<CfdnaPanelInfo> panel_list = db_.cfdnaPanelInfo(db_.processedSampleId(ps_name), processing_system_id);
				if (panel_list.size() > 0)
				{
					panel_info = panel_list.at(0);
				}
			}

			panel_info.tumor_id = Helper::toInt(db_.processedSampleId(ps_name));
			panel_info.created_by = LoginManager::userId();
			panel_info.created_date = QDate::currentDate();
			panel_info.processing_system_id = processing_system_id;


			// parse VCF and generate cfDNA panel
			VcfFile cfdna_panel = createCfdnaPanelVcf(ps_name, vcf_file_path);
			int n_monitoring = cfdna_panel.count();

			// append sample ids
			if (add_sample_identifier)
			{
				// add KASP SNPs
				for (int i=0; general_sample_ids.count(); ++i)
				{
					cfdna_panel.append(general_sample_ids[i]);
				}

				// add ID SNPs from processing system
				if (!proc_sys_sample_ids.contains(processing_system_id))
				{
					BedFile target_region = GlobalServiceProvider::database().processingSystemRegions(processing_system_id, false);
					proc_sys_sample_ids.insert(processing_system_id, db_.getIdSnpsFromProcessingSystem(processing_system_id, target_region, true));
				}
				for (int i=0; proc_sys_sample_ids.value(processing_system_id).count(); ++i)
				{
					cfdna_panel.append(proc_sys_sample_ids.value(processing_system_id)[i]);
				}

				cfdna_panel.sort();
			}

			// generate bed file
			BedFile cfdna_panel_region;
			for (int i=0; i<cfdna_panel.count(); i++)
			{
				QByteArray annotation = (cfdna_panel[i].id().contains("M"))?"patient_specific_somatic_variant:" : "SNP_for_sample_identification:";
				cfdna_panel_region.append(BedLine(cfdna_panel[i].chr(), cfdna_panel[i].start(), cfdna_panel[i].end(), QByteArrayList() << (annotation + cfdna_panel[i].ref() + ">" + cfdna_panel[i].altString() + ":" + cfdna_panel[i].info("ID_Source"))));
			}

			// import into NGSD
			db_.storeCfdnaPanel(panel_info, cfdna_panel_region.toText().toUtf8(), cfdna_panel.toText());

			writeToDbImportLog("<p style=\"text-indent: 40px\">import successful (" + QString::number(n_monitoring) + " monitoring and " + QString::number(cfdna_panel.count() - n_monitoring)
							   + " id SNPs)</p>");
			n_imported_panels++;
		}

		// if all panels were imported successfully -> apply changes to the database
		db_.commit();


		QMessageBox::information(this, "Import successful", QString::number(n_imported_panels) + " cfDNA panels successfully imported!");
	}
	catch (Exception& e)
	{
		writeToDbImportLog("Import of cfDNA panels failed!", true);
		writeToDbImportLog("ERROR:" + e.message(), true);

		// perform db rollback
		db_.rollback();

		QMessageBox::critical(this, "Import failed", "Error during importing of cfDNA panels!\n" + e.message() + "\n(See import output for details)");
	}


}
