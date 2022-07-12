#ifndef PHENOTYPESETTINGSDIALOG_H
#define PHENOTYPESETTINGSDIALOG_H

#include <QDialog>
#include "ui_PhenotypeSettingsDialog.h"
#include "Phenotype.h"

//Dialog that allows changing the phenotype settings.
class PhenotypeSettingsDialog
	: public QDialog
{
	Q_OBJECT

public:
	PhenotypeSettingsDialog(QWidget* parent);
	void setCombinationModeEnabled(bool enabled);

	void set(PhenotypeSettings& settings);
	PhenotypeSettings get() const;

private:
	Ui::PhenotypeSettingsDialog ui_;
};

#endif // PHENOTYPESETTINGSDIALOG_H
