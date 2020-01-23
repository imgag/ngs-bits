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

#include <QMessageBox>

ProcessedSampleWidget::ProcessedSampleWidget(QWidget* parent, QString ps_id)
	: QWidget(parent)
	, ui_(new Ui::ProcessedSampleWidget)
	, ps_id_(ps_id)
{
	ui_->setupUi(this);
	GUIHelper::styleSplitter(ui_->splitter);
	connect(ui_->folder_btn, SIGNAL(clicked(bool)), this, SLOT(openSampleFolder()));
	connect(ui_->run, SIGNAL(linkActivated(QString)), this, SIGNAL(openRunTab(QString)));
	connect(ui_->system, SIGNAL(linkActivated(QString)), this, SIGNAL(openProcessingSystemTab(QString)));
	connect(ui_->project, SIGNAL(linkActivated(QString)), this, SIGNAL(openProjectTab(QString)));
	connect(ui_->open_btn, SIGNAL(clicked(bool)), this, SLOT(loadVariantList()));
	connect(ui_->qc_all, SIGNAL(stateChanged(int)), this, SLOT(updateQCMetrics()));
	connect(ui_->update_btn, SIGNAL(clicked(bool)), this, SLOT(updateGUI()));
	connect(ui_->diag_status_edit_btn, SIGNAL(clicked(bool)), this, SLOT(editDiagnosticStatus()));
	connect(ui_->disease_details_edit_btn, SIGNAL(clicked(bool)), this, SLOT(editDiseaseDetails()));
	connect(ui_->sample_edit_btn, SIGNAL(clicked(bool)), this, SLOT(editSample()));
	connect(ui_->relation_add_btn, SIGNAL(clicked(bool)), this, SLOT(addRelation()));
	connect(ui_->relation_delete_btn, SIGNAL(clicked(bool)), this, SLOT(removeRelation()));
	connect(ui_->merged, SIGNAL(linkActivated(QString)), this, SIGNAL(openProcessedSampleTab(QString)));
	connect(ui_->normal_sample, SIGNAL(linkActivated(QString)), this, SIGNAL(openProcessedSampleTab(QString)));
	connect(ui_->reanalyze_btn, SIGNAL(clicked(bool)), this, SLOT(queueSampleAnalysis()));
	connect(ui_->genlab_btn, SIGNAL(clicked(bool)), this, SLOT(editDiseaseGroupAndInfo()));

	//QC value > plot
	QAction* action = new QAction("Plot", this);
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

	//sample edit button
	QMenu* menu = new QMenu();
	menu->addAction("Edit", this, SLOT(edit()));
	menu->addSeparator();
	menu->addAction(QIcon(":/Icons/Remove.png"), "Delete associated data", this, SLOT(deleteSampleData()));
	ui_->edit_btn->setMenu(menu);

	//IGV button
	menu = new QMenu();
	menu->addAction("Add BAM track", this, SLOT(addBamToIgv()));
	menu->addSeparator();
	menu->addAction("Add variant track", this, SLOT(addVariantsToIgv()));
	menu->addAction("Add CNV track", this, SLOT(addCnvsToIgv()));
	menu->addAction("Add SV track", this, SLOT(addSvsToIgv()));
	menu->addSeparator();
	menu->addAction("Add BAF track", this, SLOT(addBafsToIgv()));
	ui_->igv_btn->setMenu(menu);

	updateGUI();
}
ProcessedSampleWidget::~ProcessedSampleWidget()
{
	delete ui_;
}

