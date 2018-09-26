#include "IgvDialog.h"
#include "ui_IgvDialog.h"
#include "GUIHelper.h"
#include <QCheckBox>
#include <QFileInfo>

IgvDialog::IgvDialog(QWidget *parent)
	: QDialog(parent)
	, ui(new Ui::IgvDialog)
	, skip_session_(false)
{
	ui->setupUi(this);
	connect(ui->tree, SIGNAL(itemChanged(QTreeWidgetItem*,int)), this, SLOT(treeItemChanged(QTreeWidgetItem*)));
}

IgvDialog::~IgvDialog()
{
	delete ui;
}

void IgvDialog::addFile(QString label, QString type, QString filename, bool checked)
{
	//determine group
	QTreeWidgetItem* group = nullptr;
	for(int i=0; i<ui->tree->topLevelItemCount(); ++i)
	{
		if (ui->tree->topLevelItem(i)->text(0)==type)
		{
			group = ui->tree->topLevelItem(i);
		}
	}

	//add group if missing
	if (group==nullptr)
	{
		group = new QTreeWidgetItem(QStringList() << type);
		group->setFlags(Qt::ItemIsUserCheckable|Qt::ItemIsEnabled);
		if (type=="VCF") group->setToolTip(0, "Variant list(s)");
		if (type=="BAM") group->setToolTip(0, "Sequencing read file(s)");
		if (type=="BAF") group->setToolTip(0, "b-allele frequency file(s)");
		if (type=="BED") group->setToolTip(0, "regions file(s)");
		if (type=="custom track") group->setToolTip(0, "Custom tracks");
		ui->tree->addTopLevelItem(group);
	}

	//add file
	QTreeWidgetItem* item = new QTreeWidgetItem(QStringList() << label);
	item->setToolTip(0, filename);
	if (QFile::exists(filename))
	{
		item->setFlags(Qt::ItemIsUserCheckable|Qt::ItemIsEnabled);
		item->setCheckState(0, checked ? Qt::Checked : Qt::Unchecked);
	}
	else
	{
		item->setText(0, label + " (missing)");
		item->setFlags(Qt::ItemIsUserCheckable);
		item->setCheckState(0, Qt::Unchecked);
	}
	group->addChild(item);

	//expand all items
	ui->tree->expandAll();

	//set group check stat according to items
	bool group_checked = false;
	for(int j=0; j<group->childCount(); ++j)
	{
		if (group->child(j)->checkState(0)==Qt::Checked)
		{
			group_checked = true;
		}
	}
	group->setCheckState(0, group_checked ? Qt::Checked : Qt::Unchecked);
}

bool IgvDialog::skipForSession() const
{
	return skip_session_;
}

QStringList IgvDialog::filesToLoad()
{
	QStringList output;

	for(int i=0; i<ui->tree->topLevelItemCount(); ++i)
	{
		QTreeWidgetItem* group = ui->tree->topLevelItem(i);
		for(int j=0; j<group->childCount(); ++j)
		{
			if (group->child(j)->checkState(0)==Qt::Checked)
			{
				output << group->child(j)->toolTip(0);
			}
		}

	}

	return output;
}

void IgvDialog::on_skip_session_clicked()
{
	skip_session_ = true;
	reject();
}

void IgvDialog::treeItemChanged(QTreeWidgetItem* item)
{
	for (int i=0; i<item->childCount(); ++i)
	{
		item->child(i)->setCheckState(0, item->checkState(0));
	}
}
