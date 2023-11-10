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

	//Returns the currently selected format.
	QString selectedFormat() const;
	//Sets the default variant format. Throws an exception when the format does not exist.
	void setDefaultFormat(QString format);

	//Returns the entered variant. If case it is not valid, an exception is thrown.
	Variant variant();

private slots:
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
