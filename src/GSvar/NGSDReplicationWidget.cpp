#include "NGSDReplicationWidget.h"
#include "NGSD.h"

NGSDReplicationWidget::NGSDReplicationWidget(QWidget* parent)
	: QWidget(parent)
	, ui_()
{
	ui_.setupUi(this);
	connect(ui_.repliate_btn, SIGNAL(clicked(bool)), this, SLOT(replicate()));
}

void NGSDReplicationWidget::replicate()
{
	//clear
	ui_.output->clear();
	tables_done_.clear();

	//replicate
	try
	{
		NGSD source;
		NGSD target(false, true);
		if (ui_.pre_checks->isChecked()) performPreChecks(source, target);
		if (ui_.base_data->isChecked()) replicateBaseData(source, target);
		if (ui_.variant_data->isChecked()) replicateVariantData(source, target);
		if (ui_.post_checks->isChecked()) performPostChecks(source, target);
	}
	catch (Exception& e)
	{
		addError("Exception: " + e.message());
	}
}

void NGSDReplicationWidget::addLine(QString text)
{
	ui_.output->append(text);
	ui_.output->repaint();
}

void NGSDReplicationWidget::addHeader(QString text)
{
	addLine("############## " + text.trimmed() + " ##############");
}

void NGSDReplicationWidget::addWarning(QString text)
{
	addLine("<font color=orange>Warning:</font> " + text.trimmed());
}

void NGSDReplicationWidget::addError(QString text)
{
	addLine("<font color=red>Error:</font> " + text.trimmed());
}

void NGSDReplicationWidget::performPreChecks(NGSD& source, NGSD& target)
{
	addHeader("pre-checks");

	//check tables are in both database
	QStringList source_tables = source.tables();
	QStringList target_tables = target.tables();
	foreach(QString table, source_tables)
	{
		if (!target_tables.contains(table))
		{
			addWarning("Table '" + table + "' not in target database!");
		}
	}
	foreach(QString table, target_tables)
	{
		if (!source_tables.contains(table))
		{
			addWarning("Table '" + table + "' not in source database!");
		}
	}

	//check fields match
	foreach(QString table, source_tables)
	{
		if (!target_tables.contains(table)) continue;

		if (source.tableInfo(table).fieldNames()!=target.tableInfo(table).fieldNames())
		{
			addWarning("Table '" + table + "' has differing field list!");
		}
	}

	//check tables are pre-filled by import
	QStringList tables;
	tables << "qc_terms" << "geneinfo_germline" << "disease_gene" << "disease_term" << "omim_phenotype" << "omim_gene" << "hpo_genes" << "hpo_parent" << "hpo_term" << "gene_exon" << "gene_transcript" << "gene_alias" << "gene"
		   << "gene_exon" << "gene_transcript" << "gene_pseudogene_relation";
	foreach(QString table, tables)
	{
		int count = target.getValue("SELECT count(*) FROM " + table).toInt();
		if (count==0)
		{
			addWarning("Table '" + table + "' not pre-filled by import!");
		}
		tables_done_ << table;
	}
}

