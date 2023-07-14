#include "GenLabImportDialog.h"
#include "GenLabDB.h"
#include "GUIHelper.h"

GenLabImportDialog::GenLabImportDialog(QString ps_id, QWidget* parent)
	: QDialog(parent)
	, ui_()
	, init_timer_(this, true)
	, db_()
	, ps_id_(ps_id)
{
	ui_.setupUi(this);
	connect(ui_.show_in_ngsd, SIGNAL(stateChanged(int)), this, SLOT(initTable()));

	QString ps_name = db_.processedSampleName(ps_id_);
	setWindowTitle("Import data from GenLab for " + ps_name);
}

void GenLabImportDialog::delayedInitialization()
{
	initTable();
}

void GenLabImportDialog::initTable()
{
	try
	{
		QApplication::setOverrideCursor(Qt::BusyCursor);

		ui_.buttonBox->setEnabled(false);
		ui_.table->setRowCount(0);

		GenLabDB genlab;
		QString ps_name = db_.processedSampleName(ps_id_);

		//*** sample data ***
		QString s_id = db_.getValue("SELECT sample_id FROM processed_sample WHERE id=:0", false, ps_id_).toString();
		SampleData s_data = db_.getSampleData(s_id);

		QString gender = genlab.gender(ps_name);
		addItem("sample: gender", s_data.gender, gender);

		QString patient_identifier = genlab.patientIdentifier(ps_name);
		addItem("sample: patient identifier", s_data.patient_identifier, patient_identifier);


		QString year_of_birth = genlab.yearOfBirth(ps_name);
		addItem("sample: year of birth", s_data.year_of_birth, year_of_birth);

		auto disease_data = genlab.diseaseInfo(ps_name);
		addItem("sample: disease group", s_data.disease_group, disease_data.first);
		addItem("sample: disease status", s_data.disease_status, disease_data.second);

		//*** disease details *** 'HPO term id','ICD10 code','OMIM disease/phenotype identifier','Orpha number','CGI cancer type','tumor fraction','age of onset','clinical phenotype (free text)','RNA reference tissue')
		QStringList anamnesis_list = genlab.anamnesis(ps_name);
		foreach (QString anamnesis, anamnesis_list)
		{
			QString anamnesis_ngsd = diseaseDataContains(s_id, "clinical phenotype (free text)", anamnesis) ? anamnesis : "";
			addItem("disease details: free-text phenotype", anamnesis_ngsd, anamnesis);
		}

		QStringList orpha_list = genlab.orphanet(ps_name);
		foreach (QString orpha, orpha_list)
		{
			QString orpha_ngsd = diseaseDataContains(s_id, "Orpha number", orpha) ? orpha : "";
			addItem("disease details: Orpha number", orpha_ngsd, orpha);
		}

		QStringList icd10_list = genlab.diagnosis(ps_name);
		foreach (QString icd10, icd10_list)
		{
			QString icd10_ngsd = diseaseDataContains(s_id, "ICD10 code", icd10) ? icd10 : "";
			addItem("disease details: ICD10 code", icd10_ngsd, icd10);
		}

		QStringList tumor_fraction_list = genlab.tumorFraction(ps_name);
		foreach (QString tumor_fraction, tumor_fraction_list)
		{
			QString tumor_fraction_ngsd = diseaseDataContains(s_id, "tumor fraction", tumor_fraction) ? tumor_fraction : "";
			addItem("disease details: tumor fraction", tumor_fraction_ngsd, tumor_fraction);
		}

		PhenotypeList phenos = genlab.phenotypes(ps_name);
		foreach (const Phenotype& pheno, phenos)
		{
			QString hpo_ngsd = diseaseDataContains(s_id, "HPO term id", pheno.accession()) ? pheno.toString() : "";
			addItem("disease details: HPO term", hpo_ngsd, pheno.toString());
		}

		//*** sample relations ***
		QList<SampleRelation> relation_list = genlab.relatives(ps_name);
		foreach (const SampleRelation& relation, relation_list)
		{
			QString relation_text = relation.sample1 + " - " + relation.relation + " - " + relation.sample2;
			QString relation_ngsd = relationDataContains(s_id, relation) ? relation_text : "";
			addItem("sample relations", relation_ngsd, relation_text);
		}

		//*** studies ***
		QStringList studies = genlab.studies(ps_name);
		foreach (QString study, studies)
		{
			QString study_ngsd = studyDataContains(s_id, study) ? study : "";
			addItem("study", study_ngsd, study);
		}

		GUIHelper::resizeTableCells(ui_.table, 400);

		ui_.buttonBox->setEnabled(true);

		QApplication::restoreOverrideCursor();
	}
	catch (Exception& e)
	{
		GUIHelper::showException(this, e, "GenLab data import");
	}
}