void ProcessedSampleWidget::styleQualityLabel(QLabel* label, const QString& quality)
{
	//icon
	QString filename = ":/Icons/quality_unset.png";
	if (quality=="good") filename = ":/Icons/quality_good.png";
	else if (quality=="medium") filename = ":/Icons/quality_medium.png";
	else if (quality=="bad") filename = ":/Icons/quality_bad.png";
	label->setPixmap(QPixmap(filename));

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
	QString name_short = db.getValue("SELECT name_short FROM processing_system WHERE name_manufacturer='" + ps_data.processing_system + "'").toString();
	ui_->system->setText("<a href=\"" + name_short + "\">"+ps_data.processing_system+"</a>");
	ui_->project->setText("<a href=\"" + ps_data.project_name + "\">"+ps_data.project_name+"</a>");
	QString run = ps_data.run_name;
	QString run_quality = db.getValue("SELECT quality FROM sequencing_run WHERE name='" + run + "'").toString();
	styleQualityLabel(ui_->r_quality, run_quality);
	ui_->run->setText("<a href=\"" + run + "\">"+run+"</a>");
	ui_->merged->setText(mergedSamples());
	ui_->lab_operator->setText(ps_data.lab_operator);
	ui_->processing_input->setText(ps_data.processing_input);
	ui_->molarity->setText(ps_data.molarity);
	QString normal_sample = ps_data.normal_sample_name;
	ui_->normal_sample->setText("<a href=\"" + normal_sample + "\">"+normal_sample+"</a>");

	//#### sample details ####
	QString s_id = db.getValue("SELECT sample_id FROM processed_sample WHERE id='" + ps_id_ + "'").toString();
	SampleData s_data = db.getSampleData(s_id);
	styleQualityLabel(ui_->s_quality, s_data.quality);
	ui_->s_name->setText(s_data.name);
	ui_->name_external->setText(s_data.name_external);
	ui_->sender->setText(s_data.sender + " (received on " + s_data.received + " by " + s_data.received_by +")");
	ui_->species_type->setText(s_data.species + " / " + s_data.type);
	ui_->tumor_ffpe->setText(QString(s_data.is_tumor ? "<font color=red>yes</font>" : "no") + " / " + (s_data.is_ffpe ? "<font color=red>yes</font>" : "no"));
	ui_->gender->setText(s_data.gender);
	ui_->disease_group_status->setText(s_data.disease_group + " (" + s_data.disease_status + ")");
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
			if (swap_prob>1.0) value = "<font color=red>"+value+"%</font>";
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
	DBTable rel_table = db.createTable("sample_relations", "SELECT id, (SELECT name FROM sample WHERE id=sample1_id), (SELECT sample_type FROM sample WHERE id=sample1_id), relation, (SELECT name FROM sample WHERE id=sample2_id), (SELECT sample_type  FROM sample WHERE id=sample2_id) FROM sample_relations WHERE sample1_id='" + s_id + "' OR sample2_id='" + s_id + "'");
	rel_table.setHeaders(QStringList() << "sample 1" << "type 1" << "relation" << "sample 2" << "type 2");
	ui_->sample_relations->setData(rel_table);

	//#### QC ####
	updateQCMetrics();
}

