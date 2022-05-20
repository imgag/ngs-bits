#include "GenLabImportDialog.h"
#include "GenLabDB.h"
#include "GUIHelper.h"

GenLabImportDialog::GenLabImportDialog(QString ps_id, QWidget* parent)
	: QDialog(parent)
	, ui_()
	, db_()
	, ps_id_(ps_id)
{
	ui_.setupUi(this);
	QString ps_name = db_.processedSampleName(ps_id_);
	setWindowTitle("Import data from GenLab for " + ps_name);

	initTable();
}

void GenLabImportDialog::importSelected()
{
	//TODO
}

void GenLabImportDialog::initTable()
{
	GenLabDB genlab;

	QString ps_name = db_.processedSampleName(ps_id_);

	//*** sample data ***
	QString s_id = db_.getValue("SELECT sample_id FROM processed_sample WHERE id=:0", false, ps_id_).toString();
	SampleData s_data = db_.getSampleData(s_id);

	QString gender = genlab.gender(ps_name);
	addItem("sample: gender", s_data.gender, gender);

	QString patient_identifier = genlab.patientIdentifier(ps_name);
	addItem("sample: patient identifier", s_data.patient_identifier, patient_identifier);

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


	GUIHelper::resizeTableCells(ui_.table, 400);
}

void GenLabImportDialog::addItem(QString label, QString v_ngsd, QString v_genlab)
{
	//skip if no data in GenLab
	v_genlab = v_genlab.trimmed();
	if (v_genlab.isEmpty() || v_genlab=="n/a") return;

	//set base data
	int r = ui_.table->rowCount();
	ui_.table->setRowCount(r+1);
	ui_.table->setItem(r, 0, GUIHelper::createTableItem(label));
	ui_.table->setItem(r, 1, GUIHelper::createTableItem(v_ngsd));
	ui_.table->setItem(r, 2, GUIHelper::createTableItem(v_genlab));

	//action
	v_ngsd = v_ngsd.trimmed();
	QString action = "";
	if (v_ngsd!=v_genlab && v_ngsd=="") action = "add";
	if (v_ngsd!=v_genlab && v_ngsd!="") action = "replace";
	QTableWidgetItem* action_item = GUIHelper::createTableItem(action);
	if (action!="")
	{
		action_item->setFlags(action_item->flags() | Qt::ItemIsUserCheckable); // add checkbox
		action_item->setCheckState(Qt::Checked);
	}
	ui_.table->setItem(r, 3, action_item);
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
