#include "FilterCascadeWidget.h"
#include "FilterEditDialog.h"
#include "GSvarHelper.h"
#include <QTextDocument>
#include <QMenu>
#include <QDir>
#include <QFileDialog>
#include <QMessageBox>
#include <QStandardPaths>

FilterCascadeWidget::FilterCascadeWidget(QWidget* parent)
	: QWidget(parent)
	, ui_()
	, subject_(VariantType::INVALID)
{
	ui_.setupUi(this);

	connect(ui_.filters_entries, SIGNAL(itemSelectionChanged()), this, SLOT(updateButtons()));
	connect(ui_.filters_entries, SIGNAL(itemActivated(QListWidgetItem*)), this, SLOT(editSelectedFilter()));
	connect(ui_.filters_entries, SIGNAL(itemChanged(QListWidgetItem*)), this, SLOT(toggleSelectedFilter(QListWidgetItem*)));
	connect(ui_.filter_delete, SIGNAL(pressed()), this, SLOT(deleteSelectedFilter()));
	connect(ui_.filter_edit, SIGNAL(pressed()), this, SLOT(editSelectedFilter()));
	connect(ui_.filter_add, SIGNAL(pressed()), this, SLOT(addFilter()));
	connect(ui_.filter_up, SIGNAL(pressed()), this, SLOT(moveUpSelectedFilter()));
	connect(ui_.filter_down, SIGNAL(pressed()), this, SLOT(moveDownSelectedFilter()));
	connect(ui_.load_btn, SIGNAL(pressed()), this, SLOT(loadFilter()));
	connect(ui_.store_btn, SIGNAL(pressed()), this, SLOT(storeFilter()));
}

void FilterCascadeWidget::setSubject(VariantType subject)
{
	subject_ = subject;
}

int FilterCascadeWidget::currentFilterIndex() const
{
	auto selection = ui_.filters_entries->selectedItems();
	if (selection.count()!=1) return -1;

	return ui_.filters_entries->row(selection[0]);
}

void FilterCascadeWidget::markFailedFilters()
{
	for(int i=0; i<ui_.filters_entries->count(); ++i)
	{
		QListWidgetItem* item = ui_.filters_entries->item(i);

		QStringList errors = filters_.errors(i);
		if (errors.isEmpty())
		{
			item->setBackground(QBrush());
		}
		else
		{
			item->setToolTip(filters_[i]->description().join("\n") + "\n\nErrors:\n" + errors.join("\n"));
            item->setBackground(QBrush(QColor(255,160,110)));
		}
	}
}

void FilterCascadeWidget::editColumnFilter(QString column)
{
	//check if column filter is already present
	int index = -1;
	for (int i=0; i<filters_.count(); ++i)
	{
		if (filters_[i]->name()!="Column match") continue;

		foreach(const FilterParameter& param, filters_[i]->parameters())
		{
			if (param.name!="column") continue;
			if (param.value.toString()!=column) continue;

			index = i;
		}
	}

	//edit filter that is present
	if (index!=-1)
	{
		FilterEditDialog dlg(filters_[index], this);
		if (dlg.exec()==QDialog::Accepted)
		{
			updateGUI();
			focusFilter(index);
			emit filterCascadeChanged();
		}
	}
	else //add filter
	{
		QSharedPointer<FilterBase> filter(new FilterColumnMatchRegexp());
		filter->setString("column", column);
		filter->setString("action", "FILTER");

		FilterEditDialog dlg(filter, this);
		if (dlg.exec()==QDialog::Accepted)
		{
			filters_.add(filter);
			updateGUI();
			focusFilter(filters_.count()-1);
			emit filterCascadeChanged();
		}
	}
}

void FilterCascadeWidget::clear()
{
	filters_.clear();
	updateGUI();
	emit filterCascadeChanged();
}

void FilterCascadeWidget::focusFilter(int index)
{
	//no entries > return
	int filter_count = ui_.filters_entries->count();
	if (filter_count==0) return;

	if (index<0) //index too small > focus first item
	{
		ui_.filters_entries->item(0)->setSelected(true);
	}
	else if (index>=filter_count) //index too big > focus last item
	{
		ui_.filters_entries->item(filter_count-1)->setSelected(true);
	}
	else //index ok > focus selected item
	{
		ui_.filters_entries->item(index)->setSelected(true);
	}
}

void FilterCascadeWidget::setFilters(const FilterCascade& filters)
{
	filters_ = filters;

	//overwrite valid entries of 'filter' column
	for(int i=0; i<filters_.count(); ++i)
	{
		// Variant filter column
		if (filters_[i]->name()=="Filter columns")
		{
			filters_[i]->overrideConstraint("entries", "valid", valid_filter_entries_.join(','));
		}

		// SV filter column
		if (filters_[i]->name()=="SV filter columns")
		{
			filters_[i]->overrideConstraint("entries", "valid", valid_filter_entries_.join(','));
		}

	}

	updateGUI();

	emit filterCascadeChanged();
}

void FilterCascadeWidget::setValidFilterEntries(const QStringList& filter_entries)
{
	valid_filter_entries_ = filter_entries;
}

const FilterCascade&FilterCascadeWidget::filters() const
{
	return filters_;
}

void FilterCascadeWidget::updateGUI()
{
	ui_.filters_entries->clear();
	for(int i=0; i<filters_.count(); ++i)
	{
		//remove HTML special characters for GUI
		QTextDocument converter;
		converter.setHtml(filters_[i]->toText());
		QString text = converter.toPlainText();

		QListWidgetItem* item = new QListWidgetItem(text);
		item->setCheckState(filters_[i]->enabled() ? Qt::Checked : Qt::Unchecked);
		item->setToolTip(filters_[i]->description().join("\n"));
		ui_.filters_entries->addItem(item);
	}
	updateButtons();
}



