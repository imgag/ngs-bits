#include "ProcessedSampleWidget.h"
#include "ui_ProcessedSampleWidget.h"
#include "DBQCWidget.h"
#include "GUIHelper.h"
#include "DiagnosticStatusWidget.h"
#include "DiseaseInfoWidget.h"
#include "SampleDiseaseInfoWidget.h"
#include "SampleRelationDialog.h"
#include "ProcessedSampleDataDeletionDialog.h"
#include "SingleSampleAnalysisDialog.h"
#include "DBEditor.h"
#include "GSvarHelper.h"
#include "LoginManager.h"
#include "GlobalServiceProvider.h"
#include <QMessageBox>
#include "GlobalServiceProvider.h"
#include "AnalysisInformationWidget.h"
#include "ExpressionDataWidget.h"
#include "FusionWidget.h"
#include "GenLabImportDialog.h"
#include "GenLabDB.h"

ProcessedSampleWidget::ProcessedSampleWidget(QWidget* parent, QString ps_id)
	: QWidget(parent)
	, ui_(new Ui::ProcessedSampleWidget)
	, ps_id_(ps_id)
{
	ui_->setupUi(this);
	GUIHelper::styleSplitter(ui_->splitter);
	connect(ui_->folder_btn, SIGNAL(clicked(bool)), this, SLOT(openSampleFolder()));
	connect(ui_->run, SIGNAL(linkActivated(QString)), this, SLOT(openRunTab(QString)));
	connect(ui_->system, SIGNAL(linkActivated(QString)), this, SLOT(openProcessingSystemTab(QString)));
	connect(ui_->project, SIGNAL(linkActivated(QString)), this, SLOT(openProjectTab(QString)));
	connect(ui_->open_btn, SIGNAL(clicked(bool)), this, SLOT(loadVariantList()));
	connect(ui_->qc_all, SIGNAL(stateChanged(int)), this, SLOT(updateQCMetrics()));
	connect(ui_->update_btn, SIGNAL(clicked(bool)), this, SLOT(updateGUI()));
	connect(ui_->diag_status_edit_btn, SIGNAL(clicked(bool)), this, SLOT(editDiagnosticStatus()));
	connect(ui_->disease_details_edit_btn, SIGNAL(clicked(bool)), this, SLOT(editDiseaseDetails()));
	connect(ui_->sample_edit_btn, SIGNAL(clicked(bool)), this, SLOT(editSample()));
	connect(ui_->edit_btn, SIGNAL(clicked(bool)), this, SLOT(edit()));
	connect(ui_->delete_btn, SIGNAL(clicked(bool)), this, SLOT(deleteSampleData()));
	connect(ui_->relation_add_btn, SIGNAL(clicked(bool)), this, SLOT(addRelation()));
	connect(ui_->relation_delete_btn, SIGNAL(clicked(bool)), this, SLOT(removeRelation()));
	connect(ui_->study_edit_btn, SIGNAL(clicked(bool)), this, SLOT(editStudy()));
	connect(ui_->study_add_btn, SIGNAL(clicked(bool)), this, SLOT(addStudy()));
	connect(ui_->study_delete_btn, SIGNAL(clicked(bool)), this, SLOT(removeStudy()));
	connect(ui_->merged, SIGNAL(linkActivated(QString)), this, SLOT(openProcessedSampleTab(QString)));
	connect(ui_->normal_sample, SIGNAL(linkActivated(QString)), this, SLOT(openProcessedSampleTab(QString)));
	connect(ui_->reanalyze_btn, SIGNAL(clicked(bool)), this, SLOT(queueSampleAnalysis()));
	connect(ui_->analysis_info_btn, SIGNAL(clicked(bool)), this, SLOT(showAnalysisInfo()));
	connect(ui_->genlab_import_btn, SIGNAL(clicked(bool)), this, SLOT(genLabImportDialog()));
	ui_->genlab_import_btn->setEnabled(GenLabDB::isAvailable());

	//check user has access rights
	NGSD db;
	if (!db.userCanAccess(LoginManager::userId(), ps_id.toInt()))
	{
		INFO(AccessDeniedException, "You do not have permissions to open this sample!");
	}


	//QC value > plot
	QAction* action = new QAction(QIcon(":/Icons/chart.png"), "Plot", this);
	ui_->qc_table->addAction(action);
	connect(action, SIGNAL(triggered(bool)), this, SLOT(showPlot()));

	//sample realtions > open sample
	action = new QAction(QIcon(":/Icons/NGSD_sample.png"), "Open processed sample tab", this);
	ui_->sample_relations->addAction(action);
	connect(action, SIGNAL(triggered(bool)), this, SLOT(openSampleTab()));

	//sample details > open external data sources
	action = new QAction(QIcon(":/Icons/Link.png"), "Open external database (if available)", this);
	ui_->disease_details->addAction(action);
	connect(action, SIGNAL(triggered(bool)), this, SLOT(openExternalDiseaseDatabase()));

	// determine sample type
	QString sample_type = db.getSampleData(db.sampleId(db.processedSampleName(ps_id_))).type;

	QMenu* menu = new QMenu();
	addIgvMenuEntry(menu, PathType::BAM);

	if(sample_type == "cfDNA")
	{
		menu->addSeparator();
		addIgvMenuEntry(menu, PathType::VCF_CF_DNA);
	}
	else if(sample_type == "RNA")
	{
		menu->addSeparator();
		addIgvMenuEntry(menu, PathType::FUSIONS_BAM);
		addIgvMenuEntry(menu, PathType::SPLICING_BED);
	}
	else if(sample_type.startsWith("DNA"))
	{
		addIgvMenuEntry(menu, PathType::LOWCOV_BED);
		addIgvMenuEntry(menu, PathType::BAF);
		menu->addSeparator();
		addIgvMenuEntry(menu, PathType::VCF);
		addIgvMenuEntry(menu, PathType::COPY_NUMBER_RAW_DATA);
		addIgvMenuEntry(menu, PathType::STRUCTURAL_VARIANTS);
		menu->addSeparator();
		addIgvMenuEntry(menu, PathType::MANTA_EVIDENCE);
	}

	ui_->igv_btn->setMenu(menu);

	//init RNA menu
	ui_->rna_btn->setEnabled(false);
	if(sample_type == "RNA")
	{
		QMenu* rna_menu = new QMenu();

		QAction* expr_action = rna_menu->addAction("open RNA expression data widget", this, SLOT(openExpressionWidget()));
		expr_action->setEnabled(GlobalServiceProvider::database().processedSamplePath(ps_id_, PathType::EXPRESSION).exists);
		QAction* fusion_action = rna_menu->addAction("open RNA fusion widget", this, SLOT(openFusionWidget()));
		fusion_action->setEnabled(GlobalServiceProvider::database().processedSamplePath(ps_id_, PathType::FUSIONS).exists);

		ui_->rna_btn->setMenu(rna_menu);
		ui_->rna_btn->setEnabled(true);
	}

	updateGUI();
}

