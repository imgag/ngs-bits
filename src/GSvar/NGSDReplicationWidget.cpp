#include "NGSDReplicationWidget.h"
#include "NGSD.h"
#include "GSvarHelper.h"
#include "Settings.h"
#include <QDir>

NGSDReplicationWidget::NGSDReplicationWidget(QWidget* parent)
	: QWidget(parent)
	, ui_()
{
	ui_.setupUi(this);
	connect(ui_.repliate_btn, SIGNAL(clicked(bool)), this, SLOT(replicate()));

	//init databases and genome indices
	try
	{
		if (GSvarHelper::build()!=GenomeBuild::HG38) THROW(ArgumentException, "This dialog can only be used from GSvar with HG38 genome build!");
		db_source_ = QSharedPointer<NGSD>(new NGSD(false, "_hg19"));
		db_target_ = QSharedPointer<NGSD>(new NGSD());
	}
	catch(Exception& e)
	{
		addError(e.message());
		ui_.repliate_btn->setEnabled(false);
	}
}

void NGSDReplicationWidget::replicate()
{
	ui_.repliate_btn->setEnabled(false);
	QApplication::setOverrideCursor(Qt::BusyCursor);

	//clear
	ui_.output->clear();
	tables_done_.clear();

	//replicate
	try
	{
		if (ui_.pre_checks->isChecked()) performPreChecks();
		if (ui_.base_data->isChecked())
		{
			replicateBaseData();
			replicateBaseDataNoId();
			addSampleGroups();
		}
		if (ui_.variant_data->isChecked()) replicateVariantData();
		if (ui_.report_configuration->isChecked()) replicateReportConfiguration();
		if (ui_.post_production->isCheckable()) replicatePostProduction();
		if (ui_.post_checks->isChecked()) performPostChecks();
	}
	catch (Exception& e)
	{
		addError("Exception: " + e.message());
	}

	ui_.repliate_btn->setEnabled(true);
	QApplication::restoreOverrideCursor();
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

void NGSDReplicationWidget::performPreChecks()
{
	addHeader("pre-checks");

	//check tables are in both database
	QStringList source_tables = db_source_->tables();
	QStringList target_tables = db_target_->tables();
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

		if (db_source_->tableInfo(table, false).fieldNames()!=db_target_->tableInfo(table, false).fieldNames())
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
		int count = db_target_->getValue("SELECT count(*) FROM " + table).toInt();
		if (count==0)
		{
			addWarning("Table '" + table + "' not pre-filled by import!");
		}
		tables_done_ << table;
	}
}

void NGSDReplicationWidget::replicateBaseData()
{
	addHeader("replication of base data");

	//replicate base tables with 'id' column
	foreach(QString table, QStringList() << "device" << "genome" << "mid" << "user" << "species" << "preferred_transcripts" << "processing_system"  << "project" << "sender"  << "study" << "sequencing_run" << "runqc_read" << "runqc_lane" << "sample" << "sample_disease_info" << "sample_relations" << "processed_sample" << "evaluation_sheet_data" << "report_configuration" << "study_sample" << "somatic_report_configuration" << "somatic_gene_role")
	{
		updateTable(table);
	}
}

void NGSDReplicationWidget::updateTable(QString table, bool contains_variant_id, QString where_clause)
{
	int hg38_id = db_source_->getValue("SELECT id FROM genome WHERE build='GRCh38'").toInt();

	//check table has 'id' column
	QStringList fields = db_target_->tableInfo(table).fieldNames();
	if(!fields.contains("id"))
	{
		THROW(ProgrammingException, "Table '" + table + "' has no id column!");
	}

	//check table contains variant data
	if (contains_variant_id && !fields.contains("variant_id"))
	{
		THROW(ProgrammingException, "Table '" + table + "' has no column 'variant_id'!");
	}
	else if (!contains_variant_id && fields.contains("variant_id"))
	{
		THROW(ProgrammingException, "Table '" + table + "' has column 'variant_id', but should not!");
	}

	//init
	QTime timer;
	timer.start();

	int c_added = 0;
	int c_kept = 0;
	int c_removed = 0;
	int c_updated = 0;
	int c_not_mappable = 0;
	int c_not_in_ngsd = 0;

	SqlQuery q_del = db_target_->getQuery();
	q_del.prepare("DELETE FROM "+table+" WHERE id=:0");

	SqlQuery q_add = db_target_->getQuery();
	QString extra_fields;
	if (table=="project") extra_fields = ", '0'";
	q_add.prepare("INSERT INTO "+table+" VALUES (:" + fields.join(", :") + extra_fields + ")");

	SqlQuery q_get = db_target_->getQuery();
	q_get.prepare("SELECT * FROM "+table+" WHERE id=:0");

	SqlQuery q_update = db_target_->getQuery();
	QString query_str = "UPDATE "+table+" SET";
	bool first = true;
	foreach(const QString& field, fields)
	{
		if (field=="id") continue;

		//special handling of variant_id (lifted-over)
		if (contains_variant_id && field=="variant_id") continue;

		query_str += (first? " " : ", ") + field + "=:" + field;

		if (first) first = false;
	}
	query_str += " WHERE id=:id";
	q_update.prepare(query_str);

	//delete removed entries
	QSet<int> source_ids = db_source_->getValuesInt("SELECT id FROM " + table + " " +  where_clause + " ORDER BY id ASC").toSet();
	QSet<int> target_ids = db_target_->getValuesInt("SELECT id FROM " + table + " " +  where_clause + " ORDER BY id ASC").toSet();
	foreach(int id, target_ids)
	{
		if (!source_ids.contains(id))
		{
			q_del.bindValue(0, id);
			q_del.exec();

			++c_removed;
		}
	}

	//add/update entries
	SqlQuery query = db_source_->getQuery();
	query.exec("SELECT * FROM " + table + " " +  where_clause + " ORDER BY id ASC");
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
				//special handling of variant_id (lifted-over)
				if (contains_variant_id && field=="variant_id") continue;

				if (q_get.value(field)!=query.value(field))
				{
					changed = true;
				}
			}

			//special handling of processing system table because it was adapted manually for HG38.
			if (changed && table=="processing_system" && q_get.value("genome_id")==hg38_id)
			{
				changed = false;
				addLine("    Notice: Skipping update of processing_system '" + q_get.value("name_short").toString() + " because it is based on HG38!");
			}

			//update if changed
			if (changed)
			{
				foreach(const QString& field, fields)
				{
					//special handling of variant_id (lifted-over)
					if (contains_variant_id && field=="variant_id") continue;

					QVariant value = query.value(field);

					//special handling of normal_sample if not in target database yet
					if (table=="processed_sample" && field=="normal_id")
					{
						if (!value.isNull())
						{
							QVariant normal_id = db_target_->getValue("SELECT id FROM processed_sample WHERE id=" + value.toString());
							if (!normal_id.isValid()) value.clear();
						}
					}

					q_update.bindValue(":"+field, value);
				}
				q_update.exec();

				++c_updated;
			}
			else
			{
				++c_kept;
			}
		}
		else //row not in target table > add
		{
			int source_variant_id = -1;
			int target_variant_id = -1;
			if (contains_variant_id)
			{
				source_variant_id = query.value("variant_id").toInt();

				bool causal_variant = table=="report_configuration_variant" && query.value("causal").toInt()==1;
				bool pathogenic_variant = table=="variant_classification" && query.value("class").toInt()>3;
				target_variant_id = liftOverVariant(source_variant_id, causal_variant||pathogenic_variant);

				//warn if causal/pathogenic variant could not be lifted
				if (target_variant_id<0)
				{
					if (causal_variant)
					{
						QString ps = db_source_->processedSampleName(db_source_->getValue("SELECT processed_sample_id FROM report_configuration WHERE id=" + query.value("report_configuration_id").toString()).toString());
						addWarning("Causal variant " + db_source_->variant(query.value("variant_id").toString()).toString() + " of sample " + ps + " could not be lifted (" + QString::number(target_variant_id) + ")!");
					}
					else if (pathogenic_variant)
					{
						addWarning("Pathogenic variant " + db_source_->variant(query.value("variant_id").toString()).toString() + " (class " + query.value("class").toString() + ") could not be lifted (" + QString::number(target_variant_id) + ")!");
					}
				}
			}
			if (!contains_variant_id || target_variant_id>=0)
			{
				foreach(const QString& field, fields)
				{
					if (contains_variant_id && field=="variant_id") //special handling of variant_id (lifted-over)
					{
						q_add.bindValue(":"+field, target_variant_id);
						continue;
					}

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
						qDebug() << "Could not add processed sample " + db_source_->processedSampleName(QString::number(id))+":";
						qDebug() << e.message();
					}
					else
					{
						THROW(Exception, "Could not add table "+table+" entry with id "+QString::number(id)+": "+e.message());
					}
				}
				++c_added;
			}
			else if (target_variant_id>=-3)
			{
				++c_not_mappable;
			}
			else if (target_variant_id==-4)
			{
				++c_not_in_ngsd;
			}
			else
			{
				THROW(NotImplementedException, "Unhandled variant error: " + QString::number(target_variant_id));
			}
		}
	}

	//output
	QStringList details;
	if (c_added>0) details << "added " + QString::number(c_added);
	if (contains_variant_id)
	{
		if (c_not_mappable>0) details << "skipped unmappable variants " + QString::number(c_not_mappable);
		if (c_not_in_ngsd>0) details << "skipped variants not in NGSD " + QString::number(c_not_in_ngsd);
	}
	if (c_kept>0) details << "kept " + QString::number(c_kept);
	if (c_updated>0) details << "updated " + QString::number(c_updated);
	if (c_removed>0) details << "removed " + QString::number(c_removed);

	addLine("  Table '"+table+"' replicated: "+details.join(", ")+". Time: " + Helper::elapsedTime(timer));
	tables_done_ << table;
}

