#include "ExportHistoryDialog.h"
#include "NGSD.h"
#include "GUIHelper.h"

ExportHistoryDialog::ExportHistoryDialog(QWidget* parent, QString cm_id, QString network)
	: QDialog(parent)
	, ui_()
	, cm_id_(cm_id)
{
	ui_.setupUi(this);
	setWindowTitle(windowTitle() + " of " + cm_id);

	ui_.cm_id->setText(cm_id_);
	ui_.network->setText(network);

	updateTable();
}

void ExportHistoryDialog::updateTable()
{
	//clear
	ui_.table->setRowCount(0);

	//init
	NGSD mvh_db(true, "mvh");
	QString id = mvh_db.getValue("SELECT id FROM case_data WHERE cm_id='"+cm_id_+"'", false).toString().trimmed();

	//GRZ
	SqlQuery query = mvh_db.getQuery();
	query.exec("SELECT * FROM submission_grz WHERE case_id='" + id + "' ORDER BY id ASC");
	while(query.next())
	{
		addTableRow("GRZ", query.value("date").toDate(), query.value("type").toString(), query.value("tang").toString(), query.value("status").toString(), query.value("submission_id").toString(), query.value("submission_output").toString());
	}

	//KDK SE
	query.exec("SELECT * FROM submission_kdk_se WHERE case_id='" + id + "' ORDER BY id ASC");
	while(query.next())
	{
		addTableRow("KDK SE", query.value("date").toDate(), query.value("type").toString(), query.value("tank").toString(), query.value("status").toString(), query.value("submission_id").toString(), query.value("submission_output").toString());
	}

	GUIHelper::resizeTableCellWidths(ui_.table);
	GUIHelper::resizeTableCellHeightsToMinimum(ui_.table);
}

void ExportHistoryDialog::addTableRow(QString data_center, QDate date, QString type, QString tan, QString status, QString submission_id, QString submission_output)
{
	int r = ui_.table->rowCount();
	ui_.table->setRowCount(r+1);
	ui_.table->setItem(r, 0, GUIHelper::createTableItem(data_center));
	ui_.table->setItem(r, 1, GUIHelper::createTableItem(date.toString(Qt::ISODate)));
	ui_.table->setItem(r, 2, GUIHelper::createTableItem(type));
	ui_.table->setItem(r, 3, GUIHelper::createTableItem(tan));
	QTableWidgetItem* item = GUIHelper::createTableItem(status);
	if (status=="failed") item->setForeground(Qt::red);
	if (status=="done") item->setForeground(Qt::darkGreen);
	ui_.table->setItem(r, 4, item);
	ui_.table->setItem(r, 5, GUIHelper::createTableItem(submission_id));
	item = GUIHelper::createTableItem(submission_output.isEmpty() ? "" : "[see tooltip]");
	item->setToolTip(submission_output);
	ui_.table->setItem(r, 6, item);
}
