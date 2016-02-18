#ifndef CLASSIFICATIONDIALOG_H
#define CLASSIFICATIONDIALOG_H

#include <QDialog>
#include "ui_ClassificationDialog.h"
#include "VariantList.h"

///Dialog for variant classification
class ClassificationDialog
	: public QDialog
{
	Q_OBJECT

public:
	///Constructor
	ClassificationDialog(QWidget* parent, const Variant& variant);

	///Returns the classification.
	QString classification() const;
	///Returns the classification comment.
	QString comment() const;

private slots:
	///Updates the comment when the classification changes
	void classificationChanged();

private:
	Ui::ClassificationDialog ui_;
};

#endif // CLASSIFICATIONDIALOG_H