void NGSDReplicationWidget::replicateBaseData(NGSD& source, NGSD& target)
{
	addHeader("replication of base data");

	//replicate base tables with 'id' column
	foreach(QString table, QStringList() << "device" << "genome" << "mid" << "user" << "species" << "preferred_transcripts" << "processing_system"  << "project" << "sender"  << "study" << "sequencing_run" << "runqc_read" << "runqc_lane" << "sample" << "sample_disease_info" << "sample_relations" << "processed_sample" << "evaluation_sheet_data" << "report_configuration" << "study_sample" << "somatic_report_configuration" << "somatic_gene_role")
	{
		//init
		QTime timer;
		timer.start();
		int c_added = 0;
		int c_removed = 0;
		int c_updated = 0;

		//check table has 'id' column
		QStringList fields = target.tableInfo(table).fieldNames();
		if(!fields.contains("id"))
		{
			addWarning("Table '" + table + "' has no id column!");
			continue;
		}

		//prepare queries
		SqlQuery q_del = target.getQuery();
		q_del.prepare("DELETE FROM "+table+" WHERE id=:0");

		SqlQuery q_add = target.getQuery();
		q_add.prepare("INSERT INTO "+table+" VALUES (:" + fields.join(", :") + ")");

		SqlQuery q_get = target.getQuery();
		q_get.prepare("SELECT * FROM "+table+" WHERE id=:0");

		SqlQuery q_update = target.getQuery();
		QString query_str = "UPDATE "+table+" SET";
		bool first = true;
		foreach(const QString& field, fields)
		{
			if (field=="id") continue;

			query_str += (first? " " : ", ") + field + "=:" + field;

			if (first) first = false;
		}
		query_str += " WHERE id=:id";
		q_update.prepare(query_str);

		//delete removed entries
		QSet<int> source_ids = source.getValuesInt("SELECT id FROM " + table + " ORDER BY id ASC").toSet();
		QSet<int> target_ids = target.getValuesInt("SELECT id FROM " + table + " ORDER BY id ASC").toSet();
		foreach(int id, target_ids)
		{
			if (!source_ids.contains(id))
			{
				q_del.bindValue(0, id);
				q_del.exec();

				++c_removed;
			}
		}

		//add new entries
		SqlQuery query = source.getQuery();
		query.exec("SELECT * FROM "+table+" ORDER BY id ASC");
		while(query.next())
		{
			int id = query.value("id").toInt();
			if (target_ids.contains(id))
			{
				//check if changed
				q_get.bindValue(0, id);
				q_get.exec();
				q_get.next();
				bool changed = false;
				foreach(const QString& field, fields)
				{
					if (q_get.value(field)!=query.value(field))
					{
						//qDebug() << table << id << field << q_get.value(field) << query.value(field);
						changed = true;
					}
				}

				//update if changed
				if (changed)
				{
					foreach(const QString& field, fields)
					{
						q_update.bindValue(":"+field, query.value(field));
					}
					q_update.exec();

					++c_updated;
				}

				continue;
			}

			foreach(const QString& field, fields)
			{
				q_add.bindValue(":"+field, query.value(field));
			}
			try
			{
				q_add.exec();
			}
			catch (Exception& e)
			{
				if (table=="processed_sample") //special handling because of 'normal_id' column
				{
					qDebug() << "Could not add processed sample " + source.processedSampleName(QString::number(id))+":";
					qDebug() << e.message();
				}
				else
				{
					THROW(Exception, "Could not add table "+table+" entry with id "+id+": "+e.message());
				}
			}
			++c_added;
		}

		addLine("  Table "+table+" replicated: added " + QString::number(c_added) + " rows, updated " + QString::number(c_updated) + " rows, removed  " + QString::number(c_removed) + " rows. Time: " + Helper::elapsedTime(timer));
		tables_done_ << table;
	}

	//replicate base tables without 'id' column
	foreach(QString table, QStringList() << "omim_preferred_phenotype" << "processed_sample_ancestry" << "diag_status" << "kasp_status" << "merged_processed_samples")
	{
		//init
		QTime timer;
		timer.start();
		int c_added = 0;

		//check table has 'id' column
		QStringList fields = target.tableInfo(table).fieldNames();
		if(fields.contains("id"))
		{
			addWarning("Table '" + table + "' has id column!");
			continue;
		}

		//prepare queries
		SqlQuery q_add = target.getQuery();
		q_add.prepare("INSERT IGNORE INTO "+table+" VALUES (:" + fields.join(", :") + ")");

		//replicate
		SqlQuery query = source.getQuery();
		query.exec("SELECT * FROM "+table);
		while(query.next())
		{
			foreach(const QString& field, fields)
			{
				q_add.bindValue(":"+field, query.value(field));
			}
			q_add.exec();

			++c_added;
		}

		addLine("  Table (no 'id') "+table+" replicated : added " + QString::number(c_added) + " rows. Time: " + Helper::elapsedTime(timer));
		tables_done_ << table;
	}

	//transfer sample group info to sample comment
	{
		SqlQuery q_update = target.getQuery();
		q_update.prepare("UPDATE sample SET comment=:0 WHERE id=:1");

		SqlQuery query = source.getQuery();
		query.exec("SELECT nm.sample_id, g.name, g.comment FROM nm_sample_sample_group nm, sample_group g WHERE nm.sample_group_id=g.id ORDER BY nm.sample_id");
		while(query.next())
		{
			QString s_id = query.value(0).toString();
			QString group_name = query.value(1).toString().trimmed();
			QString group_comment = query.value(2).toString().trimmed();

			QString group_text = "sample group: " + group_name;
			if (!group_comment.isEmpty()) group_text += " (" + group_comment + ")";

			QString comment = target.getValue("SELECT comment FROM sample WHERE id=" + s_id).toString();
			if (!comment.contains(group_text))
			{
				comment += "\n" + group_text;
				q_update.bindValue(0, comment);
				q_update.bindValue(1, s_id);
				q_update.exec();
			}
		}
		tables_done_ << "nm_sample_sample_group";
		tables_done_ << "sample_group";
	}
}

