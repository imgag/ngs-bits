#include "ColumnConfigWidget.h"
#include "VariantType.h"
#include "GlobalServiceProvider.h"
#include "GUIHelper.h"

#include <QCheckBox>
#include <QMessageBox>
#include <QToolTip>

ColumnConfigWidget::ColumnConfigWidget(QWidget* parent)
	: QWidget(parent)
	, ui_()
	, title_("Column settings")
	, current_type_()
	, configs_()
{
	ui_.setupUi(this);
	connect(ui_.import_btn, SIGNAL(clicked(bool)), this, SLOT(importColumns()));
	connect(ui_.clear_btn, SIGNAL(clicked(bool)), this, SLOT(clearColumns()));
	connect(ui_.up_btn, SIGNAL(clicked(bool)), this, SLOT(moveRowUp()));
	connect(ui_.down_btn, SIGNAL(clicked(bool)), this, SLOT(moveRowDown()));
	connect(ui_.table, SIGNAL(cellChanged(int,int)), this, SLOT(sizeChanged(int,int)));
	ui_.load_store_btn->setMenu(new QMenu());
	ui_.load_store_btn->menu()->addAction("Export column settings", this, SLOT(exportCurrent()));
	ui_.load_store_btn->menu()->addAction("Export column settings of all types", this, SLOT(exportAll()));
	ui_.load_store_btn->menu()->addSeparator();
	ui_.load_store_btn->menu()->addAction("Imoport column settings", this, SLOT(import()));

	//add types and load configs
	for (int i=0; i<(int)VariantType::INVALID; ++i)
	{
		QString name = variantTypeToString((VariantType)i);
		ui_.type->addItem(name);

		QString key = variantTypeToKey(name);
		if(Settings::contains(key))
		{
			configs_.insert(name, ColumnConfig::fromString(Settings::string(key)));
		}
		else
		{
			configs_.insert(name, ColumnConfig());
		}
	}

	//load small variant config and connect signal afterwards
	loadConfig("small variant");
	connect(ui_.type, SIGNAL(currentTextChanged(QString)), this, SLOT(typeChanged(QString)));

	ui_.table->setColumnWidth(0, 200);
}

void ColumnConfigWidget::importColumns()
{
	//determine current columns names
	QSet<QString> current_names;
	for (int row=0; row<ui_.table->rowCount(); ++row)
	{
		current_names << ui_.table->item(row, 0)->text();
	}

	//add missing columns
	int c_added = 0;
	VariantType type = stringToVariantType(ui_.type->currentText());
	if (type==VariantType::SNVS_INDELS)
	{
		//make sure we do not add genotype columns
		const VariantList& vars = GlobalServiceProvider::getSmallVariantList();
		foreach(SampleInfo info, vars.getSampleHeader(false))
		{
			current_names<< info.name;
		}

		//add missing columns
		foreach(const VariantAnnotationHeader& header, vars.annotations())
		{
			QString name = header.name();
			if (!current_names.contains(name))
			{
				++c_added;
				addColumn(name);
			}
		}
	}
	else
	{
		//TODO Marc: other types
	}

	GUIHelper::resizeTableCellHeightsToFirst(ui_.table);

	//inform user
	if (c_added>0)
	{
		QMessageBox::information(this, title_, "Appended "+QString::number(c_added)+" column names to the end of the list!");
	}
}

void ColumnConfigWidget::clearColumns()
{
	if (ui_.table->rowCount()==0) return;

	if (QMessageBox::question(this, title_, "Do you want to remove all column settings?")==QMessageBox::Yes)
	{
		ui_.table->setRowCount(0);
	}
}

void ColumnConfigWidget::addColumn(const QString& name)
{
	addColumn(name, ColumnInfo());
}

void ColumnConfigWidget::addColumn(const QString& name, const ColumnInfo& info)
{
	int row = ui_.table->rowCount();
	ui_.table->setRowCount(row+1);

	ui_.table->setItem(row, 0, GUIHelper::createTableItem(name));

	QTableWidgetItem* min_w = GUIHelper::createTableItem(info.min_width<=0 ? "" : QString::number(info.min_width));
	min_w->setFlags(min_w->flags() | Qt::ItemIsEditable);
	ui_.table->setItem(row, 1, min_w);

	QTableWidgetItem* max_w = GUIHelper::createTableItem(info.max_width<=0 ? "" : QString::number(info.max_width));
	max_w->setFlags(max_w->flags() | Qt::ItemIsEditable);
	ui_.table->setItem(row, 2, max_w);

	QTableWidgetItem* hidden = GUIHelper::createTableItem("");
	hidden->setCheckState(info.hidden ? Qt::Checked : Qt::Unchecked);
	ui_.table->setItem(row, 3, hidden);
}

