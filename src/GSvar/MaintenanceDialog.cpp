#include "MaintenanceDialog.h"
#include "Helper.h"
#include "NGSD.h"
#include "GenLabDB.h"
#include <QTime>
#include <QMetaMethod>
#include <QScrollBar>

MaintenanceDialog::MaintenanceDialog(QWidget *parent)
	: QDialog(parent)
	, ui_()
{
	ui_.setupUi(this);
	setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);

	connect(ui_.exec_btn, SIGNAL(clicked()), this, SLOT(executeAction()));
	connect(ui_.action, SIGNAL(currentTextChanged(QString)), this, SLOT(updateDescription(QString)));
}

void MaintenanceDialog::executeAction()
{
	//init
	ui_.exec_btn->setEnabled(false);
	ui_.output->clear();
	appendOutputLine("START");
	appendOutputLine("");

	//perform action
    QElapsedTimer timer;
	timer.start();

	try
	{
		QString action = ui_.action->currentText().trimmed();
        if (action.isEmpty() || action[0]==QString("["))
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
				THROW(ProgrammingException, "No slot with name '" + action_simplified + "' found!");
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

    QSet<int> var_ids_pub = LIST_TO_SET(db.getValuesInt("SELECT variant_id FROM variant_publication WHERE variant_table='variant'"));

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

	//import
	foreach(const QString& study, studies_genlab)
	{
		if (missing.contains(study)) continue;

		appendOutputLine("");
		appendOutputLine("Processing study " + study + "...");

		QString study_id = db.getValue("SELECT id FROM study WHERE name COLLATE UTF8_GENERAL_CI LIKE '" + study + "'", false).toString();

		//determine processed sample in study
		QStringList errors;
        QSet<int> genlab_ps_ids = LIST_TO_SET(genlab.studySamples(study, errors));
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
        QSet<int> extra_ids = LIST_TO_SET(db.getValuesInt("SELECT processed_sample_id FROM study_sample WHERE study_id='" + study_id + "'"));
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
    QSet<QString> hpo_terms_valid = LIST_TO_SET(db.getValues("SELECT hpo_id FROM hpo_term"));
    QSet<QString> hpo_terms_obsolete = LIST_TO_SET(db.getValues("SELECT hpo_id FROM hpo_obsolete"));

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
			QVariant replace_term_id = db.getValue("SELECT replaced_by FROM hpo_obsolete WHERE hpo_id='" + hpo_id + "'", false);
			if(!replace_term_id.isNull()) //replacement term available => replace
			{
				QString replace_term = db.getValue("SELECT hpo_id FROM hpo_term WHERE id='" + replace_term_id.toString() + "'", false).toString().trimmed();
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

void MaintenanceDialog::replaceObsolteGeneSymbols()
{
	NGSD db;
	fixGeneNames(db, "geneinfo_germline", "symbol");
	fixGeneNames(db, "report_polymorphisms", "symbol");
	fixGeneNames(db, "somatic_gene_role", "symbol");
	fixGeneNames(db, "somatic_pathway_gene", "symbol");
	//TODO Marc/Leon: fixGeneNames(db, "expression_gene", "symbol");
	fixGeneNames(db, "hpo_genes", "gene");
	fixGeneNames(db, "omim_gene", "gene");
	fixGeneNames(db, "omim_preferred_phenotype", "gene");
	fixGeneNames(db, "disease_gene", "gene");
	fixGeneNames(db, "report_configuration_other_causal_variant", "gene");
}

void MaintenanceDialog::appendOutputLine(QString line)
{
	while(line.endsWith(' ') || line.endsWith('\t') || line.endsWith('\r') || line.endsWith('\n')) line.chop(1);

	if (!line.isEmpty()) line = QDateTime::currentDateTime().toString(Qt::ISODate).replace("T", " ") + "\t" + line;

	ui_.output->append(line);

	ui_.output->verticalScrollBar()->setValue(ui_.output->verticalScrollBar()->maximum());

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

void MaintenanceDialog::findInconsistenciesForCausalDiagnosticVariants()
{
	NGSD db;
	QStringList ps_names = db.getValues("SELECT DISTINCT CONCAT(s.name, '_0', ps.process_id) FROM sample s, processed_sample ps, report_configuration rc, report_configuration_variant rcv, project p, processing_system sys WHERE ps.processing_system_id=sys.id AND (sys.type='WGS' OR sys.type='WES' OR sys.type='lrGS') AND ps.project_id=p.id AND p.type='diagnostic' AND ps.sample_id=s.id AND ps.quality!='bad' AND rc.processed_sample_id=ps.id AND rcv.report_configuration_id=rc.id AND rcv.causal='1' AND rcv.type='diagnostic variant' ORDER BY ps.id ASC");
	int ps_nr = 0;
	foreach(QString ps, ps_names)
	{
		if (ps_nr % 100==0)
		{
			appendOutputLine("##Progress " + QString::number(ps_nr) + " / " + QString::number(ps_names.count()));
		}
		++ps_nr;

		QStringList errors;

		//check outcome
		QString ps_id = db.processedSampleId(ps);
		DiagnosticStatusData diag_status_data = db.getDiagnosticStatus(ps_id);
		if (diag_status_data.outcome!="significant findings")
		{
			errors << "causal variant, but outcome not 'significant findings'";
		}

		//check disease status
		QString s_id = db.sampleId(ps);
		SampleData sample_data = db.getSampleData(s_id);
		if (sample_data.disease_status!="Affected")
		{
			errors << "causal variant, but disease status not 'Affected'";
		}

		//check HPO terms set
		if (sample_data.phenotypes.isEmpty())
		{
			errors << "causal variant, but no HPO terms set";
		}

		//check causal variants
		QString gsvar = db.processedSamplePath(ps_id, PathType::GSVAR);
		VariantList variants;
		variants.load(gsvar);

		//check rank of causal variant
		int rc_id = db.reportConfigId(ps_id);
		if (rc_id==-1) THROW(ProgrammingException, "Processes sample '" + ps + "' has no report config!");
		CnvList cnvs;
		BedpeFile svs;
		RepeatLocusList res;
		QSharedPointer<ReportConfiguration> rc_ptr = db.reportConfig(rc_id, variants, cnvs, svs, res);
		foreach(const ReportVariantConfiguration& var_conf, rc_ptr->variantConfig())
		{
			if (var_conf.causal && var_conf.variant_type==VariantType::SNVS_INDELS && var_conf.report_type=="diagnostic variant")
			{
				int var_index = var_conf.variant_index;
				if (var_index<0) THROW(ProgrammingException, "Processes sample '" + ps + "' variant list does not contain causal variant!");

				const Variant& v = variants[var_index];

				//check inheritance
				QString inheritance = var_conf.inheritance;
				if (inheritance.isEmpty() || inheritance=="n/a")
				{
					errors << "Causal variant " + v.toString() + " without inheritance mode in report configuration";
				}

				//check classification
				QString classification = db.getClassification(v).classification;
				if (classification!='3' && classification!='4' && classification!='5')
				{
					errors << "Causal variant " + v.toString() + " not classified as class 3/4/5 (is '" + classification + "')";
				}
			}
		}

		if (!errors.isEmpty())
		{
			QStringList users;
			users << rc_ptr->createdBy();
			users << rc_ptr->lastUpdatedBy();
			users << diag_status_data.user;
			users.removeDuplicates();
			users.removeAll("");
			users.removeAll("Unknown user");
			appendOutputLine(ps + "\t" + errors.join(" // ") + "\t" + users.join(", "));
		}
	}
}

void MaintenanceDialog::importYearOfBirth()
{
	QApplication::setOverrideCursor(Qt::BusyCursor);

	NGSD db;
	GenLabDB genlab;

	int c_imported = 0;
	int c_not_in_genlab = 0;

	//import
	QStringList ps_list = db.getValues("SELECT CONCAT(s.name,'_',LPAD(ps.process_id,2,'0')) FROM processed_sample ps, sample s, project p WHERE ps.sample_id=s.id AND ps.project_id=p.id and p.type='diagnostic' AND ps.id NOT IN (SELECT processed_sample_id FROM merged_processed_samples) ORDER BY ps.id ASC");
	foreach(const QString& ps, ps_list)
	{
		QString yob = genlab.yearOfBirth(ps).trimmed();
		if (yob.isEmpty())
		{
			++c_not_in_genlab;
			continue;
		}

		//if already in NGSD, check if consistent
		QString s_id = db.sampleId(ps);
		QVariant yob_ngsd = db.getValue("SELECT year_of_birth FROM sample WHERE id='" + s_id + "'");
		if (!yob_ngsd.isNull() && yob_ngsd.toString()!=yob)
		{
			appendOutputLine(ps  + " skipped: NGSD contains " + yob_ngsd.toString() + ", but GenLab contains '" + yob);
			continue;
		}

		//update NGSD
		db.getQuery().exec("UPDATE sample SET year_of_birth='" + yob +"' WHERE id='" + s_id + "'");
		++c_imported;
	}

	QApplication::restoreOverrideCursor();

	//output
	appendOutputLine("");
	appendOutputLine("Imported dates: " + QString::number(c_imported));
	appendOutputLine("Skipped because no date available in GenLab: " + QString::number(c_not_in_genlab));
}

void MaintenanceDialog::importTissue()
{
	QApplication::setOverrideCursor(Qt::BusyCursor);

	NGSD db;
	GenLabDB genlab;

	int c_imported = 0;
	int c_not_in_genlab = 0;

	//import
	QStringList ps_list = db.getValues("SELECT CONCAT(s.name,'_',LPAD(ps.process_id,2,'0')) FROM processed_sample ps, sample s, project p WHERE ps.sample_id=s.id AND ps.project_id=p.id and p.type='diagnostic' AND ps.id NOT IN (SELECT processed_sample_id FROM merged_processed_samples) ORDER BY ps.id ASC");
	for(int i=0; i<ps_list.count(); ++i)
	{
		QString ps = ps_list[i];
		QString tissue = genlab.tissue(ps).trimmed();
		if (tissue.isEmpty())
		{
			++c_not_in_genlab;
			continue;
		}

		//if already in NGSD, check if consistent
		QString s_id = db.sampleId(ps);
		QString tissue_ngsd = db.getValue("SELECT tissue FROM sample WHERE id='" + s_id + "'").toString();
		if (tissue_ngsd!="n/a" && tissue_ngsd!=tissue)
		{
			appendOutputLine(ps  + " skipped: NGSD contains " + tissue_ngsd + ", but GenLab contains " + tissue);
			continue;
		}

		//update NGSD
		db.getQuery().exec("UPDATE sample SET tissue='" + tissue +"' WHERE id='" + s_id + "'");
		++c_imported;
	}

	QApplication::restoreOverrideCursor();

	//output
	appendOutputLine("");
	appendOutputLine("Imported tissues: " + QString::number(c_imported));
	appendOutputLine("Skipped because no valid tissue available in GenLab: " + QString::number(c_not_in_genlab));
}

void MaintenanceDialog::importPatientIDs()
{
	QApplication::setOverrideCursor(Qt::BusyCursor);

	NGSD db;
	GenLabDB genlab;

	int c_not_in_genlab = 0;
	int c_imported = 0;

	//import
	SqlQuery query = db.getQuery();
	query.exec("SELECT CONCAT(s.name,'_',LPAD(ps.process_id,2,'0')) as ps, s.patient_identifier, s.id as sample_id FROM processed_sample ps, sample s, project p WHERE ps.sample_id=s.id AND ps.project_id=p.id and p.type='diagnostic' AND ps.id NOT IN (SELECT processed_sample_id FROM merged_processed_samples) ORDER BY ps.id ASC");

	int i = 0;
	while(query.next())
	{
		QString ps = query.value("ps").toString();
		if (i%500==0) appendOutputLine("processed " + QString::number(i) + " of " + QString::number(query.size()) + " processed samples");
		++i;

		QString patient_id = genlab.patientIdentifier(ps).trimmed();
		if (patient_id.isEmpty())
		{
			++c_not_in_genlab;
			continue;
		}

		//if already in NGSD, check if consistent
		QString patient_id_ngsd = query.value("patient_identifier").toString().trimmed();
		if (patient_id_ngsd!="" && patient_id!=patient_id_ngsd)
		{
			appendOutputLine(ps  + " note: NGSD contains " + patient_id_ngsd + ", but GenLab contains " + patient_id + " > updating ID");
		}

		//check if already set
		if (patient_id==patient_id_ngsd) continue;

		//update NGSD
		db.getQuery().exec("UPDATE sample SET patient_identifier='" + patient_id +"' WHERE id='" + query.value("sample_id").toString() + "'");
		++c_imported;
	}

	QApplication::restoreOverrideCursor();

	//output
	appendOutputLine("");
	appendOutputLine("Skipped because no patient ID available in GenLab: " + QString::number(c_not_in_genlab));
	appendOutputLine("Imported patient IDs: " + QString::number(c_imported));
}

void MaintenanceDialog::importOrderAndSamplingDate()
{
	QApplication::setOverrideCursor(Qt::BusyCursor);

	NGSD db;
	GenLabDB genlab;

	int c_already_in_ngsd = 0;
	int c_not_in_genlab = 0;
	int c_imported = 0;

	//import
	SqlQuery query = db.getQuery();
	query.exec("SELECT CONCAT(s.name,'_',LPAD(ps.process_id,2,'0')) as ps, s.id as sample_id, s.order_date, s.sampling_date FROM processed_sample ps, sample s, project p WHERE ps.sample_id=s.id AND ps.project_id=p.id and p.type='diagnostic' AND ps.id NOT IN (SELECT processed_sample_id FROM merged_processed_samples) ORDER BY ps.id ASC");

	int i = 0;
	while(query.next())
	{
		QString ps = query.value("ps").toString();
		QString sample_id = query.value("sample_id").toString();
		QString order_date = query.value("order_date").toDate().toString(Qt::ISODate);
		QString sampling_date = query.value("sampling_date").toDate().toString(Qt::ISODate);

		if (i%500==0) appendOutputLine("processed " + QString::number(i) + " of " + QString::number(query.size()) + " processed samples");
		++i;

		if (!order_date.isEmpty() || !sampling_date.isEmpty())
		{
			++c_already_in_ngsd;
			continue;
		}

		QString patient_id = genlab.patientIdentifier(ps).trimmed();
		if (patient_id.isEmpty())
		{
			++c_not_in_genlab;
			continue;
		}

		bool imported = false;
		QString gl_order_date = genlab.orderEntryDate(ps);
		if (!gl_order_date.isEmpty())
		{
			db.getQuery().exec("UPDATE sample SET order_date='" + gl_order_date +"' WHERE id='" + sample_id + "'");
			imported = true;
		}

		QString gl_sampling_date = genlab.samplingDate(ps);
		if (!gl_sampling_date.isEmpty())
		{
			db.getQuery().exec("UPDATE sample SET sampling_date='" + gl_sampling_date +"' WHERE id='" + sample_id + "'");
			imported = true;
		}

		if (imported) ++c_imported;
	}

	QApplication::restoreOverrideCursor();

	//output
	appendOutputLine("");
	appendOutputLine("Skipped because at least one of the dates is already in NGSD: " + QString::number(c_already_in_ngsd));
	appendOutputLine("Skipped because sample was not found in GenLab: " + QString::number(c_not_in_genlab));
	appendOutputLine("Imported one/two dates for samples: " + QString::number(c_imported));
}

void MaintenanceDialog::importSampleRelations()
{
	QApplication::setOverrideCursor(Qt::BusyCursor);

	NGSD db;
	GenLabDB genlab;

	int c_already_in_ngsd = 0;
	int c_not_in_genlab = 0;
	int c_imported = 0;

	//import
	SqlQuery query = db.getQuery();
	query.exec("SELECT CONCAT(s.name,'_',LPAD(ps.process_id,2,'0')) as ps, s.id as sample_id, s.order_date, s.sampling_date FROM processed_sample ps, sample s, project p WHERE ps.sample_id=s.id AND ps.project_id=p.id and p.type='diagnostic' AND ps.id NOT IN (SELECT processed_sample_id FROM merged_processed_samples) ORDER BY ps.id ASC");

	int i = 0;
	while(query.next())
	{
		QString ps = query.value("ps").toString();
		QString s_id = query.value("sample_id").toString();

		if (i%500==0) appendOutputLine("processed " + QString::number(i) + " of " + QString::number(query.size()) + " processed samples - added: " + QString::number(c_imported) + " - skipped because already present: " + QString::number(c_already_in_ngsd));
		++i;

		QString patient_id = genlab.patientIdentifier(ps).trimmed();
		if (patient_id.isEmpty())
		{
			++c_not_in_genlab;
			continue;
		}

		//relatives patient relations (parents, siblings)
		foreach (const SampleRelation& genlab_relation, genlab.relatives(ps))
		{
			QSet<int> sample_ids_ngsd = db.relatedSamples(s_id.toInt(), genlab_relation.relation);
			int sample2_id = db.sampleId(genlab_relation.sample1).toInt();

			if (!sample_ids_ngsd.contains(sample2_id))
			{

				appendOutputLine(ps + ": adding relation: " + genlab_relation.sample1 + " - " + genlab_relation.relation + " - " + genlab_relation.sample2);

				SqlQuery insert = db.getQuery();
				insert.prepare("INSERT INTO sample_relations (sample1_id, relation, sample2_id) VALUES (:0, :1, :2)");
				insert.bindValue(0, db.sampleId(genlab_relation.sample1));
				insert.bindValue(1, genlab_relation.relation);
				insert.bindValue(2, db.sampleId(genlab_relation.sample2));
				insert.exec();

				++c_imported;
			}
			else
			{
				++c_already_in_ngsd;
			}
		}
	}

	QApplication::restoreOverrideCursor();

	//output
	appendOutputLine("");
	appendOutputLine("Skipped because sample was not found in GenLab: " + QString::number(c_not_in_genlab));
	appendOutputLine("Skipped because relation is already in NGSD: " + QString::number(c_already_in_ngsd));
	appendOutputLine("Imported relations: " + QString::number(c_imported));
}

void MaintenanceDialog::linkSamplesFromSamePatient()
{
	QApplication::setOverrideCursor(Qt::BusyCursor);

	NGSD db;
	int user_id = db.userId("genlab_import");

	//get sample names for each patient identifier
	QHash<QString, QStringList> pat2samples;
	SqlQuery query = db.getQuery();
	query.exec("SELECT name, patient_identifier FROM sample WHERE patient_identifier IS NOT NULL");
	while(query.next())
	{
		QString patient_id = query.value("patient_identifier").toString().trimmed();
		if (patient_id.isEmpty()) continue;

		pat2samples[patient_id] << query.value("name").toString();
	}


	int c_multi_sample = 0;
	int c_relation_missing = 0;
	int c_relation_present = 0;

	for (auto it=pat2samples.begin(); it!=pat2samples.end(); ++it)
	{
		const QStringList& samples = it.value();
		if (samples.count()<2) continue;
		++c_multi_sample;

		for (int i=0; i<samples.count(); ++i)
		{
			for (int j=i+1; j<samples.count(); ++j)
			{
				QString s1 = samples[i];
				int s1_id = db.sampleId(s1).toInt();
				QString s2 = samples[j];
				int s2_id = db.sampleId(s2).toInt();

				QSet<int> s1_related = db.relatedSamples(s1_id);
				if (!s1_related.contains(s2_id))
				{
					++c_relation_missing;

					db.addSampleRelation(SampleRelation{s1.toUtf8(), "same patient", s2.toUtf8()}, user_id);
					appendOutputLine("Added 'same sample' relation for " + s1 + " and " + s2);
				}
				else
				{
					++c_relation_present;
				}
			}
		}

	}

	appendOutputLine("Patients with patient identifier: " + QString::number(pat2samples.count()));
	appendOutputLine("Patients with at least 2 samples: " + QString::number(c_multi_sample));

	appendOutputLine("Relations present: " + QString::number(c_relation_present));
	appendOutputLine("Relations missing - added now: " + QString::number(c_relation_missing));

	QApplication::restoreOverrideCursor();
}

void MaintenanceDialog::deleteVariantsOfBadSamples()
{
	QApplication::setOverrideCursor(Qt::BusyCursor);

	NGSD db;
	clearUnusedReportConfigs(db); //we use report config below, so we have to clear them up first

	QSet<int> ps_with_vars = psWithVariants(db);
	appendOutputLine("Found " + QString::number(ps_with_vars.count()) + " processed samples with germine variants (small variants, CNVs, SVs or REs).");

	int c_deleted = 0;
	int c_failed = 0;
	QStringList ps_skipped_lrgs;
	QStringList ps_skipped_rc_exists;
	QList<int> ps_bad = db.getValuesInt("SELECT ps.id FROM processed_sample ps, sample s WHERE ps.sample_id=s.id AND ps.quality='bad' AND s.tumor=0 AND s.ffpe=0");
	QList<int> ps_lr = db.getValuesInt("SELECT ps.id FROM processed_sample ps, processing_system sys WHERE ps.processing_system_id=sys.id AND sys.type='lrGS'");
	appendOutputLine("Found " + QString::number(ps_bad.count()) + " non-tumor processed samples with 'bad' quality.");
	foreach(int ps_id, ps_bad)
	{
		if (ps_with_vars.contains(ps_id))
		{
			//skip lrGS
			if (ps_lr.contains(ps_id))
			{
				ps_skipped_lrgs << db.processedSampleName(QString::number(ps_id));
				continue;
			}

			//skip if report config exists
			if (db.reportConfigId(QString::number(ps_id))!=-1)
			{
				ps_skipped_rc_exists << db.processedSampleName(QString::number(ps_id));
				continue;
			}

			deleteVariants(db, ps_id, c_deleted, c_failed);
		}
	}
	appendOutputLine("Processed samples with variants (deleted): " + QString::number(c_deleted));
	appendOutputLine("Processed samples with variants (deletion failed): " + QString::number(c_failed));
	appendOutputLine("Skipped " + QString::number(ps_skipped_lrgs.count()) + " bad samples with variants (is lrGS): " + ps_skipped_lrgs.join(", "));
	appendOutputLine("Skipped " + QString::number(ps_skipped_rc_exists.count()) + " bad samples with variants (report configuration exists): " + ps_skipped_rc_exists.join(", "));

	QApplication::restoreOverrideCursor();
}

void MaintenanceDialog::deleteDataOfMergedSamples()
{
	QApplication::setOverrideCursor(Qt::BusyCursor);

	NGSD db;

	QList<int> ps_merged = db.getValuesInt("SELECT processed_sample_id FROM merged_processed_samples");
	appendOutputLine("Found " + QString::number(ps_merged.count()) + " processed samples marked as merged");

	//delete variantssed samples marked as merged into other sample.");
	QSet<int> ps_with_vars = psWithVariants(db);
	appendOutputLine("Found " + QString::number(ps_with_vars.count()) + " processed samples with germine variants (small variants, CNVs, SVs or REs).");
	int c_deleted = 0;
	int c_failed = 0;
	foreach(int ps_id, ps_merged)
	{
		if (ps_with_vars.contains(ps_id))
		{
			deleteVariants(db, ps_id, c_deleted, c_failed);
		}
	}
	appendOutputLine("Deleted variants of merged processed samples: " + QString::number(c_deleted));
	appendOutputLine("Deleted variants of merged processed samples failed: " + QString::number(c_failed));

	//delete QC metrics (except for read count)
	QString qc_id_read_count = db.getValue("SELECT id FROM qc_terms WHERE qcml_id='QC:2000005'").toString();
	c_deleted = 0;
    QSet<int> ps_with_qc = LIST_TO_SET(db.getValuesInt("SELECT processed_sample_id FROM processed_sample_qc"));
	appendOutputLine("Found " + QString::number(ps_with_qc.count()) + " processed samples with QC data.");
	foreach(int ps_id, ps_merged)
	{
		if (ps_with_qc.contains(ps_id))
		{
			int qc_term_count = db.getValue("SELECT count(*) FROM processed_sample_qc WHERE processed_sample_id="+QString::number(ps_id) + " AND qc_terms_id!="+qc_id_read_count).toInt();
			if (qc_term_count>0)
			{
				db.getQuery().exec("DELETE FROM processed_sample_qc WHERE processed_sample_id="+QString::number(ps_id) + " AND qc_terms_id!="+qc_id_read_count);
				++c_deleted;
			}
		}
	}
	appendOutputLine("Deleted QC data of merged processed samples: " + QString::number(c_deleted));

	//delete KASP data
	c_deleted = 0;
    QSet<int> ps_with_kasp = LIST_TO_SET(db.getValuesInt("SELECT processed_sample_id FROM kasp_status"));
	appendOutputLine("Found " + QString::number(ps_with_kasp.count()) + " processed samples with KASP data.");
	foreach(int ps_id, ps_merged)
	{
		if (ps_with_kasp.contains(ps_id))
		{
			db.getQuery().exec("DELETE FROM kasp_status WHERE processed_sample_id="+QString::number(ps_id));
			++c_deleted;
		}
	}
	appendOutputLine("Deleted KASP data of merged processed samples: " + QString::number(c_deleted));

	QApplication::restoreOverrideCursor();
}

void MaintenanceDialog::compareStructureOfTestAndProduction()
{
	NGSD db_p;
	NGSD db_t(true);

	//check for missing tables
	QStringList tables_p = db_p.tables();
	QStringList tables_t = db_t.tables();
    QSet<QString> tables_both = LIST_TO_SET(tables_p).intersect(LIST_TO_SET(tables_t));
	foreach(QString table_p, tables_p)
	{
		if (!tables_both.contains(table_p)) appendOutputLine("Missing table in test database: " + table_p);
	}
	foreach(QString table_t, tables_t)
	{
		if (!tables_both.contains(table_t)) appendOutputLine("Missing table in production database: " + table_t);
	}

	//check for differing columns
	foreach(QString table, tables_both)
	{
		TableInfo info_p = db_p.tableInfo(table, false); //no cache, because the cache is shared between instances
		TableInfo info_t = db_t.tableInfo(table, false); //no cache, because the cache is shared between instances

		//missing/extra columns
		QStringList fields_p = info_p.fieldNames();
		QStringList fields_t = info_t.fieldNames();
        QSet<QString> fields_both = LIST_TO_SET(fields_p).intersect(LIST_TO_SET(fields_t));
		foreach(QString field_p, fields_p)
		{
			if (!fields_both.contains(field_p)) appendOutputLine("Missing field in test database: " + table+"/"+field_p);
		}
		foreach(QString field_t, fields_t)
		{
			if (!fields_both.contains(field_t)) appendOutputLine("Missing field in production database: " + table+"/"+field_t);
		}

		//differing type/keys
		foreach(QString field, fields_both)
		{
			if (info_p.fieldInfo(field).toString()!=info_t.fieldInfo(field).toString())
			{
				appendOutputLine("differing type/key in "+table+"/"+field+" for production/test:");
				appendOutputLine("  "+info_p.fieldInfo(field).toString());
				appendOutputLine("  "+info_t.fieldInfo(field).toString());
			}
		}

		//differing comment
		foreach(QString field, fields_both)
		{
			if (info_p.fieldInfo(field).tooltip!=info_t.fieldInfo(field).tooltip)
			{
				appendOutputLine("differing comment in "+table+"/"+field+" for production/test:");
				appendOutputLine("  "+info_p.fieldInfo(field).tooltip);
				appendOutputLine("  "+info_t.fieldInfo(field).tooltip);
			}
		}
	}
}

QString MaintenanceDialog::compareCount(NGSD& db_p, NGSD& db_t, QString query)
{
	QString output;

	bool ok = false;
	int c_p = db_p.getValue(query).toInt(&ok);
	if (!ok) return "ERROR: count not convert query result to integer";
	int c_t = db_t.getValue(query).toInt(&ok);
	if (!ok) return "ERROR: count not convert query result to integer";

	int diff = c_t-c_p;
	QString prefix = diff>0 ? "+" : "";
	output += QString::number(c_p) + " > "  + QString::number(c_t) + " (diff: " + prefix+ QString::number(diff) + ")";

	return output;
}

void MaintenanceDialog::deleteVariants(NGSD& db, int ps_id, int c_deleted, int c_failed)
{
	QString ps = db.processedSampleName(QString::number(ps_id));
	QString project = db.getValue("SELECT p.name FROM project p, processed_sample ps WHERE ps.project_id=p.id and ps.id='" + QString::number(ps_id) + "'").toString();
	try
	{
		appendOutputLine("Deleting variants of " + ps + " (" + project + ")");

		db.deleteVariants(QString::number(ps_id));
		++c_deleted;
	}
	catch (Exception& e)
	{
		++c_failed;
		appendOutputLine("Deleting variants  of " + ps + " failed: " + e.message());
	}
}

QSet<int> MaintenanceDialog::psWithVariants(NGSD& db)
{
	QSet<int> ps_with_vars;

    ps_with_vars += LIST_TO_SET(db.getValuesInt("SELECT DISTINCT processed_sample_id FROM detected_variant"));
    ps_with_vars += LIST_TO_SET(db.getValuesInt("SELECT processed_sample_id FROM cnv_callset"));
    ps_with_vars += LIST_TO_SET(db.getValuesInt("SELECT processed_sample_id FROM sv_callset"));
    ps_with_vars += LIST_TO_SET(db.getValuesInt("SELECT processed_sample_id FROM re_callset"));

	return ps_with_vars;
}

void MaintenanceDialog::clearUnusedReportConfigs(NGSD& db)
{
	//determine used report configs
	QSet<int> used_rc_ids;
    used_rc_ids += LIST_TO_SET(db.getValuesInt("SELECT DISTINCT report_configuration_id FROM report_configuration_variant"));
    used_rc_ids += LIST_TO_SET(db.getValuesInt("SELECT DISTINCT report_configuration_id FROM report_configuration_cnv"));
    used_rc_ids += LIST_TO_SET(db.getValuesInt("SELECT DISTINCT report_configuration_id FROM report_configuration_sv"));
    used_rc_ids += LIST_TO_SET(db.getValuesInt("SELECT DISTINCT report_configuration_id FROM report_configuration_re"));
    used_rc_ids += LIST_TO_SET(db.getValuesInt("SELECT DISTINCT report_configuration_id FROM report_configuration_other_causal_variant"));

	//delete unused report configs
	SqlQuery query = db.getQuery();
	query.exec("SELECT id FROM report_configuration");
	while(query.next())
	{
		int rc_id = query.value(0).toInt();
		if (!used_rc_ids.contains(rc_id))
		{
			db.getQuery().exec("DELETE FROM report_configuration WHERE id=" + QString::number(rc_id));
		}
	}
}

void MaintenanceDialog::fixGeneNames(NGSD& db, QString table, QString column)
{
	SqlQuery query = db.getQuery();
	query.exec("SELECT DISTINCT " + column + " FROM " + table + " tmp WHERE NOT EXISTS(SELECT * FROM gene WHERE symbol=tmp." + column + ")");
	while(query.next())
	{
		QString gene = query.value(0).toString().trimmed();
		if (gene=="") continue;

		appendOutputLine("Outdated gene name in table '" + table + "': " + gene);
		auto approved_data = db.geneToApprovedWithMessage(gene);
		if (approved_data.second.startsWith("ERROR"))
		{
			appendOutputLine("  FAIL: Cannot correct '" + gene + "' because: " + approved_data.second);
		}
		else
		{
			try
			{
				db.getQuery().exec("UPDATE " + table + " SET " + column + "='" + approved_data.first + "' WHERE " + column + "='" + gene +"'");
			}
			catch (Exception& e)
			{
				appendOutputLine("  FAIL: Error correcting '" + gene + "': " + e.message());
			}
		}
	}
}

void MaintenanceDialog::deleteVariant(NGSD& db, QString var_id, QString reason)
{
	QString message = "Deleting invalid variant "+db.variant(var_id).toString()+" with ID "+var_id;
	if (!reason.isEmpty()) message += ": " + reason;
	appendOutputLine(message);

	//delete detected_variant entries
	QStringList dv_ps_ids = db.getValues("SELECT processed_sample_id FROM detected_variant WHERE variant_id="+var_id);
	if (dv_ps_ids.count()>0)
	{
		db.getQuery().exec("DELETE FROM detected_variant WHERE variant_id="+var_id);
	}

	QStringList dsv_ps_ids = db.getValues("SELECT processed_sample_id_tumor FROM detected_somatic_variant WHERE variant_id="+var_id);
	if (dsv_ps_ids.count()>0)
	{
		db.getQuery().exec("DELETE FROM detected_somatic_variant WHERE variant_id="+var_id);
	}

	QStringList rc_ps_ids = db.getValues("SELECT rc.processed_sample_id FROM report_configuration rc, report_configuration_variant rcv WHERE rcv.report_configuration_id=rc.id AND rcv.variant_id="+var_id);
	if (rc_ps_ids.count()>0)
	{
		appendOutputLine("  cannot delete variant because it is referenced by 'report_configuration_variant' from "+ QString::number(rc_ps_ids.count()) + " samples");
		return;
	}

	try
	{
		SqlQuery query2 = db.getQuery();
		query2.exec("DELETE FROM variant WHERE id="+var_id);
	}
	catch (Exception& e)
	{
		appendOutputLine("  error while deleting variant: " + e.message());
	}

}

void MaintenanceDialog::compareBaseDataOfTestAndProduction()
{
	NGSD db_p;
	NGSD db_t(true);

	//gene
	appendOutputLine("genes: " + compareCount(db_p, db_t, "SELECT count(*) FROM gene"));
	appendOutputLine("genes (protein-coding): " + compareCount(db_p, db_t, "SELECT count(*) FROM gene WHERE type='protein-coding gene'"));
	appendOutputLine("genes (pseudogene): " + compareCount(db_p, db_t, "SELECT count(*) FROM gene WHERE type='pseudogene'"));
	appendOutputLine("genes (non-coding RNA): " + compareCount(db_p, db_t, "SELECT count(*) FROM gene WHERE type='non-coding RNA'"));
	appendOutputLine("genes (other): " + compareCount(db_p, db_t, "SELECT count(*) FROM gene WHERE type='other'"));

	//alias
	appendOutputLine("");
	appendOutputLine("genes alias (synonymous): " + compareCount(db_p, db_t, "SELECT count(*) FROM gene_alias WHERE type='synonym'"));
	appendOutputLine("genes alias (previous): " + compareCount(db_p, db_t, "SELECT count(*) FROM gene_alias WHERE type='previous'"));

	//transcript
	appendOutputLine("");
	appendOutputLine("transcripts: " + compareCount(db_p, db_t, "SELECT count(*) FROM gene_transcript"));
	appendOutputLine("transcripts (Ensembl): " + compareCount(db_p, db_t, "SELECT count(*) FROM gene_transcript WHERE source='Ensembl'"));
	appendOutputLine("transcripts (CCDS): " + compareCount(db_p, db_t, "SELECT count(*) FROM gene_transcript WHERE source='CCDS'"));
	appendOutputLine("transcripts (GenCode basic): " + compareCount(db_p, db_t, "SELECT count(*) FROM gene_transcript WHERE is_gencode_basic='1'"));
	appendOutputLine("transcripts (MANE select): " + compareCount(db_p, db_t, "SELECT count(*) FROM gene_transcript WHERE is_mane_select='1'"));
	appendOutputLine("transcripts (MANE plus clinical): " + compareCount(db_p, db_t, "SELECT count(*) FROM gene_transcript WHERE is_mane_plus_clinical='1'"));

	//exon
	appendOutputLine("");
	appendOutputLine("exon: " + compareCount(db_p, db_t, "SELECT count(*) FROM gene_exon"));

	//pseudogene
	appendOutputLine("");
	appendOutputLine("pseudogene (in NGSD): " + compareCount(db_p, db_t, "SELECT count(*) FROM gene_pseudogene_relation WHERE pseudogene_gene_id IS NOT NULL"));
	appendOutputLine("pseudogene (not in NGSD): " + compareCount(db_p, db_t, "SELECT count(*) FROM gene_pseudogene_relation WHERE gene_name IS NOT NULL"));

	//geneinfo
	appendOutputLine("");
	appendOutputLine("geneinfo (germline): " + compareCount(db_p, db_t, "SELECT count(*) FROM geneinfo_germline"));

	//HPO
	appendOutputLine("");
	appendOutputLine("HPO terms: " + compareCount(db_p, db_t, "SELECT count(*) FROM hpo_term"));
	appendOutputLine("HPO terms (obsolete): " + compareCount(db_p, db_t, "SELECT count(*) FROM hpo_obsolete"));
	appendOutputLine("HPO term-gene releations: " + compareCount(db_p, db_t, "SELECT count(*) FROM hpo_genes"));

	//OMIM
	appendOutputLine("");
	appendOutputLine("OMIM genes: " + compareCount(db_p, db_t, "SELECT count(*) FROM omim_gene"));
	appendOutputLine("OMIM phenotype: " + compareCount(db_p, db_t, "SELECT count(*) FROM omim_phenotype"));

	//disease
	appendOutputLine("");
	appendOutputLine("disease gene: " + compareCount(db_p, db_t, "SELECT count(*) FROM disease_gene"));
	appendOutputLine("disease term (ORPHA): " + compareCount(db_p, db_t, "SELECT count(*) FROM disease_term"));
}

void MaintenanceDialog::findTumorSamplesWithGermlineVariants()
{
	NGSD db;

	SqlQuery query = db.getQuery();
	query.exec("SELECT CONCAT(s.name,'_',LPAD(ps.process_id,2,'0')), ps.id FROM sample s, processed_sample ps WHERE ps.sample_id=s.id AND s.tumor='1' AND EXISTS(SELECT * FROM detected_variant WHERE processed_sample_id=ps.id)");
	while(query.next())
	{
		appendOutputLine("tumor sample with germline variants: " + query.value(0).toString());
	}
}

void MaintenanceDialog::deleteInvalidVariants()
{
	FastaFileIndex reference(Settings::string("reference_genome", true));

	NGSD db;
	SqlQuery query = db.getQuery();
	foreach(QString chr, db.getEnum("variant", "chr"))
	{
		appendOutputLine("Retrieving variants for " + chr);
		query.exec("SELECT id, start, end, ref, obs FROM variant WHERE chr='"+chr+"' ORDER BY start ASC");
		appendOutputLine("Processing " + chr + " (" + QString::number(query.size()) + " variants)");
		while(query.next())
		{
			//ref/alt contains N
			Sequence ref = query.value(3).toByteArray();
			if (ref.contains("N"))
			{
				deleteVariant(db, query.value(0).toString(), "Reference sequence contains 'N'");
				continue;
			}

			Sequence alt = query.value(4).toByteArray();
			if (alt.contains("N"))
			{
				deleteVariant(db, query.value(0).toString(), "Alternative sequence contains 'N'");
				continue;
			}

			//too large
			if (ref.size()>MAX_VARIANT_SIZE)
			{
				deleteVariant(db, query.value(0).toString(), "Reference sequence larger than "+QString::number(MAX_VARIANT_SIZE));
				continue;
			}
			if (alt.size()>MAX_VARIANT_SIZE)
			{
				deleteVariant(db, query.value(0).toString(), "Alternative sequence larger than "+QString::number(MAX_VARIANT_SIZE));
				continue;
			}
			int start = query.value(1).toInt();
			int end = query.value(2).toInt();
			if (end-start+1>MAX_VARIANT_SIZE)
			{
				deleteVariant(db, query.value(0).toString(), "Start/end range larger than "+QString::number(MAX_VARIANT_SIZE));
				continue;
			}

			//check reference sequence matches genome
			if (ref!="-")
			{
				Sequence ref_expected = reference.seq(chr, start, end-start+1);
				if (ref!=ref_expected)
				{
					deleteVariant(db, query.value(0).toString(), "Expected reference sequence '"+ref_expected + "'");
				}
			}
		}
	}
}

void MaintenanceDialog::updateDescription(QString text)
{
	ui_.description->clear();

	QString action = text.trimmed().toLower().replace(" ", "");
	if (action=="deleteunusedsamples")
	{
		ui_.description->setText("Deletes samples without processed sample.");
	}
	else if (action=="deleteunusedvariants")
	{
		ui_.description->setText("Deletes variants that are referenced form any other table.");
	}
	else if (action=="importstudysamples")
	{
		ui_.description->setText("Batch import of studies for all samples from GenLab.");
	}
	else if (action=="replaceobsoltehpoterms")
	{
		ui_.description->setText("Replaces obsolete HPO terms of samples with their replacement term as specified in the HPO OBO file.");
	}
	else if (action=="findinconsistenciesforcausaldiagnosticvariants")
	{
		ui_.description->setText("Find causal variants with missing/inconsistent meta data:\n - outcome is not 'significant findings'\n- disease status is not 'Affected'\n- no HPO terms set\n- no inheritance mode in report configuration set\n- variant not classified as class 3/4/5");
	}
	else if (action=="importyearofbirth")
	{
		ui_.description->setText("Batch import of year-of-birth for all samples from GenLab.");
	}
	else if (action=="importtissue")
	{
		ui_.description->setText("Batch import of tissue for all samples from GenLab.");
	}
	else if (action=="importpatientids")
	{
		ui_.description->setText("Batch import of patient identifiers for all samples from GenLab.");
	}
	else if (action=="linksamplesfromsamepatient")
	{
		ui_.description->setText("Add same-sample relation to samples that have the same patient identifier.");
	}
	else if (action=="deletevariantsofbadsamples")
	{
		ui_.description->setText("Deletes germline variants of non-tumor samples that have 'bad' processed sample quality.");
	}
	else if (action=="deletedataofmergedsamples")
	{
		ui_.description->setText("Deletes germline variants, QC data and KASP data of samples that are marked as 'merged'.");
	}
	else if (action=="comparebasedataoftestandproduction")
	{
		ui_.description->setText("Compares base data counts (genes, transcripts, pseudogenes, HPO, OMIM, Orpha) of NGSD production and test instance.");
	}
	else if (action=="comparestructureoftestandproduction")
	{
		ui_.description->setText("Compares table structure of NGSD production and test instance.");
	}
	else if (action=="deleteinvalidvariants")
	{
		ui_.description->setText("Deletes variants with invalid base(s):\n-ref containing N\n-alt containing N\n-ref not matching genome sequence\n-size larger than maximum variant size (" + QString::number(MAX_VARIANT_SIZE) +")");
	}
	else if (action=="replaceobsoltegenesymbols")
	{
		ui_.description->setText("Replaces outdated gene symbols with current approved symbol if possible.");
	}
}