void NGSDReplicationWidget::replicateVariantData(NGSD& source, NGSD& target)
{
	addHeader("replication of variant data");

	QStringList ps_ids;
	if (!ui_.samples->text().trimmed().isEmpty())
	{
		QStringList ps_names = ui_.samples->text().split(",");
		foreach(QString ps, ps_names)
		{
			ps_ids << source.processedSampleId(ps);
		}
	}
	else
	{
		source.getQuery().exec("SELECT id FROM processed_sample ORDER BY id ASC");
	}

	foreach(QString ps_id, ps_ids)
	{
		qDebug() << ps_id;
	}

	//TODO
	/*
	Warning: Table 'variant_classification' not filled!
	Warning: Table 'variant_publication' not filled!
	Warning: Table 'variant_validation' not filled!
	Warning: Table 'report_configuration_variant' not filled!
	Warning: Table 'report_configuration_cnv' not filled!
	Warning: Table 'report_configuration_sv' not filled!

	Warning: Table 'somatic_variant_classification' not filled!
	Warning: Table 'somatic_vicc_interpretation' not filled!
	Warning: Table 'somatic_report_configuration_cnv' not filled!
	Warning: Table 'somatic_report_configuration_germl_var' not filled!
	Warning: Table 'somatic_report_configuration_variant' not filled!

	Warning: Table 'gaps' not filled!
	Warning: Table 'cfdna_panel_genes' not filled!
	Warning: Table 'cfdna_panels' not filled!
	*/
}

void NGSDReplicationWidget::performPostChecks(NGSD& /*source*/, NGSD& target)
{
	addHeader("post-checks");

	//test if all tables are processed
	QStringList target_tables = target.tables();
	foreach(QString table, target_tables)
	{
		//skip - analysis jobs
		if (table=="analysis_job") continue;
		if (table=="analysis_job_history") continue;
		if (table=="analysis_job_sample") continue;
		//skip - sample variant and QC import
		if (table=="cnv_callset") continue;
		if (table=="cnv") continue;
		if (table=="detected_somatic_variant") continue;
		if (table=="detected_variant") continue;
		if (table=="somatic_cnv_callset") continue;
		if (table=="somatic_cnv") continue;
		if (table=="sv_callset") continue;
		if (table=="sv_deletion") continue;
		if (table=="sv_duplication") continue;
		if (table=="sv_insertion") continue;
		if (table=="sv_inversion") continue;
		if (table=="sv_translocation") continue;
		if (table=="processed_sample_qc") continue;
		//skip - others
		if (table=="secondary_analysis") continue;
		if (table=="subpanels") continue;

		if (!tables_done_.contains(table))
		{
			addWarning("Table '" + table + "' not filled!");
		}
	}
}
