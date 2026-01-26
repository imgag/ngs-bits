#include "ColumnConfigWidget.h"
#include "VariantType.h"
#include "GlobalServiceProvider.h"
#include "GUIHelper.h"
#include "Settings.h"
#include <QFileDialog>
#include <QMessageBox>

ColumnConfigWidget::ColumnConfigWidget(QWidget* parent)
	: QWidget(parent)
	, ui_()
	, title_("Column settings")
	, current_type_()
	, configs_()
{
	ui_.setupUi(this);
	connect(ui_.up_btn, SIGNAL(clicked(bool)), this, SLOT(moveRowUp()));
	connect(ui_.down_btn, SIGNAL(clicked(bool)), this, SLOT(moveRowDown()));
	connect(ui_.delete_selected_btn, SIGNAL(clicked(bool)), this, SLOT(deleteSelectedColumn()));
	connect(ui_.table, SIGNAL(cellChanged(int,int)), this, SLOT(sizeChanged(int,int)));
	ui_.load_store_btn->setMenu(new QMenu());
	ui_.load_store_btn->menu()->addAction(QIcon(":/Icons/Add_batch.png"), "Import missing columns from current sample", this, SLOT(addColumnsFromSample()));
	ui_.load_store_btn->menu()->addSeparator();
	ui_.load_store_btn->menu()->addAction(QIcon(":/Icons/Export.png"), "Export column settings of current type", this, SLOT(exportCurrent()));
	ui_.load_store_btn->menu()->addAction(QIcon(":/Icons/Export.png"), "Export column settings of all types", this, SLOT(exportAll()));
	ui_.load_store_btn->menu()->addAction(QIcon(":/Icons/Import.png"), "Import column settings", this, SLOT(import()));
	ui_.load_store_btn->menu()->addSeparator();
	ui_.load_store_btn->menu()->addAction(QIcon(":/Icons/Remove.png"), "Clear columns", this, SLOT(clearColumns()));

	//add types and load configs
	for (int i=0; i<(int)VariantType::INVALID; ++i)
	{
		if (i==(int)VariantType::RES) continue;

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
	connect(ui_.type, SIGNAL(currentTextChanged(QString)), this, SLOT(switchToType(QString)));

	ui_.table->setColumnWidth(0, 200);
}

void ColumnConfigWidget::addColumnsFromSample()
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
			current_names << info.name;
		}
		current_names << "tumor_af" << "tumor_dp" << "normal_af" << "normal_dp"; //special columns for somatic/cfDNA

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
	else if (type==VariantType::CNVS)
	{
		//add missing columns
		const CnvList& vars = GlobalServiceProvider::getCnvList();
		foreach(const QByteArray& name, vars.annotationHeaders())
		{
			if (!current_names.contains(name))
			{
				++c_added;
				addColumn(name);
			}
		}
	}
	else if (type==VariantType::SVS)
	{
		//make sure we do not add genotype columns
		const BedpeFile& vars = GlobalServiceProvider::getSvList();
		foreach(QString sample_name, vars.sampleHeaderInfo().sampleNames())
		{
			current_names << sample_name;
		}

		//add missing columns
		foreach(const QByteArray& name, vars.annotationHeaders())
		{
			if(name.startsWith("STRAND_") || name.startsWith("NAME_") || name=="ID" || name=="FORMAT" || name=="INFO_A" || name=="INFO_B") continue;

			if (!current_names.contains(name))
			{
				++c_added;
				addColumn(name);
			}
		}
	}
	else
	{
		THROW(Exception, "Unhandled variant type in ColumnConfigWidget: "+variantTypeToString(type)+"!");
	}

	GUIHelper::resizeTableCellHeightsToFirst(ui_.table);

	//inform user
	QMessageBox::information(this, title_, "Appended "+QString::number(c_added)+" column names to the end of the list!");
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

void ColumnConfigWidget::deleteSelectedColumn()
{
	//no selection > abort
	QList<int> seleted_rows = GUIHelper::selectedTableRows(ui_.table);
	if (seleted_rows.isEmpty()) return;

	//delete
	ui_.table->removeRow(seleted_rows[0]);
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

void ColumnConfigWidget::switchToType(QString new_type)
{
	if (new_type==current_type_) return;

	//check type is valid
	if (ui_.type->findText(new_type)==-1) THROW(ArgumentException, "Invalid variant type string '" + new_type + "' in ColumnConfigWidget::switchToType!");

	//store old config
	writeBackCurrentConfig();

	//load new config
	loadConfig(new_type);

	//set current type
	current_type_ = new_type;

	//update combobox if necessary
	if (ui_.type->currentText()!=new_type)
	{
		ui_.type->setCurrentText(new_type);
	}
}

void ColumnConfigWidget::exportCurrent()
{
	//write back current column settings
	writeBackCurrentConfig();

	QString title = title_ + " - export";
	try
	{
		//create output
		QStringList lines;
		QString name = ui_.type->currentText();
		QString key = variantTypeToKey(name);
		lines << key + " = " + configs_[name].toString();

		//get filename
		QString filename = QFileDialog::getSaveFileName(this, title, QDir::homePath() + QDir::separator() + key + ".txt", tr("TXT (*.txt);;All Files (*)"));
		if (filename.isEmpty()) return;

		//store
		QSharedPointer<QFile> file = Helper::openFileForWriting(filename);
		Helper::storeTextFile(file, lines);
	}
	catch (Exception& e)
	{
		GUIHelper::showException(this, e, title);
	}
}

void ColumnConfigWidget::exportAll()
{
	//write back current column settings
	writeBackCurrentConfig();

	QString title = title_ + " - export";
	try
	{
		//create output
		QStringList lines;
		for(int i=0; i<ui_.type->count(); ++i)
		{
			QString name = ui_.type->itemText(i);
			QString key = variantTypeToKey(name);
			lines << key + " = " + configs_[name].toString();
		}

		//get filename
		QString filename = QFileDialog::getSaveFileName(this, title, QDir::homePath() + QDir::separator() + "column_config_all.txt", tr("TXT (*.txt);;All Files (*)"));
		if (filename.isEmpty()) return;

		//store
		QSharedPointer<QFile> file = Helper::openFileForWriting(filename);
		Helper::storeTextFile(file, lines);
	}
	catch (Exception& e)
	{
		GUIHelper::showException(this, e, title);
	}
}

void ColumnConfigWidget::import()
{
	QString title = title_ + " - export";
	try
	{
		//get filename
		QString filename = QFileDialog::getOpenFileName(this, title, QDir::homePath() + QDir::separator() + ".txt", tr("TXT (*.txt);;All Files (*)"));
		if (filename.isEmpty()) return;

		//import column configs
		QStringList lines = Helper::loadTextFile(filename);
		foreach(QString line, lines)
		{
			line = line.replace("column_config_", "").trimmed();
			if (line.isEmpty()) continue;

			int sep = line.indexOf('=');
			if (sep!=-1)
			{
				QString type = line.left(sep).replace("_", " ").trimmed();
				if (!configs_.contains(type)) THROW(FileParseException, "Cannot import invalid variant type '" + type + "'!");
				configs_[type] = ColumnConfig::fromString(line.mid(sep+1).trimmed());
			}
		}

		//update GUI
		loadConfig(ui_.type->currentText());
	}
	catch (Exception& e)
	{
		GUIHelper::showException(this, e, title);
	}
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
	if (current_type_.isEmpty()) return;

	//create config
	ColumnConfig config;

	for (int row=0; row<ui_.table->rowCount(); ++row)
	{
		QString name = ui_.table->item(row, 0)->text();
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
	return "column_config_"+type.replace(" ", "_");
}
