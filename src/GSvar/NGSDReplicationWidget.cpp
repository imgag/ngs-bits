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

		genome_index_ = QSharedPointer<FastaFileIndex>(new FastaFileIndex(Settings::string("reference_genome")));
		genome_index_hg19_ = QSharedPointer<FastaFileIndex>(new FastaFileIndex(Settings::string("reference_genome_hg19")));
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
	q_add.prepare("INSERT INTO "+table+" VALUES (:" + fields.join(", :") + ")");

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
	//TODO
	/*
	 Replicate missing variant data by insert only (otherwise work done in the GRCh38 database might be lost)
	 - report config (small variants, CNVs, SVs)
	 - variant classification
	 - variant publication
	 - variant validation
	*/
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
	Variant var = db_source_->variant(QString::number(source_variant_id));
	//lift-over coordinates
	BedLine coords;
	try
	{
		coords = GSvarHelper::liftOver(var.chr(), var.start(), var.end());
	}
	catch(Exception& e)
	{
		if (debug_output) qDebug() << "-1" << var.toString() << e.message();
		return -1;
	}

	//check new chromosome is ok
	if (!coords.chr().isNonSpecial())
	{
		if (debug_output) qDebug() << "-2" << var.toString() << coords.chr().str()+":"+QString::number(coords.start())+"-"+QString::number(coords.end());
		return -2;
	}

	//check sequence context is the same (ref +-5 bases). If it is not, the strand might have changed, e.g. in NIPA1, GDF2, ANKRD35, TPTE, ...
	bool strand_changed = false;
	Sequence context_old = genome_index_->seq(var.chr(), var.start()-5, 10 + var.ref().length());
	Sequence context_new = genome_index_hg19_->seq(coords.chr(), coords.start()-5, 10 + var.ref().length());
	if (context_old!=context_new)
	{
		context_new.reverseComplement();
		if (context_old==context_new)
		{
			strand_changed = true;
		}
		else
		{
			context_new.reverseComplement();
			if (debug_output) qDebug() << "-3" << var.toString() << coords.chr().str()+":"+QString::number(coords.start())+"-"+QString::number(coords.end()) << "old_context="+context_old << "new_context="+context_new;
			return -3;
		}
	}

	//check for variant in target NGSD
	Sequence ref = var.ref();
	if (strand_changed && ref!="-") ref.reverseComplement();
	Sequence obs = var.obs();
	if (strand_changed && obs!="-") obs.reverseComplement();
	QString variant_id = db_target_->variantId(Variant(coords.chr(), coords.start(), coords.end(), ref, obs), false);
	if (variant_id.isEmpty())
	{
		if (debug_output) qDebug() << "-4" << var.toString() << coords.chr().str()+":"+QString::number(coords.start())+"-"+QString::number(coords.end()) << "strand_changed="+QString(strand_changed?"yes":"no") << "af="+db_source_->getValue("SELECT gnomad FROM variant WHERE id="+QString::number(source_variant_id)).toString();
		return -4;
	}

	return variant_id.toInt();
}

int NGSDReplicationWidget::liftOverCnv(int source_cnv_id, int callset_id, QString& error_message)
{
	CopyNumberVariant var = db_source_->cnv(source_cnv_id);

	//lift-over coordinates
	BedLine coords;
	try
	{
		coords = GSvarHelper::liftOver(var.chr(), var.start(), var.end());
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
	annotation_headers << "QUAL" << "FILTER" << "ALT_A" << "INFO_A";
	dummy.setAnnotationHeaders(annotation_headers);
	BedpeLine sv = db_source_->structuralVariant(source_sv_id, sv_type, dummy, false);

	//lift-over coordinates
	BedLine coords1, coords2;
	try
	{
		coords1 = GSvarHelper::liftOver(sv.chr1(), sv.start1(), sv.end1());
	}
	catch(Exception& e)
	{
		error_message = "lift-over failed (pos1): " + e.message().replace("\t", " ").replace("<br>", " ").replace("&nbsp;", " ");
		return -1;
	}
	try
	{
		coords2 = GSvarHelper::liftOver(sv.chr2(), sv.start2(), sv.end2());
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

