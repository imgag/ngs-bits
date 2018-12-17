#include "ui_GDBODialog.h"
#include "GDBODialog.h"
#include "GBDOFKEdit.h"
#include "ProcessedSampleFKEdit.h"
#include <QLabel>
#include <QComboBox>
#include <QLineEdit>
#include <QTextEdit>
#include <QSpinBox>
#include <QDateEdit>
#include <QDoubleSpinBox>
#include <QCheckBox>

GDBODialog::GDBODialog(QWidget *parent, GDBO& gdbo, QStringList hidden)
	: QDialog(parent)
	, ui(new Ui::GDBODialog)
	, gdbo_(gdbo)
	, hidden_(hidden.toSet())
{
	ui->setupUi(this);

	init();
}

void GDBODialog::init()
{
	QFormLayout* layout = ui->formLayout;
	foreach(const QString& field, db_.fieldNames(gdbo_.table()))
	{

		if (hidden_.contains(field)) continue;
		const TableFieldInfo& field_info = gdbo_.fieldInfo(field);

		QString label = field;
		QString value = gdbo_.get(field);
		QWidget* edit = nullptr;
		switch(field_info.type)
		{
			case TableFieldInfo::FK:
			{
				label = field_info.fk_label;

				GBDOFKEdit* ptr = nullptr;

				if (field_info.fk_table == "processed_sample" && field_info.fk_field == "process_id")
				{
					ptr = new ProcessedSampleFKEdit();
				}
				else
				{
					ptr = new GBDOFKEdit(field_info.fk_table, field_info.fk_field);
				}
				if (value!="") ptr->setId(value.toInt());
				connect(ptr, SIGNAL(textChanged(QString)), this, SLOT(validate()));
				edit = ptr;
				break;
			}
			case TableFieldInfo::BOOL:
			{
				auto ptr = new QCheckBox();
				ptr->setChecked(value=="1");
				connect(ptr, SIGNAL(stateChanged(int)), this, SLOT(validate()));
				edit = ptr;
				break;
			}
			case TableFieldInfo::INT:
			{
				auto ptr = new QLineEdit();
				ptr->setValidator(new QIntValidator());
				ptr->setText(value);
				connect(ptr, SIGNAL(textChanged(QString)), this, SLOT(validate()));
				edit = ptr;
				break;
			}
			case TableFieldInfo::FLOAT:
			{
				auto ptr = new QLineEdit();
				ptr->setValidator(new QDoubleValidator());
				ptr->setText(value);
				connect(ptr, SIGNAL(textChanged(QString)), this, SLOT(validate()));
				edit = ptr;
				break;
			}
			case TableFieldInfo::TEXT:
			{
				auto ptr = new QTextEdit();
				ptr->setText(value);
				connect(ptr, SIGNAL(textChanged()), this, SLOT(validate()));
				edit = ptr;
				break;
			}
			case TableFieldInfo::VARCHAR:
			{
				auto ptr = new QLineEdit();
				bool ok;
				int length = field_info.type_restiction.toInt(&ok);
				if (ok) ptr->setMaxLength(length);
				ptr->setText(value);
				connect(ptr, SIGNAL(textChanged(QString)), this, SLOT(validate()));
				edit = ptr;
				break;
			}
			case TableFieldInfo::ENUM:
			{
				auto ptr = new QComboBox();
				if (field_info.nullable) ptr->addItem("");
				ptr->addItems(field_info.type_restiction.toStringList());
				ptr->setCurrentText(value);
				connect(ptr, SIGNAL(currentIndexChanged(int)), this, SLOT(validate()));
				edit = ptr;
				break;
			}
			case TableFieldInfo::DATE:
			{
				auto ptr = new QLineEdit();
				ptr->setText(value);
				connect(ptr, SIGNAL(textChanged(QString)), this, SLOT(validate()));
				edit = ptr;
				break;
			}
		}

		//decorate label
		label += ":";
		if (!field_info.nullable) label = "<b>" + label + "</b>";

		//set up and perform validation
		QLabel* label_widget = new QLabel(label);
		validation[field] = ValidationInfo{true, label_widget};
		edit->setObjectName(field);
		validate(edit);

		layout->addRow(label_widget, edit);
	}
}

GDBODialog::~GDBODialog()
{
	delete ui;
}

void GDBODialog::validate()
{
	QWidget* w = qobject_cast<QWidget*>(sender());
	validate(w);
}

void GDBODialog::validate(QWidget* w)
{
	QString field = w->objectName();

	//validate
	bool valid = false;
	QString value = "<unset>";
	const TableFieldInfo& field_info = gdbo_.fieldInfo(field);
	switch(field_info.type)
	{
		case TableFieldInfo::FK:
		{
			auto edit = qobject_cast<GBDOFKEdit*>(w);
			if (edit->id()!=-1)
			{
				valid = true;
				value = QString::number(edit->id());
			}
			else if (field_info.nullable && edit->text()=="")
			{
				valid = true;
				value = "";
			}
			break;
		}
		case TableFieldInfo::BOOL:
		{
			auto edit = qobject_cast<QCheckBox*>(w);
			value = (edit->isChecked() ? "1" : "0");
			valid = true;
			break;
		}
		case TableFieldInfo::INT:
		{
			auto edit = qobject_cast<QLineEdit*>(w);
			value = edit->text();
			if (value!="") valid = true;
			else if (field_info.nullable && value=="") valid = true;
			break;
		}
		case TableFieldInfo::FLOAT:
		{
			auto edit = qobject_cast<QLineEdit*>(w);
			value = edit->text();
			if (value!="") valid = true;
			else if (field_info.nullable && value=="") valid = true;
			break;
		}
		case TableFieldInfo::TEXT:
		{
			auto edit = new QTextEdit();
			value = edit->toPlainText();
			valid = true; //nothing to check here
			break;
		}
		case TableFieldInfo::VARCHAR:
		{
			auto edit = qobject_cast<QLineEdit*>(w);
			value = edit->text();
			valid = true; //nothing to check here
			break;
		}
		case TableFieldInfo::ENUM:
		{
			auto edit = qobject_cast<QComboBox*>(w);
			value = edit->currentText();
			valid = true; //nothing to check here
			break;
		}
		case TableFieldInfo::DATE:
		{
			auto edit = qobject_cast<QLineEdit*>(w);
			value = edit->text();
			if (value!="" && isValidDate(value)) valid = true;
			else if (field_info.nullable && value=="") valid = true;
			break;
		}
	}

	//update validation info
	ValidationInfo& val_info = validation[field];
	val_info.valid = valid;
	//color label
	val_info.label->setStyleSheet(valid ? "" : "QLabel{ color:#B6160A};");

	//set value if valid
	if (valid) gdbo_.set(field, value);
	//qDebug() << "VAL" << field << " " << value << valid << w;

	//update ok button
	updateOkButton();
}

void GDBODialog::updateOkButton()
{
	bool valid = true;
	QMap<QString, ValidationInfo>::iterator it = validation.begin();
	while(it != validation.end())
	{
		valid &= it->valid;
		++it;
	}
	ui->okButton->setEnabled(valid);
}

bool GDBODialog::isValidDate(QString text)
{
	QStringList parts = text.split("-");
	if (parts.count()!=3) return false;
	bool ok;
	int y = parts[0].toInt(&ok);
	if (!ok) return false;
	int m = parts[1].toInt(&ok);
	if (!ok) return false;
	int d = parts[2].toInt(&ok);
	if (!ok) return false;
	return QDate(y, m, d).isValid();
}
