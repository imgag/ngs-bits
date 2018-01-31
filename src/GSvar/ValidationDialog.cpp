#include "ValidationDialog.h"
#include "NGSD.h"
#include "Helper.h"
#include "BasicStatistics.h"
#include <QStringList>

ValidationDialog::ValidationDialog(QWidget* parent, QString filename, const Variant& variant, int quality_annotation_index)
	: QDialog(parent)
	, ui_()
{
	ui_.setupUi(this);
	connect(ui_.status, SIGNAL(currentTextChanged(QString)), this, SLOT(statusChanged()));

	QStringList status = NGSD().getEnum("variant_validation", "status");
	foreach(QString s, status)
	{
		ui_.status->addItem(s);
	}

	QStringList type = NGSD().getEnum("variant_validation", "type");
	foreach(QString t, type)
	{
		ui_.type->addItem(t);
	}

	//set variant
	QString var = variant.toString();
	if (!variant.isSNV())
	{
		var.append(" <font color='red'>(INDEL)</font>");
	}
	ui_.variant->setText(var);

	//set variant quality
	if (quality_annotation_index==-1)
	{
		ui_.quality->setText("n/a (no quality annotation)");
	}
	else
	{
		QList<QByteArray> qual_parts = variant.annotations()[quality_annotation_index].split(';');
		for (int i=0; i<qual_parts.count(); ++i)
		{
			QList<QByteArray> key_value = qual_parts[i].split('=');
			if (key_value.count()!=2 || !BasicStatistics::isValidFloat(key_value[1]))
			{
				qual_parts[i] = "<font color='gray'>" + qual_parts[i] + "</font>";
			}
			else if (
					(key_value[0]=="QUAL" && key_value[1].toDouble()<100)
					||
					(key_value[0]=="MQM" && key_value[1].toDouble()<50)
					||
					(key_value[0]=="AF" && key_value[1].toDouble()<0.25)
					||
					(key_value[0]=="DP" && key_value[1].toDouble()<20)
					 )
			{
				qual_parts[i] = "<font color='red'>" + qual_parts[i] + "</font>";
			}
		}
		ui_.quality->setText(qual_parts.join(" "));
	}

	//get validation data from NGSD
	ValidationInfo info = NGSD().getValidationStatus(filename, variant);
	ui_.status->setCurrentText(info.status);
	ui_.type->setCurrentText(info.type);
	ui_.comment->setPlainText(info.comments);
}

ValidationInfo ValidationDialog::info() const
{
	return ValidationInfo{ ui_.status->currentText(), ui_.type->currentText(), ui_.comment->toPlainText()};
}

void ValidationDialog::statusChanged()
{
	QString text = ui_.comment->toPlainText().trimmed();
	if (text!="") text += "\n";
	text += "[" + ui_.status->currentText() + "] " + Helper::userName() + " " + QDate::currentDate().toString("dd.MM.yyyy");
	ui_.comment->setPlainText(text);
}
