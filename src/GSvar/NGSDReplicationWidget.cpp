#include "NGSDReplicationWidget.h"
#include "NGSD.h"
#include "GSvarHelper.h"
#include "Settings.h"

NGSDReplicationWidget::NGSDReplicationWidget(QWidget* parent)
	: QWidget(parent)
	, ui_()
{
	ui_.setupUi(this);
	connect(ui_.repliate_btn, SIGNAL(clicked(bool)), this, SLOT(replicate()));

	//init databases and genome indices
	try
	{
		db_source_ = QSharedPointer<NGSD>(new NGSD());
		db_target_ = QSharedPointer<NGSD>(new NGSD(false, true));

		genome_index_ = QSharedPointer<FastaFileIndex>(new FastaFileIndex(Settings::string("reference_genome")));
		genome_index_hg38_ = QSharedPointer<FastaFileIndex>(new FastaFileIndex(Settings::string("reference_genome_hg38")));
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

		if (db_source_->tableInfo(table).fieldNames()!=db_target_->tableInfo(table).fieldNames())
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

void NGSDReplicationWidget::updateTable(QString table, bool contains_variant_id, QString sample_id)
{
	int hg38_id = db_source_->getValue("SELECT id FROM genome WHERE build='GRCH38'").toInt();

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
	QSet<int> source_ids = db_source_->getValuesInt("SELECT id FROM " + table + " ORDER BY id ASC").toSet();
	QSet<int> target_ids = db_target_->getValuesInt("SELECT id FROM " + table + " ORDER BY id ASC").toSet();
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

					q_update.bindValue(":"+field, query.value(field));
				}
				q_update.exec();

				++c_updated;
			}
		}
		else //row not in target table > add
		{
			int source_variant_id = -1;
			int target_variant_id = -1;
			if (contains_variant_id)
			{
				source_variant_id = query.value("variant_id").toInt();
				target_variant_id = liftOverVariant(source_variant_id);
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
						THROW(Exception, "Could not add table "+table+" entry with id "+id+": "+e.message());
					}
				}
				++c_added;
			}
			else if (target_variant_id<=3)
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
	details << "added " + QString::number(c_added);
	if (contains_variant_id)
	{
		details << "skipped unmappable variants " + QString::number(c_not_mappable);
		details << "skipped variants not in NGSD " + QString::number(c_not_in_ngsd);
	}
	details << "updated " + QString::number(c_updated);
	details << "removed " + QString::number(c_removed);

	addLine("  Table '"+table+"' replicated: "+details.join(", ")+". Time: " + Helper::elapsedTime(timer));
	tables_done_ << table;
}

void NGSDReplicationWidget::replicateBaseDataNoId()
{
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

	//process tables that are not sample-specific
	updateTable("variant_classification", true);
	updateTable("somatic_variant_classification", true);

	//select processed samples to use
	QStringList ps_ids;
	if (!ui_.samples->text().trimmed().isEmpty()) //samples from GUI
	{
		foreach(QString ps, ui_.samples->text().split(","))
		{
			ps_ids << db_target_->processedSampleId(ps);
		}
	}
	else //all samples that have variants
	{
		ps_ids = db_target_->getValues("SELECT id FROM processed_sample ps WHERE EXISTS(SELECT * FROM detected_variant WHERE processed_sample_id=ps.id) OR EXISTS(SELECT * FROM detected_somatic_variant WHERE processed_sample_id_tumor=ps.id ) ORDER BY id ASC");
	}
	qDebug() << ps_ids.count();
	return; //TODO


	//process
	foreach(QString ps_id, ps_ids)
	{
		QMap<int, int> c_fail;
		int c_ok = 0;
		SqlQuery query = db_source_->getQuery();
		query.exec("SELECT variant_id FROM detected_variant WHERE processed_sample_id="+ps_id);
		while(query.next())
		{
			int variant_id = query.value(0).toInt();
			int id = liftOverVariant(variant_id);

			if (id<0) c_fail[id] +=1;
			else ++c_ok;
		}
		qDebug() << ps_id << c_ok;
		for(auto it=c_fail.begin(); it!=c_fail.end(); ++it)
		{
			qDebug() << it.key() << it.value();
		}
	}

	//TODO
	/*
	Warning: Table 'variant_publication' not filled!
	Warning: Table 'variant_validation' not filled!
	Warning: Table 'report_configuration_variant' not filled!
	Warning: Table 'report_configuration_cnv' not filled!
	Warning: Table 'report_configuration_sv' not filled!

	Warning: Table 'somatic_vicc_interpretation' not filled!
	Warning: Table 'somatic_report_configuration_cnv' not filled!
	Warning: Table 'somatic_report_configuration_germl_var' not filled!
	Warning: Table 'somatic_report_configuration_variant' not filled!

	Warning: Table 'gaps' not filled!
	Warning: Table 'cfdna_panel_genes' not filled!
	Warning: Table 'cfdna_panels' not filled!
	*/
}

int NGSDReplicationWidget::liftOverVariant(int source_variant_id)
{
	Variant var = db_source_->variant(QString::number(source_variant_id));
	//lift-over coordinates
	BedLine coords;
	try
	{
		coords = GSvarHelper::liftOver(var.chr(), var.start(), var.end());
	}
	catch(Exception& /*e*/)
	{
		return -1;
	}

	//check new chromosome is ok
	if (!coords.chr().isNonSpecial())
	{
		return -2;
	}

	//check sequence context is the same (ref +-10 bases)
	Sequence context_old = genome_index_->seq(var.chr(), var.start()-10, 20 + var.ref().length());
	Sequence context_new = genome_index_hg38_->seq(coords.chr(), coords.start()-10, 20 + var.ref().length());
	if (context_old!=context_new)
	{
		return -3;
	}

	//check for variant in target NGSD
	QString variant_id = db_target_->variantId(Variant(coords.chr(), coords.start(), coords.end(), var.ref(), var.obs()), false);
	if (variant_id.isEmpty())
	{
		return -4;
	}

	return variant_id.toInt();
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
