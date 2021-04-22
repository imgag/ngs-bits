#include "SomaticVariantInterpreterWidget.h"
#include "ui_SomaticVariantInterpreterWidget.h"
#include "LoginManager.h"
#include "NGSD.h"
#include "QMessageBox"
#include <QDebug>

SomaticVariantInterpreterWidget::SomaticVariantInterpreterWidget(int variant_index, const VariantList& vl, QWidget *parent)
	: QWidget(parent)
	, ui_(new Ui::SomaticVariantInterpreterWidget)
	, variant_index_(variant_index)
	, snv_(vl[variant_index])
	, vl_(vl)
{
	ui_->setupUi(this);

	if(!LoginManager::active()) disableNGSD();

	for(QButtonGroup* buttongroup: findChildren<QButtonGroup*>(QRegularExpression("^benign_*|onco_*")) )
	{
		connect(buttongroup, SIGNAL(buttonToggled(int,bool)), this, SLOT(disableUnapplicableParameters()));
		connect(buttongroup, SIGNAL(buttonToggled(int,bool)), this, SLOT(predict()));
	}


	QString variant_description = vl[variant_index].toString(true);


	int i_co_sp = vl.annotationIndexByName("coding_and_splicing", true, false);
	if(i_co_sp != -1)
	{
		VariantTranscript trans = vl[variant_index].transcriptAnnotations(i_co_sp).first();

		variant_description +=  " <b>" + trans.gene + "</b>:" +trans.hgvs_c + " <b>" + trans.gene + "</b>:" + trans.hgvs_p;
	}

	ui_->label_variant->setText(variant_description);


	//connect buttons
	connect(ui_->button_select_from_input_anno, SIGNAL(clicked(bool)), this, SLOT(preselectFromInputAnno()));
	connect(ui_->button_select_from_NGSD, SIGNAL(clicked(bool)), this, SLOT(preselectFromNGSD()));
	connect(ui_->button_store_in_ngsd, SIGNAL(clicked(bool)), this, SLOT(storeInNGSD()));


	//Preselect from NGSD, if not existing according annotations
	if( !preselectFromNGSD() )	preselectFromInputAnno();

	//load NGSD metadata
	setNGSDMetaData();

}

SomaticVariantInterpreterWidget::~SomaticVariantInterpreterWidget()
{
	delete ui_;
}

void SomaticVariantInterpreterWidget::disableNGSD()
{
	ui_->label_result_in_ngsd->setText("NGSD disabled");
	ui_->label_result_in_ngsd->setStyleSheet("color: gray");

	ui_->comment->setEnabled(false);
	ui_->button_select_from_NGSD->setEnabled(false);
	ui_->button_store_in_ngsd->setEnabled(false);

}

SomaticViccData SomaticVariantInterpreterWidget::getParameters()
{
	SomaticViccData out;

	out.null_mutation_in_tsg = getSelection("onco_null_mutation_in_tsg");
	out.known_oncogenic_aa = getSelection("onco_known_oncogenic_aa");
	out.oncogenic_functional_studies = getSelection("onco_oncogenic_functional_studies");
	out.strong_cancerhotspot = getSelection("onco_strong_cancerhotspot");
	out.located_in_canerhotspot = getSelection("onco_located_in_canerhotspot");
	out.absent_from_controls = getSelection("onco_absent_from_controls");
	out.protein_length_change = getSelection("onco_protein_length_change");
	out.other_aa_known_oncogenic = getSelection("onco_other_aa_known_oncogenic");
	out.weak_cancerhotspot = getSelection("onco_weak_cancerhotspot");
	out.computational_evidence = getSelection("onco_computational_evidence");
	out.mutation_in_gene_with_etiology = getSelection("onco_mutation_in_gene_with_etiology");
	out.very_weak_cancerhotspot = getSelection("onco_very_weak_cancerhotspot");
	out.very_high_maf = getSelection("benign_very_high_maf");
	out.benign_functional_studies = getSelection("benign_benign_functional_studies");
	out.high_maf = getSelection("benign_high_maf");
	out.benign_computational_evidence = getSelection("benign_benign_computational_evidence");
	out.synonymous_mutation = getSelection("benign_synonymous_mutation");

	out.comment = ui_->comment->toPlainText();

	return out;
}

