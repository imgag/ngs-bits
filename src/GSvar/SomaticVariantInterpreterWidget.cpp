#include "SomaticVariantInterpreterWidget.h"
#include "ui_SomaticVariantInterpreterWidget.h"
#include "LoginManager.h"
#include "NGSD.h"
#include "QMessageBox"
#include <QDebug>

SomaticVariantInterpreterWidget::SomaticVariantInterpreterWidget(const Variant& var, const VariantList& vl, QWidget *parent): QWidget(parent),
	ui_(new Ui::SomaticVariantInterpreterWidget),
	snv_(var),
	vl_(vl)
{
	ui_->setupUi(this);

	if(!SomaticVariantInterpreter::checkAnnoForPrediction(vl))
	{
		disableGUI();
	}

	for(QButtonGroup* buttongroup: findChildren<QButtonGroup*>(QRegularExpression("^benign_*|onco_*")) )
	{
		connect(buttongroup, SIGNAL(buttonToggled(int,bool)), this, SLOT(disableUnapplicableParameters()));
		connect(buttongroup, SIGNAL(buttonToggled(int,bool)), this, SLOT(predict()));
	}

	ui_->label_variant->setText(var.toString(true));


	//connect buttons
	connect(ui_->button_select_from_input_anno, SIGNAL(clicked(bool)), this, SLOT(preselectFromInputAnno()));
	connect(ui_->button_select_from_NGSD, SIGNAL(clicked(bool)), this, SLOT(preselectFromNGSD()));
	connect(ui_->button_store_in_ngsd, SIGNAL(clicked(bool)), this, SLOT(storeInNGSD()));


	//Preselect form values according annotations
	preselectFromInputAnno();

}

SomaticVariantInterpreterWidget::~SomaticVariantInterpreterWidget()
{
	delete ui_;
}

void SomaticVariantInterpreterWidget::disableGUI()
{
	QList<QRadioButton*> radiobuttons = findChildren<QRadioButton*>();
	for(auto radiobutton : radiobuttons)
	{
		radiobutton->setChecked(false);
		radiobutton->setEnabled(false);
	}
}

SomaticViccData SomaticVariantInterpreterWidget::getParameters()
{
	SomaticViccData out;

	QList<QButtonGroup*> button_groups = findChildren<QButtonGroup*>(QRegularExpression("^benign_*|onco_*"));
	for(auto button_group : button_groups)
	{
		if(!button_group->objectName().contains("onco_") && !button_group->objectName().contains("benign_")) continue;

		//determine state that is checked
		SomaticViccData::state state;
		for(auto radiobox : button_group->buttons())
		{
			if( radiobox->text().contains("true") && radiobox->isChecked() ) state = SomaticViccData::TRUE123;
			else if( radiobox->text().contains("false") && radiobox->isChecked() ) state = SomaticViccData::FALSE123;
			else if( radiobox->text().contains("unapplicable") && radiobox->isChecked()) state = SomaticViccData::NOT_APPLICABLE;
		}

		if(button_group->objectName() == "onco_null_mutation_in_tsg" ) out.null_mutation_in_tsg = state;
		if(button_group->objectName() == "onco_known_oncogenic_aa" ) out.known_oncogenic_aa = state;
		if(button_group->objectName() == "onco_oncogenic_functional_studies") out.oncogenic_functional_studies = state;
		if(button_group->objectName() == "onco_strong_cancerhotspot") out.strong_cancerhotspot = state;
		if(button_group->objectName() == "onco_located_in_canerhotspot") out.located_in_canerhotspot = state;
		if(button_group->objectName() == "onco_absent_from_controls") out.absent_from_controls = state;
		if(button_group->objectName() == "onco_protein_length_change") out.protein_length_change = state;
		if(button_group->objectName() == "onco_other_aa_known_oncogenic") out.other_aa_known_oncogenic = state;
		if(button_group->objectName() == "onco_weak_cancerhotspot") out.weak_cancerhotspot = state;
		if(button_group->objectName() == "onco_computational_evidence") out.computational_evidence = state;
		if(button_group->objectName() == "onco_mutation_in_gene_with_etiology") out.mutation_in_gene_with_etiology = state;
		if(button_group->objectName() == "onco_very_weak_cancerhotspot") out.very_weak_cancerhotspot = state;
		if(button_group->objectName() == "benign_very_high_maf") out.very_high_maf = state;
		if(button_group->objectName() == "benign_benign_functional_studies") out.benign_functional_studies = state;
		if(button_group->objectName() == "benign_high_maf") out.high_maf = state;
		if(button_group->objectName() == "benign_benign_computational_evidence") out.benign_computational_evidence = state;
		if(button_group->objectName() == "benign_synonymous_mutation") out.synonymous_mutation = state;
	}

	return out;
}

