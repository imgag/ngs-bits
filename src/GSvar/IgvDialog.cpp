#include "IgvDialog.h"
#include "ui_IgvDialog.h"
#include "GUIHelper.h"
#include <QCheckBox>
#include <QFileInfo>

IgvDialog::IgvDialog(QWidget *parent) :
	QDialog(parent),
	ui(new Ui::IgvDialog)
{
	ui->setupUi(this);
}

IgvDialog::~IgvDialog()
{
	delete ui;
}

void IgvDialog::addFile(QString label, QString filename, bool checked)
{
	bool missing = !QFile::exists(filename);

	QCheckBox* box = new QCheckBox();
	box->setText(label + (missing ? " (missing)" : ""));
	box->setToolTip(filename);
	box->setChecked(checked);
	box->setEnabled(!missing);
	ui->layout->addWidget(box);
}

void IgvDialog::addSeparator()
{
	ui->layout->addWidget(GUIHelper::horizontalLine());
}

QStringList IgvDialog::filesToLoad()
{
	QStringList output;
	QList<QCheckBox*> boxes = findChildren<QCheckBox*>();
	foreach(QCheckBox* box, boxes)
	{
		if (box->isEnabled() && box->isChecked())
		{
			output.append(box->toolTip());
		}
	}

	return output;
}
