#include "TransferredVariantDialog.h"
#include "QtWidgets/qwidget.h"
#include "GlobalServiceProvider.h"
#include "qdialog.h"
#include "qformlayout.h"
#include "qmenu.h"
#include "ui_TransferredVariantDialog.h"
#include <QTableWidgetItem>

#include <GUIHelper.h>

TransferredVariantDialog::TransferredVariantDialog(int ps_id, QWidget *parent)
	: QWidget(parent)
	, ui_(new Ui::TransferredVariantDialog)
	, ps_id_(ps_id)
{
	//init
	ui_->setupUi(this);

	//connect signals and slots
	connect(ui_->tw_variants,SIGNAL(customContextMenuRequested(QPoint)),this,SLOT(showContextMenu(QPoint)));
	connect(ui_->tw_variants,SIGNAL(itemDoubleClicked(QTableWidgetItem*)),this,SLOT(varDoubleClicked(QTableWidgetItem*)));


	updateTable();
}

TransferredVariantDialog::~TransferredVariantDialog()
{
	delete ui_;
}

void TransferredVariantDialog::showContextMenu(QPoint pos)
{
	qDebug() << "show context menu";
	QList<int> selected_rows = GUIHelper::selectedTableRows(ui_->tw_variants);
	if(selected_rows.count() != 1) return;
	int row = selected_rows.at(0);
	int id = ui_->tw_variants->item(row, 0)->text().toInt();

	//create menu
	QMenu menu(ui_->tw_variants);

	//detailed view
	QAction* a_detailed_view = menu.addAction("Show details");

	//change status
	QMenu* sub_menu = menu.addMenu("Change variant status to ...");
	QStringList available_status = db_.getEnum("report_configuration_failed_transfer", "status");
	qDebug() << available_status;
	available_status.removeAll(transferred_variant_data_.value(id).value("Status"));
	qDebug() << available_status;
	QMap<QAction*,QString> change_actions;
	foreach (const QString& status, available_status)
	{
		QAction* action = sub_menu->addAction(status);
		change_actions.insert(action, status);
	}

	//execute menu
	QAction* action_to_execute = menu.exec(ui_->tw_variants->viewport()->mapToGlobal(pos));
	if (action_to_execute == nullptr) return;

	if (action_to_execute == a_detailed_view)
	{
		showDetailedView(id);
	}
	else if (change_actions.contains(action_to_execute))
	{
		changeVariantStatus(id, change_actions.value(action_to_execute));
	}
}

void TransferredVariantDialog::showDetailedView(int id)
{
	QWidget* detail_view = new QWidget(this);
	QFormLayout* layout = new QFormLayout(detail_view);
	for (auto [key, value] : transferred_variant_data_.value(id).asKeyValueRange())
	{
		if ((key == "id") || (key.endsWith("_id"))) continue;
		QLabel* value_label = new QLabel(value);
		value_label->setTextInteractionFlags(Qt::TextSelectableByMouse);
		layout->addRow(key + ":", value_label);
	}
	detail_view->setLayout(layout);

	auto dlg = GUIHelper::createDialog(detail_view, "Detailed view of " + transferred_variant_data_.value(id).value("Variant", ""));
	dlg->resize(400, dlg->height());
	GlobalServiceProvider::addModelessDialog(dlg);
}

void TransferredVariantDialog::changeVariantStatus(int id, const QString& status)
{
	QApplication::setOverrideCursor(Qt::BusyCursor);
	SqlQuery query = db_.getQuery();
	query.prepare("UPDATE `report_configuration_failed_transfer` SET `status`=:0 WHERE id=:1");
	query.bindValue(0, status);
	query.bindValue(1, id);

	query.exec();

	updateTable();
	QApplication::restoreOverrideCursor();
}

void TransferredVariantDialog::varDoubleClicked(QTableWidgetItem* item)
{
	int id = ui_->tw_variants->item(item->row(), 0)->text().toInt();
	showDetailedView(id);
}

void TransferredVariantDialog::updateTable()
{
	ui_->tw_variants->setEnabled(false);
	QApplication::setOverrideCursor(Qt::BusyCursor);
	//reset table
	ui_->tw_variants->setRowCount(0);
	ui_->tw_variants->setSelectionBehavior(QAbstractItemView::SelectRows);
	ui_->tw_variants->setContextMenuPolicy(Qt::CustomContextMenu);
	//create header
	QStringList header;
	header << "id" << "status" << "variant type" << "variant" << "type" << "causal";
	ui_->tw_variants->setColumnCount(header.size());

	for (int col_idx = 0; col_idx < header.size(); ++col_idx)
	{
		ui_->tw_variants->setHorizontalHeaderItem(col_idx, GUIHelper::createTableItem(header.at(col_idx)));
	}

	// get db entries
	SqlQuery query = db_.getQuery();
	query.exec("SELECT * FROM report_configuration_failed_transfer WHERE processed_sample_id=" + QString::number(ps_id_));
	ui_->tw_variants->setRowCount(query.size());
	//clear cache
	transferred_variant_data_.clear();
	int row_idx = 0;
	QString source_sample;
	while(query.next())
	{

		int id = query.value("id").toInt();
		QString status = query.value("status").toString();
		// parse variant description
		QMap<QString,QString> variant_info;
		variant_info.insert("Status", status);
		foreach (const QString& kv_pair, query.value("variant_description").toString().split('\t'))
		{
			QStringList tmp = kv_pair.split(':');
			QString key = tmp.at(0);
			tmp.removeFirst();
			QString value = tmp.join(':');
			variant_info.insert(key, value);
		}
		transferred_variant_data_.insert(id, variant_info);

		// fill table
		int col_idx = 0;
		// id
		ui_->tw_variants->setItem(row_idx, col_idx++, GUIHelper::createTableItem(id));
		// status
		ui_->tw_variants->setItem(row_idx, col_idx++, GUIHelper::createTableItem(status));
		//source sample
		source_sample = variant_info.value("SourceSample", "");
		//variant type
		QString variant_type = variant_info.value("VariantType", "");
		ui_->tw_variants->setItem(row_idx, col_idx++, GUIHelper::createTableItem(variant_type));
		//variant
		QString variant = variant_info.value("Variant", "");
		ui_->tw_variants->setItem(row_idx, col_idx++, GUIHelper::createTableItem(variant));
		//type
		QString type = variant_info.value("type", "");
		ui_->tw_variants->setItem(row_idx, col_idx++, GUIHelper::createTableItem(type));
		//causal
		QString causal = variant_info.value("causal", "");
		ui_->tw_variants->setItem(row_idx, col_idx++, GUIHelper::createTableItem((causal=="1")?"true":"false"));

		row_idx++;
	}

	//hide id column
	ui_->tw_variants->hideColumn(0);

	//update description
	ui_->l_description->setText("The following variants from a previous report configuration (source sample: " + source_sample + ") coudn't be transferred to this sample:");

	//resize widget
	GUIHelper::resizeTableCellWidths(ui_->tw_variants, 400);
	GUIHelper::resizeTableCellHeightsToMinimum(ui_->tw_variants);
	GUIHelper::resizeTableHeight(ui_->tw_variants);


	//activate button and reset busy curser
	ui_->tw_variants->setEnabled(true);
	QApplication::restoreOverrideCursor();
}