void SomaticVariantInterpreterWidget::preselect(const SomaticViccData &data)
{
	setSelection("onco_null_mutation_in_tsg", data.null_mutation_in_tsg);
	setSelection("onco_known_oncogenic_aa", data.known_oncogenic_aa);
	setSelection("onco_oncogenic_functional_studies", data.oncogenic_functional_studies);
	setSelection("onco_strong_cancerhotspot", data.strong_cancerhotspot);
	setSelection("onco_located_in_canerhotspot", data.located_in_canerhotspot);
	setSelection("onco_absent_from_controls", data.absent_from_controls);
	setSelection("onco_protein_length_change", data.protein_length_change);
	setSelection("onco_other_aa_known_oncogenic", data.other_aa_known_oncogenic);
	setSelection("onco_weak_cancerhotspot", data.weak_cancerhotspot);
	setSelection("onco_computational_evidence", data.computational_evidence);
	setSelection("onco_mutation_in_gene_with_etiology", data.mutation_in_gene_with_etiology);
	setSelection("onco_very_weak_cancerhotspot", data.very_weak_cancerhotspot);
	setSelection("benign_very_high_maf", data.very_high_maf);
	setSelection("benign_benign_functional_studies", data.benign_functional_studies);
	setSelection("benign_high_maf", data.high_maf);
	setSelection("benign_benign_computational_evidence", data.benign_computational_evidence);
	setSelection("benign_synonymous_mutation", data.synonymous_mutation);

	ui_->comment->setText(data.comment);

	//predict new VICC score after all radio buttons were set
	predict();
}

void SomaticVariantInterpreterWidget::preselectFromInputAnno()
{
	if(!SomaticVariantInterpreter::checkAnnoForPrediction(vl_))
	{
		ui_->label_status->setText("Cannot preselect from annotations.");
		ui_->label_status->setStyleSheet("color: red");
		return;
	}

	SomaticViccData preselection = SomaticVariantInterpreter::predictViccValue(vl_, snv_);
	preselect(preselection);
}

bool SomaticVariantInterpreterWidget::preselectFromNGSD()
{
	if(!LoginManager::active()) return false;
	NGSD db;
	int id = db.getSomaticViccId(snv_);
	if(id == -1 ) return false;
	preselect(db.getSomaticViccData(snv_) );
	return true;
}

void SomaticVariantInterpreterWidget::predict()
{
	QString result = SomaticVariantInterpreter::viccScoreAsString(getParameters());

	ui_->label_live_result->setText(result);
}

void SomaticVariantInterpreterWidget::disableUnapplicableParameters()
{
	SomaticViccData data = getParameters();

	//located in cancerhotspot
	if(data.known_oncogenic_aa == SomaticViccData::State::VICC_TRUE || data.strong_cancerhotspot == SomaticViccData::State::VICC_TRUE)
	{
		setSelectionEnabled("onco_located_in_canerhotspot", false);
	}
	else setSelectionEnabled("onco_located_in_canerhotspot", true);

	//other aa change is known
	if(data.known_oncogenic_aa == SomaticViccData::State::VICC_TRUE || data.strong_cancerhotspot == SomaticViccData::State::VICC_TRUE || data.located_in_canerhotspot == SomaticViccData::State::VICC_TRUE)
	{
		setSelectionEnabled("onco_other_aa_known_oncogenic", false);
	}
	else setSelectionEnabled("onco_other_aa_known_oncogenic", true);

	//weak cancerhospot
	if(data.located_in_canerhotspot == SomaticViccData::State::VICC_TRUE || data.other_aa_known_oncogenic == SomaticViccData::State::VICC_TRUE)
	{
		setSelectionEnabled("onco_weak_cancerhotspot", false);
	}
	else setSelectionEnabled("onco_weak_cancerhotspot", true);

}