void GenLabImportDialog::addItem(QString label, QString v_ngsd, QString v_genlab)
{
	//skip if no data in GenLab
	v_genlab = v_genlab.trimmed();
	if (v_genlab=="n/a") v_genlab = "";
	if (v_genlab.isEmpty()) return;

	//determine action
	v_ngsd = v_ngsd.trimmed();
	if (v_ngsd=="n/a") v_ngsd = "";
	QString action = "";
	if (v_ngsd!=v_genlab && v_ngsd=="") action = "add";
	if (v_ngsd!=v_genlab && v_ngsd!="") action = "replace";

	//add to table
	if (action!="" || ui_.show_in_ngsd->isChecked())
	{
		int r = ui_.table->rowCount();
		ui_.table->setRowCount(r+1);
		ui_.table->setItem(r, 0, GUIHelper::createTableItem(label));
		ui_.table->setItem(r, 1, GUIHelper::createTableItem(v_ngsd));
		ui_.table->setItem(r, 2, GUIHelper::createTableItem(v_genlab));

		QTableWidgetItem* action_item = GUIHelper::createTableItem(action);
		if (action!="")
		{
			action_item->setFlags(action_item->flags() | Qt::ItemIsUserCheckable); // add checkbox
			if (action=="add")
			{
				action_item->setCheckState(Qt::Checked);
			}
			else
			{
				action_item->setCheckState(Qt::Unchecked);
			}
		}
		ui_.table->setItem(r, 3, action_item);
	}
}

bool GenLabImportDialog::diseaseDataContains(QString sample_id, QString type, QString value)
{
	QList<SampleDiseaseInfo> disease_info = db_.getSampleDiseaseInfo(sample_id, type);
	foreach(const SampleDiseaseInfo& info, disease_info)
	{
		if (info.disease_info.trimmed()==value.trimmed()) return true;
	}

	return false;
}

bool GenLabImportDialog::relationDataContains(QString sample_id, SampleRelation relation) //GenLab sample relation contain the actual sample as sample2
{
	QSet<int> sample_ids = db_.relatedSamples(sample_id.toInt(), relation.relation);
	int sample2_id = db_.sampleId(relation.sample1).toInt();

	return sample_ids.contains(sample2_id);
}

bool GenLabImportDialog::studyDataContains(QString sample_id, QString study)
{
	QList<QString> studies = db_.getValues("SELECT st.name FROM study st, study_sample ss, processed_sample ps WHERE ss.study_id=st.id AND ps.id=ss.processed_sample_id AND ps.sample_id=" + sample_id);

	//compare independent of case
	study = study.toLower();
	for(int i=0; i<studies.count(); ++i)
	{
		studies[i] = studies[i].toLower();
	}

	return studies.contains(study);
}

void GenLabImportDialog::addDiseaseDetail(QList<SampleDiseaseInfo>& disease_details, QString type, QString data)
{
	//map type
	QString type_ngsd;
	if (type=="disease details: HPO term")
	{
		type_ngsd = "HPO term id";
		if (data.contains(" - ")) //remove HPO term name - we need only the ID
		{
			data = data.split(" - ")[0];
		}
	}
	else if (type=="disease details: ICD10 code")
	{
		type_ngsd = "ICD10 code";
	}
	else if (type=="disease details: Orpha number")
	{
		type_ngsd = "Orpha number";
	}
	else if (type=="disease details: free-text phenotype")
	{
		type_ngsd = "clinical phenotype (free text)";
	}
	else if (type=="disease details: tumor fraction")
	{
		type_ngsd = "tumor fraction";
	}
	else //OMIM disease/phenotype identifier, CGI cancer type, age of onset, RNA reference tissue
	{
		THROW(ProgrammingException, "Unhandled data type '" + type + "' in disease details handling!");
	}

	//add
	SampleDiseaseInfo new_entry;
	new_entry.disease_info = data;
	new_entry.type = type_ngsd;
	new_entry.user = "genlab_import";
	new_entry.date = QDateTime::currentDateTime();
	disease_details << new_entry;
}