void ColumnConfigWidget::moveRowUp()
{
	//no selection > abort
	QList<int> seleted_rows = GUIHelper::selectedTableRows(ui_.table);
	if (seleted_rows.isEmpty()) return;

	//already at top > abort
	int row = seleted_rows[0];
	if (row==0) return;

	swapRows(row, row-1);
	ui_.table->setCurrentItem(ui_.table->item(row-1, 0));
}

void ColumnConfigWidget::moveRowDown()
{
	//no selection > abort
	QList<int> seleted_rows = GUIHelper::selectedTableRows(ui_.table);
	if (seleted_rows.isEmpty()) return;

	//already at bottom > abort
	int row = seleted_rows[0];
	if (row==ui_.table->rowCount()-1) return;

	swapRows(row, row+1);
	ui_.table->setCurrentItem(ui_.table->item(row+1, 0));
}

void ColumnConfigWidget::sizeChanged(int row, int col)
{
	//only for size column
	if (col!=1 && col!=2) return;

	QTableWidgetItem* item = ui_.table->item(row, col);
	if (item==nullptr) return;

	if (!Helper::isNumeric(item->text()) || item->text().toInt()<=0)
	{
		item->setText("");
	}
}

void ColumnConfigWidget::typeChanged(QString new_type)
{
	if (new_type==current_type_) return;

	//store old config
	writeBackCurrentConfig();

	//load new config
	loadConfig(new_type);
	current_type_ = new_type;
}

void ColumnConfigWidget::exportCurrent()
{

}

void ColumnConfigWidget::exportAll()
{

}

void ColumnConfigWidget::import()
{

}

void ColumnConfigWidget::store()
{
	//make sure to write back the latest changes before storing
	writeBackCurrentConfig();

	//store condig of all variant types to settings
	for(int i=0; i<ui_.type->count(); ++i)
	{
		QString name = ui_.type->itemText(i);
		QString key = variantTypeToKey(name);
		Settings::setString(key, configs_[name].toString());
	}
}

void ColumnConfigWidget::swapRows(int from, int to)
{
	QString tmp = ui_.table->item(to, 0)->text();
	ui_.table->item(to, 0)->setText(ui_.table->item(from, 0)->text());
	ui_.table->item(from, 0)->setText(tmp);

	tmp = ui_.table->item(to, 1)->text();
	ui_.table->item(to, 1)->setText(ui_.table->item(from, 1)->text());
	ui_.table->item(from, 1)->setText(tmp);

	tmp = ui_.table->item(to, 2)->text();
	ui_.table->item(to, 2)->setText(ui_.table->item(from, 2)->text());
	ui_.table->item(from, 2)->setText(tmp);

	Qt::CheckState tmp2 = ui_.table->item(to, 3)->checkState();
	ui_.table->item(to, 3)->setCheckState(ui_.table->item(from, 3)->checkState());
	ui_.table->item(from, 3)->setCheckState(tmp2);
}

void ColumnConfigWidget::writeBackCurrentConfig()
{
	qDebug() << __LINE__ << current_type_;
	if (current_type_.isEmpty()) return;

	//create config
	ColumnConfig config;

	for (int row=0; row<ui_.table->rowCount(); ++row)
	{
		qDebug() << __LINE__ << row;
		QString name = ui_.table->item(row, 0)->text();
		qDebug() << __LINE__ << name;
		ColumnInfo info;

		QString tmp = ui_.table->item(row, 1)->text();
		if (Helper::isNumeric(tmp))
		{
			info.min_width = tmp.toInt();
		}

		tmp = ui_.table->item(row, 2)->text();
		if (Helper::isNumeric(tmp))
		{
			info.max_width = tmp.toInt();
		}

		info.hidden = ui_.table->item(row, 3)->checkState()==Qt::Checked;

		config.append(name, info);
	}

	configs_[current_type_] = config;
}

void ColumnConfigWidget::loadConfig(QString type)
{
	qDebug() << __LINE__ << type;

	//clear
	ui_.table->setRowCount(0);

	//add new items
	ColumnConfig config = configs_[type];
	foreach (const QString& name, config.columns())
	{
		addColumn(name, config.info(name));
	}

	//set new type
	current_type_ = type;
}

QString ColumnConfigWidget::variantTypeToKey(QString type)
{
	return "column_config_"+type.replace(" ", "_").toLower();
}

//TODO:
//- context menu to delete single column