ProcessedSampleWidget::~ProcessedSampleWidget()
{
	delete ui_;
}

void ProcessedSampleWidget::styleQualityLabel(QLabel* label, const QString& quality)
{
	//init
	static QPixmap i_good = QPixmap(":/Icons/quality_good.png");
	static QPixmap i_medium = QPixmap(":/Icons/quality_medium.png");
	static QPixmap i_bad = QPixmap(":/Icons/quality_bad.png");
	static QPixmap i_na = QPixmap(":/Icons/quality_unset.png");

	//icon
	if (quality=="good") label->setPixmap(i_good);
	else if (quality=="medium") label->setPixmap(i_medium);
	else if (quality=="bad") label->setPixmap(i_bad);
	else label->setPixmap(i_na);

	//tooltip
	label->setToolTip(quality);
}

void ProcessedSampleWidget::updateGUI()
{
	NGSD db;

	//#### processed sample details ####
	ProcessedSampleData ps_data = db.getProcessedSampleData(ps_id_);
	styleQualityLabel(ui_->quality, ps_data.quality);
	ui_->name->setText(ps_data.name);
	ui_->comments_processed_sample->setText(ps_data.comments);
	QString name_short = db.getValue("SELECT name_short FROM processing_system WHERE name_manufacturer=:0", true, ps_data.processing_system).toString();
	ui_->system->setText("<a href=\"" + name_short + "\">"+ps_data.processing_system+"</a>");
	ui_->project->setText("<a href=\"" + ps_data.project_name + "\">"+ps_data.project_name+"</a>");
	QString run = ps_data.run_name;
	if (run.isEmpty())
	{
		ui_->r_quality->setVisible(false);
	}
	else
	{
		QString run_quality = db.getValue("SELECT quality FROM sequencing_run WHERE name=:0", true, run).toString();
		styleQualityLabel(ui_->r_quality, run_quality);
	}
	ui_->run->setText("<a href=\"" + run + "\">"+run+"</a>");
	ui_->merged->setText(mergedSamples());
	ui_->lab_operator->setText(ps_data.lab_operator);
	ui_->processing_input->setText(ps_data.processing_input);
	ui_->molarity->setText(ps_data.molarity);
	QString normal_sample = ps_data.normal_sample_name;
	ui_->normal_sample->setText("<a href=\"" + normal_sample + "\">"+normal_sample+"</a>");
	ui_->ancestry->setText(NGSHelper::populationCodeToHumanReadable(ps_data.ancestry));
	QStringList ancestry_details;
	ancestry_details << "Raw scores:";
	ancestry_details << "AFR (African): " + db.getValue("SELECT score_afr FROM processed_sample_ancestry WHERE processed_sample_id="+ps_id_, true).toString();
	ancestry_details << "EUR (European): " + db.getValue("SELECT score_eur FROM processed_sample_ancestry WHERE processed_sample_id="+ps_id_, true).toString();
	ancestry_details << "SAS (South asian): " + db.getValue("SELECT score_sas FROM processed_sample_ancestry WHERE processed_sample_id="+ps_id_, true).toString();
	ancestry_details << "EAS (East asian): " + db.getValue("SELECT score_eas FROM processed_sample_ancestry WHERE processed_sample_id="+ps_id_, true).toString();
	ui_->ancestry->setToolTip(ancestry_details.join("\n"));

	//#### sample details ####
	QString s_id = db.getValue("SELECT sample_id FROM processed_sample WHERE id='" + ps_id_ + "'").toString();
	SampleData s_data = db.getSampleData(s_id);
	styleQualityLabel(ui_->s_quality, s_data.quality);
	ui_->s_name->setText(s_data.name);
	ui_->name_external->setText(s_data.name_external);
	ui_->patient_identifier->setText(s_data.patient_identifier);
	ui_->sender->setText(s_data.sender + " (received on " + s_data.received + " by " + s_data.received_by +")");
	ui_->species_type->setText(s_data.species + " / " + s_data.type);
	ui_->tumor_ffpe->setText(QString(s_data.is_tumor ? "<font color=red>yes</font>" : "no") + " / " + (s_data.is_ffpe ? "<font color=red>yes</font>" : "no"));
	ui_->gender->setText(s_data.gender);
	ui_->disease_group_status->setText(s_data.disease_group + " (" + s_data.disease_status + ")");
	ui_->tissue->setText(s_data.tissue);
	ui_->comments_sample->setText(s_data.comments);
	QStringList groups;
	foreach(SampleGroup group, s_data.sample_groups)
	{
		groups << group.name;
	}
	ui_->sample_groups->setText(groups.join(", "));

	//#### diagnostic status ####
	DiagnosticStatusData diag = db.getDiagnosticStatus(ps_id_);
	ui_->status->setText(diag.dagnostic_status + " (by " + diag.user + " on " + diag.date.toString("dd.MM.yyyy")+")");
	ui_->outcome->setText(diag.outcome);
	ui_->comments_diag->setText(diag.comments);
	ui_->report_config->setText(db.reportConfigSummaryText(ps_id_));


	//#### kasp status ####
	SqlQuery query = db.getQuery();
	query.exec("SELECT * FROM kasp_status WHERE processed_sample_id='" + ps_id_ + "'");
	if (query.next())
	{
		ui_->kasp_snps->setText(query.value("snps_match").toString() + "/" + query.value("snps_evaluated").toString());

		double swap_prob = 100.0 * query.value("random_error_prob").toDouble();
		if (swap_prob<0.0 || swap_prob>100.0)
		{
			ui_->kasp_swap->setText("KASP not performed (no DNA left for KASP, sample bad, or processing system does not contain SNPs of assay)");
		}
		else
		{
			QString value = QString::number(swap_prob, 'f', 4) + "%";
			if (swap_prob>1.1) value = "<font color=red>"+value+"</font>";
			ui_->kasp_swap->setText(value);
		}
	}
	else
	{
		ui_->kasp_snps->setText("n/a");
		ui_->kasp_swap->setText("n/a");
	}

	//#### disease details ####
	DBTable dd_table = db.createTable("sample_disease_info", "SELECT sdi.id, sdi.type, sdi.disease_info, u.name, sdi.date, t.name as hpo_name FROM user u, processed_sample ps, sample_disease_info sdi LEFT JOIN hpo_term t ON sdi.disease_info=t.hpo_id WHERE sdi.sample_id=ps.sample_id AND sdi.user_id=u.id AND ps.id='" + ps_id_ + "' ORDER BY sdi.date ASC");
	//append merge HPO id and name
	QStringList hpo_names = dd_table.takeColumn(dd_table.columnIndex("hpo_name"));
	QStringList info_entries = dd_table.extractColumn(dd_table.columnIndex("disease_info"));
	for (int i=0; i<info_entries.count(); ++i)
	{
		if (info_entries[i].startsWith("HP:"))
		{
			info_entries[i] += " (" + hpo_names[i] + ")";
		}
	}
	dd_table.setColumn(dd_table.columnIndex("disease_info"), info_entries);
	ui_->disease_details->setData(dd_table);


	//#### sample relations ####
	DBTable rel_table = db.createTable("sample_relations", "SELECT id, (SELECT name FROM sample WHERE id=sample1_id), (SELECT sample_type FROM sample WHERE id=sample1_id), relation, (SELECT name FROM sample WHERE id=sample2_id), (SELECT sample_type  FROM sample WHERE id=sample2_id), (SELECT name FROM user WHERE id=sample_relations.user_id), date FROM sample_relations WHERE sample1_id='" + s_id + "' OR sample2_id='" + s_id + "'");
	rel_table.setHeaders(QStringList() << "sample 1" << "type 1" << "relation" << "sample 2" << "type 2" << "added_by" << "added_date");
	ui_->sample_relations->setData(rel_table);

	//#### studies ####
	DBTable study_table = db.createTable("study_sample", "SELECT id, (SELECT name FROM study WHERE id=study_id), study_sample_idendifier FROM study_sample WHERE processed_sample_id='" + ps_id_ + "'");
	study_table.setHeaders(QStringList() << "study" << "study sample identifier");
	ui_->studies->setData(study_table);

	//#### QC ####
	updateQCMetrics();
}

