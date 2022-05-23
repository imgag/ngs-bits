#include "PhenotypeSettingsDialog.h"

PhenotypeSettingsDialog::PhenotypeSettingsDialog(QWidget *parent)
	: QDialog(parent)
	, ui_()
{
	ui_.setupUi(this);
}

void PhenotypeSettingsDialog::setCombinationModeEnabled(bool enabled)
{
	ui_.m_intersect->setEnabled(false);
	ui_.m_merge->setEnabled(false);
}

void PhenotypeSettingsDialog::set(PhenotypeSettings& settings)
{
	//source
	ui_.s_clinvar ->setChecked(settings.sources.contains(PhenotypeSource::CLINVAR));
	ui_.s_decipher->setChecked(settings.sources.contains(PhenotypeSource::DECIPHER));
	ui_.s_gencc->setChecked(settings.sources.contains(PhenotypeSource::GENCC));
	ui_.s_hgmd->setChecked(settings.sources.contains(PhenotypeSource::HGMD));
	ui_.s_hpo->setChecked(settings.sources.contains(PhenotypeSource::HPO));
	ui_.s_omim->setChecked(settings.sources.contains(PhenotypeSource::OMIM));

	//evidence level
	ui_.e_high->setChecked(settings.evidence_levels.contains(PhenotypeEvidenceLevel::HIGH));
	ui_.e_medium->setChecked(settings.evidence_levels.contains(PhenotypeEvidenceLevel::MEDIUM));
	ui_.e_low->setChecked(settings.evidence_levels.contains(PhenotypeEvidenceLevel::LOW));
	ui_.e_na->setChecked(settings.evidence_levels.contains(PhenotypeEvidenceLevel::NA));

	//mode
	ui_.m_merge->setChecked(settings.mode==PhenotypeCombimnationMode::MERGE);
	ui_.m_intersect->setChecked(settings.mode==PhenotypeCombimnationMode::INTERSECT);
}

PhenotypeSettings PhenotypeSettingsDialog::get() const
{
	PhenotypeSettings output;

	output.sources.clear();
	if (ui_.s_clinvar->isChecked()) output.sources << PhenotypeSource::CLINVAR;
	if (ui_.s_decipher->isChecked()) output.sources << PhenotypeSource::DECIPHER;
	if (ui_.s_gencc->isChecked()) output.sources << PhenotypeSource::GENCC;
	if (ui_.s_hgmd->isChecked()) output.sources << PhenotypeSource::HGMD;
	if (ui_.s_hpo->isChecked()) output.sources << PhenotypeSource::HPO;
	if (ui_.s_omim->isChecked()) output.sources << PhenotypeSource::OMIM;

	output.evidence_levels.clear();
	if (ui_.e_high->isChecked()) output.evidence_levels << PhenotypeEvidenceLevel::HIGH;
	if (ui_.e_medium->isChecked()) output.evidence_levels << PhenotypeEvidenceLevel::MEDIUM;
	if (ui_.e_low->isChecked()) output.evidence_levels << PhenotypeEvidenceLevel::LOW;
	if (ui_.e_na->isChecked()) output.evidence_levels << PhenotypeEvidenceLevel::NA;

	if (ui_.m_merge->isChecked()) output.mode = PhenotypeCombimnationMode::MERGE;
	if (ui_.m_intersect->isChecked()) output.mode = PhenotypeCombimnationMode::INTERSECT;

	return  output;
}

