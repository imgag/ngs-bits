#include "MaintenanceDialog.h"
#include "Helper.h"
#include "NGSD.h"
#include <QTime>
#include <QMetaMethod>

MaintenanceDialog::MaintenanceDialog(QWidget *parent)
	: QDialog(parent)
	, ui_()
{
	ui_.setupUi(this);
	connect(ui_.exec_btn, SIGNAL(clicked()), this, SLOT(executeAction()));
}

void MaintenanceDialog::executeAction()
{
	//init
	ui_.exec_btn->setEnabled(false);
	ui_.output->clear();
	appendOutputLine("START");
	appendOutputLine("");

	//perform action
	QTime timer;
	timer.start();

	try
	{
		QString action = ui_.action->currentText().trimmed();
		if (action.isEmpty() || action[0]=="[")
		{
			appendOutputLine("No action selected!");
		}
		else
		{
			//execute method with the same name as the action
			bool method_found = false;
			QString action_simplified = action.toLower().replace(" ", "");
			const QMetaObject* metaObject = this->metaObject();
			for(int i = metaObject->methodOffset(); i < metaObject->methodCount(); ++i)
			{
				QString method_simplified = metaObject->method(i).name().toLower().trimmed();
				if (method_simplified==action_simplified)
				{
					metaObject->method(i).invoke(this,  Qt::DirectConnection);
					method_found = true;
				}
			}

			if (!method_found)
			{
				THROW(ProgrammingException, "No slot with name " + action_simplified + "' found!");
			}
		}
	}
	catch (Exception& e)
	{
		appendOutputLine("Error:");
		appendOutputLine(e.message());
	}

	//add time to output
	appendOutputLine("");
	appendOutputLine("END");
	appendOutputLine("Time elapsed: " + Helper::elapsedTime(timer));
	ui_.exec_btn->setEnabled(true);
}

void MaintenanceDialog::deleteUnusedSamples()
{
	NGSD db;
	QSet<QString> ref_tables = tablesReferencing(db, "sample");

	QList<int> s_ids = db.getValuesInt("SELECT id FROM sample");
	foreach(int s_id , s_ids)
	{
		QString sample_id_str = QString::number(s_id);

		bool remove = true;
		foreach (const QString& table, ref_tables)
		{
			QString condition = "sample_id="+sample_id_str;
			if (table=="sample_relations") condition = "sample1_id="+sample_id_str + " OR sample2_id="+sample_id_str;
			long long count = db.getValue("SELECT count(*) FROM " + table + " WHERE " + condition).toLongLong();
			if (count>0)
			{
				remove = false;
				break;
			}
		}

		if (remove)
		{
			appendOutputLine("Deleting sample " + db.getValue("SELECT name FROM sample WHERE id='" + sample_id_str + "'").toString());
			db.getQuery().exec("DELETE FROM sample WHERE id="+sample_id_str);
		}
	}
}

void MaintenanceDialog::deleteUnusedVariants()
{
	NGSD db;
	QSet<QString> ref_tables = tablesReferencing(db, "variant");

	QSet<int> var_ids_pub = db.getValuesInt("SELECT variant_id FROM variant_publication WHERE variant_table='variant'").toSet();

	foreach (const QString& chr_name, db.getEnum("variant", "chr"))
	{
		QList<int> v_ids = db.getValuesInt("SELECT id FROM variant WHERE chr='" + chr_name + "'");
		appendOutputLine(chr_name + " - variants to process: " + QString::number(v_ids.count()));
		int c_deleted = 0;
		int c_processed = 0;
		foreach(int v_id , v_ids)
		{
			QString var_id_str = QString::number(v_id);
			++c_processed;

			bool remove = true;
			foreach (const QString& table, ref_tables)
			{
				long long count = db.getValue("SELECT count(*) FROM " + table + " WHERE variant_id="+var_id_str).toLongLong();
				if (count>0)
				{
					remove = false;
					break;
				}
			}

			if (var_ids_pub.contains(v_id))
			{
				remove = false;
			}

			if (remove)
			{
				db.getQuery().exec("DELETE FROM variant WHERE id="+var_id_str);
				++c_deleted;
			}

			if (c_processed%1000000==0)
			{
				appendOutputLine(chr_name + " - deleted " + QString::number(c_deleted) + " of " + QString::number(c_processed) + " processed_variants");
			}
		}
		appendOutputLine(chr_name + " - deleted: " + QString::number(c_deleted));
	}
}

void MaintenanceDialog::appendOutputLine(QString line)
{
	line = line.trimmed();

	if (!line.isEmpty()) line = QDateTime::currentDateTime().toString(Qt::ISODate).replace("T", " ") + " " + line;

	ui_.output->append(line);

	QApplication::processEvents();
}

QSet<QString> MaintenanceDialog::tablesReferencing(NGSD& db, QString referenced_table)
{
	QSet<QString> output;

	foreach(QString table, db.tables())
	{
		const TableInfo& table_info = db.tableInfo(table);
		foreach(QString field, table_info.fieldNames())
		{
			const TableFieldInfo& field_info = table_info.fieldInfo(field);
			if (field_info.type==TableFieldInfo::FK && field_info.fk_table==referenced_table && field_info.fk_field=="id")
			{
				output << table;
			}
		}
	}

	return output;
}