void ProcessedSampleWidget::updateQCMetrics()
{
	try
	{
		NGSD db;
		QString sample_type = db.getSampleData(db.sampleId(db.processedSampleName(ps_id_))).type;

		//create table
		QString conditions;
		if (!ui_->qc_all->isChecked())
		{
			QStringList preferred_qc_parameters = limitedQCParameter(sample_type);
			conditions = "AND (t.qcml_id IN ('" + preferred_qc_parameters.join("', '") + "'))";
//			conditions = "AND (t.qcml_id='QC:2000007' OR 'QC:2000008' OR t.qcml_id='QC:2000010' OR t.qcml_id='QC:2000013' OR t.qcml_id='QC:2000014' OR t.qcml_id='QC:2000015' OR t.qcml_id='QC:2000016' OR t.qcml_id='QC:2000017' OR t.qcml_id='QC:2000018' OR t.qcml_id='QC:2000020' OR t.qcml_id='QC:2000021' OR t.qcml_id='QC:2000022' OR t.qcml_id='QC:2000023' OR t.qcml_id='QC:2000024' OR t.qcml_id='QC:2000025' OR t.qcml_id='QC:2000027' OR t.qcml_id='QC:2000049' OR t.qcml_id='QC:2000050' OR t.qcml_id='QC:2000051')";
		}
		DBTable qc_table = db.createTable("processed_sample_qc", "SELECT qc.id, t.qcml_id, t.name, qc.value, t.description FROM processed_sample_qc qc, qc_terms t WHERE qc.qc_terms_id=t.id AND t.obsolete=0 AND qc.processed_sample_id='" + ps_id_ + "' " + conditions + " ORDER BY t.qcml_id ASC");

		//use descriptions as tooltip
		QStringList descriptions = qc_table.takeColumn(qc_table.columnIndex("description"));
		ui_->qc_table->setData(qc_table);
		ui_->qc_table->setColumnTooltips("name", descriptions);

		//colors
		QString sys_type = db.getValue("SELECT sys.type FROM processed_sample ps, processing_system sys WHERE ps.processing_system_id=sys.id AND ps.id='"+ps_id_+"'").toString();
		QColor orange = QColor(255,150,0,125);
		QColor red = QColor(255,0,0,125);
		QList<QColor> colors;
		for (int r=0; r<qc_table.rowCount(); ++r)
		{
			QColor color;
			bool ok;
			double value = qc_table.row(r).value(2).toDouble(&ok);
			if (ok)
			{
				QString accession = qc_table.row(r).value(0);
				if (accession=="QC:2000014") //known variants %
				{
					if (value<95) color = orange;
					if (value<90) color = red;
				}
				else if (accession=="QC:2000025") //avg depth
				{
					if (sys_type=="WGS")
					{
						if (value<35) color = orange;
						if (value<30) color = red;
					}
					else
					{
						if (value<80) color = orange;
						if (value<50) color = red;
					}
				}
				else if (accession=="QC:2000027") //cov 20x
				{
					if (value<95) color = orange;
					if (value<90) color = red;
				}
				else if (accession=="QC:2000051") //AF deviation
				{
					if (value>3) color = orange;
					if (value>6) color = red;
				}
				else if(accession=="QC:2000045") //known somatic variants percentage
				{
					if (value>4) color = orange;
					if (value>5) color = red;
				}
			}

			colors << color;
		}
		ui_->qc_table->setColumnColors("value", colors);
	}
	catch (Exception& e)
	{
		QMessageBox::warning(this, "Update failed", "Could not update data:\n" + e.message());
	}
}

