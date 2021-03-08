#include "IgvDialog.h"
#include "GUIHelper.h"
#include <QCheckBox>
#include <QFileInfo>

IgvDialog::IgvDialog(QWidget *parent)
	: QDialog(parent)
	, init_action_(IgvDialog::INIT)
{
	ui_.setupUi(this);
	connect(ui_.tree, SIGNAL(itemChanged(QTreeWidgetItem*,int)), this, SLOT(treeItemChanged(QTreeWidgetItem*)));
}

void IgvDialog::addFile(const FileLocation file, bool checked)
{
	//determine group
	QTreeWidgetItem* group = nullptr;
	for(int i=0; i<ui_.tree->topLevelItemCount(); ++i)
	{
		if (ui_.tree->topLevelItem(i)->text(0)==FileLocationHelper::pathTypeToString(file.type))
		{
			group = ui_.tree->topLevelItem(i);
		}
	}

	//add group if missing
	if (group==nullptr)
	{
		group = new QTreeWidgetItem(QStringList() << FileLocationHelper::pathTypeToString(file.type));
		group->setFlags(Qt::ItemIsUserCheckable|Qt::ItemIsEnabled);		
		if (file.type==PathType::VCF) group->setToolTip(0, "Variant list(s)");
		if (file.type==PathType::BAM) group->setToolTip(0, "Sequencing read file(s)");
		if (file.type==PathType::COPY_NUMBER_CALLS) group->setToolTip(0, "Copy-number file(s)");
		if (file.type==PathType::BAF) group->setToolTip(0, "b-allele frequency file(s)");
		//if (file.type==PathType::CNV_CALLS) group->setToolTip(0, "regions file(s)");
		if (file.type==PathType::OTHER) group->setToolTip(0, "Custom tracks");
		ui_.tree->addTopLevelItem(group);
	}

	//add file
	QTreeWidgetItem* item = new QTreeWidgetItem(QStringList() << file.id);
	item->setToolTip(0, file.filename);
	if (file.is_found)
	{
		item->setFlags(Qt::ItemIsUserCheckable|Qt::ItemIsEnabled);
		item->setCheckState(0, checked ? Qt::Checked : Qt::Unchecked);
	}
	else
	{
		item->setText(0, file.id + " (missing)");
		item->setFlags(Qt::ItemIsUserCheckable);
		item->setCheckState(0, Qt::Unchecked);
	}
	group->addChild(item);

	//expand all items
	ui_.tree->expandAll();

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

IgvDialog::InitAction IgvDialog::initializationAction() const
{
	return init_action_;
}

QStringList IgvDialog::filesToLoad()
{
	QStringList output;

	for(int i=0; i<ui_.tree->topLevelItemCount(); ++i)
	{
		QTreeWidgetItem* group = ui_.tree->topLevelItem(i);
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

void IgvDialog::on_skip_once_clicked()
{
	init_action_ = SKIP_ONCE;
	accept();
}

void IgvDialog::on_skip_session_clicked()
{
	init_action_ = SKIP_SESSION;
	accept();
}

void IgvDialog::treeItemChanged(QTreeWidgetItem* item)
{
	for (int i=0; i<item->childCount(); ++i)
	{
		item->child(i)->setCheckState(0, item->checkState(0));
	}
}