void ProcessedSampleWidget::updateQCMetrics()
{
	try
	{
		QString conditions;
		if (!ui_->qc_all->isChecked())
		{
			conditions = "AND (t.qcml_id='QC:2000007' OR 'QC:2000008' OR t.qcml_id='QC:2000010' OR t.qcml_id='QC:2000013' OR t.qcml_id='QC:2000014' OR t.qcml_id='QC:2000015' OR t.qcml_id='QC:2000016' OR t.qcml_id='QC:2000017' OR t.qcml_id='QC:2000018' OR t.qcml_id='QC:2000020' OR t.qcml_id='QC:2000021' OR t.qcml_id='QC:2000022' OR t.qcml_id='QC:2000023' OR t.qcml_id='QC:2000024' OR t.qcml_id='QC:2000025' OR t.qcml_id='QC:2000027' OR t.qcml_id='QC:2000049' OR t.qcml_id='QC:2000050' OR t.qcml_id='QC:2000051')";
		}

		//create table
		DBTable qc_table = NGSD().createTable("processed_sample_qc", "SELECT qc.id, t.qcml_id, t.name, qc.value, t.description FROM processed_sample_qc qc, qc_terms t WHERE qc.qc_terms_id=t.id AND t.obsolete=0 AND qc.processed_sample_id='" + ps_id_ + "' " + conditions + " ORDER BY t.qcml_id ASC");

		//use descriptions as tooltip
		QStringList descriptions = qc_table.takeColumn(qc_table.columnIndex("description"));
		ui_->qc_table->setData(qc_table);
		ui_->qc_table->setColumnTooltips("name", descriptions);

		//colors
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
					if (value<80) color = orange;
					if (value<30) color = red;
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
	QString folder = NGSD().processedSamplePath(ps_id_, NGSD::SAMPLE_FOLDER);
	if(!QFile::exists(folder))
	{
		QMessageBox::warning(this, "Error opening processed sample folder", "Folder does not exist:\n" + folder);
		return;
	}
	QDesktopServices::openUrl(QUrl(folder));
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

		QStringList tmp = db.getValues("SELECT ps.id FROM processed_sample ps, sample s WHERE ps.sample_id=s.id AND s.name=:0", s);
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
		emit openProcessedSampleTab(ps_names[0]);
	}
	else if (ps_names.count()>0)
	{
		bool ok;
		QString ps = QInputDialog::getItem(this, "Select processed sample", "sample:", ps_names, 0, false, &ok);
		if (ok)
		{
			emit openProcessedSampleTab(ps);
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
			link = "http://www.icd-code.de/suche/icd/recherche.html?sp=" + value;
		}
		else if (type=="HPO term id")
		{
			value = value.left(10); //extract identifier
			link = "https://hpo.jax.org/app/browse/term/" + value;
		}
		else if (type=="OMIM disease/phenotype identifier")
		{
			value.replace("#", ""); //remove prefix
			link = "http://omim.org/entry/" + value;
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
	NGSD().getQuery().exec("INSERT INTO `sample_relations`(`sample1_id`, `relation`, `sample2_id`) VALUES (" + dlg->sample1Id() + ",'" + dlg->relation() + "'," + dlg->sample2Id() + ")");


	//update GUI
	updateGUI();
}

void ProcessedSampleWidget::removeRelation()
{
	//check that a relation is selected
	QList<int> selected_rows = ui_->sample_relations->selectedRows().toList();
	if (selected_rows.isEmpty())
	{
		QMessageBox::warning(this, "Sample relation - deletion", "Please select at least one relation!");
		return;
	}

	//make sure the user did not click accidentally on the button
	QMessageBox::StandardButton reply = QMessageBox::question(this, "Deleting sample relations", "Are you sure you want to delete the selected relation(s)?", QMessageBox::Yes|QMessageBox::No);
	if (reply==QMessageBox::No) return;

	//delete relations
	NGSD db;
	foreach(int row, selected_rows)
	{
		QString rel_id = ui_->sample_relations->getId(row);
		db.getQuery().exec("DELETE FROM sample_relations WHERE id='" + rel_id + "'");
	}

	//update GUI
	updateGUI();
}

void ProcessedSampleWidget::deleteSampleData()
{
	ProcessedSampleDataDeletionDialog* dlg = new ProcessedSampleDataDeletionDialog(this, QStringList() << ps_id_);
	dlg->exec();
}

void ProcessedSampleWidget::loadVariantList()
{
	emit openProcessedSampleFromNGSD(NGSD().processedSampleName(ps_id_));
}

void ProcessedSampleWidget::addBamToIgv()
{
	QString bam = NGSD().processedSamplePath(ps_id_, NGSD::BAM);

	executeIGVCommands(QStringList() << "load \"" + QDir::toNativeSeparators(bam) + "\"");
}

void ProcessedSampleWidget::addVariantsToIgv()
{
	QString vcf = NGSD().processedSamplePath(ps_id_, NGSD::VCF);
	executeIGVCommands(QStringList() << "load \"" + QDir::toNativeSeparators(vcf) + "\"");
}

void ProcessedSampleWidget::addCnvsToIgv()
{
	QString bam = NGSD().processedSamplePath(ps_id_, NGSD::BAM);

	QString base_name = bam.left(bam.length()-4);
	QString segfile = base_name + "_cnvs_clincnv.seg";
	if (QFile::exists(segfile))
	{
		executeIGVCommands(QStringList() << "load \"" + QDir::toNativeSeparators(segfile) + "\"");
	}
	else
	{
		segfile = base_name + "_cnvs.seg";
		if (QFile::exists(segfile))
		{
			executeIGVCommands(QStringList() << "load \"" + QDir::toNativeSeparators(segfile) + "\"");
		}
	}
}

void ProcessedSampleWidget::addSvsToIgv()
{
	QString bam = NGSD().processedSamplePath(ps_id_, NGSD::BAM);
	QString vcf = bam.left(bam.length()-4) + "_manta_var_structural.vcf.gz";
	executeIGVCommands(QStringList() << "load \"" + QDir::toNativeSeparators(vcf) + "\"");
}

void ProcessedSampleWidget::addBafsToIgv()
{
	QString bam = NGSD().processedSamplePath(ps_id_, NGSD::BAM);
	QString bafs = bam.left(bam.length()-4) + "_bafs.igv";

	executeIGVCommands(QStringList() << "load \"" + QDir::toNativeSeparators(bafs) + "\"");
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

void ProcessedSampleWidget::editDiseaseGroupAndInfo()
{
	NGSD db;

	QString ps_name = processedSampleName();
	QString sample_id = db.sampleId(ps_name);

	DiseaseInfoWidget* widget = new DiseaseInfoWidget(ps_name, sample_id, this);
	auto dlg = GUIHelper::createDialog(widget, "Disease information", "", true);
	if (dlg->exec() != QDialog::Accepted) return;

	db.setSampleDiseaseData(sample_id, widget->diseaseGroup(), widget->diseaseStatus());
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

void ProcessedSampleWidget::queueSampleAnalysis()
{
	NGSD db;

	//prepare sample list
	QList<AnalysisJobSample> job_list;
	job_list << AnalysisJobSample {db.processedSampleName(ps_id_), ""};

	//show dialog
	SingleSampleAnalysisDialog dlg(this);
	dlg.setSamples(job_list);
	if (dlg.exec()!=QDialog::Accepted) return;

	//start analysis
	foreach(const AnalysisJobSample& sample,  dlg.samples())
	{
		db.queueAnalysis("single sample", dlg.highPriority(), dlg.arguments(), QList<AnalysisJobSample>() << sample);
	}
}