void ProcessedSampleWidget::showPlot()
{
	NGSD db;

	QList<int> selected_rows = ui_->qc_table->selectedRows().toList();
	if (selected_rows.count()<1 || selected_rows.count()>2)
	{
		QMessageBox::information(this, "Plot error", "Please select <b>one or two</b> quality metric for plotting!");
		return;
	}

	//get database IDs
	QString qc_term_id = db.getValue("SELECT qc_terms_id FROM processed_sample_qc WHERE id='" + ui_->qc_table->getId(selected_rows[0]) + "'").toString();
	QString sys_id = db.getValue("SELECT ps.processing_system_id FROM processed_sample_qc qc, processed_sample ps WHERE qc.processed_sample_id=ps.id AND qc.id='" + ui_->qc_table->getId(selected_rows[0]) + "'").toString();


	//show widget
	DBQCWidget* qc_widget = new DBQCWidget(this);
	qc_widget->addHighlightedProcessedSampleById(ps_id_);
	qc_widget->setSystemId(sys_id);
	qc_widget->setTermId(qc_term_id);
	if (selected_rows.count()==2)
	{
		QString qc_term_id2 = db.getValue("SELECT qc_terms_id FROM processed_sample_qc WHERE id='" + ui_->qc_table->getId(selected_rows[1]) + "'").toString();
		qc_widget->setSecondTermId(qc_term_id2);
	}
	auto dlg = GUIHelper::createDialog(qc_widget, "QC plot");
	dlg->exec();
}