void NGSDReplicationWidget::replicateBaseDataNoId()
{
	addHeader("replication of base data (no ID)");

	foreach(QString table, QStringList() << "omim_preferred_phenotype" << "processed_sample_ancestry" << "diag_status" << "kasp_status" << "merged_processed_samples")
	{
		//init
		QTime timer;
		timer.start();
		int c_added = 0;

		//check table has 'id' column
		QStringList fields = db_target_->tableInfo(table).fieldNames();
		if(fields.contains("id"))
		{
			addWarning("Table '" + table + "' has id column!");
			continue;
		}

		//prepare queries
		SqlQuery q_add = db_target_->getQuery();
		q_add.prepare("INSERT IGNORE INTO "+table+" VALUES (:" + fields.join(", :") + ")");

		//replicate
		SqlQuery query = db_source_->getQuery();
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

		addLine("  Table without id '"+table+"' replicated : added " + QString::number(c_added) + " rows. Time: " + Helper::elapsedTime(timer));
		tables_done_ << table;
	}
}

void NGSDReplicationWidget::addSampleGroups()
{
	addHeader("special handling of sample group tables");

	SqlQuery q_update = db_target_->getQuery();
	q_update.prepare("UPDATE sample SET comment=:0 WHERE id=:1");

	SqlQuery query = db_source_->getQuery();
	query.exec("SELECT nm.sample_id, g.name, g.comment FROM nm_sample_sample_group nm, sample_group g WHERE nm.sample_group_id=g.id ORDER BY nm.sample_id");
	while(query.next())
	{
		QString s_id = query.value(0).toString();
		QString group_name = query.value(1).toString().trimmed();
		QString group_comment = query.value(2).toString().trimmed();

		QString group_text = "sample group: " + group_name;
		if (!group_comment.isEmpty()) group_text += " (" + group_comment + ")";

		QString comment = db_target_->getValue("SELECT comment FROM sample WHERE id=" + s_id).toString();
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

void NGSDReplicationWidget::replicateVariantData()
{
	addHeader("replication of variant data");

	updateTable("variant_classification", true);
	updateTable("somatic_variant_classification", true);
	updateTable("somatic_vicc_interpretation", true);
	updateTable("variant_publication", true);
	updateTable("variant_validation", true, "WHERE variant_id IS NOT NULL"); //small variants
	updateCnvTable("variant_validation", "WHERE cnv_id IS NOT NULL"); //CNVs
	updateSvTable("variant_validation", StructuralVariantType::DEL, "WHERE sv_deletion_id IS NOT NULL"); //SVs (DEL)
	updateSvTable("variant_validation", StructuralVariantType::DUP, "WHERE sv_duplication_id IS NOT NULL"); //SVs (DUP)
	updateSvTable("variant_validation", StructuralVariantType::INS, "WHERE sv_insertion_id IS NOT NULL"); //SVs (INS)
	updateSvTable("variant_validation", StructuralVariantType::INV, "WHERE sv_inversion_id IS NOT NULL"); //SVs (INV)
	updateSvTable("variant_validation", StructuralVariantType::BND, "WHERE sv_translocation_id IS NOT NULL"); //SVs (BND)
}

void NGSDReplicationWidget::replicateReportConfiguration()
{
	addHeader("replication of report configuration data");

	//(1) germline

	//import report variants for samples with imported variants that are not already imported
	QStringList rc_todo = db_target_->getValues("SELECT id FROM report_configuration WHERE processed_sample_id IN (SELECT DISTINCT processed_sample_id FROM detected_variant)");
	QStringList rc_ids_already_present = db_target_->getValues("SELECT DISTINCT report_configuration_id FROM report_configuration_variant");
	foreach(QString id, rc_ids_already_present)
	{
		rc_todo.removeAll(id);
	}
	qDebug() << "report config small variants: samples todo:" << rc_todo.count() << " already imported:" << rc_ids_already_present.count();
	updateTable("report_configuration_variant", true, "WHERE report_configuration_id IN (" + rc_todo.join(",") + ")");

	//import report CNVs for samples with imported CNVs that are not already imported
	rc_todo = db_target_->getValues("SELECT id FROM report_configuration WHERE processed_sample_id IN (SELECT processed_sample_id FROM cnv_callset WHERE id IN (SELECT cnv_callset_id FROM cnv))");
	rc_ids_already_present = db_target_->getValues("SELECT DISTINCT report_configuration_id FROM report_configuration_cnv");
	foreach(QString id, rc_ids_already_present)
	{
		rc_todo.removeAll(id);
	}
	qDebug() << "report config CNVs: samples todo:" << rc_todo.count() << " already imported:" << rc_ids_already_present.count();
	updateCnvTable("report_configuration_cnv", "WHERE report_configuration_id IN (" + rc_todo.join(",") + ")");


	//import report SVs for samples with imported SVs that are not already imported
	rc_ids_already_present = db_target_->getValues("SELECT DISTINCT report_configuration_id FROM report_configuration_sv");
	//DEL
	rc_todo = db_target_->getValues("SELECT id FROM report_configuration WHERE processed_sample_id IN (SELECT processed_sample_id FROM sv_callset WHERE id IN (SELECT sv_callset_id FROM sv_deletion))");
	foreach(QString id, rc_ids_already_present)
	{
		rc_todo.removeAll(id);
	}
	qDebug() << "report config SVs (DEL): samples todo:" << rc_todo.count() << " already imported:" << rc_ids_already_present.count();
	updateSvTable("report_configuration_sv", StructuralVariantType::DEL, "WHERE report_configuration_id IN (" + rc_todo.join(",") + ") AND sv_deletion_id IS NOT NULL");
	//DUP
	rc_todo = db_target_->getValues("SELECT id FROM report_configuration WHERE processed_sample_id IN (SELECT processed_sample_id FROM sv_callset WHERE id IN (SELECT sv_callset_id FROM sv_duplication))");
	foreach(QString id, rc_ids_already_present)
	{
		rc_todo.removeAll(id);
	}
	qDebug() << "report config SVs (DUP): samples todo:" << rc_todo.count() << " already imported:" << rc_ids_already_present.count();
	updateSvTable("report_configuration_sv", StructuralVariantType::DUP, "WHERE report_configuration_id IN (" + rc_todo.join(",") + ") AND sv_duplication_id IS NOT NULL");
	//INS
	rc_todo = db_target_->getValues("SELECT id FROM report_configuration WHERE processed_sample_id IN (SELECT processed_sample_id FROM sv_callset WHERE id IN (SELECT sv_callset_id FROM sv_insertion))");
	foreach(QString id, rc_ids_already_present)
	{
		rc_todo.removeAll(id);
	}
	qDebug() << "report config SVs (INS): samples todo:" << rc_todo.count() << " already imported:" << rc_ids_already_present.count();
	updateSvTable("report_configuration_sv", StructuralVariantType::INS, "WHERE report_configuration_id IN (" + rc_todo.join(",") + ") AND sv_insertion_id IS NOT NULL");
	//INV
	rc_todo = db_target_->getValues("SELECT id FROM report_configuration WHERE processed_sample_id IN (SELECT processed_sample_id FROM sv_callset WHERE id IN (SELECT sv_callset_id FROM sv_inversion))");
	foreach(QString id, rc_ids_already_present)
	{
		rc_todo.removeAll(id);
	}
	qDebug() << "report config SVs (INV): samples todo:" << rc_todo.count() << " already imported:" << rc_ids_already_present.count();
	updateSvTable("report_configuration_sv", StructuralVariantType::INV, "WHERE report_configuration_id IN (" + rc_todo.join(",") + ") AND sv_inversion_id IS NOT NULL");
	//BND
	rc_todo = db_target_->getValues("SELECT id FROM report_configuration WHERE processed_sample_id IN (SELECT processed_sample_id FROM sv_callset WHERE id IN (SELECT sv_callset_id FROM sv_translocation))");
	foreach(QString id, rc_ids_already_present)
	{
		rc_todo.removeAll(id);
	}
	qDebug() << "report config SVs (BND): samples todo:" << rc_todo.count() << " already imported:" << rc_ids_already_present.count();
	updateSvTable("report_configuration_sv", StructuralVariantType::BND, "WHERE report_configuration_id IN (" + rc_todo.join(",") + ") AND sv_translocation_id IS NOT NULL");


	//(2) somatic

	updateTable("somatic_report_configuration_variant", true);
	updateTable("somatic_report_configuration_germl_var", true);
}

void NGSDReplicationWidget::replicatePostProduction()
{
	//lift-over statistics (variant publication variants for DX21 samples)
	//results from 17.12.2021:
	//SNV: 244 240 ~ 98%
	//INS: 39 39 ~ 100%
	//DEL: 89 86 - 96%
	//SNV+INS+DEL: 98.11%
	if (false)
	{
		int c_snv = 0;
		int c_snv_lift = 0;
		int c_ins = 0;
		int c_ins_lift = 0;
		int c_del = 0;
		int c_del_lift = 0;

		SqlQuery query_s = db_source_->getQuery();
		query_s.exec("SELECT DISTINCT vp.variant_id FROM variant_publication vp, sample s WHERE vp.sample_id=s.id AND s.name LIKE 'DX21%'");
		while(query_s.next())
		{
			int source_variant_id = query_s.value("variant_id").toInt();
			int target_variant_id = liftOverVariant(source_variant_id, false);

			Variant tmp = db_source_->variant(QString::number(source_variant_id));
			if (tmp.isSNV())
			{
				++c_snv;
				if (target_variant_id>0) ++c_snv_lift;
			}
			else if (tmp.ref()=="-") //ins
			{
				++c_ins;
				if (target_variant_id>0) ++c_ins_lift;
			}
			else //del
			{
				++c_del;
				if (target_variant_id>0) ++c_del_lift;
			}
		}
		qDebug() << c_snv << c_snv_lift;
		qDebug() << c_ins << c_ins_lift;
		qDebug() << c_del << c_del_lift;
		return;
	}

	//variant comments
	{
		int c_update = 0;
		SqlQuery query_s = db_source_->getQuery();
		query_s.exec("SELECT id, comment FROM `variant` WHERE `comment` IS NOT NULL");
		while(query_s.next())
		{
			QString source_comment = query_s.value(1).toString().trimmed();
			if (source_comment.isEmpty()) continue;

			//lift-over
			int source_variant_id = query_s.value(0).toInt();
			int target_variant_id = liftOverVariant(source_variant_id, false);
			if (target_variant_id<0) continue;

			//check if comment set > skip then
			QString target_comment = db_target_->getValue("SELECT comment FROM variant WHERE id='" + QString::number(target_variant_id) + "'", true).toString();
			if (target_comment!="") continue;

			//update target entry
			SqlQuery query_t = db_target_->getQuery();
			query_t.prepare("UPDATE variant SET comment=:0 WHERE id=:1");
			query_t.bindValue(0, source_comment);
			query_t.bindValue(1, target_variant_id);
			query_t.exec();

			++c_update;
		}

		addLine("  Table 'variant' comments updated: "+QString::number(c_update));
	}

	//geneinfo_germline
	{
		int c_update = 0;
		SqlQuery query_s = db_source_->getQuery();
		query_s.exec("SELECT symbol, comments FROM geneinfo_germline");
		while(query_s.next())
		{
			QString gene = query_s.value(0).toString().trimmed();
			QString source_comment = query_s.value(1).toString().trimmed();
			if (source_comment.isEmpty()) continue;

			//replicate only if target entry exists and has no comment
			QVariant target_comment = db_target_->getValue("SELECT comments FROM geneinfo_germline WHERE symbol='"+gene+"'", true);
			if (!target_comment.isValid()) continue;
			if (!target_comment.toString().trimmed().isEmpty()) continue;

			//update target entry
			SqlQuery query_t = db_target_->getQuery();
			query_t.prepare("UPDATE geneinfo_germline SET comments=:0 WHERE symbol=:1");
			query_t.bindValue(0, source_comment);
			query_t.bindValue(1, gene);
			query_t.exec();

			++c_update;
		}
		addLine("  Table 'geneinfo_germline' comments updated: "+QString::number(c_update));
	}

	//variant_validation
	{
		int c_add = 0;
		SqlQuery query_s = db_source_->getQuery();
		SqlQuery q_add = db_target_->getQuery();
		q_add.prepare("INSERT INTO `variant_validation`(`user_id`, `date`, `sample_id`, `variant_type`, `variant_id`,  `genotype`, `validation_method`, `status`, `comment`) VALUES (:0,:1,:2,:3,:4,:5,:6,:7,:8)");
		query_s.exec("SELECT * FROM variant_validation WHERE variant_type='SNV_INDEL'");
		while(query_s.next())
		{
			QString sample_id = query_s.value("sample_id").toString();
			int source_variant_id = query_s.value("variant_id").toInt();
			int target_variant_id = liftOverVariant(source_variant_id, false);

			//warn if causal/pathogenic variant could not be lifted
			if (target_variant_id>=0)
			{
				 QVariant target_id = db_target_->getValue("SELECT id FROM variant_validation WHERE variant_id='" + QString::number(target_variant_id) + "' AND sample_id='"+sample_id+"' LIMIT 1", true);
				 if (!target_id.isValid())
				 {
					QString source_sample = db_source_->getValue("SELECT name FROM sample WHERE id='"+sample_id+"'").toString();
					QString target_sample = db_target_->getValue("SELECT name FROM sample WHERE id='"+sample_id+"'").toString();
					if (source_sample==target_sample)
					{
						q_add.bindValue(0, query_s.value("user_id"));
						q_add.bindValue(1, query_s.value("date"));
						q_add.bindValue(2, sample_id);
						q_add.bindValue(3, "SNV_INDEL");
						q_add.bindValue(4, target_variant_id);
						q_add.bindValue(5, query_s.value("genotype"));
						q_add.bindValue(6, query_s.value("validation_method"));
						q_add.bindValue(7, query_s.value("status"));
						q_add.bindValue(8, query_s.value("comment"));
						q_add.exec();

						++c_add;
					}
				 }
			}
		}
		addLine("  Table 'variant_validation' added: "+QString::number(c_add));
		QApplication::processEvents();
	}

	//variant_publication
	{
		int c_add = 0;
		SqlQuery query_s = db_source_->getQuery();
		SqlQuery q_add = db_target_->getQuery();
		q_add.prepare("INSERT INTO `variant_publication`(`sample_id`, `variant_id`, `db`, `class`, `details`, `user_id`, `date`) VALUES (:0,:1,:2,:3,:4,:5,:6)");
		query_s.exec("SELECT * FROM variant_publication ORDER BY id DESC");
		while(query_s.next())
		{
			QString sample_id = query_s.value("sample_id").toString();
			int source_variant_id = query_s.value("variant_id").toInt();
			int target_variant_id = liftOverVariant(source_variant_id, false);

			//warn if causal/pathogenic variant could not be lifted
			if (target_variant_id>=0)
			{
				 QVariant target_id = db_target_->getValue("SELECT id FROM variant_publication WHERE variant_id='" + QString::number(target_variant_id) + "' AND sample_id='"+sample_id+"' LIMIT 1", true);
				 if (!target_id.isValid())
				 {
					QString source_sample = db_source_->getValue("SELECT name FROM sample WHERE id='"+sample_id+"'").toString();
					QString target_sample = db_target_->getValue("SELECT name FROM sample WHERE id='"+sample_id+"'").toString();
					if (source_sample==target_sample)
					{
						q_add.bindValue(0, sample_id);
						q_add.bindValue(1, target_variant_id);
						q_add.bindValue(2, query_s.value("db"));
						q_add.bindValue(3, query_s.value("class"));
						q_add.bindValue(4, query_s.value("details"));
						q_add.bindValue(5, query_s.value("user_id"));
						q_add.bindValue(6, query_s.value("date"));
						q_add.exec();

						++c_add;
					}
				 }
			}
		}
		addLine("  Table 'variant_publication' added: "+QString::number(c_add));
		QApplication::processEvents();
	}

	//variant_classification
	{

		QFile file(QCoreApplication::applicationDirPath()  + QDir::separator() + "liftover_class_4_or_5.tsv");
		file.open(QIODevice::WriteOnly);
		QTextStream debug_stream(&file);
		debug_stream << "#variant\tps\tproject\tsystem\n";

		int c_add = 0;
		SqlQuery query_s = db_source_->getQuery();
		SqlQuery q_add = db_target_->getQuery();
		q_add.prepare("INSERT INTO `variant_classification`(`variant_id`, `class`, `comment`) VALUES (:0,:1,:2)");
		query_s.exec("SELECT * FROM variant_classification");
		while(query_s.next())
		{
			int source_variant_id = query_s.value("variant_id").toInt();
			int target_variant_id = liftOverVariant(source_variant_id, false);

			//warn if class 4/5 variant could not be found
			if (target_variant_id<0)
			{
				QString source_class = query_s.value("class").toString();
				if (target_variant_id==-4 && (source_class=="4" || source_class=="5")) //target_variant_id==-4 means that the variant is liftable, but was not found in the target NGSD
				{
					QString variant_id = QString::number(source_variant_id);
					qDebug() << "variant_classification: class " << source_class  << " variant " << db_source_->variant(variant_id).toString() << " not found in target NGSD!";

					QList<int> ps_ids = db_source_->getValuesInt("SELECT processed_sample_id FROM detected_variant WHERE variant_id="+ variant_id);
					foreach(int ps_id, ps_ids)
					{
						QString ps_id_str = QString::number(ps_id);
						debug_stream << db_source_->variant(variant_id).toString() + "\t" + db_source_->processedSampleName(ps_id_str) + "\t" + db_source_->getValue("SELECT p.name FROM project p, processed_sample ps WHERE p.id=ps.project_id AND ps.id=" + ps_id_str).toString() + "\t" + db_source_->getValue("SELECT sys.name_short FROM processing_system sys, processed_sample ps WHERE sys.id=ps.processing_system_id AND ps.id=" + ps_id_str).toString() << endl;
					}
				}
			}
			else
			{
				 QVariant target_id = db_target_->getValue("SELECT id FROM variant_classification WHERE variant_id='" + QString::number(target_variant_id) + "'", true);
				 if (!target_id.isValid())
				 {
					 q_add.bindValue(0, target_variant_id);
					 q_add.bindValue(1, query_s.value("class"));
					 q_add.bindValue(2, query_s.value("comment"));
					 q_add.exec();

					++c_add;
				 }
			}
		}
		addLine("  Table 'variant_classification' added: "+QString::number(c_add));
		QApplication::processEvents();
	}

	//report config (small variants, CNVs, SVs)
	{
		//init
		int c_add_small = 0;
		int c_add_cnv = 0;
		int c_add_sv = 0;

		//iterate over report_configuration entries in source DB
		SqlQuery  s_query = db_source_->getQuery();
		s_query.exec("SELECT ps.id, s.name, ps.process_id, rc.id FROM sample s, processed_sample ps, report_configuration rc WHERE s.id=ps.sample_id AND ps.id=rc.processed_sample_id");
		while(s_query.next())
		{
			QString ps_id = s_query.value(0).toString();
			QString s_name = s_query.value(1).toString();
			QString process_id = s_query.value(2).toString();
			QString rc_id_source = s_query.value(3).toString();

			//check if report config in target exists (it should unless it was intentionally deleted in the target database)
			QString rc_id_target = db_target_->getValue("SELECT rc.id FROM sample s, processed_sample ps, report_configuration rc WHERE s.id=ps.sample_id AND ps.id=rc.processed_sample_id AND s.name='" + s_name + "' AND ps.process_id='"+process_id+"'", true).toString();
			if (rc_id_target.isEmpty())
			{
				qDebug() << "Notice: Report configuration missing for " << (s_name+"_0"+process_id) << ". Skipped!";
				continue;
			}

			//replicate small variants (only if variants are imported)
			if (db_target_->importStatus(ps_id).small_variants>0)
			{
				QList<int> source_variant_ids = db_source_->getValuesInt("SELECT variant_id FROM report_configuration_variant WHERE report_configuration_id='" + rc_id_source + "'");
				foreach (int source_variant_id, source_variant_ids)
				{
					//lift-over
					int target_variant_id = liftOverVariant(source_variant_id, false);
					if (target_variant_id<0) continue;

					//check if exists
					QVariant target_rcv_id = db_target_->getValue("SELECT id FROM report_configuration_variant WHERE report_configuration_id='"+rc_id_target+"' AND variant_id='"+QString::number(target_variant_id)+"'", true);
					if (!target_rcv_id.isValid())
					{
						++c_add_small;

						//get souce data
						SqlQuery s_get = db_source_->getQuery();
						s_get.exec("SELECT * FROM report_configuration_variant WHERE report_configuration_id='" + rc_id_source +"' AND variant_id='"+QString::number(source_variant_id)+"'");
						if (!s_get.next())
						{
							qDebug() << "Error: Report config entry missing for small variant. This should not happen! Skipped!";
							continue;
						}

						//insert into target
						SqlQuery t_add = db_target_->getQuery();
						t_add.prepare("INSERT INTO `report_configuration_variant`(`report_configuration_id`, `variant_id`, `type`, `causal`, `inheritance`, `de_novo`, `mosaic`, `compound_heterozygous`, `exclude_artefact`, `exclude_frequency`, `exclude_phenotype`, `exclude_mechanism`, `exclude_other`, `comments`, `comments2`) VALUES (:0,:1,:2,:3,:4,:5,:6,:7,:8,:9,:10,:11,:12,:13,:14)");
						t_add.bindValue(0, rc_id_target);
						t_add.bindValue(1, target_variant_id);
						t_add.bindValue(2, s_get.value("type"));
						t_add.bindValue(3, s_get.value("causal"));
						t_add.bindValue(4, s_get.value("inheritance"));
						t_add.bindValue(5, s_get.value("de_novo"));
						t_add.bindValue(6, s_get.value("mosaic"));
						t_add.bindValue(7, s_get.value("compound_heterozygous"));
						t_add.bindValue(8, s_get.value("exclude_artefact"));
						t_add.bindValue(9, s_get.value("exclude_frequency"));
						t_add.bindValue(10, s_get.value("exclude_phenotype"));
						t_add.bindValue(11, s_get.value("exclude_mechanism"));
						t_add.bindValue(12, s_get.value("exclude_other"));
						t_add.bindValue(13, s_get.value("comments"));
						t_add.bindValue(14, s_get.value("comments2"));

						t_add.exec();
					}
				}
			}

			//replicate CNVs
			QList<int> source_cnv_ids = db_source_->getValuesInt("SELECT cnv_id FROM report_configuration_cnv WHERE report_configuration_id='" + rc_id_source + "'");
			foreach (int source_cnv_id, source_cnv_ids)
			{
				//get callset id (not present if sample CNVs are not imported)
				bool ok = true;
				int target_callset_id = db_target_->getValue("SELECT id FROM cnv_callset WHERE processed_sample_id="+ps_id).toInt(&ok);
				if (!ok)
				{
					qDebug() << "CNV callset not imported for " << (s_name+"_0"+process_id) << ". Skipped!";
					continue;
				}

				//lift-over
				QString error_message = "";
				int target_cnv_id = liftOverCnv(source_cnv_id, target_callset_id, error_message);
				if (target_cnv_id<0) continue;

				//check if exists
				QVariant target_rcc_id = db_target_->getValue("SELECT id FROM report_configuration_cnv WHERE report_configuration_id='"+rc_id_target+"' AND cnv_id='"+QString::number(target_cnv_id)+"'", true);
				if (!target_rcc_id.isValid())
				{
					++c_add_cnv;

					//get souce data
					SqlQuery s_get = db_source_->getQuery();
					s_get.exec("SELECT * FROM report_configuration_cnv WHERE report_configuration_id='" + rc_id_source +"' AND cnv_id='"+QString::number(source_cnv_id)+"'");
					if (!s_get.next())
					{
						qDebug() << "Error: Report config entry missing for CNV. This should not happen! Skipped!";
						continue;
					}

					//insert into target
					SqlQuery t_add = db_target_->getQuery();
					t_add.prepare("INSERT INTO `report_configuration_cnv`(`report_configuration_id`, `cnv_id`, `type`, `causal`, `class`, `inheritance`, `de_novo`, `mosaic`, `compound_heterozygous`, `exclude_artefact`, `exclude_frequency`, `exclude_phenotype`, `exclude_mechanism`, `exclude_other`, `comments`, `comments2`) VALUES (:0,:1,:2,:3,:4,:5,:6,:7,:8,:9,:10,:11,:12,:13,:14,:15)");
					t_add.bindValue(0, rc_id_target);
					t_add.bindValue(1, target_cnv_id);
					t_add.bindValue(2, s_get.value("type"));
					t_add.bindValue(3, s_get.value("causal"));
					t_add.bindValue(4, s_get.value("class"));
					t_add.bindValue(5, s_get.value("inheritance"));
					t_add.bindValue(6, s_get.value("de_novo"));
					t_add.bindValue(7, s_get.value("mosaic"));
					t_add.bindValue(8, s_get.value("compound_heterozygous"));
					t_add.bindValue(9, s_get.value("exclude_artefact"));
					t_add.bindValue(10, s_get.value("exclude_frequency"));
					t_add.bindValue(11, s_get.value("exclude_phenotype"));
					t_add.bindValue(12, s_get.value("exclude_mechanism"));
					t_add.bindValue(13, s_get.value("exclude_other"));
					t_add.bindValue(14, s_get.value("comments"));
					t_add.bindValue(15, s_get.value("comments2"));

					t_add.exec();
				}
			}

			//replicate SVs
			SqlQuery rc_svs = db_source_->getQuery();
			rc_svs.exec("SELECT * FROM report_configuration_sv WHERE report_configuration_id='" + rc_id_source + "'");
			while(rc_svs.next())
			{
				//get callset id (not present if sample SVs are not imported)
				bool ok = true;
				int target_callset_id = db_target_->getValue("SELECT id FROM sv_callset WHERE processed_sample_id="+ps_id).toInt(&ok);
				if (!ok)
				{
					qDebug() << "SV callset not imported for " << (s_name+"_0"+process_id) << ". Skipped!";
					continue;
				}

				//determine type and ID
				int source_sv_id = -1;
				StructuralVariantType sv_type = StructuralVariantType::UNKNOWN;
				QString sv_col = "";
				if (!rc_svs.value("sv_deletion_id").isNull())
				{
					sv_type = StructuralVariantType::DEL;
					source_sv_id = rc_svs.value("sv_deletion_id").toInt();
					sv_col = "sv_deletion_id";
				}
				else if (!rc_svs.value("sv_duplication_id").isNull())
				{
					sv_type = StructuralVariantType::DUP;
					source_sv_id = rc_svs.value("sv_duplication_id").toInt();
					sv_col = "sv_duplication_id";
				}
				else if (!rc_svs.value("sv_insertion_id").isNull())
				{
					sv_type = StructuralVariantType::INS;
					source_sv_id = rc_svs.value("sv_insertion_id").toInt();
					sv_col = "sv_insertion_id";
				}
				else if (!rc_svs.value("sv_inversion_id").isNull())
				{
					sv_type = StructuralVariantType::INV;
					source_sv_id = rc_svs.value("sv_inversion_id").toInt();
					sv_col = "sv_inversion_id";
				}
				else if (!rc_svs.value("sv_translocation_id").isNull())
				{
					sv_type = StructuralVariantType::BND;
					source_sv_id = rc_svs.value("sv_translocation_id").toInt();
					sv_col = "sv_translocation_id";
				}

				//lift-over
				QString error_message = "";
				int target_sv_id = liftOverSv(source_sv_id, sv_type, target_callset_id, error_message);
				if (target_sv_id<0) continue;
				//check if exists
				QVariant target_rcs_id = db_target_->getValue("SELECT id FROM report_configuration_sv WHERE report_configuration_id='"+rc_id_target+"' AND "+sv_col+"='"+QString::number(target_sv_id)+"'", true);
				if (!target_rcs_id.isValid())
				{
					++c_add_sv;
					SqlQuery t_add = db_target_->getQuery();
					t_add.prepare("INSERT INTO `report_configuration_sv`(`report_configuration_id`, `sv_deletion_id`, `sv_duplication_id`, `sv_insertion_id`, `sv_inversion_id`, `sv_translocation_id`, `type`, `causal`, `class`, `inheritance`, `de_novo`, `mosaic`, `compound_heterozygous`, `exclude_artefact`, `exclude_frequency`, `exclude_phenotype`, `exclude_mechanism`, `exclude_other`, `comments`, `comments2`) VALUES (:0,:1,:2,:3,:4,:5,:6,:7,:8,:9,:10,:11,:12,:13,:14,:15,:16,:17,:18,:19)");
					t_add.bindValue(0, rc_id_target);
					t_add.bindValue(1, sv_type==StructuralVariantType::DEL ? QVariant(target_sv_id) : QVariant());
					t_add.bindValue(2, sv_type==StructuralVariantType::DUP ? QVariant(target_sv_id) : QVariant());
					t_add.bindValue(3, sv_type==StructuralVariantType::INS ? QVariant(target_sv_id) : QVariant());
					t_add.bindValue(4, sv_type==StructuralVariantType::INV ? QVariant(target_sv_id) : QVariant());
					t_add.bindValue(5, sv_type==StructuralVariantType::BND ? QVariant(target_sv_id) : QVariant());
					t_add.bindValue(6, rc_svs.value("type"));
					t_add.bindValue(7, rc_svs.value("causal"));
					t_add.bindValue(8, rc_svs.value("class"));
					t_add.bindValue(9, rc_svs.value("inheritance"));
					t_add.bindValue(10, rc_svs.value("de_novo"));
					t_add.bindValue(11, rc_svs.value("mosaic"));
					t_add.bindValue(12, rc_svs.value("compound_heterozygous"));
					t_add.bindValue(13, rc_svs.value("exclude_artefact"));
					t_add.bindValue(14, rc_svs.value("exclude_frequency"));
					t_add.bindValue(15, rc_svs.value("exclude_phenotype"));
					t_add.bindValue(16, rc_svs.value("exclude_mechanism"));
					t_add.bindValue(17, rc_svs.value("exclude_other"));
					t_add.bindValue(18, rc_svs.value("comments"));
					t_add.bindValue(19, rc_svs.value("comments2"));

					t_add.exec();
				}
			}
		}
		addLine("  Table 'report_configuration_variant' - added variants: "+QString::number(c_add_small));
		addLine("  Table 'report_configuration_cnv' - added CNVs: "+QString::number(c_add_cnv));
		addLine("  Table 'report_configuration_sv' - added SVs: "+QString::number(c_add_sv));
	}
}

void NGSDReplicationWidget::performPostChecks()
{
	addHeader("post-checks");

	//test if all tables are processed
	QStringList target_tables = db_target_->tables();
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

int NGSDReplicationWidget::liftOverVariant(int source_variant_id, bool debug_output)
{
	//cache results to speed things up
	static QHash<int, int> cache;
	if (cache.contains(source_variant_id))
	{
		return cache[source_variant_id];
	}

	const Variant var = db_source_->variant(QString::number(source_variant_id));

	//lift-over variant
	Variant var2;
	try
	{
		var2 = GSvarHelper::liftOverVariant(var, true);
	}
	catch(Exception& e)
	{
		if (debug_output) qDebug() << "-1" << var.toString() << "Lift-over failed: " + e.message();
		cache[source_variant_id] = -1;
		return -1;
	}

	//check for variant in target NGSD
	QString variant_id = db_target_->variantId(var2, false);
	if (variant_id.isEmpty())
	{
		if (debug_output) qDebug() << "-4" << var.toString() << "Lifted variant not found in target NGSD: " + var2.toString();
		cache[source_variant_id] = -4;
		return -4;
	}

	cache[source_variant_id] = variant_id.toInt();
	return variant_id.toInt();
}

int NGSDReplicationWidget::liftOverCnv(int source_cnv_id, int callset_id, QString& error_message)
{
	CopyNumberVariant var = db_source_->cnv(source_cnv_id);

	//lift-over coordinates
	BedLine coords;
	try
	{
		coords = GSvarHelper::liftOver(var.chr(), var.start(), var.end(), true);
	}
	catch(Exception& e)
	{
		error_message = "lift-over failed: " + e.message().replace("\t", " ").replace("<br>", " ").replace("&nbsp;", " ");
		return -1;
	}

	//check new chromosome is ok
	if (!coords.chr().isNonSpecial())
	{
		error_message = "chromosome is now special chromosome: " + coords.chr().strNormalized(true);
		return -2;
	}

	//check for CNV in target NGSD
	QString cn = db_source_->getValue("SELECT cn FROM cnv WHERE id="+QString::number(source_cnv_id)).toString();
	QString cnv_id = db_target_->getValue("SELECT id FROM cnv WHERE cnv_callset_id="+QString::number(callset_id)+" AND chr='"+coords.chr().strNormalized(true)+"' AND start='"+QString::number(coords.start())+"' AND end='"+QString::number(coords.start())+"' AND cn="+cn, true).toString();
	if (cnv_id.isEmpty())
	{
		//no exact match, try fuzzy match (at least 70% of original/liftover size)
		int size_hg19 = var.size();
		int size_hg38 = coords.length();

		SqlQuery query = db_target_->getQuery();
		query.exec("SELECT * FROM cnv WHERE cnv_callset_id='" + QString::number(callset_id) + "' AND cn='"+cn+"' AND chr='" + coords.chr().strNormalized(true) + "' AND end>" + QString::number(coords.start()) + " AND start<" + QString::number(coords.end()) + "");
		while (query.next())
		{
			int size = query.value("end").toInt() - query.value("start").toInt();
			if (size > 0.7 * size_hg19 && size > 0.7 * size_hg38)
			{
				return query.value("id").toInt();
			}
		}

		error_message = "CNV not found in NGSD";
		return -4;
	}

	return cnv_id.toInt();
}

int NGSDReplicationWidget::liftOverSv(int source_sv_id, StructuralVariantType sv_type, int callset_id, QString& error_message)
{
	BedpeFile dummy; //dummy BEDPE file to pass as structure
	QList<QByteArray> annotation_headers;
	annotation_headers << "QUAL" << "FILTER" << "ALT_A" << "INFO_A" << "FORMAT" << "";
	dummy.setAnnotationHeaders(annotation_headers);
	BedpeLine sv = db_source_->structuralVariant(source_sv_id, sv_type, dummy, false);

	//lift-over coordinates
	BedLine coords1, coords2;
	try
	{
		coords1 = GSvarHelper::liftOver(sv.chr1(), sv.start1(), sv.end1(), true);
	}
	catch(Exception& e)
	{
		error_message = "lift-over failed (pos1): " + e.message().replace("\t", " ").replace("<br>", " ").replace("&nbsp;", " ");
		return -1;
	}
	try
	{
		coords2 = GSvarHelper::liftOver(sv.chr2(), sv.start2(), sv.end2(), true);
	}
	catch(Exception& e)
	{
		error_message = "lift-over failed (pos2): " + e.message().replace("\t", " ").replace("<br>", " ").replace("&nbsp;", " ");
		return -1;
	}


	//check new chromosome is ok
	if (!coords1.chr().isNonSpecial())
	{
		error_message = "chromosome 1 is now special chromosome: " + coords1.chr().strNormalized(true);
		return -2;
	}
	if (!coords2.chr().isNonSpecial())
	{
		error_message = "chromosome 2 is now special chromosome: " + coords2.chr().strNormalized(true);
		return -2;
	}

	//check for strand changes
	if (sv_type != StructuralVariantType::INS && coords2 < coords1)
	{
		// switch positions
		error_message = "(WARNING: strand changed!) ";
		BedLine tmp = coords1;
		coords1 = coords2;
		coords2 = tmp;
	}

	//check for SV in target NGSD
	sv.setChr1(coords1.chr());
	sv.setStart1(coords1.start());
	sv.setEnd1(coords1.end());
	sv.setChr2(coords2.chr());
	sv.setStart2(coords2.start());
	sv.setEnd2(coords2.end());
	QString sv_id = db_target_->svId(sv, callset_id, dummy, false);
	if (sv_id.isEmpty())
	{
		// no exact match found -> try fuzzy match (overlapping of CI)
		SqlQuery query = db_target_->getQuery();
		QByteArray query_string;

		// prepare query
		if (sv.type() == StructuralVariantType::BND)
		{
			//BND
			query_string = "SELECT id FROM sv_translocation sv WHERE sv_callset_id = :0 AND sv.chr1 = :1 AND sv.start1 <= :2 AND :3 <= sv.end1 AND sv.chr2 = :4 AND sv.start2 <= :5 AND :6 <= sv.end2";
			query.prepare(query_string);
			query.bindValue(0, callset_id);
			query.bindValue(1, sv.chr1().strNormalized(true));
			query.bindValue(2, sv.end1());
			query.bindValue(3, sv.start1());
			query.bindValue(4, sv.chr2().strNormalized(true));
			query.bindValue(5, sv.end2());
			query.bindValue(6, sv.start2());

		}
		else if (sv.type() == StructuralVariantType::INS)
		{
			//INS

			//get min and max position
			int min_pos = std::min(sv.start1(), sv.start2());
			int max_pos = std::max(sv.end1(), sv.end2());

			query_string = "SELECT id FROM sv_insertion sv WHERE sv_callset_id = :0 AND sv.chr = :1 AND sv.pos <= :2 AND :3 <= (sv.pos + sv.ci_upper)";
			query.prepare(query_string);
			query.bindValue(0, callset_id);
			query.bindValue(1, sv.chr1().strNormalized(true));
			query.bindValue(2, max_pos);
			query.bindValue(3, min_pos);
		}
		else
		{
			//DEL, DUP, INV
			query_string = "SELECT id FROM " + db_target_->svTableName(sv.type()).toUtf8() + " sv WHERE sv_callset_id = :0 AND sv.chr = :1 AND sv.start_min <= :2 AND :3 <= sv.start_max AND sv.end_min <= :4 AND :5 <= sv.end_max";
			query.prepare(query_string);
			query.bindValue(0, callset_id);
			query.bindValue(1, sv.chr1().strNormalized(true));
			query.bindValue(2, sv.end1());
			query.bindValue(3, sv.start1());
			query.bindValue(4, sv.end2());
			query.bindValue(5, sv.start2());
		}

		query.exec();

		//parse result
		if (query.size() == 0)
		{
			error_message = "SV not found in NGSD";
			return -4;
		}
		else if (query.size() > 1)
		{
			error_message = "Multiple matching SVs found in NGSD";
			return -5;
		}
		query.next();
		return query.value(0).toInt();

	}
	// return id
	return sv_id.toInt();

}

void NGSDReplicationWidget::updateCnvTable(QString table, QString where_clause)
{
	QFile file(QCoreApplication::applicationDirPath()  + QDir::separator() + "liftover_" + table + ".tsv");
	file.open(QIODevice::WriteOnly);
	QTextStream debug_stream(&file);
	debug_stream << "#ps\tCNV\tcn\tsize_kb\tregs\tll_per_regs\tfound_in_HG38\terror\tcomments\n";

	QStringList fields = db_target_->tableInfo(table).fieldNames();

	//init
	QTime timer;
	timer.start();

	int c_added = 0;
	int c_kept = 0;
	int c_removed = 0;
	int c_updated = 0;
	int c_not_mappable = 0;
	int c_not_in_ngsd = 0;
	int c_no_callset = 0;

	SqlQuery q_del = db_target_->getQuery();
	q_del.prepare("DELETE FROM "+table+" WHERE id=:0");

	SqlQuery q_add = db_target_->getQuery();
	q_add.prepare("INSERT INTO "+table+" VALUES (:" + fields.join(", :") + ")");

	SqlQuery q_get = db_target_->getQuery();
	q_get.prepare("SELECT * FROM "+table+" WHERE id=:0");

	SqlQuery q_update = db_target_->getQuery();
	QString query_str = "UPDATE "+table+" SET";
	bool first = true;
	foreach(const QString& field, fields)
	{
		if (field=="id") continue;

		//special handling of CNV (lifted-over)
		if (field=="cnv_id") continue;

		query_str += (first? " " : ", ") + field + "=:" + field;

		if (first) first = false;
	}
	query_str += " WHERE id=:id";
	q_update.prepare(query_str);

	//delete removed entries
	QSet<int> source_ids = db_source_->getValuesInt("SELECT id FROM " + table + " " + where_clause + " ORDER BY id ASC").toSet();
	QSet<int> target_ids = db_target_->getValuesInt("SELECT id FROM " + table + " " + where_clause + " ORDER BY id ASC").toSet();
	foreach(int id, target_ids)
	{
		if (!source_ids.contains(id))
		{
			q_del.bindValue(0, id);
			q_del.exec();

			++c_removed;
		}
	}

	//add/update entries
	SqlQuery query = db_source_->getQuery();
	query.exec("SELECT * FROM " + table + " " + where_clause + " ORDER BY id ASC");
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
				//special handling of CNVs (lifted-over)
				if (field=="cnv_id") continue;

				if (q_get.value(field)!=query.value(field))
				{
					changed = true;
				}
			}

			//update if changed
			if (changed)
			{
				foreach(const QString& field, fields)
				{
					//special handling of CNVs (lifted-over)
					if (field=="cnv_id") continue;

					q_update.bindValue(":"+field, query.value(field));
				}
				q_update.exec();

				++c_updated;
			}
			else
			{
				++c_kept;
			}
		}
		else //row not in target table > add
		{
			int source_cnv_id = query.value("cnv_id").toInt();
			int target_cnv_id = -1;

			QString ps_id = db_source_->getValue("SELECT cs.processed_sample_id FROM cnv c, cnv_callset cs WHERE c.cnv_callset_id=cs.id AND c.id=" + QString::number(source_cnv_id)).toString();
			QString ps = db_source_->processedSampleName(ps_id);
			QVariant callset_id = db_target_->getValue("SELECT id FROM cnv_callset WHERE processed_sample_id=" + ps_id);
			if (!callset_id.isValid())
			{
				++c_no_callset;
				continue;
			}

			bool causal_variant = table=="report_configuration_cnv" && query.value("causal").toInt()==1;
			bool pathogenic_variant = table=="report_configuration_cnv" && query.value("class").toInt()>3;
			QString error_message = "";
			target_cnv_id = liftOverCnv(source_cnv_id, callset_id.toInt(), error_message);

			int cn = db_source_->getValue("SELECT cn FROM cnv WHERE id=" + QString::number(source_cnv_id), false).toInt();
			int ll = -1;
			int regs = -1;
			QString metrics = db_source_->getValue("SELECT quality_metrics FROM cnv WHERE id=" + QString::number(source_cnv_id), false).toString();
			foreach(QString metric, metrics.split(","))
			{
				metric.replace("\"", "");
				metric.replace("{", "");
				metric.replace("}", "");
				if (metric.contains("loglikelihood"))
				{
					ll = metric.split(":")[1].toInt();
				}
				if (metric.contains("regions") || metric.contains("no_of_regions"))
				{
					regs = metric.split(":")[1].toInt();
				}
			}
			QStringList comments;
			if (causal_variant) comments << "causal";
			if (pathogenic_variant) comments << "pathogenic";
			CopyNumberVariant var = db_source_->cnv(source_cnv_id);

			debug_stream << ps << "\t" << var.toString() << "\t" << cn << "\t" << QString::number((double)var.size()/1000, 'f', 2) << "\t"  << regs << "\t" << QString::number((double)ll/regs, 'f', 2) << "\t"  << (target_cnv_id>=0 ? "yes ("+QString::number(target_cnv_id)+")" : "no") << "\t" << error_message << "\t" << comments.join(", ") << "\n";
			debug_stream.flush();

			//warn if causal/pathogenic CNV could not be lifted
			if (target_cnv_id<0)
			{
				if (causal_variant)
				{
					addWarning("Causal CNV " + db_source_->cnv(source_cnv_id).toString() + " of sample " + ps + " could not be lifted (" + QString::number(target_cnv_id) + ")!");
				}
				else if (pathogenic_variant)
				{
					addWarning("Pathogenic CNV " + db_source_->cnv(source_cnv_id).toString() + " of sample " + ps + " (class " + query.value("class").toString() + ") could not be lifted (" + QString::number(target_cnv_id) + ")!");
				}
			}

			if (target_cnv_id>=0)
			{
				foreach(const QString& field, fields)
				{
					if (field=="cnv_id") //special handling of CNVs (lifted-over)
					{
						q_add.bindValue(":"+field, target_cnv_id);
						continue;
					}

					q_add.bindValue(":"+field, query.value(field));
				}
				try
				{
					q_add.exec();
				}
				catch (Exception& e)
				{
					if (table=="report_configuration_cnv") //due to lift-over we sometimes have several entries that map to the same ps-cnv combination => skip them
					{
						addWarning("Could not add report_configuration_cnv entry: " + e.message());
					}
					else
					{
						THROW(Exception, "Could not add table "+table+" entry with id "+QString::number(id)+": "+e.message());
					}
				}
				++c_added;
			}
			else if (target_cnv_id>=-3)
			{
				++c_not_mappable;
			}
			else if (target_cnv_id==-4)
			{
				++c_not_in_ngsd;
			}
			else
			{
				THROW(NotImplementedException, "Unhandled CNV error: " + QString::number(target_cnv_id));
			}
		}
	}

	//output
	QStringList details;
	if (c_added>0) details << "added " + QString::number(c_added);
	if (c_not_mappable>0) details << "skipped unmappable CNVs " + QString::number(c_not_mappable);
	if (c_not_in_ngsd>0) details << "skipped CNVs not found in NGSD " + QString::number(c_not_in_ngsd);
	if (c_no_callset>0) details << "skipped CNVs of samples without callset " + QString::number(c_no_callset);
	if (c_kept>0) details << "kept " + QString::number(c_kept);
	if (c_updated>0) details << "updated " + QString::number(c_updated);
	if (c_removed>0) details << "removed " + QString::number(c_removed);

	addLine("  Table '"+table+"' replicated: "+details.join(", ")+". Time: " + Helper::elapsedTime(timer));
	tables_done_ << table;
}