void SomaticVariantInterpreterWidget::preselect(const SomaticViccData &data)
{
	setSelection("onco_null_mutation_in_tsg", data.null_mutation_in_tsg);
	setSelection("onco_known_oncogenic_aa", data.known_oncogenic_aa);
	setSelection("onc_oncogenic_functional_studies", data.oncogenic_functional_studies);
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
}

void SomaticVariantInterpreterWidget::preselectFromInputAnno()
{
	SomaticViccData preselection = SomaticVariantInterpreter::predictViccValue(vl_, snv_);
	preselect(preselection);
}

void SomaticVariantInterpreterWidget::preselectFromNGSD()
{
	if(!LoginManager::active()) return;
	NGSD db;
	int id = db.getSomaticViccId(snv_);
	if(id == -1 ) return;
	preselect(db.getSomaticViccData(snv_) );
}

void SomaticVariantInterpreterWidget::predict()
{
	QString result = SomaticVariantInterpreter::viccScoreAsString(getParameters());

	ui_->label_vicc_score->setText(result);
}

void SomaticVariantInterpreterWidget::disableUnapplicableParameters()
{
	SomaticViccData data = getParameters();

	//located in cancerhotspot
	if(data.known_oncogenic_aa != SomaticViccData::NOT_APPLICABLE || data.strong_cancerhotspot != SomaticViccData::NOT_APPLICABLE)
	{
		setSelectionEnabled("onco_located_in_canerhotspot", false);
	}
	else setSelectionEnabled("onco_located_in_canerhotspot", true);

	//other aa change is known
	if(data.known_oncogenic_aa != SomaticViccData::NOT_APPLICABLE || data.strong_cancerhotspot != SomaticViccData::NOT_APPLICABLE || data.located_in_canerhotspot != SomaticViccData::state::NOT_APPLICABLE)
	{
		setSelectionEnabled("onco_other_aa_known_oncogenic", false);
	}
	else setSelectionEnabled("onco_other_aa_known_oncogenic", true);

	//weak cancerhospot
	if(data.located_in_canerhotspot != SomaticViccData::NOT_APPLICABLE || data.other_aa_known_oncogenic != SomaticViccData::NOT_APPLICABLE)
	{
		setSelectionEnabled("onco_weak_cancerhotspot", false);
	}
	else setSelectionEnabled("onco_weak_cancerhotspot", true);

}

void SomaticVariantInterpreterWidget::storeInNGSD()
{
	if(!LoginManager::active()) return;
	NGSD db;

	SomaticViccData vicc_data = getParameters();
	if(!vicc_data.isValid()) return;
	try
	{
		db.setSomaticViccData(snv_, vicc_data, LoginManager::user());
	}
	catch(Exception e)
	{
		QMessageBox::warning(this, "Could not store somatic VICC interpretation", "Could not store somatic VICC interpretation in NGSD. Error message: " + e.message());
	}
}


void SomaticVariantInterpreterWidget::setSelection(QString name, SomaticViccData::state vicc_state)
{
	QButtonGroup* buttongroup = findChild<QButtonGroup*>(name);
	if(buttongroup == nullptr) return;
	for(auto radiobutton : buttongroup->buttons())
	{
		if(radiobutton->text() == "true" && vicc_state == SomaticViccData::TRUE123) radiobutton->setChecked(true);
		if(radiobutton->text() == "false" && vicc_state == SomaticViccData::FALSE123) radiobutton->setChecked(true);
		if(radiobutton->text() == "unapplicable" && vicc_state == SomaticViccData::NOT_APPLICABLE) radiobutton->setChecked(true);
	}
}

SomaticViccData::state SomaticVariantInterpreterWidget::getSelection(QString name)
{
	QButtonGroup* groupbox = findChild<QButtonGroup*>(name);
	if(groupbox == nullptr) return SomaticViccData::NOT_APPLICABLE;
	for(auto radiobox : groupbox->buttons())
	{
		if(radiobox->text() == "true" && radiobox->isChecked() ) return SomaticViccData::TRUE123;
		if(radiobox->text() == "false" && radiobox->isChecked() ) return SomaticViccData::FALSE123;
	}
	return SomaticViccData::NOT_APPLICABLE;
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