void ProcessedSampleWidget::openSampleFolder()
{
	FileLocation folder = GlobalServiceProvider::database().processedSamplePath(ps_id_, PathType::SAMPLE_FOLDER);
	if (folder.isHttpUrl())
	{
		QMessageBox::information(this, "Open processed sample folder", "Cannot open processed sample folder in client-server mode!");
		return;
	}
	else if (!QFile::exists(folder.filename))
	{
		QMessageBox::warning(this, "Error opening processed sample folder", "Folder does not exist:\n" + folder.filename);
		return;
	}
	QDesktopServices::openUrl(QUrl(folder.filename));
}

void ProcessedSampleWidget::openSampleTab()
{
	NGSD db;

	//check that a relation is selected
	QList<int> selected_rows = ui_->sample_relations->selectedRows().toList();
	if (selected_rows.isEmpty())
	{
		QMessageBox::warning(this, "Sample relation - processed sample tab", "Please select at least one relation!");
		return;
	}

	//determine processed sample names
	QStringList ps_names;
	QString s_name = db.getValue("SELECT s.name FROM sample s, processed_sample ps WHERE ps.sample_id=s.id AND ps.id='" + ps_id_ + "'").toString();
	foreach(int row, selected_rows)
	{
		QString s = ui_->sample_relations->item(row, 0)->text();
		if (s==s_name)
		{
			s = ui_->sample_relations->item(row, 3)->text();
		}

		QStringList tmp = db.getValues("SELECT ps.id FROM processed_sample ps, sample s WHERE ps.sample_id=s.id AND s.name=:0 AND ps.id NOT IN (SELECT processed_sample_id FROM merged_processed_samples)", s);
		foreach(QString ps_id, tmp)
		{
			ps_names << db.processedSampleName(ps_id);
		}
	}

	//open tabs
	if (ps_names.count()==0)
	{
		QMessageBox::information(this, "No sample", "No processed sample found for this sample!");
	}
	else if (ps_names.count()==1)
	{
		GlobalServiceProvider::openProcessedSampleTab(ps_names[0]);
	}
	else if (ps_names.count()>0)
	{
		bool ok;
		QString ps = QInputDialog::getItem(this, "Select processed sample", "sample:", ps_names, 0, false, &ok);
		if (ok)
		{
			GlobalServiceProvider::openProcessedSampleTab(ps);
		}
	}
}

void ProcessedSampleWidget::openExternalDiseaseDatabase()
{
	QList<int> selected_rows = ui_->disease_details->selectedRows().toList();
	foreach(int row, selected_rows)
	{
		QString type = ui_->disease_details->item(row, 0)->text();
		QString value = ui_->disease_details->item(row, 1)->text();

		QString link;
		if (type=="ICD10 code")
		{
			link = "https://www.icd-code.de/suche/icd/recherche.html?sp=" + value;
		}
		else if (type=="HPO term id")
		{
			value = value.left(10); //extract identifier
			link = "https://hpo.jax.org/app/browse/term/" + value;
		}
		else if (type=="OMIM disease/phenotype identifier")
		{
			value.replace("#", ""); //remove prefix
			link = "https://omim.org/entry/" + value;
		}
		else if (type=="Orpha number")
		{
			value.replace("ORPHA:", ""); //remove prefix
			link = "https://www.orpha.net/consor/cgi-bin/OC_Exp.php?lng=en&Expert=" + value;
		}

		if (!link.isEmpty())
		{
			QDesktopServices::openUrl(QUrl(link));
		}
	}
}

