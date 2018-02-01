#include "ClassificationDialog.h"
#include "NGSD.h"
#include "Helper.h"

ClassificationDialog::ClassificationDialog(QWidget* parent, const Variant& variant)
	: QDialog(parent)
	, ui_()
{
	ui_.setupUi(this);
	connect(ui_.classification, SIGNAL(currentTextChanged(QString)), this, SLOT(classificationChanged()));

	QStringList status = NGSD().getEnum("variant_classification", "class");
	foreach(QString s, status)
	{
		ui_.classification->addItem(s);
	}

	//set variant
	ui_.variant->setText(variant.toString());

	//get classification data from NGSD
	ClassificationInfo class_info = NGSD().getClassification(variant);
	ui_.classification->setCurrentText(class_info.classification);
	ui_.comment->setPlainText(class_info.comments);
}

ClassificationInfo ClassificationDialog::classificationInfo() const
{
	return ClassificationInfo { ui_.classification->currentText(), ui_.comment->toPlainText() };
}

void ClassificationDialog::classificationChanged()
{
	QString text = ui_.comment->toPlainText().trimmed();
	if (text!="") text += "\n";
	text += "[" + ui_.classification->currentText() + "] " + Helper::userName() + " " + QDate::currentDate().toString("dd.MM.yyyy");
	ui_.comment->setPlainText(text);
}
