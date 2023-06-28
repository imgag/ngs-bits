#ifndef VARIANTOPENDIALOG_H
#define VARIANTOPENDIALOG_H

#include <QDialog>

#include "VariantList.h"
#include "DelayedInitializationTimer.h"
#include "ui_VariantOpenDialog.h"

//Dialog for entering a variant.
class VariantOpenDialog
	: public QDialog
{
	Q_OBJECT

public:
	//Constructor
	VariantOpenDialog(QWidget* parent);
	//Sets the default variant style. Throws an exception when the style does not exist.
	void setDefaultStyle(QString style);

	//Returns the entered variant. If case it is not valid, an exception is thrown.
	Variant variant();

private slots:
	//updates the style hint based on the selected style
	void updateStyleHint();
	//checks if the variant text is valid. If not, a error message is shown.
	bool checkValid();
	//checks if the variant text is valid before accepting the dialog. If not, a error message is shown and the dialog is not accepted.
	void checkValidBeforeAccept();
	//set focus
	void delayedInitialization();

private:
	Ui::VariantOpenDialog ui_;
	DelayedInitializationTimer init_timer_;
	FastaFileIndex ref_genome_idx_;
};

#endif // VARIANTOPENDIALOG_H
