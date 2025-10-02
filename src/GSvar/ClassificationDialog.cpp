#include "ClassificationDialog.h"
#include "Helper.h"
#include "LoginManager.h"

ClassificationDialog::ClassificationDialog(QWidget* parent, const Variant& variant, bool is_somatic)
	: QDialog(parent)
	, ui_()
{
	ui_.setupUi(this);
	connect(ui_.classification, SIGNAL(currentTextChanged(QString)), this, SLOT(classificationChanged()));
	if (is_somatic)
	{
		setWindowTitle("Variant classification (somatic)");
	}

	//set status options
	NGSD db;
	QStringList status = db.getEnum(is_somatic ? "somatic_variant_classification" : "variant_classification", "class");
	foreach(QString s, status)
	{
		ui_.classification->addItem(s);
	}

	//set variant
	ui_.variant->setText(variant.toString());

	//get classification data from NGSD
	if(is_somatic)
	{
		ClassificationInfo class_info = db.getSomaticClassification(variant);
		ui_.classification->setCurrentText(class_info.classification);
		ui_.comment->setPlainText(class_info.comments);
	}
	else
	{
		ClassificationInfo class_info = db.getClassification(variant);
		ui_.classification->setCurrentText(class_info.classification);
		ui_.comment->setPlainText(class_info.comments);
	}
}

ClassificationInfo ClassificationDialog::classificationInfo() const
{
	return ClassificationInfo { ui_.classification->currentText(), ui_.comment->toPlainText() };
}

void ClassificationDialog::classificationChanged()
{
	//prepend new classification
	QString text_old = ui_.comment->toPlainText().trimmed();
	QString text_new = "[" + ui_.classification->currentText() + "] " + LoginManager::userLogin() + " " + QDate::currentDate().toString("dd.MM.yyyy") +"\n\n\n";
	ui_.comment->setPlainText(text_new + text_old);

	//set cursor
	QTextCursor cursor = ui_.comment->textCursor();
	int pos = text_new.count()-2;
	cursor.setPosition(pos);
	ui_.comment->setTextCursor(cursor);

	//make sure the cursor is shown
	ui_.comment->setFocus();
}
