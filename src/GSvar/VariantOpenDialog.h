#ifndef VARIANTOPENDIALOG_H
#define VARIANTOPENDIALOG_H

#include <QDialog>

#include "VariantList.h"
#include "ui_VariantOpenDialog.h"

//Dialog for entering a variant.
class VariantOpenDialog
	: public QDialog
{
	Q_OBJECT

public:
	VariantOpenDialog(QWidget* parent);

	//Returns the entered variant. If case it is not valid, an exception is thrown.
	Variant variant() const;

private slots:
	//updates the style hint based on the selected style
	void updateStyleHint();
	//checks if the variant text is valid. If not, a error message is shown.
	bool checkValid();
	//checks if the variant text is valid before accepting the dialog. If not, a error message is shown and the dialog is not accepted.
	void checkValidBeforeAccept();

private:
	Ui::VariantOpenDialog ui_;
	FastaFileIndex ref_genome_idx_;
};

#endif // VARIANTOPENDIALOG_H