void ProcessedSampleWidget::addRelation()
{
	//show dialog
	SampleRelationDialog* dlg = new SampleRelationDialog(this);
	dlg->setSample1(sampleName(), false);
	if (dlg->exec()!=QDialog::Accepted) return;

	//add relation
	NGSD().addSampleRelation(dlg->sampleRelation());

	//update GUI
	updateGUI();
}

void ProcessedSampleWidget::removeRelation()
{
	//check that a relation is selected
	QList<int> selected_rows = ui_->sample_relations->selectedRows().toList();
	if (selected_rows.isEmpty())
	{
		QMessageBox::warning(this, "Sample relation", "Please select a relation!");
		return;
	}

	//make sure the user did not click accidentally on the button
	QMessageBox::StandardButton reply = QMessageBox::question(this, "Deleting sample relations", "Are you sure you want to delete the selected relation?", QMessageBox::Yes|QMessageBox::No);
	if (reply==QMessageBox::No) return;

	//delete relations
	NGSD db;
	QString id = ui_->sample_relations->getId(selected_rows[0]);
	db.getQuery().exec("DELETE FROM sample_relations WHERE id='" + id + "'");

	//update GUI
	updateGUI();
}

void ProcessedSampleWidget::editStudy()
{
	//check that a study
	QList<int> selected_rows = ui_->studies->selectedRows().toList();
	if (selected_rows.isEmpty())
	{
		QMessageBox::warning(this, "Study", "Please select a study!");
		return;
	}

	//get new sample id
	NGSD db;
	QString id = ui_->studies->getId(selected_rows[0]);
	QString study_sample_idendifier = db.getValue("SELECT study_sample_idendifier FROM study_sample WHERE id=" + id).toString();
	QString study_sample_idendifier_new = QInputDialog::getText(this, "Study", "sample identifier in the study (optional):", QLineEdit::Normal, study_sample_idendifier);

	//update study
	SqlQuery query = db.getQuery();
	query.prepare("UPDATE study_sample SET study_sample_idendifier=:0 WHERE id=" + id);
	query.bindValue(0, study_sample_idendifier_new);
	query.exec();

	//update GUI
	updateGUI();
}

void ProcessedSampleWidget::addStudy()
{
	NGSD db;

	//select study
	DBComboBox* widget = new DBComboBox(this);
	widget->fill(db.createTable("study", "SELECT id, name FROM study ORDER BY name ASC"), false);
	auto dlg = GUIHelper::createDialog(widget, "Study", "select study:", true);
	dlg->exec();
	QString study_id = widget->getCurrentId();
	if (study_id.isEmpty()) return;

	//set study sample identifier
	QString sample_identifier = QInputDialog::getText(this, "Study", "sample identifier in the study (optional):");

	//add study
	SqlQuery query = db.getQuery();
	query.prepare("INSERT INTO `study_sample`(`study_id`, `processed_sample_id`, `study_sample_idendifier`) VALUES (:0,:1,:2)");
	query.bindValue(0, study_id);
	query.bindValue(1, ps_id_);
	query.bindValue(2, sample_identifier);
	query.exec();

	//update GUI
	updateGUI();
}

void ProcessedSampleWidget::removeStudy()
{
	//check that a study
	QList<int> selected_rows = ui_->studies->selectedRows().toList();
	if (selected_rows.isEmpty())
	{
		QMessageBox::warning(this, "Study deletion", "Please select a study!");
		return;
	}

	//make sure the user did not click accidentally on the button
	QMessageBox::StandardButton reply = QMessageBox::question(this, "Deleting study", "Are you sure you want to delete the selected study?", QMessageBox::Yes|QMessageBox::No);
	if (reply==QMessageBox::No) return;

	//delete study
	NGSD db;
	QString id = ui_->studies->getId(selected_rows[0]);
	db.getQuery().exec("DELETE FROM study_sample WHERE id='" + id + "'");

	//update GUI
	updateGUI();
}

void ProcessedSampleWidget::deleteSampleData()
{
	ProcessedSampleDataDeletionDialog* dlg = new ProcessedSampleDataDeletionDialog(this, QStringList() << ps_id_);

	connect(dlg, SIGNAL(somRepDeleted()), this, SLOT(somRepDeleted()));

	dlg->exec();
}

void ProcessedSampleWidget::loadVariantList()
{
	GlobalServiceProvider::openGSvarViaNGSD(NGSD().processedSampleName(ps_id_), true);
}

void ProcessedSampleWidget::addIgvMenuEntry(QMenu* menu, PathType file_type)
{
	QAction* action = menu->addAction(FileLocation::typeToHumanReadableString(file_type), this, SLOT(openIgvTrack()));
	action->setData((int)file_type);
	action->setEnabled(GlobalServiceProvider::database().processedSamplePath(ps_id_, file_type).exists);
}

