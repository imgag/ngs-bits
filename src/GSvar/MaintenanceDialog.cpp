#include "MaintenanceDialog.h"
#include "Helper.h"
#include "NGSD.h"
#include "GenLabDB.h"
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
			appendOutputLine("Deleting sample " + db.sampleName(sample_id_str));
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

void MaintenanceDialog::importStudySamples()
{
	//init
	NGSD db;
	QStringList studies_ngsd = db.getValues("SELECT name FROM study");
	GenLabDB genlab;
	QStringList studies_genlab = genlab.studies();

	//determine studies in GenLab that are not in NGSD
	QStringList missing;
	foreach(const QString& study, studies_genlab)
	{
		if(!studies_ngsd.contains(study, Qt::CaseInsensitive))
		{
			missing << study;
		}
	}
	appendOutputLine("Notice: The following studies are present in GenLab and not in NGSD: " + missing.join(", "));

	//import study samples into NGSD
	foreach(const QString& study, studies_genlab)
	{
		if (missing.contains(study)) continue;

		appendOutputLine("");
		appendOutputLine("Processing study " + study + "...");

		QString study_id = db.getValue("SELECT id FROM study WHERE name COLLATE UTF8_GENERAL_CI LIKE '" + study + "'", false).toString();

		//determine processed sample in study
		QStringList errors;
		QSet<int> genlab_ps_ids = genlab.studySamples(study, errors).toSet();
		appendOutputLine("  Processed samples in study according to GenLab: " + QString::number(genlab_ps_ids.count()));

		//add missing processed samples
		int c_added = 0;
		foreach(int ps_id, genlab_ps_ids)
		{
			//add if not present
			QVariant entry_id = db.getValue("SELECT id FROM study_sample WHERE study_id='"+study_id+"' AND processed_sample_id='"+QString::number(ps_id)+"'", true);
			if (!entry_id.isValid())
			{
				++c_added;
				SqlQuery query = db.getQuery();
				query.exec("INSERT INTO study_sample (study_id, processed_sample_id, study_sample_idendifier) VALUES ("+study_id+", "+QString::number(ps_id)+", 'GenLab import by "+Helper::userName()+" on "+QDate::currentDate().toString()+"')");
			}
		}
		appendOutputLine("  Added processed sample to NGSD: " + QString::number(c_added));

		//show samples that are only in study according to NGSD
		QSet<int> extra_ids = db.getValuesInt("SELECT processed_sample_id FROM study_sample WHERE study_id='" + study_id + "'").toSet();
		extra_ids.subtract(genlab_ps_ids);
		if (extra_ids.count()>0)
		{
			QStringList ps_names;
			foreach(int ps_id, extra_ids)
			{
				ps_names << db.processedSampleName(QString::number(ps_id));
			}
			appendOutputLine("  Notice: Samples in NGSD that are not in GenLab: " + ps_names.join(", "));
		}
	}
}

void MaintenanceDialog::replaceObsolteHPOTerms()
{
	//init
	NGSD db;
	QSet<QString> hpo_terms_valid = db.getValues("SELECT hpo_id FROM hpo_term").toSet();
	QSet<QString> hpo_terms_obsolete = db.getValues("SELECT hpo_id FROM hpo_obsolete").toSet();

	//process disease info
	int c_valid = 0;
	int c_replaced = 0;
	int c_not_replaced = 0;
	int c_invalid = 0;
	SqlQuery query = db.getQuery();
	query.exec("SELECT id, sample_id, disease_info FROM sample_disease_info WHERE type='HPO term id' ORDER BY sample_id ASC");
	while(query.next())
	{
		QByteArray sample_id = query.value("sample_id").toByteArray().trimmed();
		QByteArray hpo_id = query.value("disease_info").toByteArray().trimmed();

		if (hpo_terms_valid.contains(hpo_id))
		{
			++c_valid;
		}
		else if (hpo_terms_obsolete.contains(hpo_id)) //try to replace
		{
			QString replace_term_id = db.getValue("SELECT replaced_by FROM hpo_obsolete WHERE hpo_id='" + hpo_id + "'", false).toString().trimmed();
			if(!replace_term_id.isEmpty()) //replacement term available => replace
			{
				QString replace_term = db.getValue("SELECT hpo_id FROM hpo_term WHERE id='" + replace_term_id + "'", false).toString().trimmed();
				db.getQuery().exec("UPDATE sample_disease_info SET disease_info='" + replace_term + "' WHERE id=" + query.value("id").toByteArray());
				++c_replaced;
			}
			else
			{
				appendOutputLine("Notice: HPO term '" + hpo_id  + "' for sample " + db.sampleName(sample_id) + " is obsolete, but could not be replaced!");
				++c_not_replaced;
			}
		}
		else
		{
			appendOutputLine("Warning: Invalid HPO term '" + hpo_id  + "' found for sample " + db.sampleName(sample_id) + "!");
			++c_invalid;
		}
	}

	//output
	appendOutputLine("");
	appendOutputLine("Found " + QString::number(c_valid) + " valid HPO terms.");
	appendOutputLine("Found " + QString::number(c_invalid) + " invalid HPO terms.");
	appendOutputLine("Found " + QString::number(c_replaced) + " obsolete HPO terms that were replaced.");
	appendOutputLine("Found " + QString::number(c_not_replaced) + " obsolete HPO terms that could not be replaced.");
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