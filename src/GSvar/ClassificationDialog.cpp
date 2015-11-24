#include "ClassificationDialog.h"
#include "NGSD.h"
#include "Helper.h"

ClassificationDialog::ClassificationDialog(QWidget* parent, const Variant& variant)
	: QDialog(parent)
	, ui_()
{
	ui_.setupUi(this);
	connect(ui_.classification, SIGNAL(currentTextChanged(QString)), this, SLOT(statusChanged()));

	QStringList status = NGSD().getEnum("variant_classification", "class");
	foreach(QString s, status)
	{
		ui_.classification->addItem(s);
	}

	//set variant
	ui_.variant->setText(variant.toString());

	//get classification data from NGSD
	QPair<QString, QString> class_data = NGSD().getClassification(variant);
	ui_.classification->setCurrentText(class_data.first);
	ui_.comment->setPlainText(class_data.second);
}

QString ClassificationDialog::classification() const
{
	return ui_.classification->currentText();
}

QString ClassificationDialog::comment() const
{
	return ui_.comment->toPlainText();
}

void ClassificationDialog::classificationChanged()
{
	QString text = ui_.comment->toPlainText().trimmed();
	if (text!="") text += "\n";
	text += "[" + classification() + "] " + Helper::userName() + " " + QDate::currentDate().toString("dd.MM.yyyy");
	ui_.comment->setPlainText(text);
}
