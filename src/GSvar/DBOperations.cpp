#include "DBOperations.h"
#include <QFileDialog>
#include <QMessageBox>

DBOperations::DBOperations(QWidget *parent) :
	QWidget(parent)
  , ui_()
  , init_timer_(this, true)
{
	ui_.setupUi(this);
	connect(ui_.export_table_btn, SIGNAL(clicked(bool)), this, SLOT(exportTable()));
}

void DBOperations::delayedInitialization()
{
	ui_.table_list->setSelectionMode(QAbstractItemView::ExtendedSelection);
	ui_.table_list->addItems(db_.tables());
}

void DBOperations::exportTable()
{
	QString sql_data;
	QList<QListWidgetItem*> selected_tables = ui_.table_list->selectedItems();
	for (int i = 0; i < selected_tables.count(); i++)
	{
		QString table_name = selected_tables[i]->text();
		Log::info("Exporting table `" + table_name + "`");
		QString data = db_.exportTable(table_name);
		if (data.isEmpty()) data = "-- No records found --\n";
		sql_data += "--\n-- TABLE `" + table_name + "`\n--\n\n" + data + "\n";
	}

	QString file_name = QFileDialog::getSaveFileName(this, "Export database tables", QDir::homePath()+QDir::separator()+"db_data_"+QDateTime::currentDateTime().toString("dd_MM_yyyy")+".sql", "SQL (*.sql);;All files (*.*)");
	if (!file_name.isEmpty())
	{
		QSharedPointer<QFile> file = Helper::openFileForWriting(file_name, false);
		QTextStream out(file.data());
		out << sql_data;
		QMessageBox::information(this, "Table export", "Exported " + QString::number(selected_tables.count()) + " table(s) to " + file_name);
	}
}