void ProcessedSampleWidget::openIgvTrack()
{
	QAction* action = qobject_cast<QAction*>(sender());
	PathType type = static_cast<PathType>(action->data().toInt());

	QString file = GlobalServiceProvider::database().processedSamplePath(ps_id_, type).filename;
	GlobalServiceProvider::loadFileInIGV(file, false);
}

void ProcessedSampleWidget::somRepDeleted()
{
	emit clearMainTableSomReport(ps_id_);
}

void ProcessedSampleWidget::openProcessedSampleTab(QString ps)
{
	GlobalServiceProvider::openProcessedSampleTab(ps);
}

void ProcessedSampleWidget::openRunTab(QString name)
{
	GlobalServiceProvider::openRunTab(name);
}

void ProcessedSampleWidget::openProjectTab(QString project_name)
{
	GlobalServiceProvider::openProjectTab(project_name);
}

void ProcessedSampleWidget::openProcessingSystemTab(QString system_short_name)
{
	GlobalServiceProvider::openProcessingSystemTab(system_short_name);
}

void ProcessedSampleWidget::openExpressionWidget()
{
	FileLocation file_location = GlobalServiceProvider::database().processedSamplePath(ps_id_, PathType::EXPRESSION);
	if (file_location.exists)
	{
		NGSD db;
		int sys_id = db.processingSystemIdFromProcessedSample(processedSampleName());
		QString tissue = db.getSampleData(db.sampleId(sampleName())).tissue;
		ExpressionDataWidget* widget = new ExpressionDataWidget(file_location.filename, sys_id, tissue, this);
		auto dlg = GUIHelper::createDialog(widget, "Expression Data");
		dlg->exec();
	}
	else
	{
		QMessageBox::warning(this, "Expression data file not found!", "Couldn't find expression data file at '" + file_location.filename + "'!");
	}

}

void ProcessedSampleWidget::openFusionWidget()
{
	FileLocation file_location = GlobalServiceProvider::database().processedSamplePath(ps_id_, PathType::FUSIONS);
	if (file_location.exists)
	{
		FusionWidget* widget = new FusionWidget(file_location.filename, this);
		auto dlg = GUIHelper::createDialog(widget, "Fusions of " + processedSampleName() + " (arriba)");
		dlg->exec();
	}
	else
	{
		QMessageBox::warning(this, "Fusion data file not found!", "Couldn't find fusions data file at '" + file_location.filename + "'!");
	}
}

void ProcessedSampleWidget::editSample()
{
	int sample_id = NGSD().sampleId(sampleName()).toInt();

	DBEditor* widget = new DBEditor(this, "sample", sample_id);
	auto dlg = GUIHelper::createDialog(widget, "Edit sample " + sampleName() ,"", true);
	if (dlg->exec()==QDialog::Accepted)
	{
		widget->store();
		updateGUI();
	}
}

void ProcessedSampleWidget::editDiagnosticStatus()
{
	NGSD db;

	DiagnosticStatusWidget* widget = new DiagnosticStatusWidget(this);
	widget->setStatus(db.getDiagnosticStatus(ps_id_));
	auto dlg = GUIHelper::createDialog(widget, "Diagnostic status of " + processedSampleName(), "", true);
	if (dlg->exec()!=QDialog::Accepted) return;

	db.setDiagnosticStatus(ps_id_, widget->status());
	updateGUI();
}

void ProcessedSampleWidget::editDiseaseDetails()
{
	NGSD db;

	QString sample_id = db.sampleId(processedSampleName());

	SampleDiseaseInfoWidget* widget = new SampleDiseaseInfoWidget(processedSampleName(), this);
	widget->setDiseaseInfo(db.getSampleDiseaseInfo(sample_id));
	auto dlg = GUIHelper::createDialog(widget, "Sample disease details", "", true);
	if (dlg->exec() != QDialog::Accepted) return;

	db.setSampleDiseaseInfo(sample_id, widget->diseaseInfo());
	updateGUI();
}

void ProcessedSampleWidget::genLabImportDialog()
{
	try
	{
		GenLabImportDialog dlg(ps_id_, this);

		if (dlg.exec()==QDialog::Accepted)
		{
			dlg.importSelectedData();
			updateGUI();
		}
	}
	catch(Exception& e)
	{
		GUIHelper::showException(this, e, "GenLab data import");
	}
}

void ProcessedSampleWidget::edit()
{
	DBEditor* widget = new DBEditor(this, "processed_sample", ps_id_.toInt());
	auto dlg = GUIHelper::createDialog(widget, "Edit processed sample " + processedSampleName() ,"", true);
	if (dlg->exec()==QDialog::Accepted)
	{
		widget->store();
		updateGUI();
	}
}

QString ProcessedSampleWidget::sampleName() const
{
	return ui_->s_name->text();
}

