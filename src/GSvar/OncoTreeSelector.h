#ifndef ONCOTREESELECTOR_H
#define ONCOTREESELECTOR_H

#include <QDialog>
#include "ui_OncoTreeSelector.h"

#include "NGSD.h"
#include "DBSelector.h"

namespace Ui {
class OncoTreeSelector;
}

//Processed sample selection dialog.
class OncoTreeSelector
	: public QDialog
{
	Q_OBJECT

public:
	//Constructor
	OncoTreeSelector(QWidget* parent);
	//Set label text
	void setLabel(QString label);
	//Set processed sample selection text
	void setSelection(QString processed_sample);

	//Returns if a valid processed sample was selected
	bool isValidSelection() const;
	//Returns the Oncotree code database ID, or "" if the text it not a valid processed sample.
	QString oncotreeCodeId() const;
	//Returns the oncotree code, or "" if the text it not a valid processed sample.
	QString oncotreeCode() const;


protected slots:
	void initAutoCompletion();

private:
	Ui::OncoTreeSelector ui_;
	mutable NGSD db_;
};

#endif // ONCOTREESELECTOR_H