void GenLabImportDialog::importSelectedData()
{
	try
	{
		QString s_id = db_.getValue("SELECT sample_id FROM processed_sample WHERE id=:0", false, ps_id_).toString();

		for (int r=0; r<ui_.table->rowCount(); ++r)
		{
			QTableWidgetItem* item = ui_.table->item(r, 3);
			if (item==nullptr || item->checkState()!=Qt::Checked) continue;

			QString type = ui_.table->item(r, 0)->text().trimmed();
			QString data = ui_.table->item(r, 2)->text().trimmed();
			if (type=="sample: gender")
			{
				db_.getQuery().exec("UPDATE sample SET gender='" + data + "' WHERE id=" + s_id);
			}
			else if (type=="sample: patient identifier")
			{
				db_.getQuery().exec("UPDATE sample SET patient_identifier='" + data + "' WHERE id=" + s_id);
			}
			else if (type=="sample: year of birth")
			{
				db_.getQuery().exec("UPDATE sample SET year_of_birth='" + data + "' WHERE id=" + s_id);
			}
			else if (type=="sample: disease group")
			{
				db_.getQuery().exec("UPDATE sample SET disease_group='" + data + "' WHERE id=" + s_id);
			}
			else if (type=="sample: disease status")
			{
				db_.getQuery().exec("UPDATE sample SET disease_status='" + data + "' WHERE id=" + s_id);
			}
			else if (type=="disease details: free-text phenotype")
			{
				QList<SampleDiseaseInfo> disease_details = db_.getSampleDiseaseInfo(s_id);
				addDiseaseDetail(disease_details, type, data);
				db_.setSampleDiseaseInfo(s_id, disease_details);
			}
			else if (type=="disease details: Orpha number")
			{
				QList<SampleDiseaseInfo> disease_details = db_.getSampleDiseaseInfo(s_id);
				addDiseaseDetail(disease_details, type, data);
				db_.setSampleDiseaseInfo(s_id, disease_details);
			}
			else if (type=="disease details: ICD10 code")
			{
				QList<SampleDiseaseInfo> disease_details = db_.getSampleDiseaseInfo(s_id);
				addDiseaseDetail(disease_details, type, data);
				db_.setSampleDiseaseInfo(s_id, disease_details);
			}
			else if (type=="disease details: HPO term")
			{
				QList<SampleDiseaseInfo> disease_details = db_.getSampleDiseaseInfo(s_id);
				addDiseaseDetail(disease_details, type, data);
				db_.setSampleDiseaseInfo(s_id, disease_details);
			}
			else if (type=="disease details: tumor fraction")
			{
				QList<SampleDiseaseInfo> disease_details = db_.getSampleDiseaseInfo(s_id);
				addDiseaseDetail(disease_details, type, data);
				db_.setSampleDiseaseInfo(s_id, disease_details);
			}
			else if (type=="sample relations")
			{
				QStringList parts = data.split(" - ");
				SampleRelation rel;
				rel.sample1 = parts[0].trimmed().toUtf8();
				rel.relation = parts[1].trimmed().toUtf8();
				rel.sample2 = parts[2].trimmed().toUtf8();
				db_.addSampleRelation(rel);
			}
			else if (type=="study") //do studies at the end. If the spelling is not correct the import fails :(
			{
				QVariant study_id = db_.getValue("SELECT id FROM study WHERE name=:0", true, data);
				if (!study_id.isValid()) INFO(ArgumentException, "GenLab study name '" + data + "' not found in NGSD! Please add the study to NGSD, or correcte the study name in GenLab!");
				db_.getQuery().exec("INSERT INTO `study_sample`(`study_id`, `processed_sample_id`) VALUES ("+study_id.toString()+", "+ps_id_+")");
			}
			else
			{
				THROW(ProgrammingException, "Unhandled data type '" + type + "'!");
			}
		}
	}
	catch (Exception& e)
	{
		GUIHelper::showException(this, e, "GenLab data import");
	}
}