QString ProcessedSampleWidget::processedSampleName() const
{
	return ui_->name->text();
}

QString ProcessedSampleWidget::mergedSamples() const
{
	NGSD db;

	QStringList output;

	//other samples merged into this sample
	QStringList merged_ps_ids = db.getValues("SELECT processed_sample_id FROM merged_processed_samples WHERE merged_into='" + ps_id_ + "'");
	foreach(QString ps_id, merged_ps_ids)
	{
		QString ps_name = db.processedSampleName(ps_id);
		output << ("<a href=\"" + ps_name + "\">"+ps_name+"</a> was merged into this sample");
	}

	//this sample merged into other sample
	QString ps_id = db.getValue("SELECT merged_into FROM merged_processed_samples WHERE processed_sample_id='" + ps_id_ + "'").toString();
	if (ps_id!="")
	{
		QString ps_name = db.processedSampleName(ps_id);
		output << ("<font color='red'>this sample was merged into</font> <a href=\"" + ps_name + "\">"+ps_name+"</a>");
	}

	return output.join(", ");
}

QStringList ProcessedSampleWidget::limitedQCParameter(const QString& sample_type)
{
	QStringList parameter_list;

	// add common parameter
	parameter_list << "QC:2000007"; // Q20 read percentage
	parameter_list << "QC:2000008"; // Q30 base percentage
	parameter_list << "QC:2000010"; // gc content percentage
	parameter_list << "QC:2000020"; // mapped read percentage
	parameter_list << "QC:2000021"; // on-target read percentage
	parameter_list << "QC:2000025"; // target region read depth
	parameter_list << "QC:2000049"; // bases sequenced (MB)
	parameter_list << "QC:2000050"; // bases usable (MB)

	// add type-specific parameter
	if (sample_type == "DNA" || sample_type == "DNA (amplicon)" || sample_type == "DNA (native)")
	{
		parameter_list << "QC:2000013"; // variant count
		parameter_list << "QC:2000014"; // known variants percentage
		parameter_list << "QC:2000015"; // high-impact variants percentage
		parameter_list << "QC:2000016"; // homozygous variants percentage
		parameter_list << "QC:2000017"; // indel variants percentage
		parameter_list << "QC:2000018"; // transition/transversion ratio
		parameter_list << "QC:2000022"; // properly-paired read percentage
		parameter_list << "QC:2000023"; // insert size
		parameter_list << "QC:2000024"; // duplicate read percentage
		parameter_list << "QC:2000027"; // target region 20x percentage
		parameter_list << "QC:2000051"; // SNV allele frequency deviation
	}
	else if(sample_type == "cfDNA")
	{
		parameter_list << "QC:2000065"; // target region 1000x percentage
		parameter_list << "QC:2000067"; // target region 5000x percentage
		parameter_list << "QC:2000069"; // target region 10000x percentage
		parameter_list << "QC:2000071"; // target region read depth 2-fold duplication
		parameter_list << "QC:2000073"; // target region read depth 4-fold duplication
		parameter_list << "QC:2000077"; // monitoring variant read depth
		parameter_list << "QC:2000078"; // ID variant read depth
		parameter_list << "QC:2000079"; // monitoring variant count
		parameter_list << "QC:2000080"; // monitoring variant 250x percentage
		parameter_list << "QC:2000081"; // ID variant count
		parameter_list << "QC:2000082"; // ID variant 250x percentage
		parameter_list << "QC:2000083"; // cfDNA-tumor correlation
		parameter_list << "QC:2000084"; // cfDNA-cfDNA correlation

	}
	else if(sample_type == "RNA")
	{
		parameter_list << "QC:2000027"; // target region 20x percentage
		parameter_list << "QC:2000100"; // housekeeping genes read percentage
		parameter_list << "QC:2000101"; // housekeeping genes read depth
		parameter_list << "QC:2000103"; // housekeeping genes 20x percentage
		parameter_list << "QC:2000109"; // covered gene count
		parameter_list << "QC:2000110"; // aberrant spliced gene count
		parameter_list << "QC:2000111"; // outlier gene count
	}
	return parameter_list;
}

void ProcessedSampleWidget::queueSampleAnalysis()
{
	NGSD db;

	//prepare sample list
	QString ps_name = db.processedSampleName(ps_id_);
	QList<AnalysisJobSample> job_list;
	job_list << AnalysisJobSample {ps_name, ""};

	GSvarHelper::queueSampleAnalysis(AnalysisType::GERMLINE_SINGLESAMPLE, job_list, this);

}

void ProcessedSampleWidget::showAnalysisInfo()
{
	AnalysisInformationWidget* widget = new AnalysisInformationWidget(ps_id_, this);
	auto dlg = GUIHelper::createDialog(widget, "Analysis information of " + processedSampleName());
	dlg->exec();
}