void SomaticVariantInterpreterWidget::storeInNGSD()
{
	SomaticViccData vicc_data = getParameters();

	try
	{
		if(!vicc_data.isValid())
		{
			THROW(ArgumentException, "VICC data is not valid!");
		}

		NGSD().setSomaticViccData(snv_, vicc_data, LoginManager::user());
	}
	catch(Exception e)
	{
		QMessageBox::warning(this, "Error storing VICC data", "Could not store somatic VICC interpretation to NGSD.\n\nError message:\n" + e.message());
		return;
	}

	//update NGSD meta data labels
	setNGSDMetaData();

	emit stored(variant_index_, SomaticVariantInterpreter::viccScoreAsString(vicc_data), vicc_data.comment);
}


void SomaticVariantInterpreterWidget::setSelection(QString name, SomaticViccData::State vicc_state)
{
	QButtonGroup* buttongroup = findChild<QButtonGroup*>(name);
	if(buttongroup == nullptr) return;
	for(auto radiobutton : buttongroup->buttons())
	{
		if(radiobutton->text() == "true" && vicc_state == SomaticViccData::State::VICC_TRUE) radiobutton->setChecked(true);
		if(radiobutton->text() == "false" && vicc_state == SomaticViccData::State::VICC_FALSE) radiobutton->setChecked(true);
		if(radiobutton->text() == "unapplicable" && vicc_state == SomaticViccData::State::NOT_APPLICABLE) radiobutton->setChecked(true);
	}
}

SomaticViccData::State SomaticVariantInterpreterWidget::getSelection(QString name)
{
	QButtonGroup* groupbox = findChild<QButtonGroup*>(name);
	if(groupbox == nullptr) return SomaticViccData::State::NOT_APPLICABLE;
	for(auto radiobox : groupbox->buttons())
	{
		if(radiobox->text() == "true" && radiobox->isChecked() ) return SomaticViccData::State::VICC_TRUE;
		if(radiobox->text() == "false" && radiobox->isChecked() ) return SomaticViccData::State::VICC_FALSE;
	}
	return SomaticViccData::State::NOT_APPLICABLE;
}

void SomaticVariantInterpreterWidget::setSelectionEnabled(QString name, bool state)
{
	QButtonGroup* buttongroup = findChild<QButtonGroup*>(name);
	if(buttongroup == nullptr) return;
	for(auto radiobutton : buttongroup->buttons())
	{
		radiobutton->setEnabled(state);
		if(radiobutton->text() == "unapplicable" && state == false) radiobutton->setChecked(true);
	}
}

void SomaticVariantInterpreterWidget::setNGSDMetaData()
{
	if(!LoginManager::active()) return;
	NGSD db;

	if(db.getSomaticViccId(snv_) == -1) return;

	SomaticViccData vicc_from_ngsd;
	try
	{
		vicc_from_ngsd = db.getSomaticViccData(snv_);
	}
	catch(Exception)
	{
		return; //Do nothing in case of error
	}

	ui_->label_result_in_ngsd->setText( SomaticVariantInterpreter::viccScoreAsString(vicc_from_ngsd) );


	ui_->label_creation_date->setText(vicc_from_ngsd.created_at.toString("yyyy-MM-dd hh:mm:ss"));
	ui_->label_created_by->setText(vicc_from_ngsd.created_by);
	ui_->label_last_update_by->setText(vicc_from_ngsd.last_updated_by);
	ui_->label_last_update_date->setText(vicc_from_ngsd.last_updated_at.toString("yyyy-MM-dd hh:mm:ss"));
	ui_->label_comment->setText(vicc_from_ngsd.comment);
}