void FilterCascadeWidget::updateButtons()
{
	int index = currentFilterIndex();
	ui_.filter_delete->setEnabled(index!=-1);
	ui_.filter_edit->setEnabled(index!=-1 && filters_[index]->parameters().count()>0);
	ui_.filter_up->setEnabled(index>0);
	ui_.filter_down->setEnabled(index!=-1 && index!=ui_.filters_entries->count()-1);
}

void FilterCascadeWidget::addFilter()
{
	//show filter menu
	QMenu menu;
	foreach(QString filter_name, FilterFactory::filterNames(subject_))
	{
		menu.addAction(filter_name);
	}
	QAction* action = menu.exec(QCursor::pos());
	if(action==nullptr) return;

	//create filter
	QSharedPointer<FilterBase> filter = FilterFactory::create(action->text());
	// Variant filter column
	if (filter->name()=="Filter columns")
	{
		filter->overrideConstraint("entries", "valid", valid_filter_entries_.join(','));
	}
	// SV filter column
	if (filter->name()=="SV filter columns")
	{
		filter->overrideConstraint("entries", "valid", valid_filter_entries_.join(','));
	}

	//set genome build if the filter contains the parameter
	if (filter->hasParameter("build", FilterParameterType::STRING))
	{
		filter->setString("build", buildToString(GSvarHelper::build()));
	}

	//determine if filter should be added
	bool add_filter = false;
	if (filter->parameters().count()>0)
	{
		FilterEditDialog dlg(filter, this);
		add_filter = dlg.exec()==QDialog::Accepted;
	}
	else
	{
		add_filter = true;
	}

	//add filter
	if (add_filter)
	{
		filters_.add(filter);
		updateGUI();
		focusFilter(filters_.count()-1);
		emit filterCascadeChanged();
	}
}

void FilterCascadeWidget::editSelectedFilter()
{
	int index = currentFilterIndex();
	if (index==-1) return;

	//no parameters => no edit dialog
	if (filters_[index]->parameters().count()==0) return;

	FilterEditDialog dlg(filters_[index], this);
	if (dlg.exec()==QDialog::Accepted)
	{
		updateGUI();
		focusFilter(index);
		emit filterCascadeChanged();
	}
}

void FilterCascadeWidget::deleteSelectedFilter()
{
	int index = currentFilterIndex();
	if (index==-1) return;

	filters_.removeAt(index);

	updateGUI();
	focusFilter(index);
	emit filterCascadeChanged();
}

void FilterCascadeWidget::moveUpSelectedFilter()
{
	int index = currentFilterIndex();
	if (index==-1) return;

	filters_.moveUp(index);

	updateGUI();
	focusFilter(index-1);
	emit filterCascadeChanged();
}

void FilterCascadeWidget::moveDownSelectedFilter()
{
	int index = currentFilterIndex();
	if (index==-1) return;

	filters_.moveDown(index);

	updateGUI();
	focusFilter(index+1);
	emit filterCascadeChanged();
}

void FilterCascadeWidget::toggleSelectedFilter(QListWidgetItem* item)
{
	//determine item index
	int index = ui_.filters_entries->row(item);
	if (index==-1) return;

	//check that check state changed (this slot is called for every change of an item)
	if ( (item->checkState()==Qt::Checked && !filters_[index]->enabled()) || (item->checkState()==Qt::Unchecked && filters_[index]->enabled()))
	{
		filters_[index]->toggleEnabled();

		emit filterCascadeChanged();
	}
}

QString FilterCascadeWidget::filtersPath(VariantType type)
{
	QStringList default_paths = QStandardPaths::standardLocations(QStandardPaths::AppLocalDataLocation);
	if(default_paths.isEmpty()) THROW(Exception, "No local application data path was found!");

	QString path = default_paths[0] + QDir::separator() + "filters" + QDir::separator();
	if (type==VariantType::SNVS_INDELS)
	{
		path += "SNVS_INDELS";
	}
	else if (type==VariantType::CNVS)
	{
		path += "CNVS";
	}
	else if (type==VariantType::SVS)
	{
		path += "SVS";
	}
	else
	{
		THROW(ProgrammingException, "Unhandled variant type in FilterCascadeWidget::filtersPath()!");
	}

	if(Helper::mkdir(path)==-1)
	{
		THROW(ProgrammingException, "Could not create application filter path '" + path + "'!");
	}

	return path;
}

void FilterCascadeWidget::loadFilter()
{
	try
	{
		//get filename
		QString path = filtersPath(subject_);
		QString filename = QFileDialog::getOpenFileName(this, "Load filter", path, "*.txt");
		if (filename.isEmpty()) return;

		//load
		filters_.load(filename);

		//GUI
		updateGUI();
		emit customFilterLoaded();
	}
	catch(Exception& e)
	{
		QMessageBox::critical(this, "Load filter", "Could not load filter:\n" + e.message());
	}
}

void FilterCascadeWidget::storeFilter()
{
	try
	{
		//get filename
		QString path = filtersPath(subject_);
		QString filename = QFileDialog::getSaveFileName(this, "Store filter", path, "*.txt");
		if (filename.isEmpty()) return;

		//store
		filters_.store(filename);
	}
	catch(Exception& e)
	{
		QMessageBox::critical(this, "Store filter", "Could not store filter:\n" + e.message());
	}
}