void NGSDReplicationWidget::updateSvTable(QString table, StructuralVariantType sv_type, QString where_clause)
{
	BedpeFile dummy; //dummy BEDPE file to pass as structure
	QList<QByteArray> annotation_headers;
	annotation_headers << "QUAL" << "FILTER" << "ALT_A" << "INFO_A";
	dummy.setAnnotationHeaders(annotation_headers);

	QFile file(QCoreApplication::applicationDirPath()  + QDir::separator() + "liftover_" + table + "_" + StructuralVariantTypeToString(sv_type) + ".tsv");
	file.open(QIODevice::WriteOnly);
	QTextStream debug_stream(&file);
	debug_stream << "#ps\tSV\ttype\tfound_in_HG38\terror\tcomments\n";

	QStringList fields = db_target_->tableInfo(table).fieldNames();

	//init
	QTime timer;
	timer.start();

	int c_added = 0;
	int c_kept = 0;
	int c_removed = 0;
	int c_updated = 0;
	int c_not_mappable = 0;
	int c_not_in_ngsd = 0;
	int c_no_callset = 0;
	int c_strand_changed = 0;

	SqlQuery q_del = db_target_->getQuery();
	q_del.prepare("DELETE FROM "+table+" WHERE id=:0");

	SqlQuery q_add = db_target_->getQuery();
	q_add.prepare("INSERT INTO "+table+" VALUES (:" + fields.join(", :") + ")");

	SqlQuery q_get = db_target_->getQuery();
	q_get.prepare("SELECT * FROM "+table+" WHERE id=:0");

	SqlQuery q_update = db_target_->getQuery();
	QString query_str = "UPDATE "+table+" SET";
	bool first = true;
	foreach(const QString& field, fields)
	{
		if (field=="id") continue;

		//special handling of SV (lifted-over)
		if (field=="sv_id") continue;

		query_str += (first? " " : ", ") + field + "=:" + field;

		if (first) first = false;
	}
	query_str += " WHERE id=:id";
	q_update.prepare(query_str);

	//delete removed entries
	QSet<int> source_ids = db_source_->getValuesInt("SELECT id FROM " + table + " " + where_clause + " ORDER BY id ASC").toSet();
	QSet<int> target_ids = db_target_->getValuesInt("SELECT id FROM " + table + " " + where_clause + " ORDER BY id ASC").toSet();
	foreach(int id, target_ids)
	{
		if (!source_ids.contains(id))
		{
			q_del.bindValue(0, id);
			q_del.exec();

			++c_removed;
		}
	}

	//add/update entries
	SqlQuery query = db_source_->getQuery();
	query.exec("SELECT * FROM " + table + " " + where_clause + " ORDER BY id ASC");
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
				//special handling of SV ids (lifted-over)
				if (sv_type == StructuralVariantType::DEL && field=="sv_deletion_id") continue;
				if (sv_type == StructuralVariantType::DUP && field=="sv_duplication_id") continue;
				if (sv_type == StructuralVariantType::INS && field=="sv_insertion_id") continue;
				if (sv_type == StructuralVariantType::INV && field=="sv_inversion_id") continue;
				if (sv_type == StructuralVariantType::BND && field=="sv_translocation_id") continue;

				if (q_get.value(field)!=query.value(field))
				{
					changed = true;
				}
			}

			//update if changed
			if (changed)
			{
				foreach(const QString& field, fields)
				{
					//special handling of SV ids (lifted-over)
					if (sv_type == StructuralVariantType::DEL && field=="sv_deletion_id") continue;
					if (sv_type == StructuralVariantType::DUP && field=="sv_duplication_id") continue;
					if (sv_type == StructuralVariantType::INS && field=="sv_insertion_id") continue;
					if (sv_type == StructuralVariantType::INV && field=="sv_inversion_id") continue;
					if (sv_type == StructuralVariantType::BND && field=="sv_translocation_id") continue;

					q_update.bindValue(":"+field, query.value(field));
				}
				q_update.exec();

				++c_updated;
			}
			else
			{
				++c_kept;
			}
		}
		else //row not in target table > add
		{
			int source_sv_id;
			QString ps_id;
			switch (sv_type)
			{
				case StructuralVariantType::DEL:
					source_sv_id = query.value("sv_deletion_id").toInt();
					ps_id = db_source_->getValue("SELECT cs.processed_sample_id FROM sv_deletion sv, sv_callset cs WHERE sv.sv_callset_id=cs.id AND sv.id=" + QString::number(source_sv_id)).toString();
					break;
				case StructuralVariantType::DUP:
					source_sv_id = query.value("sv_duplication_id").toInt();
					ps_id = db_source_->getValue("SELECT cs.processed_sample_id FROM sv_duplication sv, sv_callset cs WHERE sv.sv_callset_id=cs.id AND sv.id=" + QString::number(source_sv_id)).toString();
					break;
				case StructuralVariantType::INS:
					source_sv_id = query.value("sv_insertion_id").toInt();
					ps_id = db_source_->getValue("SELECT cs.processed_sample_id FROM sv_insertion sv, sv_callset cs WHERE sv.sv_callset_id=cs.id AND sv.id=" + QString::number(source_sv_id)).toString();
					break;
				case StructuralVariantType::INV:
					source_sv_id = query.value("sv_inversion_id").toInt();
					ps_id = db_source_->getValue("SELECT cs.processed_sample_id FROM sv_inversion sv, sv_callset cs WHERE sv.sv_callset_id=cs.id AND sv.id=" + QString::number(source_sv_id)).toString();
					break;
				case StructuralVariantType::BND:
					source_sv_id = query.value("sv_translocation_id").toInt();
					ps_id = db_source_->getValue("SELECT cs.processed_sample_id FROM sv_translocation sv, sv_callset cs WHERE sv.sv_callset_id=cs.id AND sv.id=" + QString::number(source_sv_id)).toString();
					break;
				default:
					THROW(ArgumentException, "Invalid SV type!");
					break;
			}
			int target_sv_id = -1;
			QString ps = db_source_->processedSampleName(ps_id);
			QVariant callset_id = db_target_->getValue("SELECT id FROM sv_callset WHERE processed_sample_id=" + ps_id);
			if (!callset_id.isValid())
			{
				++c_no_callset;
				continue;
			}

			bool causal_variant = table=="report_configuration_sv" && query.value("causal").toInt()==1;
			bool pathogenic_variant = table=="report_configuration_sv" && query.value("class").toInt()>3;
			QString error_message = "";
			target_sv_id = liftOverSv(source_sv_id, sv_type, callset_id.toInt(), error_message);

			QStringList comments;
			if (causal_variant) comments << "causal";
			if (pathogenic_variant) comments << "pathogenic";
			BedpeLine sv = db_source_->structuralVariant(source_sv_id, sv_type, dummy, true);

			debug_stream << ps << "\t" << sv.toString() << "\t" << StructuralVariantTypeToString(sv_type) << "\t" << (target_sv_id>=0 ? "yes" : "no") << "\t" << error_message << "\t"
						 << comments.join(", ") << "\n";
			debug_stream.flush();


			//check for strand changes
			if (error_message.contains("(WARNING: strand changed!)")) c_strand_changed++;

			//warn if causal/pathogenic SV could not be lifed
			if (target_sv_id<0)
			{
				if (causal_variant)
				{
					addWarning("Causal SV " + db_source_->structuralVariant(source_sv_id, sv_type, dummy, true).toString() + " of sample " + ps + " could not be lifted ("
							   + QString::number(target_sv_id) + ", '" + error_message +"')!");
				}
				else if (pathogenic_variant)
				{
					addWarning("Pathogenic SV " + db_source_->structuralVariant(source_sv_id, sv_type, dummy, true).toString() + " of sample " + ps + " (class "
							   + query.value("class").toString() + ") could not be lifted (" + QString::number(target_sv_id) + ", '" + error_message +"')!");
				}
			}

			if (target_sv_id>=0)
			{
				foreach(const QString& field, fields)
				{
					//special handling of SV ids (lifted-over)
					if (sv_type == StructuralVariantType::DEL && field=="sv_deletion_id")
					{
						q_add.bindValue(":"+field, target_sv_id);
						continue;
					}
					if (sv_type == StructuralVariantType::DUP && field=="sv_duplication_id")
					{
						q_add.bindValue(":"+field, target_sv_id);
						continue;
					}
					if (sv_type == StructuralVariantType::INS && field=="sv_insertion_id")
					{
						q_add.bindValue(":"+field, target_sv_id);
						continue;
					}
					if (sv_type == StructuralVariantType::INV && field=="sv_inversion_id")
					{
						q_add.bindValue(":"+field, target_sv_id);
						continue;
					}
					if (sv_type == StructuralVariantType::BND && field=="sv_translocation_id")
					{
						q_add.bindValue(":"+field, target_sv_id);
						continue;
					}

					q_add.bindValue(":"+field, query.value(field));
				}
				try
				{
					q_add.exec();
				}
				catch (Exception& e)
				{
					THROW(Exception, "Could not add table "+table+" entry with id "+QString::number(id)+": "+e.message());
				}
				++c_added;
			}
			else if (target_sv_id>=-3)
			{
				++c_not_mappable;
			}
			else if (target_sv_id==-4)
			{
				++c_not_in_ngsd;
			}
			else
			{
				THROW(NotImplementedException, "Unhandled SV error: " + QString::number(target_sv_id));
			}
		}
	}

	//output
	QStringList details;
	if (c_added>0) details << "added " + QString::number(c_added);
	if (c_not_mappable>0) details << "skipped unmappable " + StructuralVariantTypeToString(sv_type) + "s " + QString::number(c_not_mappable);
	if (c_not_in_ngsd>0) details << "skipped " + StructuralVariantTypeToString(sv_type) + "s not found in NGSD " + QString::number(c_not_in_ngsd);
	if (c_no_callset>0) details << "skipped " + StructuralVariantTypeToString(sv_type) + "s of samples without callset " + QString::number(c_no_callset);
	if (c_kept>0) details << "kept " + QString::number(c_kept);
	if (c_updated>0) details << "updated " + QString::number(c_updated);
	if (c_removed>0) details << "removed " + QString::number(c_removed);
	if (c_strand_changed>0) details << "strand changed " + QString::number(c_strand_changed);

	addLine("  Table '" + table + "' replicated (" + StructuralVariantTypeToString(sv_type) + "): "+details.join(", ")+". Time: " + Helper::elapsedTime(timer));
	tables_done_ << table;
}

