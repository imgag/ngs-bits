#include "IgvDialog.h"
#include "GUIHelper.h"
#include "NGSHelper.h"
#include <QCheckBox>
#include <QFileInfo>

IgvDialog::IgvDialog(QWidget *parent)
	: QDialog(parent)
	, init_action_(IgvDialog::INIT)
{
	ui_.setupUi(this);
	connect(ui_.tree, SIGNAL(itemChanged(QTreeWidgetItem*,int)), this, SLOT(treeItemChanged(QTreeWidgetItem*)));
}

void IgvDialog::addFile(const FileLocation& file, bool checked)
{
	//determine group (if present)
	QTreeWidgetItem* group = nullptr;
	for(int i=0; i<ui_.tree->topLevelItemCount(); ++i)
	{
		if (ui_.tree->topLevelItem(i)->text(0)==file.typeAsHumanReadableString())
		{
			group = ui_.tree->topLevelItem(i);
		}
	}
	if (group==nullptr) //add group if missing
	{
		group = new QTreeWidgetItem(QStringList() << file.typeAsHumanReadableString());
		group->setFlags(Qt::ItemIsUserCheckable|Qt::ItemIsEnabled);
		ui_.tree->addTopLevelItem(group);
	}

	//add file
	QTreeWidgetItem* item = new QTreeWidgetItem(QStringList() << file.id);
	item->setToolTip(0, NGSHelper::stripSecureToken(file.filename));
	if (file.exists)
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
		item->child(i)->setCheckState(0, item->child(i)->isDisabled() ? Qt::Unchecked :  item->checkState(0));
	}
}
