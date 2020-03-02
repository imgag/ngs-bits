#include "NGSD.h"
#include "Exceptions.h"
#include "Helper.h"
#include "Log.h"
#include "Settings.h"
#include "ChromosomalIndex.h"
#include "NGSHelper.h"
#include "FilterCascade.h"
#include <QFileInfo>
#include <QPair>
#include <QSqlDriver>
#include <QSqlIndex>
#include <QSqlField>
#include <QSqlError>
#include <QJsonDocument>
#include <QJsonObject>
#include "cmath"

QMap<QString, TableInfo> NGSD::infos_;

NGSD::NGSD(bool test_db)
	: test_db_(test_db)
{
	db_.reset(new QSqlDatabase(QSqlDatabase::addDatabase("QMYSQL", "NGSD_" + Helper::randomString(20))));

	//connect to DB
	QString prefix = "ngsd";
	if (test_db_) prefix += "_test";
	db_->setHostName(Settings::string(prefix + "_host"));
	db_->setPort(Settings::integer(prefix + "_port"));
	db_->setDatabaseName(Settings::string(prefix + "_name"));
	db_->setUserName(Settings::string(prefix + "_user"));
	db_->setPassword(Settings::string(prefix + "_pass"));
	if (!db_->open())
	{
		THROW(DatabaseException, "Could not connect to NGSD database '" + prefix + "': " + db_->lastError().text());
	}
}

int NGSD::userId(QString user_name, bool only_active)
{
	//check user exists
	bool ok = true;
	int user_id = getValue("SELECT id FROM user WHERE user_id=:0", true, user_name).toInt(&ok);
	if (!ok)
	{
		user_id = getValue("SELECT id FROM user WHERE name=:0", true, user_name).toInt(&ok);
	}
	if (!ok)
	{
		THROW(DatabaseException, "Could not determine NGSD user ID for user name '" + user_name + "!");
	}

	//check user is active
	if (only_active && !getValue("SELECT active FROM user WHERE id=" + QString::number(user_id), false).toBool())
	{
		THROW(DatabaseException, "User with user name '" + user_name + " is no longer active!");
	}

	return user_id;
}

QString NGSD::userName(int user_id)
{
	if (user_id==-1) user_id = userId();

	return getValue("SELECT name FROM user WHERE id=:0", false, QString::number(user_id)).toString();
}

QString NGSD::userEmail(int user_id)
{
	if (user_id==-1) user_id = userId();

	return getValue("SELECT email FROM user WHERE id=:0", false,  QString::number(user_id)).toString();
}

const QString& NGSD::passordReplacement()
{
	static QString output = "********";

	return output;
}

DBTable NGSD::processedSampleSearch(const ProcessedSampleSearchParameters& p)
{
	//init
	QStringList fields;
	fields	<< "ps.id"
			<< "CONCAT(s.name,'_',LPAD(ps.process_id,2,'0')) as name"
			<< "s.name_external as name_external"
			<< "s.gender as gender"
			<< "s.tumor as is_tumor"
			<< "s.ffpe as is_ffpe"
			<< "ps.quality as quality"
			<< "sys.name_manufacturer as system_name"
			<< "sys.name_short as system_name_short"
			<< "sys.type as system_type"
			<< "p.name as project_name"
			<< "p.type as project_type"
			<< "r.name as run_name"
			<< "r.fcid as run_flowcell_id"
			<< "r.flowcell_type as run_flowcell_type"
			<< "r.recipe as run_recipe"
			<< "r.quality as run_quality"
			<< "s.disease_group as disease_group"
			<< "s.disease_status as disease_status";

	QStringList tables;
	tables	<< "sample s"
			<< "processing_system sys"
			<< "project p"
			<< "processed_sample ps LEFT JOIN sequencing_run r ON r.id=ps.sequencing_run_id LEFT JOIN diag_status ds ON ds.processed_sample_id=ps.id"; //sequencing_run and diag_status are optional

	QStringList conditions;
	conditions	<< "ps.sample_id=s.id"
				<< "ps.processing_system_id=sys.id"
				<< "ps.project_id=p.id";

	//add filters (sample)
	if (p.s_name.trimmed()!="")
	{
		conditions << "s.name LIKE '%" + escapeForSql(p.s_name) + "%'";
	}
	if (p.s_species.trimmed()!="")
	{
		tables	<< "species sp";
		conditions	<< "sp.id=s.species_id"
					<< "sp.name='" + escapeForSql(p.s_species) + "'";
	}
	if (p.s_disease_group.trimmed()!="")
	{
		conditions << "s.disease_group='" + escapeForSql(p.s_disease_group) + "'";
	}
	if (p.s_disease_status.trimmed()!="")
	{
		conditions << "s.disease_status='" + escapeForSql(p.s_disease_status) + "'";
	}
	if (!p.include_bad_quality_samples)
	{
		conditions << "ps.quality!='bad'";
	}
	if (!p.include_tumor_samples)
	{
		conditions << "s.tumor='0'";
	}
	if (!p.include_ffpe_samples)
	{
		conditions << "s.ffpe='0'";
	}
	if (!p.include_merged_samples)
	{
		conditions << "ps.id NOT IN (SELECT processed_sample_id FROM merged_processed_samples)";
	}

	//add filters (project)
	if (p.p_name.trimmed()!="")
	{
		conditions << "p.name LIKE '%" + escapeForSql(p.p_name) + "%'";
	}
	if (p.p_type.trimmed()!="")
	{
		conditions << "p.type ='" + escapeForSql(p.p_type) + "'";
	}

	//add filters (system)
	if (p.sys_name.trimmed()!="")
	{
		conditions << "(sys.name_manufacturer LIKE '" + escapeForSql(p.sys_name) + "' OR sys.name_short LIKE '" + escapeForSql(p.sys_name) + "')";
	}
	if (p.sys_type.trimmed()!="")
	{
		conditions << "sys.type ='" + escapeForSql(p.sys_type) + "'";
	}

	//add filters (run)
	if (p.r_name.trimmed()!="")
	{
		conditions << "r.name LIKE '%" + escapeForSql(p.r_name) + "%'";
	}
	if (!p.include_bad_quality_runs)
	{
		conditions << "r.quality!='bad'";
	}
	if (p.run_finished)
	{
		conditions << "r.status='analysis_finished'";
	}
	if (p.r_device_name.trimmed()!="")
	{
		tables << "device d";
		conditions << "d.id=r.device_id"
				   << "d.name LIKE '%" + escapeForSql(p.r_device_name) + "%'";
	}

	//add outcome
	if (p.add_outcome)
	{
		fields	<< "ds.outcome as outcome"
				<< "ds.comment as outcome_comment";
	}
	DBTable output = createTable("processed_sample", "SELECT " + fields.join(", ") + " FROM " + tables.join(", ") +" WHERE " + conditions.join(" AND ") + " ORDER BY s.name ASC, ps.process_id ASC");

	//add path
	if(p.add_path)
	{
		QString pfolder = Settings::string("projects_folder");
		int i_psname = output.columnIndex("name");
		int i_ptype = output.columnIndex("project_type");
		int i_pname = output.columnIndex("project_name");

		QStringList new_col;
		for (int r=0; r<output.rowCount(); ++r)
		{
			const DBRow& row = output.row(r);
			new_col << pfolder + "/" + row.value(i_ptype) + "/" + row.value(i_pname) + "/Sample_" + row.value(i_psname) + "/";
		}
		output.addColumn(new_col, "path");
	}

	if (p.add_disease_details)
	{
		//headers
		QStringList types = getEnum("sample_disease_info", "type");
		types.sort();
		QVector<QStringList> cols(types.count());

		for (int r=0; r<output.rowCount(); ++r)
		{
			SqlQuery disease_query = getQuery();
			disease_query.exec("SELECT sdi.type, sdi.disease_info FROM sample_disease_info sdi, processed_sample ps WHERE ps.sample_id=sdi.sample_id AND ps.id='" + output.row(r).id() + "' ORDER BY sdi.disease_info ASC");
			for(int i=0; i<types.count(); ++i)
			{
				const QString& type = types[i];

				QStringList tmp;
				disease_query.seek(-1);
				while(disease_query.next())
				{
					if (disease_query.value(0).toString()!=type) continue;

					QString entry = disease_query.value(1).toString();
					if (type=="HPO term id")
					{
						tmp << entry + " - " + getValue("SELECT name FROM hpo_term WHERE hpo_id=:0", true, entry).toString();
					}
					else
					{
						tmp << entry;
					}
				}

				cols[i] << tmp.join("; ");
			}
		}

		for(int i=0; i<types.count(); ++i)
		{
			output.addColumn(cols[i], "disease_details_" + types[i].replace(" ", "_"));
		}
	}

	if (p.add_qc)
	{
		//headers
		QStringList qc_names = getValues("SELECT name FROM qc_terms WHERE obsolete=0 ORDER BY qcml_id");
		QVector<QStringList> cols(qc_names.count());

		for (int r=0; r<output.rowCount(); ++r)
		{
			//get QC values
			SqlQuery qc_res = getQuery();
			qc_res.exec("SELECT n.name, nm.value FROM qc_terms n, processed_sample_qc nm WHERE nm.qc_terms_id=n.id AND nm.processed_sample_id='" + output.row(r).id() + "' AND n.obsolete=0");
			QHash<QString, QString> qc_hash;
			while(qc_res.next())
			{
				qc_hash.insert(qc_res.value(0).toString(), qc_res.value(1).toString());
			}
			for(int i=0; i<qc_names.count(); ++i)
			{
				cols[i] << qc_hash.value(qc_names[i], "");
			}
		}
		for(int i=0; i<qc_names.count(); ++i)
		{
			output.addColumn(cols[i], "qc_" + QString(qc_names[i]).replace(' ', '_'));
		}
	}

	if (p.add_report_config)
	{
		QStringList report_conf_col;
		for (int r=0; r<output.rowCount(); ++r)
		{
			QString text;
			SqlQuery rc_res = getQuery();
			rc_res.exec("SELECT rc.id, EXISTS(SELECT rcv.id FROM report_configuration_variant rcv WHERE rcv.report_configuration_id=rc.id) as vars, EXISTS(SELECT rcc.id FROM report_configuration_cnv rcc WHERE rcc.report_configuration_id=rc.id) as cnvs FROM report_configuration rc WHERE rc.processed_sample_id="+output.row(r).id());
			if (rc_res.next())
			{
				text = "exists";
				if (rc_res.value("vars").toInt()>0) text += ",has_small_variants";
				if (rc_res.value("cnvs").toInt()>0) text += ",has_cnvs";
			}
			report_conf_col << text;
		}
		output.addColumn(report_conf_col, "report_config");
	}

	return output;
}

SampleData NGSD::getSampleData(const QString& sample_id)
{
	//execute query
	SqlQuery query = getQuery();
	query.exec("SELECT s.name, s.name_external, s.gender, s.quality, s.comment, s.disease_group, s.disease_status, s.tumor, s.ffpe, s.sample_type, s.sender_id, s.species_id, s.received, s.receiver_id FROM sample s WHERE id=" + sample_id);
	if (query.size()==0)
	{
		THROW(ProgrammingException, "Invalid 'id' for table 'sample' given: '" + sample_id + "'");
	}
	query.next();

	//create output
	SampleData output;
	output.name = query.value(0).toString().trimmed();
	output.name_external = query.value(1).toString().trimmed();
	output.gender = query.value(2).toString();
	output.quality = query.value(3).toString();
	output.comments = query.value(4).toString().trimmed();
	output.disease_group = query.value(5).toString().trimmed();
	output.disease_status = query.value(6).toString().trimmed();
	QStringList hpo_ids = getValues("SELECT disease_info FROM sample_disease_info WHERE type='HPO term id' AND sample_id=" + sample_id);
	foreach(QString hpo_id, hpo_ids)
	{
		Phenotype pheno = phenotypeByAccession(hpo_id.toLatin1(), false);
		if (!pheno.name().isEmpty())
		{
			output.phenotypes << pheno;
		}
	}
	output.is_tumor = query.value(7).toString()=="1";
	output.is_ffpe = query.value(8).toString()=="1";
	output.type = query.value(9).toString();
	output.sender = getValue("SELECT name FROM sender WHERE id=:0", false, query.value(10).toString()).toString();
	output.species = getValue("SELECT name FROM species WHERE id=:0", false, query.value(11).toString()).toString();
	QVariant received_date = query.value(12);
	if (!received_date.isNull())
	{
		output.received = received_date.toDate().toString("dd.MM.yyyy");
	}
	QVariant receiver_id = query.value(13);
	if (!receiver_id.isNull())
	{
		output.received_by = userName(receiver_id.toInt());
	}

	//sample groups
	SqlQuery group_query = getQuery();
	group_query.exec("SELECT sg.name, sg.comment FROM sample_group sg, nm_sample_sample_group nm WHERE sg.id=nm.sample_group_id AND nm.sample_id=" + sample_id);
	while(group_query.next())
	{
		output.sample_groups << SampleGroup{ group_query.value(0).toString(), group_query.value(0).toString() };
	}


	return output;
}

ProcessedSampleData NGSD::getProcessedSampleData(const QString& processed_sample_id)
{
	//execute query
	SqlQuery query = getQuery();
	query.exec("SELECT CONCAT(s.name,'_',LPAD(ps.process_id,2,'0')) as ps_name, sys.name_manufacturer as sys_name, sys.type as sys_type, ps.quality, ps.comment, p.name as p_name, r.name as r_name, ps.normal_id, s.gender, ps.operator_id, ps.processing_input, ps.molarity FROM sample s, project p, processing_system sys, processed_sample ps LEFT JOIN sequencing_run r ON ps.sequencing_run_id=r.id WHERE ps.sample_id=s.id AND ps.project_id=p.id AND ps.processing_system_id=sys.id AND ps.id=" + processed_sample_id);
	if (query.size()==0)
	{
		THROW(ProgrammingException, "Invalid 'id' for table 'processed_sample' given: '" + processed_sample_id + "'");
	}
	query.next();

	//create output
	ProcessedSampleData output;
	output.name = query.value("ps_name").toString().trimmed();
	output.processing_system = query.value("sys_name").toString().trimmed();
	output.processing_system_type = query.value("sys_type").toString().trimmed();
	output.quality = query.value("quality").toString().trimmed();
	output.comments = query.value("comment").toString().trimmed();
	output.project_name = query.value("p_name").toString().trimmed();
	output.run_name = query.value("r_name").toString().trimmed();
	QVariant normal_id = query.value("normal_id");
	if (!normal_id.isNull())
	{
		output.normal_sample_name = processedSampleName(normal_id.toString());
	}
	output.gender = query.value("gender").toString().trimmed();
	QVariant operator_id = query.value("operator_id");
	if (!operator_id.isNull())
	{
		output.lab_operator = userName(operator_id.toInt());
	}
	output.processing_input = query.value("processing_input").toString().trimmed();
	output.molarity = query.value("molarity").toString().trimmed();

	return output;

}

QList<SampleDiseaseInfo> NGSD::getSampleDiseaseInfo(const QString& sample_id, QString only_type)
{
	//set up type filter
	QString type_constraint;
	if (!only_type.isEmpty())
	{
		QStringList valid_types = getEnum("sample_disease_info", "type");
		if (!valid_types.contains(only_type))
		{
			THROW(ProgrammingException, "Type '" + only_type + "' is not valid for table 'sample_disease_info'!");
		}
		type_constraint = " AND sdi.type='" + only_type + "'";
	}

	//execute query
	SqlQuery query = getQuery();
	query.exec("SELECT sdi.disease_info, sdi.type, u.user_id, sdi.date FROM sample_disease_info sdi, user u WHERE sdi.user_id=u.id AND sdi.sample_id=" + sample_id + " " + type_constraint + " ORDER BY sdi.type ASC, sdi.disease_info ASC");

	//create output
	QList<SampleDiseaseInfo> output;
	while(query.next())
	{
		SampleDiseaseInfo tmp;
		tmp.disease_info = query.value(0).toByteArray().trimmed();
		tmp.type = query.value(1).toByteArray().trimmed();
		tmp.user = query.value(2).toByteArray().trimmed();
		tmp.date = query.value(3).toDateTime();
		output << tmp;
	}
	return output;
}

void NGSD::setSampleDiseaseInfo(const QString& sample_id, const QList<SampleDiseaseInfo>& disease_info)
{
	//remove old entries
	SqlQuery query = getQuery();
	query.exec("DELETE FROM sample_disease_info WHERE sample_id=" + sample_id);

	//insert new entries
	SqlQuery query_insert = getQuery();
	query_insert.prepare("INSERT INTO sample_disease_info (`sample_id`, `disease_info`, `type`, `user_id`, `date`) VALUES (" + sample_id + ", :0, :1, :2, :3)");
	foreach(const SampleDiseaseInfo& entry, disease_info)
	{
		query_insert.bindValue(0, entry.disease_info);
		query_insert.bindValue(1, entry.type);
		query_insert.bindValue(2, userId(entry.user));
		query_insert.bindValue(3, entry.date.toString(Qt::ISODate));
		query_insert.exec();
	}
}

QString NGSD::normalSample(const QString& processed_sample_id)
{
	QVariant value = getValue("SELECT normal_id FROM processed_sample WHERE id=" + processed_sample_id, true);
	if (value.isNull()) return "";

	return processedSampleName(value.toString());
}

void NGSD::setSampleDiseaseData(const QString& sample_id, const QString& disease_group, const QString& disease_status)
{
	getQuery().exec("UPDATE sample SET disease_group='" + disease_group + "', disease_status='" + disease_status + "' WHERE id='" + sample_id + "'");
}

int NGSD::processingSystemId(QString name, bool throw_if_fails)
{
	SqlQuery query = getQuery();

	//try short name
	query.prepare("SELECT id FROM processing_system WHERE name_short=:0");
	query.bindValue(0, name);
	query.exec();
	if (query.size()==1)
	{
		query.next();
		return query.value(0).toInt();
	}

	//try long name
	query.prepare("SELECT id FROM processing_system WHERE name_manufacturer=:0");
	query.bindValue(0, name);
	query.exec();
	if (query.size()==1)
	{
		query.next();
		return query.value(0).toInt();
	}

	if(throw_if_fails)
	{
		THROW(DatabaseException, "No processing system with name '" + name + "' not found in NGSD!");
	}

	return -1;
}

int NGSD::processingSystemIdFromProcessedSample(QString ps_name)
{
	QString ps_id = processedSampleId(ps_name, true);
	return getValue("SELECT processing_system_id FROM processed_sample WHERE id="+ps_id).toInt();
}

ProcessingSystemData NGSD::getProcessingSystemData(int sys_id, bool windows_path)
{
	ProcessingSystemData output;

	SqlQuery query = getQuery();
	query.exec("SELECT sys.name_manufacturer, sys.name_short, sys.type, sys.target_file, sys.adapter1_p5, sys.adapter2_p7, sys.shotgun, sys.umi_type, g.build FROM processing_system sys, genome g WHERE sys.genome_id=g.id AND sys.id=" + QString::number(sys_id));
	query.next();

	output.name = query.value(0).toString();
	output.name_short = query.value(1).toString();
	output.type = query.value(2).toString();
	output.target_file = query.value(3).toString();
	if (windows_path)
	{
		QString p_linux = getTargetFilePath(false, false);
		QString p_win = getTargetFilePath(false, true);
		output.target_file.replace(p_linux, p_win);
	}
	output.adapter1_p5 = query.value(4).toString();
	output.adapter2_p7 = query.value(5).toString();
	output.shotgun = query.value(6).toString()=="1";
	output.umi_type = query.value(7).toString();
	output.genome = query.value(8).toString();

	return output;
}

QString NGSD::processedSampleName(const QString& ps_id, bool throw_if_fails)
{
	SqlQuery query = getQuery();
	query.prepare("SELECT CONCAT(s.name,'_',LPAD(ps.process_id,2,'0')) FROM processed_sample ps, sample s WHERE ps.sample_id=s.id AND ps.id=:0");
	query.bindValue(0, ps_id);
	query.exec();
	if (query.size()==0)
	{
		if(throw_if_fails)
		{
			THROW(DatabaseException, "Processed sample with ID '" + ps_id + "' not found in NGSD!");
		}
		else
		{
			return "";
		}
	}
	query.next();
	return query.value(0).toString();
}

QString NGSD::sampleId(const QString& filename, bool throw_if_fails)
{
	QStringList parts = QFileInfo(filename).baseName().append('_').split('_');

	//get sample ID
	SqlQuery query = getQuery(); //use binding (user input)
	query.prepare("SELECT id FROM sample WHERE name=:0");
	query.bindValue(0, parts[0]);
	query.exec();
	if (query.size()==0)
	{
		if(throw_if_fails)
		{
			THROW(DatabaseException, "Sample name '" + parts[0] + "' not found in NGSD!");
		}
		else
		{
			return "";
		}
	}
	query.next();
	return query.value(0).toString();
}

QString NGSD::processedSampleId(const QString& filename, bool throw_if_fails)
{
	QStringList parts = QFileInfo(filename.trimmed()).baseName().append('_').split('_');
	QString sample = parts[0];
	QString ps_num = parts[1];
	if (ps_num.size()>2) ps_num = ps_num.left(2);

	//get sample ID
	SqlQuery query = getQuery(); //use binding (user input)
	query.prepare("SELECT ps.id FROM processed_sample ps, sample s WHERE s.name=:0 AND ps.sample_id=s.id AND ps.process_id=:1");
	query.bindValue(0, sample);
	query.bindValue(1, QString::number(ps_num.toInt()));
	query.exec();
	if (query.size()==0)
	{
		if(throw_if_fails)
		{
			THROW(DatabaseException, "Processed sample name '" + sample + "_" + ps_num + "' not found in NGSD!");
		}
		else
		{
			return "";
		}
	}
	query.next();
	return query.value(0).toString();
}

QString NGSD::processedSamplePath(const QString& processed_sample_id, PathType type)
{
	SqlQuery query = getQuery();
	query.prepare("SELECT CONCAT(s.name,'_',LPAD(ps.process_id,2,'0')), p.type, p.name FROM processed_sample ps, sample s, project p, processing_system sys WHERE ps.processing_system_id=sys.id AND ps.sample_id=s.id AND ps.project_id=p.id AND ps.id=:0");
	query.bindValue(0, processed_sample_id);
	query.exec();
	if (query.size()==0) THROW(DatabaseException, "Processed sample with id '" + processed_sample_id + "' not found in NGSD!");
	query.next();

	//create sample folder
	QString output = Settings::string("projects_folder") + "/";
	QString ps_name = query.value(0).toString();
	QString p_type = query.value(1).toString();
	output += p_type;
	QString p_name = query.value(2).toString();
	output += "/" + p_name + "/";
	if (type!=PROJECT_FOLDER)
	{
		output += "Sample_" + ps_name + "/";
	}

	//append file name if requested
	if (type==BAM) output += ps_name + ".bam";
	else if (type==GSVAR) output += ps_name + ".GSvar";
	else if (type==VCF) output += ps_name + "_var_annotated.vcf.gz";
	else if (type!=SAMPLE_FOLDER && type!=PROJECT_FOLDER) THROW(ProgrammingException, "Unknown PathType '" + QString::number(type) + "'!");

	//convert to canonical path
	output = QFileInfo(output).absoluteFilePath();

	return output;
}

QString NGSD::addVariant(const Variant& variant, const VariantList& variant_list)
{
	SqlQuery query = getQuery(); //use binding (user input)
	query.prepare("INSERT INTO variant (chr, start, end, ref, obs, 1000g, gnomad, gene, variant_type, coding) VALUES (:0,:1,:2,:3,:4,:5,:6,:7,:8,:9)");
	query.bindValue(0, variant.chr().strNormalized(true));
	query.bindValue(1, variant.start());
	query.bindValue(2, variant.end());
	query.bindValue(3, variant.ref());
	query.bindValue(4, variant.obs());
	int idx = variant_list.annotationIndexByName("1000g");
	QByteArray tg = variant.annotations()[idx].trimmed();
	if (tg.isEmpty() || tg=="n/a")
	{
		query.bindValue(5, QVariant());
	}
	else
	{
		query.bindValue(5, tg);
	}
	idx = variant_list.annotationIndexByName("gnomAD");
	QByteArray gnomad = variant.annotations()[idx].trimmed();
	if (gnomad.isEmpty() || gnomad=="n/a")
	{
		query.bindValue(6, QVariant());
	}
	else
	{
		query.bindValue(6, gnomad);
	}
	idx = variant_list.annotationIndexByName("gene");
	query.bindValue(7, variant.annotations()[idx]);
	idx = variant_list.annotationIndexByName("variant_type");
	query.bindValue(8, variant.annotations()[idx]);
	idx = variant_list.annotationIndexByName("coding_and_splicing");
	query.bindValue(9, variant.annotations()[idx]);
	query.exec();

	return query.lastInsertId().toString();
}

QList<int> NGSD::addVariants(const VariantList& variant_list, double max_af, int& c_add, int& c_update)
{
	QList<int> output;

	//prepare queried
	SqlQuery q_id = getQuery();
	q_id.prepare("SELECT id, 1000g, gnomad, gene, variant_type, coding FROM variant WHERE chr=:0 AND start=:1 AND end=:2 AND ref=:3 AND obs=:4");

	SqlQuery q_update = getQuery(); //use binding (user input)
	q_update.prepare("UPDATE variant SET 1000g=:1, gnomad=:2, gene=:3, variant_type=:4, coding=:5 WHERE id=:6");

	SqlQuery q_insert = getQuery(); //use binding (user input)
	q_insert.prepare("INSERT INTO variant (chr, start, end, ref, obs, 1000g, gnomad, gene, variant_type, coding) VALUES (:0,:1,:2,:3,:4,:5,:6,:7,:8,:9)");

	//get annotated column indices
	int i_tg = variant_list.annotationIndexByName("1000g");
	int i_gnomad = variant_list.annotationIndexByName("gnomAD");
	int i_gene = variant_list.annotationIndexByName("gene");
	int i_type = variant_list.annotationIndexByName("variant_type");
	int i_co_sp = variant_list.annotationIndexByName("coding_and_splicing");

	c_add = 0;
	c_update = 0;
	for (int i=0; i<variant_list.count(); ++i)
	{
		const Variant& variant = variant_list[i];

		//skip variants with too high AF
		QByteArray tg = variant.annotations()[i_tg].trimmed();
		if (tg=="n/a") tg.clear();
		if (!tg.isEmpty() && tg.toDouble()>max_af)
		{
			output << -1;
			continue;
		}
		QByteArray gnomad = variant.annotations()[i_gnomad].trimmed();
		if (gnomad=="n/a") gnomad.clear();
		if (!gnomad.isEmpty() && gnomad.toDouble()>max_af)
		{
			output << -1;
			continue;
		}

		//get variant ID
		q_id.bindValue(0, variant.chr().strNormalized(true));
		q_id.bindValue(1, variant.start());
		q_id.bindValue(2, variant.end());
		q_id.bindValue(3, variant.ref());
		q_id.bindValue(4, variant.obs());
		q_id.exec();
		if (q_id.next()) //update (common case)
		{
			int id = q_id.value(0).toInt();

			//check if variant meta data needs to be updated
			if (q_id.value(1).toByteArray().toDouble()!=tg.toDouble() //numeric comparison (NULL > "" > 0.0)
				|| q_id.value(2).toByteArray().toDouble()!=gnomad.toDouble() //numeric comparison (NULL > "" > 0.0)
				|| q_id.value(3).toByteArray()!=variant.annotations()[i_gene]
				|| q_id.value(4).toByteArray()!=variant.annotations()[i_type]
				|| q_id.value(5).toByteArray()!=variant.annotations()[i_co_sp])
			{
				q_update.bindValue(0, tg.isEmpty() ? QVariant() : tg);
				q_update.bindValue(1, gnomad.isEmpty() ? QVariant() : gnomad);
				q_update.bindValue(2, variant.annotations()[i_gene]);
				q_update.bindValue(3, variant.annotations()[i_type]);
				q_update.bindValue(4, variant.annotations()[i_co_sp]);
				q_update.bindValue(5, id);
				q_update.exec();
				++c_update;
			}

			output << id;
		}
		else //insert (rare case)
		{
			q_insert.bindValue(0, variant.chr().strNormalized(true));
			q_insert.bindValue(1, variant.start());
			q_insert.bindValue(2, variant.end());
			q_insert.bindValue(3, variant.ref());
			q_insert.bindValue(4, variant.obs());
			q_insert.bindValue(5, tg.isEmpty() ? QVariant() : tg);
			q_insert.bindValue(6, gnomad.isEmpty() ? QVariant() : gnomad);
			q_insert.bindValue(7, variant.annotations()[i_gene]);
			q_insert.bindValue(8, variant.annotations()[i_type]);
			q_insert.bindValue(9, variant.annotations()[i_co_sp]);
			q_insert.exec();
			++c_add;
			output << q_insert.lastInsertId().toInt();
		}
	}

	return output;
}

QString NGSD::variantId(const Variant& variant, bool throw_if_fails)
{
	SqlQuery query = getQuery(); //use binding user input (safety)
	query.prepare("SELECT id FROM variant WHERE chr=:0 AND start=:1 AND end=:2 AND ref=:3 AND obs=:4");
	query.bindValue(0, variant.chr().strNormalized(true));
	query.bindValue(1, variant.start());
	query.bindValue(2, variant.end());
	query.bindValue(3, variant.ref());
	query.bindValue(4, variant.obs());
	query.exec();
	if (!query.next())
	{
		if (throw_if_fails)
		{
			THROW(DatabaseException, "Variant " + variant.toString() + " not found in NGSD!");
		}
		else
		{
			return "";
		}
	}

	return query.value(0).toString();
}

Variant NGSD::variant(const QString& variant_id)
{
	SqlQuery query = getQuery();
	query.exec("SELECT * FROM variant WHERE id=" + variant_id);
	if (!query.next()) THROW(DatabaseException, "Variant with identifier '" + variant_id + "' does not exist!");

	return Variant(query.value("chr").toByteArray(), query.value("start").toInt(), query.value("end").toInt(), query.value("ref").toByteArray(), query.value("obs").toByteArray());
}

QPair<int, int> NGSD::variantCounts(const QString& variant_id)
{
	//get same sample information (cached)
	static QHash<int, QList<int>> same_samples;
	if (same_samples.isEmpty())
	{
		SqlQuery query = getQuery();
		query.exec("SELECT sample1_id, sample2_id FROM sample_relations WHERE relation='same sample'");
		while (query.next())
		{
			int sample1_id = query.value(0).toInt();
			int sample2_id = query.value(1).toInt();
			same_samples[sample1_id] << sample2_id;
			same_samples[sample2_id] << sample1_id;
		}
	}

	//count variants
	int count_het = 0;
	int count_hom = 0;

	QSet<int> samples_done_het;
	QSet<int> samples_done_hom;
	SqlQuery query = getQuery();
	query.exec("SELECT s.id, dv.genotype FROM detected_variant dv, processed_sample ps, sample s WHERE dv.variant_id='" + variant_id + "' AND ps.sample_id=s.id AND dv.processed_sample_id=ps.id");
	while(query.next())
	{
		//use sample ID to prevent counting variants several times if a sample was sequenced more than once.
		int sample_id = query.value(0).toInt();
		QString genotype = query.value(1).toString();

		if (genotype=="het" && !samples_done_het.contains(sample_id))
		{
			++count_het;
			samples_done_het << sample_id;

			QList<int> tmp = same_samples.value(sample_id, QList<int>());
			foreach(int same_sample_id, tmp)
			{
				samples_done_het << same_sample_id;
			}
		}
		if (genotype=="hom" && !samples_done_hom.contains(sample_id))
		{
			++count_hom;
			samples_done_hom << sample_id;

			QList<int> tmp = same_samples.value(sample_id, QList<int>());
			foreach(int same_sample_id, tmp)
			{
				samples_done_hom << same_sample_id;
			}
		}
	}

	return qMakePair(count_het, count_hom);
}

void NGSD::deleteVariants(const QString& ps_id)
{
	deleteVariants(ps_id, VariantType::SNVS_INDELS);
	deleteVariants(ps_id, VariantType::CNVS);
}

void NGSD::deleteVariants(const QString& ps_id, VariantType type)
{
	if (type==VariantType::SNVS_INDELS)
	{
		getQuery().exec("DELETE FROM detected_variant WHERE processed_sample_id=" + ps_id);
	}
	else if (type==VariantType::CNVS)
	{
		QString callset_id = getValue("SELECT id FROM cnv_callset WHERE processed_sample_id=" + ps_id).toString();
		if (callset_id!="")
		{
			getQuery().exec("DELETE FROM cnv WHERE cnv_callset_id='" + callset_id + "'");
			getQuery().exec("DELETE FROM cnv_callset WHERE id='" + callset_id + "'");
		}
	}
	else
	{
		THROW(NotImplementedException, "Deleting variants of type '" + QString::number((int)type) + "' not implemented!");
	}
}

QString NGSD::cnvId(const CopyNumberVariant& cnv, int callset_id, bool throw_if_fails)
{
	SqlQuery query = getQuery(); //use binding user input (safety)
	query.prepare("SELECT id FROM cnv WHERE cnv_callset_id=:0 AND chr=:1 AND start=:2 AND end=:3");
	query.bindValue(0, callset_id);
	query.bindValue(1, cnv.chr().strNormalized(true));
	query.bindValue(2, cnv.start());
	query.bindValue(3, cnv.end());
	query.exec();
	if (!query.next())
	{
		if (throw_if_fails)
		{
			THROW(DatabaseException, "CNV " + cnv.toString() + " if callset with id '" + callset_id + "' not found in NGSD!");
		}
		else
		{
			return "";
		}
	}

	return query.value(0).toString();
}

CopyNumberVariant NGSD::cnv(int cnv_id)
{
	SqlQuery query = getQuery();
	query.exec("SELECT * FROM cnv WHERE id='" + QString::number(cnv_id) + "'");
	if (!query.next()) THROW(DatabaseException, "CNV with identifier '" + QString::number(cnv_id) + "' does not exist!");

	return CopyNumberVariant(query.value("chr").toByteArray(), query.value("start").toInt(), query.value("end").toInt());
}

QString NGSD::addCnv(int callset_id, const CopyNumberVariant& cnv, const CnvList& cnv_list, double max_ll)
{
	CnvCallerType caller = cnv_list.caller();

	//parse qc data
	QJsonObject quality_metrics;
	quality_metrics.insert("regions", QString::number(cnv.regions()));
	for(int i=0; i<cnv_list.annotationHeaders().count(); ++i)
	{
		const QByteArray& col_name = cnv_list.annotationHeaders()[i];
		const QByteArray& entry = cnv.annotations()[i];
		if (caller==CnvCallerType::CNVHUNTER)
		{
			if (col_name=="region_zscores")
			{
				quality_metrics.insert(QString(col_name), QString(entry));
			}
		}
		else if (caller==CnvCallerType::CLINCNV)
		{
			if (col_name=="loglikelihood")
			{
				quality_metrics.insert(QString(col_name), QString(entry));
				if (max_ll>0.0 && Helper::toDouble(entry, "log-likelihood")<max_ll)
				{
					return "";
				}
			}
			else if (col_name=="qvalue")
			{
				quality_metrics.insert(QString(col_name), QString(entry));
			}
		}
		else
		{
			THROW(ProgrammingException, "CNV caller type not handled in NGSD::addCnv")
		}
	}

	//determine CN
	int cn = cnv.copyNumber(cnv_list.annotationHeaders());

	//add cnv
	SqlQuery query = getQuery();
	query.prepare("INSERT INTO `cnv` (`cnv_callset_id`, `chr`, `start`, `end`, `cn`, `quality_metrics`) VALUES (:0,:1,:2,:3,:4,:5)");
	query.bindValue(0, callset_id);
	query.bindValue(1, cnv.chr().strNormalized(true));
	query.bindValue(2, cnv.start());
	query.bindValue(3, cnv.end());
	query.bindValue(4, cn);
	QJsonDocument json_doc;
	json_doc.setObject(quality_metrics);
	query.bindValue(5, json_doc.toJson(QJsonDocument::Compact));
	query.exec();

	//return insert ID
	return query.lastInsertId().toString();
}

int NGSD::addSv(int callset_id, const BedpeLine& sv, const BedpeFile& svs)
{
	//TODO: remove
	QTextStream out(stdout);

	// parse qc data
	QJsonObject quality_metrics;
	// get quality value
	quality_metrics.insert("quality", QString(sv.annotations()[svs.annotationIndexByName("QUAL")].trimmed()));
	// get filter values
	quality_metrics.insert("filter", QString(sv.annotations()[svs.annotationIndexByName("FILTER")].trimmed()));
	QJsonDocument json_doc;
	json_doc.setObject(quality_metrics);
	QByteArray quality_metrics_string = json_doc.toJson(QJsonDocument::Compact);

	if (sv.type() == StructuralVariantType::DEL || sv.type() == StructuralVariantType::DUP || sv.type() == StructuralVariantType::INV)
	{
		// get correct sv table
		QByteArray table;
		switch (sv.type())
		{
			case StructuralVariantType::DEL:
				table = "sv_deletion";
				break;
			case StructuralVariantType::DUP:
				table = "sv_duplication";
				break;
			case StructuralVariantType::INV:
				table = "sv_inversion";
				break;
			default:
				THROW(FileParseException, "Invalid structural variant type!");
				break;
		}

		// insert SV into the NGSD
		SqlQuery query = getQuery();
		query.prepare("INSERT INTO `" + table + "` (`sv_callset_id`, `chr`, `start_min`, `start_max`, `end_min`, `end_max`, `quality_metrics`) " +
					  "VALUES (:0, :1,  :2, :3, :4, :5, :6)");
		query.bindValue(0, callset_id);
		query.bindValue(1, sv.chr1().strNormalized(true));
		query.bindValue(2,  sv.start1());
		query.bindValue(3,  sv.end1());
		query.bindValue(4,  sv.start2());
		query.bindValue(5,  sv.end2());
		query.bindValue(6, quality_metrics_string);
		query.exec();

		//return insert ID
		return query.lastInsertId().toInt();

	}
	else if (sv.type() == StructuralVariantType::INS)
	{
		// get inserted sequence
		QByteArray inserted_sequence, known_left, known_right;
		QByteArray alt_seq = sv.annotations().at(svs.annotationIndexByName("ALT_A"));
		if (alt_seq != "<INS>")
		{
			// complete sequence available
			inserted_sequence = alt_seq;
		}
		else
		{
			// only right/left part of insertion available
			bool left_part_found = false;
			bool right_part_found = false;
			QByteArrayList info_a = sv.annotations().at(svs.annotationIndexByName("INFO_A")).split(';');
			foreach (const QByteArray& kv_pair, info_a)
			{
				if (kv_pair.startsWith("LEFT_SVINSSEQ="))
				{
					// left part
					known_left = kv_pair.split('=')[1].trimmed();
					left_part_found = true;
				}
				if (kv_pair.startsWith("RIGHT_SVINSSEQ="))
				{
					// right part
					known_right = kv_pair.split('=')[1].trimmed();
					right_part_found = true;
				}
				if (left_part_found && right_part_found) break;
			}
		}

		// insert SV into the NGSD
		SqlQuery query = getQuery();
		query.prepare(QByteArray() + "INSERT INTO `sv_insertion` (`sv_callset_id`, `chr`, `pos`, `ci_lower`, `ci_upper`, `inserted_sequence`, "
					  + "`known_left`, `known_right`, `quality_metrics`) VALUES (:0, :1,  :2, :3, :4, :5, :6, :7, :8)");
		query.bindValue(0, callset_id);
		query.bindValue(1, sv.chr1().strNormalized(true));
		query.bindValue(2, sv.start2());
		query.bindValue(3, sv.start2() - sv.start1());
		query.bindValue(4, sv.end2() - sv.start2());
		query.bindValue(5, inserted_sequence);
		query.bindValue(6, known_left);
		query.bindValue(7, known_right);
		query.bindValue(8, quality_metrics_string);
		query.exec();

		//return insert ID
		return query.lastInsertId().toInt();
	}
	else if (sv.type() == StructuralVariantType::BND)
	{
		// insert SV into the NGSD
		SqlQuery query = getQuery();
		query.prepare(QByteArray() + "INSERT INTO `sv_translocation` (`sv_callset_id`, `chr1`, `start1`, `end1`, `chr2`, `start2`, `end2`, "
					  + "`quality_metrics`) VALUES (:0, :1,  :2, :3, :4, :5, :6, :7)");
		query.bindValue(0, callset_id);
		query.bindValue(1, sv.chr1().strNormalized(true));
		query.bindValue(2, sv.start1());
		query.bindValue(3, sv.end1());
		query.bindValue(4, sv.chr2().strNormalized(true));
		query.bindValue(5, sv.start2());
		query.bindValue(6, sv.end2());
		query.bindValue(7, quality_metrics_string);
		query.exec();

		//return insert ID
		return query.lastInsertId().toInt();
	}
	else
	{
		THROW(FileParseException, "Invalid structural variant type!");
		return -1;
	}

}

QVariant NGSD::getValue(const QString& query, bool no_value_is_ok, QString bind_value) const
{
	//exeucte query
	SqlQuery q = getQuery();
	if (bind_value.isNull())
	{
		q.exec(query);
	}
	else
	{
		q.prepare(query);
		q.bindValue(0, bind_value);
		q.exec();
	}

	if (q.size()==0)
	{
		if (no_value_is_ok)
		{
			return QVariant();
		}
		else
		{
			THROW(DatabaseException, "NGSD single value query returned no value: " + query + (bind_value.isNull() ? "" : " (bind value: " + bind_value + ")"));
		}
	}
	if (q.size()>1)
	{
		THROW(DatabaseException, "NGSD single value query returned several values: " + query + (bind_value.isNull() ? "" : " (bind value: " + bind_value + ")"));
	}

	q.next();
	return q.value(0);
}

QStringList NGSD::getValues(const QString& query, QString bind_value) const
{
	SqlQuery q = getQuery();
	if (bind_value.isNull())
	{
		q.exec(query);
	}
	else
	{
		q.prepare(query);
		q.bindValue(0, bind_value);
		q.exec();
	}

	QStringList output;
	output.reserve(q.size());
	while(q.next())
	{
		output << q.value(0).toString();
	}
	return output;
}

void NGSD::executeQueriesFromFile(QString filename)
{
	QStringList lines = Helper::loadTextFile(filename, true);
	QString query = "";
	for(const QString& line : lines)
	{
		if (line.isEmpty()) continue;
		if (line.startsWith("--")) continue;

		query.append(' ');
		query.append(line);
		if (query.endsWith(';'))
		{
			//qDebug() << query;
			getQuery().exec(query);
			query.clear();
		}
	}
	if (query.endsWith(';'))
	{
		//qDebug() << query;
		getQuery().exec(query);
		query.clear();
	}
}

NGSD::~NGSD()
{
	//Log::info("MYSQL closing  - name: " + db_->connectionName());

	//close database and remove it
	QString connection_name = db_->connectionName();
	db_.clear();
	QSqlDatabase::removeDatabase(connection_name);
}

bool NGSD::isOpen() const
{
	return QSqlQuery(*db_).exec("SELECT 1");
}

QStringList NGSD::tables() const
{
	return db_->driver()->tables(QSql::Tables);
}

const TableInfo& NGSD::tableInfo(const QString& table) const
{
	//create if necessary
	if (!infos_.contains(table))
	{
		//check table exists
		if (!tables().contains(table))
		{
			THROW(DatabaseException, "Table '" + table + "' not found in NGSD!");
		}

		TableInfo output;
		output.setTable(table);

		//get PK info
		QSqlIndex index = db_->driver()->primaryIndex(table);

		//get FK info
		SqlQuery query_fk = getQuery();
		query_fk.exec("SELECT k.COLUMN_NAME, k.REFERENCED_TABLE_NAME, k.REFERENCED_COLUMN_NAME FROM information_schema.TABLE_CONSTRAINTS i LEFT JOIN information_schema.KEY_COLUMN_USAGE k ON i.CONSTRAINT_NAME = k.CONSTRAINT_NAME "
					"WHERE i.CONSTRAINT_TYPE = 'FOREIGN KEY' AND i.TABLE_SCHEMA = DATABASE() AND i.TABLE_NAME='" + table + "'");

		QList<TableFieldInfo> infos;
		SqlQuery query = getQuery();
		query.exec("DESCRIBE " + table);
		while(query.next())
		{
			TableFieldInfo info;

			//name
			info.name = query.value(0).toString();

			//index
			info.index = output.fieldCount();

			//type
			QString type = query.value(1).toString().toLower();
			info.is_unsigned = type.contains(" unsigned");
			if (info.is_unsigned)
			{
				type = type.replace(" unsigned", "");
			}
			if(type=="text") info.type = TableFieldInfo::TEXT;
			else if(type=="float") info.type = TableFieldInfo::FLOAT;
			else if(type=="date") info.type = TableFieldInfo::DATE;
			else if(type=="datetime") info.type = TableFieldInfo::DATETIME;
			else if(type=="timestamp") info.type = TableFieldInfo::TIMESTAMP;
			else if(type=="tinyint(1)") info.type = TableFieldInfo::BOOL;
			else if(type.startsWith("int(") || type.startsWith("tinyint(")) info.type = TableFieldInfo::INT;
			else if(type.startsWith("enum("))
			{
				info.type = TableFieldInfo::ENUM;
				info.type_constraints.valid_strings = getEnum(table, info.name);
			}
			else if(type.startsWith("varchar("))
			{
				info.type = TableFieldInfo::VARCHAR;
				info.type_constraints.max_length = Helper::toInt(type.mid(8, type.length()-9), "VARCHAR length");

				//password column
				if (table=="user" && info.name=="password")
				{
					info.type = TableFieldInfo::VARCHAR_PASSWORD;
				}

				//special constraints
				if (table=="mid" && info.name=="sequence") info.type_constraints.regexp = QRegularExpression("^[ACGT]*$");
				if (table=="processing_system" && info.name=="name_short") info.type_constraints.regexp = QRegularExpression("^[A-Za-z0-9_\\.-]*$");
				if (table=="processing_system" && info.name=="adapter1_p5") info.type_constraints.regexp = QRegularExpression("^[ACGT]*$");
				if (table=="processing_system" && info.name=="adapter2_p7") info.type_constraints.regexp = QRegularExpression("^[ACGT]*$");
				if (table=="processed_sample" && info.name=="lane") info.type_constraints.regexp = QRegularExpression("^[1-8](,[1-8])*$");
			}
			else
			{
				THROW(ProgrammingException, "Unhandled SQL field type '" + type + "' in field '" + info.name + "' of table '" + table + "'!");
			}

			//nullable
			info.is_nullable = query.value(2).toString().toLower()=="yes";

			//PK
			info.is_primary_key = index.contains(info.name);

			//unique
			info.is_unique = query.value(3).toString()=="UNI";

			//default value
			info.default_value =  query.value(4).isNull() ? QString() : query.value(4).toString();

			//FK
			query_fk.seek(-1);
			while (query_fk.next())
			{
				if (query_fk.value(0)==info.name)
				{
					info.fk_table = query_fk.value(1).toString();
					info.fk_field = query_fk.value(2).toString();

					//set type
					if (info.type!=TableFieldInfo::FK && info.type!=TableFieldInfo::INT)
					{
						THROW(ProgrammingException, "Found SQL foreign key with non-integer type '" + type + "' in field '" + info.name + "' of table '" + table + "'!");
					}
					info.type = TableFieldInfo::FK;

					//set name for FK
					if (table=="sequencing_run")
					{
						if (info.name=="device_id")
						{
							info.fk_name_sql = "CONCAT(name, ' (', type, ')')";
						}
					}
					else if (table=="project")
					{
						if (info.name=="internal_coordinator_id")
						{
							info.fk_name_sql = "name";
						}
					}
					else if (table=="processing_system")
					{
						if (info.name=="genome_id")
						{
							info.fk_name_sql = "build";
						}
					}
					else if (table=="sample")
					{
						if (info.name=="species_id")
						{
							info.fk_name_sql = "name";
						}
						else if (info.name=="sender_id")
						{
							info.fk_name_sql = "name";
						}
						else if (info.name=="receiver_id")
						{
							info.fk_name_sql = "name";
						}
					}
					else if (table=="processed_sample")
					{
						if (info.name=="sequencing_run_id")
						{
							info.fk_name_sql = "name";
						}
						else if (info.name=="sample_id")
						{
							info.fk_name_sql = "name";
						}
						else if ( info.name=="project_id")
						{
							info.fk_name_sql = "name";
						}
						else if (info.name=="processing_system_id")
						{
							info.fk_name_sql = "CONCAT(name_manufacturer, ' (', name_short, ')')";
						}
						else if (info.name=="operator_id")
						{
							info.fk_name_sql = "name";
						}
						else if (info.name=="normal_id")
						{
							info.fk_name_sql = "(SELECT CONCAT(s.name,'_',LPAD(ps.process_id,2,'0')) FROM sample s, processed_sample ps WHERE ps.id=processed_sample.id AND s.id=ps.sample_id)";
						}
						else if (info.name=="mid1_i7")
						{
							info.fk_name_sql = "CONCAT(name, ' (', sequence, ')')";
						}
						else if (info.name=="mid2_i5")
						{
							info.fk_name_sql = "CONCAT(name, ' (', sequence, ')')";
						}
					}
				}
			}

			//labels
			info.label = info.name;
			info.label.replace('_', ' ');
			if (table=="sequencing_run" && info.name=="fcid") info.label = "flowcell ID";
			if (table=="sequencing_run" && info.name=="device_id") info.label = "device";
			if (table=="project" && info.name=="preserve_fastqs") info.label = "preserve FASTQs";
			if (table=="project" && info.name=="internal_coordinator_id") info.label = "internal coordinator";
			if (table=="processing_system" && info.name=="adapter1_p5") info.label = "adapter read 1";
			if (table=="processing_system" && info.name=="adapter2_p7") info.label = "adapter read 2";
			if (table=="processing_system" && info.name=="genome_id") info.label = "genome";
			if (table=="sample" && info.name=="od_260_280") info.label = "od 260/280";
			if (table=="sample" && info.name=="od_260_230") info.label = "od 260/230";
			if (table=="sample" && info.name=="integrity_number") info.label = "RIN/DIN";
			if (table=="sample" && info.name=="species_id") info.label = "species";
			if (table=="sample" && info.name=="sender_id") info.label = "sender";
			if (table=="sample" && info.name=="receiver_id") info.label = "receiver";
			if (table=="processed_sample" && info.name=="sequencing_run_id") info.label = "sequencing run";
			if (table=="processed_sample" && info.name=="operator_id") info.label = "operator";
			if (table=="processed_sample" && info.name=="processing_system_id") info.label = "processing system";
			if (table=="processed_sample" && info.name=="project_id") info.label = "project";
			if (table=="processed_sample" && info.name=="processing_input") info.label = "processing input [ng]";
			if (table=="processed_sample" && info.name=="molarity") info.label = "molarity [nM]";
			if (table=="processed_sample" && info.name=="normal_id") info.label = "normal sample";
			if (table=="processed_sample" && info.name=="mid1_i7") info.label = "mid1 i7";
			if (table=="processed_sample" && info.name=="mid2_i5") info.label = "mid2 i5";
			if (table=="processed_sample" && info.name=="sample_id") info.label = "sample";

			//read-only
			if (
				(table=="sample" && info.name=="name") ||
				(table=="processing_system" && info.name=="name_short")
			   )
			{
				info.is_readonly = true;
			}

			//hidden
			if (
				info.is_primary_key ||
				info.type==TableFieldInfo::TIMESTAMP ||
				info.type==TableFieldInfo::DATETIME ||
				(table=="processed_sample" && info.name=="sample_id") ||
				(table=="processed_sample" && info.name=="process_id")
			   )
			{
				info.is_hidden = true;
			}

			//tooltip
			info.tooltip = getValue("SELECT COLUMN_COMMENT FROM INFORMATION_SCHEMA.COLUMNS WHERE TABLE_SCHEMA=database() AND TABLE_NAME='" + table + "' AND COLUMN_NAME='" + info.name + "'").toString().trimmed();
			info.tooltip.replace("<br>", "\n");

			infos.append(info);
		}
		output.setFieldInfo(infos);
		infos_.insert(table, output);
	}

	return infos_[table];
}

DBTable NGSD::createTable(QString table, QString query, int pk_col_index)
{
	SqlQuery query_result = getQuery();
	query_result.exec(query);

	DBTable output;
	output.setTableName(table);

	//headers
	QSqlRecord record = query_result.record();
	QStringList headers;
	for (int c=0; c<record.count(); ++c)
	{
		if (c==pk_col_index) continue;

		headers << record.field(c).name();
	}
	output.setHeaders(headers);

	//content
	output.reserve(query_result.size());
	while (query_result.next())
	{
		DBRow row;
		for (int c=0; c<record.count(); ++c)
		{
			QVariant value = query_result.value(c);
			QString value_as_string = value.toString();
			if (value.type()==QVariant::DateTime)
			{
				value_as_string = value_as_string.replace("T", " ");
			}
			if (c==pk_col_index)
			{
				row.setId(value_as_string);
			}
			else
			{
				row.addValue(value_as_string);
			}
		}
		output.addRow(row);
	}

	return output;
}

DBTable NGSD::createOverviewTable(QString table, QString text_filter, QString sql_order, int pk_col_index)
{
	DBTable output = createTable(table, "SELECT * FROM " + table + " ORDER BY " + sql_order, pk_col_index);

	//fix column content
	QStringList headers = output.headers();
	TableInfo table_info = tableInfo(table);
	for (int c=0; c<headers.count(); ++c)
	{
		const TableFieldInfo& field_info = table_info.fieldInfo(headers[c]);

		//FK - use name instead of id
		if(field_info.type==TableFieldInfo::FK)
		{
			QHash<QString, QString> cache; //check query result to reduce the number of SQL queries (they are slow)
			QStringList column = output.extractColumn(c);
			for(int r=0; r<column.count(); ++r)
			{
				QString value = column[r];
				if (value!="")
				{
					if (cache.contains(value))
					{
						column[r] = cache.value(value);
					}
					else
					{
						QString fk_value = getValue("SELECT " + field_info.fk_name_sql + " FROM " + field_info.fk_table + " WHERE id=" + value).toString();
						column[r] = fk_value;
						cache[value] = fk_value;
					}
				}
			}
			output.setColumn(c, column);
		}

		//BOOL - replace number by yes/no
		if(field_info.type==TableFieldInfo::BOOL)
		{
			output.formatBooleanColumn(output.columnIndex(field_info.name));
		}

		//PASSWORD - replace hashes
		if(field_info.type==TableFieldInfo::VARCHAR_PASSWORD)
		{
			QStringList column;
			while(column.count() < output.rowCount()) column << passordReplacement();
			output.setColumn(c, column);
		}
	}

	//fix headers (last because replacements before partially depend on correct header names)
	for (int i=0; i<headers.count(); ++i)
	{
		headers[i] = table_info.fieldInfo(headers[i]).label;
	}
	output.setHeaders(headers);

	//apply text filter
	text_filter = text_filter.trimmed();
	if (!text_filter.isEmpty())
	{
		output.filterRows(text_filter);
	}

	return output;
}

void NGSD::init(QString password)
{
	//remove existing tables
	SqlQuery query = getQuery();
	query.exec("SHOW TABLES");
	if (query.size()>0)
	{
		//check password for re-init of production DB
		if (!test_db_ && password!=Settings::string("ngsd_pass"))
		{
			THROW(DatabaseException, "Password provided for re-initialization of production database is incorrect!");
		}

		//get table list
		QStringList tables;
		while(query.next())
		{
			tables << query.value(0).toString();
		}

		//remove old tables
		if (!tables.isEmpty())
		{
			query.exec("SET FOREIGN_KEY_CHECKS = 0;");
			query.exec("DROP TABLE " + tables.join(","));
			query.exec("SET FOREIGN_KEY_CHECKS = 1;");
		}
	}

	//initilize
	executeQueriesFromFile(":/resources/NGSD_schema.sql");
}

QMap<QString, QString> NGSD::getProcessingSystems(bool skip_systems_without_roi, bool windows_paths)
{
	QMap<QString, QString> out;

	//load paths
	QString p_win;
	QString p_linux;
	if (windows_paths)
	{
		p_linux = getTargetFilePath(false, false);
		p_win = getTargetFilePath(false, true);
	}

	//load processing systems
	SqlQuery query = getQuery();
	query.exec("SELECT name_manufacturer, target_file FROM processing_system");
	while(query.next())
	{
		QString name = query.value(0).toString();
		QString roi = query.value(1).toString().replace(p_linux, p_win);
		if (roi=="" && skip_systems_without_roi) continue;
		out.insert(name, roi);
	}

	return out;
}

QCCollection NGSD::getQCData(const QString& processed_sample_id)
{
	//get QC data
	SqlQuery q = getQuery();
	q.exec("SELECT n.name, nm.value, n.description, n.qcml_id FROM processed_sample_qc as nm, qc_terms as n WHERE nm.processed_sample_id='" + processed_sample_id + "' AND nm.qc_terms_id=n.id AND n.obsolete=0");
	QCCollection output;
	while(q.next())
	{
		output.insert(QCValue(q.value(0).toString(), q.value(1).toString(), q.value(2).toString(), q.value(3).toString()));
	}

	//get KASP data
	SqlQuery q2 = getQuery();
	q2.exec("SELECT random_error_prob FROM kasp_status WHERE processed_sample_id='" + processed_sample_id + "'");
	QString value = "n/a";
	if (q2.size()>0)
	{
		q2.next();
		float numeric_value = 100.0 * q2.value(0).toFloat();
		if (numeric_value>100.0) //special case: random_error_prob>100%
		{
			value = "<font color=orange>KASP not performed (see NGSD)</font>";
		}
		else if (numeric_value>1.0) //random_error_prob>1% => warn
		{
			value = "<font color=red>"+QString::number(numeric_value)+"%</font>";
		}
		else
		{
			value = QString::number(numeric_value)+"%";
		}
	}
	output.insert(QCValue("kasp", value));

	return output;
}

QVector<double> NGSD::getQCValues(const QString& accession, const QString& processed_sample_id)
{
	//get processing system ID
	QString sys_id = getValue("SELECT processing_system_id FROM processed_sample WHERE id='" + processed_sample_id + "'").toString();

	//get QC id
	QString qc_id = getValue("SELECT id FROM qc_terms WHERE qcml_id=:0", true, accession).toString();

	//get QC data
	SqlQuery q = getQuery();
	q.exec("SELECT nm.value FROM processed_sample_qc as nm, processed_sample as ps WHERE ps.processing_system_id='" + sys_id + "' AND nm.qc_terms_id='" + qc_id + "' AND nm.processed_sample_id=ps.id ");

	//fill output datastructure
	QVector<double> output;
	while(q.next())
	{
		bool ok = false;
		double value = q.value(0).toDouble(&ok);
		if (ok) output.append(value);
	}

	return output;
}

ClassificationInfo NGSD::getClassification(const Variant& variant)
{
	//variant not in NGSD
	QString variant_id = variantId(variant, false);
	if (variant_id=="")
	{
		return ClassificationInfo();
	}

	//classification not present
	SqlQuery query = getQuery();
	query.exec("SELECT class, comment FROM variant_classification WHERE variant_id='" + variant_id + "'");
	if (query.size()==0)
	{
		return ClassificationInfo();
	}

	query.next();
	return ClassificationInfo {query.value(0).toString().trimmed(), query.value(1).toString().trimmed() };
}

ClassificationInfo NGSD::getSomaticClassification(const Variant& variant)
{
	//variant not in NGSD
	QString variant_id = variantId(variant, false);
	if (variant_id=="")
	{
		return ClassificationInfo();
	}

	//classification not present
	SqlQuery query = getQuery();
	query.exec("SELECT class, comment FROM somatic_variant_classification WHERE variant_id='" + variant_id + "'");
	if (query.size()==0)
	{
		return ClassificationInfo();
	}

	query.next();
	return ClassificationInfo {query.value(0).toString().trimmed(), query.value(1).toString().trimmed() };
}

void NGSD::setClassification(const Variant& variant, const VariantList& variant_list, ClassificationInfo info)
{
	QString variant_id = variantId(variant, false);
	if (variant_id=="") //add variant if missing
	{
		variant_id = addVariant(variant, variant_list);
	}

	SqlQuery query = getQuery(); //use binding (user input)
	query.prepare("INSERT INTO variant_classification (variant_id, class, comment) VALUES (" + variant_id + ",:0,:1) ON DUPLICATE KEY UPDATE class=VALUES(class), comment=VALUES(comment)");
	query.bindValue(0, info.classification);
	query.bindValue(1, info.comments);
	query.exec();
}

void NGSD::setSomaticClassification(const Variant& variant, ClassificationInfo info)
{
	SqlQuery query = getQuery();
	query.prepare("INSERT INTO somatic_variant_classification (variant_id, class, comment) VALUES (" + variantId(variant) + ",:0,:1) ON DUPLICATE KEY UPDATE class=VALUES(class), comment=VALUES(comment)");
	query.bindValue(0, info.classification);
	query.bindValue(1, info.comments);
	query.exec();
}



void NGSD::addVariantPublication(QString filename, const Variant& variant, QString database, QString classification, QString details)
{
	QString s_id = sampleId(filename);
	QString v_id = variantId(variant);
	QString user_id = QString::number(userId());

	//insert
	getQuery().exec("INSERT INTO variant_publication (sample_id, variant_id, db, class, details, user_id) VALUES ("+s_id+","+v_id+", '"+database+"', '"+classification+"', '"+details+"', "+user_id+")");
}

QString NGSD::getVariantPublication(QString filename, const Variant& variant)
{
	QString s_id = sampleId(filename, false);
	QString v_id = variantId(variant, false);

	if (s_id=="" || v_id=="") return "";

	//select
	SqlQuery query = getQuery();
	query.exec("SELECT vp.db, vp.class, vp.details, vp.date, u.name FROM variant_publication vp LEFT JOIN user u on vp.user_id=u.id WHERE sample_id="+s_id+" AND variant_id="+v_id);

	//create output
	QStringList output;
	while (query.next())
	{
		output << "db: " + query.value("db").toString() + " class: " + query.value("class").toString() + " user: " + query.value("name").toString() + " date: " + query.value("date").toString().replace("T", " ") + "\n  " + query.value("details").toString().replace(";", "\n  ").replace("=", ": ");
	}

	return output.join("\n");
}

QString NGSD::comment(const Variant& variant)
{
	return getValue("SELECT comment FROM variant WHERE id='" + variantId(variant) + "'").toString();
}

int NGSD::lastAnalysisOf(QString processed_sample_id)
{
	SqlQuery query = getQuery();
	query.exec("SELECT j.id FROM analysis_job j, analysis_job_sample js WHERE js.analysis_job_id=j.id AND js.processed_sample_id=" + processed_sample_id + " AND j.type='single sample' ORDER BY j.id DESC LIMIT 1");
	if (query.next())
	{
		return query.value(0).toInt();
	}

	return -1;
}

AnalysisJob NGSD::analysisInfo(int job_id, bool throw_if_fails)
{
	AnalysisJob output;

	SqlQuery query = getQuery();
	query.exec("SELECT * FROM analysis_job WHERE id=" + QString::number(job_id));
	if (query.next())
	{
		output.type = query.value("type").toString();
		output.high_priority = query.value("high_priority").toBool();
		output.args = query.value("args").toString();
		output.sge_id = query.value("sge_id").toString();
		output.sge_queue = query.value("sge_queue").toString();

		//extract samples
		SqlQuery query2 = getQuery();
		query2.exec("SELECT CONCAT(s.name,'_',LPAD(ps.process_id,2,'0')), js.info FROM analysis_job_sample js, processed_sample ps, sample s WHERE js.analysis_job_id=" + QString::number(job_id) + " AND js.processed_sample_id=ps.id AND ps.sample_id=s.id ORDER by js.id ASC");
		while(query2.next())
		{
			output.samples << AnalysisJobSample { query2.value(0).toString(), query2.value(1).toString() };
		}

		//extract status
		query2.exec("SELECT js.time, u.user_id, js.status, js.output FROM analysis_job_history js LEFT JOIN user u ON js.user_id=u.id  WHERE js.analysis_job_id=" + QString::number(job_id) + " ORDER BY js.id ASC");
		while(query2.next())
		{
			output.history << AnalysisJobHistoryEntry { query2.value(0).toDateTime(), query2.value(1).toString(), query2.value(2).toString(), query2.value(3).toString().split('\n') };
		}
	}
	else if (throw_if_fails)
	{
		THROW(DatabaseException, "Analysis job with id '" + QString::number(job_id) + "' not found in NGSD!");
	}

	return output;
}

void NGSD::queueAnalysis(QString type, bool high_priority, QStringList args, QList<AnalysisJobSample> samples, QString user_name)
{
	SqlQuery query = getQuery();

	//insert job
	query.exec("INSERT INTO `analysis_job`(`type`, `high_priority`, `args`) VALUES ('" + type + "','" + (high_priority ? "1" : "0") + "','" + args.join(" ") +  "')");
	QString job_id = query.lastInsertId().toString();

	//insert samples
	foreach(const AnalysisJobSample& sample, samples)
	{
		query.exec("INSERT INTO `analysis_job_sample`(`analysis_job_id`, `processed_sample_id`, `info`) VALUES (" + job_id + ",'" + processedSampleId(sample.name) + "','" + sample.info + "')");
	}

	//insert status
	query.exec("INSERT INTO `analysis_job_history`(`analysis_job_id`, `time`, `user_id`, `status`, `output`) VALUES (" + job_id + ",'" + Helper::dateTime("") + "'," + QString::number(userId(user_name)) + ",'queued', '')");
}

bool NGSD::cancelAnalysis(int job_id, QString user_name)
{
	//check if running or already canceled
	AnalysisJob job = analysisInfo(job_id, false);
	if (!job.isRunning()) return false;

	SqlQuery query = getQuery();
	query.exec("INSERT INTO `analysis_job_history`(`analysis_job_id`, `time`, `user_id`, `status`, `output`) VALUES (" + QString::number(job_id) + ",'" + Helper::dateTime("") + "'," + QString::number(userId(user_name)) + ",'cancel', '')");

	return true;
}

bool NGSD::deleteAnalysis(int job_id)
{
	QString job_id_str = QString::number(job_id);
	SqlQuery query = getQuery();
	query.exec("DELETE FROM analysis_job_sample WHERE analysis_job_id='" + job_id_str + "'");
	query.exec("DELETE FROM analysis_job_history WHERE analysis_job_id='" + job_id_str + "'");
	query.exec("DELETE FROM analysis_job WHERE id='" + job_id_str + "'");

	return query.numRowsAffected()>0;
}

QString NGSD::analysisJobFolder(int job_id)
{
	AnalysisJob job = analysisInfo(job_id, true);

	//project path
	QString output = processedSamplePath(processedSampleId(job.samples[0].name), NGSD::PROJECT_FOLDER);

	//type
	QString sample_sep;
	if (job.type=="single sample")
	{
		output += "Sample_";
	}
	else if (job.type=="multi sample")
	{
		output += "Multi_";
		sample_sep = "_";
	}
	else if (job.type=="trio")
	{
		output += "Trio_";
		sample_sep = "_";
	}
	else if (job.type=="somatic")
	{
		output += "Somatic_";
		sample_sep = "-";
	}
	else
	{
		THROW(ProgrammingException, "Unknown analysis type '" + job.type + "'!");
	}

	//samples
	bool first = true;
	foreach(const AnalysisJobSample& sample, job.samples)
	{
		if (!first)
		{
			output += sample_sep;
		}
		output += sample.name;
		first = false;
	}
	output += "/";

	return output;
}

QString NGSD::analysisJobGSvarFile(int job_id)
{
	AnalysisJob job = analysisInfo(job_id);

	//path
	QString output = analysisJobFolder(job_id);

	//file
	if (job.type=="single sample")
	{
		output += job.samples[0].name + ".GSvar";
	}
	else if (job.type=="multi sample")
	{
		output += "multi.GSvar";
	}
	else if (job.type=="trio")
	{
		output += "trio.GSvar";
	}
	else if (job.type=="somatic")
	{
		output += job.samples[0].name + "-" + job.samples[1].name + ".GSvar";
	}
	else
	{
		THROW(ProgrammingException, "Unknown analysis type '" + job.type + "'!");
	}

	return output;
}

QHash<QString, QString> NGSD::cnvCallsetMetrics(int callset_id)
{
	QHash<QString, QString> output;

	QByteArray metrics_string = getValue("SELECT quality_metrics FROM cnv_callset WHERE id=" + QString::number(callset_id), false).toByteArray();
	QJsonDocument qc_metrics = QJsonDocument::fromJson(metrics_string);
	foreach(const QString& key, qc_metrics.object().keys())
	{
		output[key] = qc_metrics.object().take(key).toString().trimmed();
	}

	return output;
}

QVector<double> NGSD::cnvCallsetMetrics(QString processing_system_id, QString metric_name)
{
	QVector<double> output;

	SqlQuery query = getQuery();
	query.exec("SELECT cs.quality_metrics FROM cnv_callset cs, processed_sample ps WHERE ps.id=cs.processed_sample_id AND ps.processing_system_id='" + processing_system_id + "'");
	while(query.next())
	{
		QJsonDocument qc_metrics = QJsonDocument::fromJson(query.value(0).toByteArray());
		bool ok = false;
		QString metric_string = qc_metrics.object().take(metric_name).toString();
		if (metric_string.contains(" (")) //special handling of CnvHunter metrics that contains the median in brackets)
		{
			metric_string = metric_string.split(" (").at(0);
		}
		double metric_numeric = metric_string.toDouble(&ok);
		if (ok && BasicStatistics::isValidFloat(metric_numeric)) output << metric_numeric;
	}

	return output;
}

QString NGSD::getTargetFilePath(bool subpanels, bool windows)
{
	QString key = windows ? "target_file_folder_windows" : "target_file_folder_linux";
	QString output = Settings::string(key);
	if (output=="")
	{
		THROW(ProgrammingException, "'" + key + "' entry is missing in settings!");
	}

	if (subpanels)
	{
		output += "/subpanels/";
	}

	return output;
}

void NGSD::updateQC(QString obo_file, bool debug)
{
	struct QCTerm
	{
		QString id;
		QString name;
		QString description;
		QString type;
		bool obsolete = false;
	};
	QList<QCTerm> terms;

	QStringList valid_types = getEnum("qc_terms", "type");

	QStringList lines = Helper::loadTextFile(obo_file, true, '#', true);
	QCTerm current;
	foreach(QString line, lines)
	{
		if (line=="[Term]")
		{
			terms << current;
			current = QCTerm();
		}
		else if (line.startsWith("id:"))
		{
			current.id = line.mid(3).trimmed();
		}
		else if (line.startsWith("name:"))
		{
			current.name = line.mid(5).trimmed();
		}
		else if (line.startsWith("def:"))
		{
			QStringList parts = line.split('"');
			current.description = parts[1].trimmed();
		}
		else if (line.startsWith("xref: value-type:xsd\\:"))
		{
			QStringList parts = line.replace('"', ':').split(':');
			current.type = parts[3].trimmed();
		}
		else if (line=="is_obsolete: true")
		{
			current.obsolete = true;
		}
	}
	terms << current;
	if (debug) qDebug() << "Terms parsed: " << terms.count();

	//remove terms not for NGS
	auto it = std::remove_if(terms.begin(), terms.end(), [](const QCTerm& term){return !term.id.startsWith("QC:2");});
	terms.erase(it, terms.end());
	if (debug) qDebug() << "Terms for NGS: " << terms.count();

	//remove QC terms of invalid types
	it = std::remove_if(terms.begin(), terms.end(), [valid_types](const QCTerm& term){return !valid_types.contains(term.type);});
	terms.erase(it, terms.end());
	if (debug) qDebug() << "Terms with valid types ("+valid_types.join(", ")+"): " << terms.count();

	//update NGSD


	// database connection
	transaction();
	QSqlQuery query = getQuery();
	query.prepare("INSERT INTO qc_terms (qcml_id, name, description, type, obsolete) VALUES (:0, :1, :2, :3, :4) ON DUPLICATE KEY UPDATE name=VALUES(name), description=VALUES(description), type=VALUES(type), obsolete=VALUES(obsolete)");

	foreach(const QCTerm& term, terms)
	{
		if (debug) qDebug() << "IMPORTING:" << term.id  << term.name  << term.type  << term.obsolete  << term.description;
		query.bindValue(0, term.id);
		query.bindValue(1, term.name);
		query.bindValue(2, term.description);
		query.bindValue(3, term.type);
		query.bindValue(4, term.obsolete);
		query.exec();
		if (debug) qDebug() << "  ID:" << query.lastInsertId();
	}
	commit();
}

void NGSD::fixGeneNames(QTextStream* messages, bool fix_errors, QString table, QString column)
{
	SqlQuery query = getQuery();
	query.exec("SELECT DISTINCT " + column + " FROM " + table + " tmp WHERE NOT EXISTS(SELECT * FROM gene WHERE symbol=tmp." + column + ")");
	while(query.next())
	{
		*messages << "Outdated gene name in '" << table << "': " << query.value(0).toString() << endl;
		if (fix_errors)
		{
			QString gene = query.value(0).toString();
			auto approved_data = geneToApprovedWithMessage(gene);
			if (approved_data.second.startsWith("ERROR"))
			{
				*messages << "  FAIL: Cannot fix error in '" << gene << "' because: " << approved_data.second << endl;
			}
			else
			{
				getQuery().exec("UPDATE " + table + " SET " + column + "='" + approved_data.first + "' WHERE " + column + "='" + gene +"'");
			}
		}
	}
}

QString NGSD::escapeForSql(const QString& text)
{
	return text.trimmed().replace("\"", "").replace("'", "").replace(";", "").replace("\n", "");
}

double NGSD::maxAlleleFrequency(const Variant& v, QList<int> af_column_index)
{
	double output = 0.0;

	foreach(int idx, af_column_index)
	{
		if (idx==-1) continue;
		bool ok;
		double value = v.annotations()[idx].toDouble(&ok);
		if (ok)
		{
			output = std::max(output, value);
		}
	}

	return output;
}

void NGSD::maintain(QTextStream* messages, bool fix_errors)
{
	SqlQuery query = getQuery();

	// (1) tumor samples variants that have been imported into 'detected_variant' table
	query.exec("SELECT CONCAT(s.name,'_',LPAD(ps.process_id,2,'0')), ps.id FROM sample s, processed_sample ps WHERE ps.sample_id=s.id AND s.tumor='1' AND EXISTS(SELECT * FROM detected_variant WHERE processed_sample_id=ps.id)");
	while(query.next())
	{
		*messages << "Tumor sample imported into germline variant table: " << query.value(0).toString() << endl;

		if (fix_errors)
		{
			deleteVariants(query.value(1).toString());
		}
	}

	// (2) outdated gene names
	fixGeneNames(messages, fix_errors, "geneinfo_germline", "symbol");
	fixGeneNames(messages, fix_errors, "hpo_genes", "gene");
	fixGeneNames(messages, fix_errors, "omim_gene", "gene");
	fixGeneNames(messages, fix_errors, "disease_gene", "gene");

	// (3) variants/qc-data/KASP present for merged processed samples
	query.exec("SELECT CONCAT(s.name,'_',LPAD(ps.process_id,2,'0')), p.type, p.name, s.id, ps.id FROM sample s, processed_sample ps, project p WHERE ps.sample_id=s.id AND ps.project_id=p.id");
	while(query.next())
	{
		QString ps_name = query.value(0).toString();
		QString p_type = query.value(1).toString();

		QString folder = Settings::string("projects_folder") + "/" + p_type + "/" + query.value(2).toString() + "/Sample_" + ps_name + "/";
		if (!QFile::exists(folder))
		{
			QString ps_id = query.value(4).toString();

			//check if merged
			bool merged = false;
			SqlQuery query2 = getQuery();
			query2.exec("SELECT CONCAT(s.name,'_',LPAD(ps.process_id,2,'0')), p.type, p.name FROM sample s, processed_sample ps, project p WHERE ps.sample_id=s.id AND ps.project_id=p.id AND s.id='" + query.value(3).toString()+"' AND ps.id!='" + ps_id + "'");
			while(query2.next())
			{
				QString folder2 = Settings::string("projects_folder") + "/" + query2.value(1).toString() + "/" + query2.value(2).toString() + "/Sample_" + query2.value(0).toString() + "/";
				if (QFile::exists(folder2))
				{
					QStringList files = Helper::findFiles(folder2, ps_name + "*.fastq.gz", false);
					if (files.count()>0)
					{
						//qDebug() << "Sample " << ps_name << " merged into sample folder " << folder2 << "!" << endl;
						merged = true;
					}
				}
			}

			//check status
			if (merged)
			{
				//check if variants are present
				int c_var = getValue("SELECT COUNT(*) FROM detected_variant WHERE processed_sample_id='" + ps_id + "'").toInt();
				int c_cnv = getValue("SELECT COUNT(*) FROM cnv_callset WHERE processed_sample_id='" + ps_id + "'").toInt();
				if (c_var>0 || c_cnv>0)
				{
					*messages << "Merged sample " << ps_name << " has variant data (small variant or CNVs)!" << endl;

					if (fix_errors)
					{
						deleteVariants(ps_id);
					}
				}
				int c_qc = getValue("SELECT COUNT(*) FROM processed_sample_qc WHERE processed_sample_id='" + ps_id + "'").toInt();
				if (c_qc>0)
				{
					*messages << "Merged sample " << ps_name << " has QC data!" << endl;

					if (fix_errors)
					{
						getQuery().exec("DELETE FROM processed_sample_qc WHERE processed_sample_id='" + ps_id + "'");
					}
				}
				if (p_type=="diagnostic")
				{
					QVariant kasp = getValue("SELECT random_error_prob FROM kasp_status WHERE processed_sample_id='" + ps_id + "'");
					if (kasp.isNull())
					{
						*messages << "Merged sample " << ps_name << " has KASP result!" << endl;

						if (fix_errors)
						{
							getQuery().exec("INSERT INTO `kasp_status`(`processed_sample_id`, `random_error_prob`, `snps_evaluated`, `snps_match`) VALUES ('" + ps_id + "',999,0,0)");
						}
					}
				}
			}
		}
	}

	//(4) variants for bad processed samples
	query.exec("SELECT CONCAT(s.name,'_',LPAD(ps.process_id,2,'0')), ps.id FROM sample s, processed_sample ps WHERE ps.sample_id=s.id AND ps.quality='bad'");
	while(query.next())
	{
		QString ps_id = query.value(1).toString();

		//check if variants are present
		int c_var = getValue("SELECT COUNT(*) FROM detected_variant WHERE processed_sample_id='" + ps_id + "'").toInt();
		if (c_var>0)
		{
			*messages << "Bad sample " << query.value(0).toString() << " has variant data!" << endl;

			if (fix_errors)
			{
				deleteVariants(ps_id);
			}
		}
	}

	//(5) invalid HPO entries in sample_disease_info
	int hpo_terms_imported = getValue("SELECT COUNT(*) FROM hpo_term").toInt();
	if (hpo_terms_imported>0)
	{
		query.exec("SELECT DISTINCT id, disease_info FROM sample_disease_info WHERE type='HPO term id' AND disease_info NOT IN (SELECT hpo_id FROM hpo_term)");
		while(query.next())
		{
			QString hpo_id = query.value(1).toString();
			*messages << "Invalid/obsolete HPO identifier '" << hpo_id << "' in table 'sample_disease_info'!" << endl;

			if (fix_errors)
			{
				getQuery().exec("DELETE FROM sample_disease_info WHERE id='" + query.value(0).toString() + "'");
			}
		}
	}
	else
	{
		*messages << "Warning: Cannot perform check for invalid HPO identifierts because not HPO terms were imported into the NGSD!" << endl;
	}
}

void NGSD::setComment(const Variant& variant, const QString& text)
{
	SqlQuery query = getQuery();
	query.prepare("UPDATE variant SET comment=:1 WHERE id='" + variantId(variant) + "'");
	query.bindValue(0, text);
	query.exec();
}

QString NGSD::nextProcessingId(const QString& sample_id)
{
	QString max_num = getValue("SELECT MAX(process_id) FROM processed_sample WHERE sample_id=" + sample_id).toString();

	return max_num.isEmpty() ? "1" : QString::number(max_num.toInt()+1);
}

QStringList NGSD::getEnum(QString table, QString column) const
{
	//check cache
	static QMap<QString, QStringList> cache;
	QString hash = table+"."+column;
	if (cache.contains(hash))
	{
		return cache.value(hash);
	}

	//DB query
	SqlQuery q = getQuery();
	q.exec("DESCRIBE "+table+" "+column);
	while (q.next())
	{
		QString type = q.value(1).toString();
		type = type.mid(6,type.length()-8);
		cache[hash] = type.split("','");
		return cache[hash];
	}

	THROW(ProgrammingException, "Could not determine enum values of column '"+column+"' in table '"+table+"'!");
}


void NGSD::tableExists(QString table)
{
	SqlQuery query = getQuery();
	query.exec("SHOW TABLES LIKE '" + table + "'");
	if (query.size()==0)
	{
		THROW(DatabaseException, "Table '" + table + "' does not exist!")
	}
}

bool NGSD::tableEmpty(QString table)
{
	SqlQuery query = getQuery();
	query.exec("SELECT COUNT(*) FROM " + table);
	query.next();
	return query.value(0).toInt()==0;
}

void NGSD::clearTable(QString table)
{
	SqlQuery query = getQuery();
	query.exec("DELETE FROM " + table);
}

int NGSD::geneToApprovedID(const QByteArray& gene)
{
	//approved
	if (approvedGeneNames().contains(gene))
	{
		return getValue("SELECT id FROM gene WHERE symbol='" + gene + "'").toInt();
	}

	//previous
	SqlQuery q_prev = getQuery();
	q_prev.prepare("SELECT g.id FROM gene g, gene_alias ga WHERE g.id=ga.gene_id AND ga.symbol=:0 AND ga.type='previous'");
	q_prev.bindValue(0, gene);
	q_prev.exec();
	if (q_prev.size()==1)
	{
		q_prev.next();
		return q_prev.value(0).toInt();
	}
	else if(q_prev.size()>1)
	{
		return -1;
	}

	//synonymous
	SqlQuery q_syn = getQuery();
	q_syn.prepare("SELECT g.id FROM gene g, gene_alias ga WHERE g.id=ga.gene_id AND ga.symbol=:0 AND ga.type='synonym'");
	q_syn.bindValue(0, gene);
	q_syn.exec();
	if (q_syn.size()==1)
	{
		q_syn.next();
		return q_syn.value(0).toInt();
	}

	return -1;
}

QByteArray NGSD::geneSymbol(int id)
{
	return getValue("SELECT symbol FROM gene WHERE id=:0", true, QString::number(id)).toByteArray();
}

QByteArray NGSD::geneToApproved(QByteArray gene, bool return_input_when_unconvertable)
{
	gene = gene.trimmed().toUpper();

	//already approved gene
	if (approvedGeneNames().contains(gene))
	{
		return gene;
	}

	//check if already cached
	static QMap<QByteArray, QByteArray> mapping;
	if (mapping.contains(gene))
	{
		if (return_input_when_unconvertable && mapping[gene].isEmpty())
		{
			return gene;
		}

		return mapping[gene];
	}

	//not cached => try to convert
	int gene_id = geneToApprovedID(gene);
	mapping[gene] = (gene_id!=-1) ? geneSymbol(gene_id) : "";

	if (return_input_when_unconvertable && mapping[gene].isEmpty())
	{
		return gene;
	}

	return mapping[gene];
}

GeneSet NGSD::genesToApproved(GeneSet genes, bool return_input_when_unconvertable)
{
	GeneSet output;

	foreach(const QByteArray& gene, genes)
	{
		QByteArray gene_new = geneToApproved(gene, return_input_when_unconvertable);
		if (!gene_new.isEmpty())
		{
			output.insert(gene_new);
		}
	}

	return output;
}

QPair<QString, QString> NGSD::geneToApprovedWithMessage(const QString& gene)
{
	//approved
	if (approvedGeneNames().contains(gene.toUtf8()))
	{
		return qMakePair(gene, QString("KEPT: " + gene + " is an approved symbol"));
	}

	//previous
	SqlQuery q_prev = getQuery();
	q_prev.prepare("SELECT g.symbol FROM gene g, gene_alias ga WHERE g.id=ga.gene_id AND ga.symbol=:0 AND ga.type='previous' ORDER BY g.id");
	q_prev.bindValue(0, gene);
	q_prev.exec();
	if (q_prev.size()==1)
	{
		q_prev.next();
		return qMakePair(q_prev.value(0).toString(), "REPLACED: " + gene + " is a previous symbol");
	}
	else if(q_prev.size()>1)
	{
		QString genes;
		while(q_prev.next())
		{
			if (!genes.isEmpty()) genes.append(", ");
			genes.append(q_prev.value(0).toString());
		}
		return qMakePair(gene, "ERROR: " + gene + " is a previous symbol of the genes " + genes);
	}

	//synonymous
	SqlQuery q_syn = getQuery();
	q_syn.prepare("SELECT g.symbol FROM gene g, gene_alias ga WHERE g.id=ga.gene_id AND ga.symbol=:0 AND ga.type='synonym' ORDER BY g.id");
	q_syn.bindValue(0, gene);
	q_syn.exec();
	if (q_syn.size()==1)
	{
		q_syn.next();
		return qMakePair(q_syn.value(0).toString(), "REPLACED: " + gene + " is a synonymous symbol");
	}
	else if(q_syn.size()>1)
	{
		QByteArray genes;
		while(q_syn.next())
		{
			if (!genes.isEmpty()) genes.append(", ");
			genes.append(q_syn.value(0).toString());
		}
		return qMakePair(gene, "ERROR: " + gene + " is a synonymous symbol of the genes " + genes);
	}

	return qMakePair(gene, QString("ERROR: " + gene + " is unknown symbol"));
}

QList<QPair<QByteArray, QByteArray> > NGSD::geneToApprovedWithMessageAndAmbiguous(const QByteArray& gene)
{
	QList<QPair<QByteArray, QByteArray>> output;

	//approved
	if (approvedGeneNames().contains(gene))
	{
		output << qMakePair(gene, "KEPT: " + gene + " is an approved symbol");
		return output;
	}

	//previous
	SqlQuery q_prev = getQuery();
	q_prev.prepare("SELECT g.symbol FROM gene g, gene_alias ga WHERE g.id=ga.gene_id AND ga.symbol=:0 AND ga.type='previous' ORDER BY g.id");
	q_prev.bindValue(0, gene);
	q_prev.exec();
	if (q_prev.size()>=1)
	{
		while(q_prev.next())
		{
			output << qMakePair(q_prev.value(0).toByteArray(), "REPLACED: " + gene + " is a previous symbol");
		}
		return output;
	}

	//synonymous
	SqlQuery q_syn = getQuery();
	q_syn.prepare("SELECT g.symbol FROM gene g, gene_alias ga WHERE g.id=ga.gene_id AND ga.symbol=:0 AND ga.type='synonym' ORDER BY g.id");
	q_syn.bindValue(0, gene);
	q_syn.exec();
	if (q_syn.size()>=1)
	{
		while(q_syn.next())
		{
			output << qMakePair(q_syn.value(0).toByteArray(), "REPLACED: " + gene + " is a synonymous symbol");
		}
		return output;
	}

	//unknown
	output << qMakePair(gene, "ERROR: " + gene + " is an unknown symbol");
	return output;
}

GeneSet NGSD::previousSymbols(int id)
{
	GeneSet output;

	SqlQuery q = getQuery();
	q.exec("SELECT symbol FROM gene_alias WHERE gene_id='" + QByteArray::number(id) + "' AND type='previous'");
	while(q.next())
	{
		output.insert(q.value(0).toByteArray());
	}

	return output;
}

GeneSet NGSD::synonymousSymbols(int id)
{
	GeneSet output;

	SqlQuery q = getQuery();
	q.exec("SELECT symbol FROM gene_alias WHERE gene_id='" + QByteArray::number(id) + "' AND type='synonymous'");
	while(q.next())
	{
		output.insert(q.value(0).toByteArray());
	}

	return output;
}

QList<Phenotype> NGSD::phenotypes(const QByteArray& symbol)
{
	QList<Phenotype> output;

	SqlQuery query = getQuery();
	query.prepare("SELECT t.hpo_id, t.name FROM hpo_term t, hpo_genes g WHERE g.gene=:0 AND t.id=g.hpo_term_id ORDER BY t.name ASC");
	query.bindValue(0, symbol);
	query.exec();
	while(query.next())
	{
		output << Phenotype(query.value(0).toByteArray(), query.value(1).toByteArray());
	}

	return output;
}

QList<Phenotype> NGSD::phenotypes(QStringList search_terms)
{
	//trim terms and remove empty terms
	std::for_each(search_terms.begin(), search_terms.end(), [](QString& term){ term = term.trimmed(); });
	search_terms.removeAll("");

	QList<Phenotype> list;

	if (search_terms.isEmpty()) //no terms => all phenotypes
	{
		SqlQuery query = getQuery();
		query.exec("SELECT hpo_id, name FROM hpo_term ORDER BY name ASC");
		while(query.next())
		{
			list << Phenotype(query.value(0).toByteArray(), query.value(1).toByteArray());
		}
	}
	else //search for terms (intersect results of all terms)
	{
		bool first = true;
		QSet<Phenotype> set;
		SqlQuery query = getQuery();
		query.prepare("SELECT hpo_id, name FROM hpo_term WHERE name LIKE :0 OR hpo_id LIKE :1 OR synonyms LIKE :2");
		foreach(const QString& term, search_terms)
		{
			query.bindValue(0, "%" + term + "%");
			query.bindValue(1, "%" + term + "%");
			query.bindValue(2, "%" + term + "%");
			query.exec();
			QSet<Phenotype> tmp;
			while(query.next())
			{
				tmp << Phenotype(query.value(0).toByteArray(), query.value(1).toByteArray());
			}

			if (first)
			{
				set = tmp;
				first = false;
			}
			else
			{
				set = set.intersect(tmp);
			}
		}

		list = set.toList();
		std::sort(list.begin(), list.end(), [](const Phenotype& a, const Phenotype& b){ return a.name()<b.name(); });
	}

	return list;
}

GeneSet NGSD::phenotypeToGenes(const Phenotype& phenotype, bool recursive)
{
	//prepare queries
	SqlQuery pid2genes = getQuery();
	pid2genes.prepare("SELECT gene FROM hpo_genes WHERE hpo_term_id=:0");
	SqlQuery pid2children = getQuery();
	pid2children.prepare("SELECT child FROM hpo_parent WHERE parent=:0");

	//convert phenotype to id
	SqlQuery tmp = getQuery();
	tmp.prepare("SELECT id FROM hpo_term WHERE name=:0");
	tmp.bindValue(0, phenotype.name());
	tmp.exec();
	if (!tmp.next()) THROW(ProgrammingException, "Unknown phenotype '" + phenotype.toString() + "'!");
	QList<int> pheno_ids;
	pheno_ids << tmp.value(0).toInt();

	GeneSet genes;
	while (!pheno_ids.isEmpty())
	{
		int id = pheno_ids.last();
		pheno_ids.removeLast();

		//add genes of current phenotype
		pid2genes.bindValue(0, id);
		pid2genes.exec();
		while(pid2genes.next())
		{
			QByteArray gene = pid2genes.value(0).toByteArray();
			genes.insert(geneToApproved(gene, true));
		}

		//add sub-phenotypes
		if (recursive)
		{
			pid2children.bindValue(0, id);
			pid2children.exec();
			while(pid2children.next())
			{
				pheno_ids << pid2children.value(0).toInt();
			}
		}
	}

	return genes;
}

QList<Phenotype> NGSD::phenotypeChildTems(const Phenotype& phenotype, bool recursive)
{
	//prepare queries
	SqlQuery pid2children = getQuery();
	pid2children.prepare("SELECT t.id, t.hpo_id, t.name  FROM hpo_parent p, hpo_term t WHERE p.parent=:0 AND p.child=t.id");

	//convert phenotype to id
	QList<int> pheno_ids;
	bool ok;
	pheno_ids << getValue("SELECT id FROM hpo_term WHERE name=:0", true, phenotype.name()).toInt(&ok);
	if (!ok) THROW(ProgrammingException, "Unknown phenotype '" + phenotype.toString() + "'!");

	QList<Phenotype> terms;
	while (!pheno_ids.isEmpty())
	{
		int id = pheno_ids.takeLast();

		pid2children.bindValue(0, id);
		pid2children.exec();
		while(pid2children.next())
		{
			terms.append(Phenotype(pid2children.value(1).toByteArray(), pid2children.value(2).toByteArray()));
			if (recursive)
			{
				pheno_ids << pid2children.value(0).toInt();
			}
		}
	}

	return terms;
}


Phenotype NGSD::phenotypeByName(const QByteArray& name, bool throw_on_error)
{
	QByteArray accession = getValue("SELECT hpo_id FROM hpo_term WHERE name=:0", true, name).toByteArray();
	if (accession.isEmpty() && throw_on_error)
	{
		THROW(ArgumentException, "Cannot find HPO phenotype with name '" + name + "' in NGSD!");
	}
	return Phenotype(accession, name);
}


Phenotype NGSD::phenotypeByAccession(const QByteArray& accession, bool throw_on_error)
{
	QByteArray name = getValue("SELECT name FROM hpo_term WHERE hpo_id=:0", true, accession).toByteArray();
	if (name.isEmpty() && throw_on_error)
	{
		THROW(ArgumentException, "Cannot find HPO phenotype with accession '" + accession + "' in NGSD!");
	}
	return Phenotype(accession, name);
}

const GeneSet& NGSD::approvedGeneNames()
{
	static GeneSet output;

	if (output.count()==0)
	{
		SqlQuery query = getQuery();
		query.exec("SELECT symbol from gene");

		while(query.next())
		{
			output.insert(query.value(0).toByteArray());
		}
	}

	return output;
}


GeneSet NGSD::genesOverlapping(const Chromosome& chr, int start, int end, int extend)
{
	//init static data (load gene regions file from NGSD to memory)
	static BedFile bed;
	static ChromosomalIndex<BedFile> index(bed);
	if (bed.count()==0)
	{
		//add transcripts
		SqlQuery query = getQuery();
		query.exec("SELECT g.symbol, gt.chromosome, MIN(ge.start), MAX(ge.end) FROM gene g, gene_transcript gt, gene_exon ge WHERE ge.transcript_id=gt.id AND gt.gene_id=g.id GROUP BY gt.id");
		while(query.next())
		{
			bed.append(BedLine(query.value(1).toString(), query.value(2).toInt(), query.value(3).toInt(), QList<QByteArray>() << query.value(0).toByteArray()));
		}

		//sort and index
		bed.sort();
		index.createIndex();
	}

	//create gene list
	GeneSet genes;
	QVector<int> matches = index.matchingIndices(chr, start-extend, end+extend);
	foreach(int i, matches)
	{
		genes << bed[i].annotations()[0];
	}
	return genes;
}

GeneSet NGSD::genesOverlappingByExon(const Chromosome& chr, int start, int end, int extend)
{
	//init static data (load gene regions file from NGSD to memory)
	static BedFile bed;
	static ChromosomalIndex<BedFile> index(bed);
	if (bed.count()==0)
	{
		SqlQuery query = getQuery();
		query.exec("SELECT DISTINCT g.symbol, gt.chromosome, ge.start, ge.end FROM gene g, gene_exon ge, gene_transcript gt WHERE ge.transcript_id=gt.id AND gt.gene_id=g.id");
		while(query.next())
		{
			bed.append(BedLine(query.value(1).toString(), query.value(2).toInt(), query.value(3).toInt(), QList<QByteArray>() << query.value(0).toByteArray()));
		}
		bed.sort();
		index.createIndex();
	}

	//create gene list
	GeneSet genes;
	QVector<int> matches = index.matchingIndices(chr, start-extend, end+extend);
	foreach(int i, matches)
	{
		genes << bed[i].annotations()[0];
	}

	return genes;
}

BedFile NGSD::geneToRegions(const QByteArray& gene, Transcript::SOURCE source, QString mode, bool fallback, bool annotate_transcript_names, QTextStream* messages)
{
	QString source_str = Transcript::sourceToString(source);

	//check mode
	QStringList valid_modes;
	valid_modes << "gene" << "exon";
	if (!valid_modes.contains(mode))
	{
		THROW(ArgumentException, "Invalid mode '" + mode + "'. Valid modes are: " + valid_modes.join(", ") + ".");
	}

	//prepare queries
	SqlQuery q_transcript = getQuery();
	q_transcript.prepare("SELECT id, chromosome, start_coding, end_coding, name FROM gene_transcript WHERE source='" + source_str + "' AND gene_id=:1");
	SqlQuery q_transcript_fallback = getQuery();
	q_transcript_fallback.prepare("SELECT id, chromosome, start_coding, end_coding, name FROM gene_transcript WHERE gene_id=:1");
	SqlQuery q_range = getQuery();
	q_range.prepare("SELECT MIN(start), MAX(end) FROM gene_exon WHERE transcript_id=:1");
	SqlQuery q_exon = getQuery();
	q_exon.prepare("SELECT start, end FROM gene_exon WHERE transcript_id=:1");

	//process input data
	BedFile output;

	//get approved gene id
	int id = geneToApprovedID(gene);
	if (id==-1)
	{
		if (messages) *messages << "Gene name '" << gene << "' is no HGNC-approved symbol. Skipping it!" << endl;
		return output;
	}
	QByteArray gene_approved = geneToApproved(gene);

	//prepare annotations
	QList<QByteArray> annos;
	annos << gene_approved;

	//GENE mode
	if (mode=="gene")
	{
		bool hits = false;

		//add source transcripts
		q_transcript.bindValue(0, id);
		q_transcript.exec();
		while(q_transcript.next())
		{
			if (annotate_transcript_names)
			{
				annos.clear();
				annos << gene_approved + " " + q_transcript.value(4).toByteArray();
			}

			q_range.bindValue(0, q_transcript.value(0).toInt());
			q_range.exec();
			q_range.next();

			output.append(BedLine("chr"+q_transcript.value(1).toByteArray(), q_range.value(0).toInt(), q_range.value(1).toInt(), annos));
			hits = true;
		}

		//add fallback transcripts
		if (!hits && fallback)
		{
			q_transcript_fallback.bindValue(0, id);
			q_transcript_fallback.exec();
			while(q_transcript_fallback.next())
			{
				if (annotate_transcript_names)
				{
					annos.clear();
					annos << gene_approved + " " + q_transcript_fallback.value(4).toByteArray();
				}


				q_range.bindValue(0, q_transcript_fallback.value(0).toInt());
				q_range.exec();
				q_range.next();

				output.append(BedLine("chr"+q_transcript_fallback.value(1).toByteArray(), q_range.value(0).toInt(), q_range.value(1).toInt(), annos));

				hits = true;
			}
		}

		if (!hits && messages!=nullptr)
		{
			*messages << "No transcripts found for gene '" + gene + "'. Skipping it!" << endl;
		}
	}

	//EXON mode
	else if (mode=="exon")
	{
		bool hits = false;

		q_transcript.bindValue(0, id);
		q_transcript.exec();
		while(q_transcript.next())
		{
			if (annotate_transcript_names)
			{
				annos.clear();
				annos << gene_approved + " " + q_transcript.value(4).toByteArray();
			}

			int trans_id = q_transcript.value(0).toInt();
			bool is_coding = !q_transcript.value(2).isNull() && !q_transcript.value(3).isNull();
			int start_coding = q_transcript.value(2).toInt();
			int end_coding = q_transcript.value(3).toInt();

			q_exon.bindValue(0, trans_id);
			q_exon.exec();
			while(q_exon.next())
			{
				int start = q_exon.value(0).toInt();
				int end = q_exon.value(1).toInt();
				if (is_coding)
				{
					start = std::max(start_coding, start);
					end = std::min(end_coding, end);

					//skip non-coding exons of coding transcripts
					if (end<start_coding || start>end_coding) continue;
				}

				output.append(BedLine("chr"+q_transcript.value(1).toByteArray(), start, end, annos));
				hits = true;
			}
		}

		//fallback
		if (!hits && fallback)
		{
			q_transcript_fallback.bindValue(0, id);
			q_transcript_fallback.exec();
			while(q_transcript_fallback.next())
			{
				if (annotate_transcript_names)
				{
					annos.clear();
					annos << gene + " " + q_transcript_fallback.value(4).toByteArray();
				}

				int trans_id = q_transcript_fallback.value(0).toInt();
				bool is_coding = !q_transcript_fallback.value(2).isNull() && !q_transcript_fallback.value(3).isNull();
				int start_coding = q_transcript_fallback.value(2).toInt();
				int end_coding = q_transcript_fallback.value(3).toInt();
				q_exon.bindValue(0, trans_id);
				q_exon.exec();
				while(q_exon.next())
				{
					int start = q_exon.value(0).toInt();
					int end = q_exon.value(1).toInt();
					if (is_coding)
					{
						start = std::max(start_coding, start);
						end = std::min(end_coding, end);

						//skip non-coding exons of coding transcripts
						if (end<start_coding || start>end_coding) continue;
					}

					output.append(BedLine("chr"+q_transcript_fallback.value(1).toByteArray(), start, end, annos));
					hits = true;
				}
			}
		}

		if (!hits && messages!=nullptr)
		{
			*messages << "No transcripts found for gene '" << gene << "'. Skipping it!" << endl;
		}
	}

	output.sort(!annotate_transcript_names);

	return output;
}

BedFile NGSD::genesToRegions(const GeneSet& genes, Transcript::SOURCE source, QString mode, bool fallback, bool annotate_transcript_names, QTextStream* messages)
{
	BedFile output;

	foreach(const QByteArray& gene, genes)
	{
		output.add(geneToRegions(gene, source, mode, fallback, annotate_transcript_names, messages));
	}

	output.sort(!annotate_transcript_names);

	return output;
}

QList<Transcript> NGSD::transcripts(int gene_id, Transcript::SOURCE source, bool coding_only)
{
	QList<Transcript> output;

	//get chromosome
	QString gene_id_str = QString::number(gene_id);

	//get transcripts
	SqlQuery query = getQuery();
	query.exec("SELECT id, name, chromosome, start_coding, end_coding, strand FROM gene_transcript WHERE gene_id=" + gene_id_str + " AND source='" + Transcript::sourceToString(source) + "' " + (coding_only ? "AND start_coding IS NOT NULL AND end_coding IS NOT NULL" : "") + " ORDER BY name");
	while(query.next())
	{
		//get base information
		Transcript transcript;
		transcript.setName(query.value(1).toByteArray());
		transcript.setSource(source);
		transcript.setStrand(Transcript::stringToStrand(query.value(5).toByteArray()));

		//get exons
		BedFile regions;
		QByteArray chr = query.value(2).toByteArray();
		int start_coding = query.value(3).toUInt();
		int end_coding = query.value(4).toUInt();
		SqlQuery query2 = getQuery();
		int id = query.value(0).toUInt();
		query2.exec("SELECT start, end FROM gene_exon WHERE transcript_id=" + QString::number(id) + " ORDER BY start");
		while(query2.next())
		{
			int start = query2.value(0).toUInt();
			int end = query2.value(1).toUInt();
			if (coding_only)
			{
				start = std::max(start, start_coding);
				end = std::min(end, end_coding);
				if (end<start_coding || start>end_coding) continue;
			}
			regions.append(BedLine(chr, start, end));
		}
		regions.merge();
		transcript.setRegions(regions);

		output.push_back(transcript);
	}

	return output;
}

Transcript NGSD::longestCodingTranscript(int gene_id, Transcript::SOURCE source, bool fallback_alt_source, bool fallback_alt_source_nocoding)
{
	QList<Transcript> list = transcripts(gene_id, source, true);
	Transcript::SOURCE alt_source = (source==Transcript::CCDS) ? Transcript::ENSEMBL : Transcript::CCDS;
	if (list.isEmpty() && fallback_alt_source)
	{
		list = transcripts(gene_id, alt_source, true);
	}
	if (list.isEmpty() && fallback_alt_source_nocoding)
	{
		list = transcripts(gene_id, alt_source, false);
	}

	if (list.isEmpty()) return Transcript();

	//get longest transcript (transcripts regions are merged!)
	auto max_it = std::max_element(list.begin(), list.end(), [](const Transcript& a, const Transcript& b){ return a.regions().baseCount() < b.regions().baseCount(); });
	return *max_it;
}

DiagnosticStatusData NGSD::getDiagnosticStatus(const QString& processed_sample_id)
{
	//get processed sample ID
	if (processed_sample_id=="") return DiagnosticStatusData();

	//get status data
	SqlQuery q = getQuery();
	q.exec("SELECT s.status, u.name, s.date, s.outcome, s.comment FROM diag_status as s, user as u WHERE s.processed_sample_id='" + processed_sample_id +  "' AND s.user_id=u.id");
	if (q.size()==0) return DiagnosticStatusData();

	//process
	q.next();
	DiagnosticStatusData output;
	output.dagnostic_status = q.value(0).toString();
	output.user = q.value(1).toString();
	output.date = q.value(2).toDateTime();
	output.outcome = q.value(3).toString();
	output.comments = q.value(4).toString();

	return output;
}

void NGSD::setDiagnosticStatus(const QString& processed_sample_id, DiagnosticStatusData status, QString user_name)
{
	//get current user ID
	QString user_id = QString::number(userId(user_name));

	//update status
	SqlQuery query = getQuery();
	query.prepare("INSERT INTO diag_status (processed_sample_id, status, user_id, outcome, comment) " \
					"VALUES ("+processed_sample_id+",'"+status.dagnostic_status+"', "+user_id+", '"+status.outcome+"', :0) " \
					"ON DUPLICATE KEY UPDATE status=VALUES(status), user_id=VALUES(user_id), outcome=VALUES(outcome), comment=VALUES(comment)"
					);
	query.bindValue(0, status.comments);
	query.exec();
}

int NGSD::reportConfigId(const QString& processed_sample_id)
{
	QVariant id = getValue("SELECT id FROM report_configuration WHERE processed_sample_id=:0", true, processed_sample_id);
	return id.isValid() ? id.toInt() : -1;
}

ReportConfigurationCreationData NGSD::reportConfigCreationData(int id)
{
	SqlQuery query = getQuery();
	query.exec("SELECT created_by, created_date, (SELECT name FROM user WHERE id=last_edit_by) as last_edit_by, last_edit_date FROM report_configuration WHERE id=" + QString::number(id));
	query.next();

	ReportConfigurationCreationData output;
	output.created_by = userName(query.value("created_by").toInt());
	QDateTime created_date = query.value("created_date").toDateTime();
	output.created_date = created_date.isNull() ? "" : created_date.toString("dd.MM.yyyy hh:mm:ss");
	output.last_edit_by = query.value("last_edit_by").toString();
	QDateTime last_edit_date = query.value("last_edit_date").toDateTime();
	output.last_edit_date = last_edit_date.isNull() ? "" : last_edit_date.toString("dd.MM.yyyy hh:mm:ss");

	return output;
}

ReportConfiguration NGSD::reportConfig(const QString& processed_sample_id, const VariantList& variants, const CnvList& cnvs, QStringList& messages)
{
	ReportConfiguration output;

	int conf_id = reportConfigId(processed_sample_id);
	if (conf_id==-1) THROW(DatabaseException, "Report configuration for processed sample with database id '" + processed_sample_id + "' does not exist!");

	//load main object
	SqlQuery query = getQuery();
	query.exec("SELECT u.name, rc.created_date FROM report_configuration rc, user u WHERE rc.id=" + QString::number(conf_id) + " AND u.id=rc.created_by");
	query.next();
	output.setCreatedBy(query.value("name").toString());
	output.setCreatedAt(query.value("created_date").toDateTime());

	//load variant data
	query.exec("SELECT * FROM report_configuration_variant WHERE report_configuration_id=" + QString::number(conf_id));
	while(query.next())
	{
		ReportVariantConfiguration var_conf;

		//get variant id
		Variant var = variant(query.value("variant_id").toString());
		for (int i=0; i<variants.count(); ++i)
		{
			if (var==variants[i])
			{
				var_conf.variant_index = i;
			}
		}
		if (var_conf.variant_index==-1)
		{
			messages << "Could not find variant '" + var.toString() + "' in given variant list!";
			continue;
		}

		var_conf.report_type = query.value("type").toString();
		var_conf.causal = query.value("causal").toBool();
		var_conf.inheritance = query.value("inheritance").toString();
		var_conf.de_novo = query.value("de_novo").toBool();
		var_conf.mosaic = query.value("mosaic").toBool();
		var_conf.comp_het = query.value("compound_heterozygous").toBool();
		var_conf.exclude_artefact = query.value("exclude_artefact").toBool();
		var_conf.exclude_frequency = query.value("exclude_frequency").toBool();
		var_conf.exclude_phenotype = query.value("exclude_phenotype").toBool();
		var_conf.exclude_mechanism = query.value("exclude_mechanism").toBool();
		var_conf.exclude_other = query.value("exclude_other").toBool();
		var_conf.comments = query.value("comments").toString();
		var_conf.comments2 = query.value("comments2").toString();

		output.set(var_conf);
	}

	//load CNV data
	query.exec("SELECT * FROM report_configuration_cnv WHERE report_configuration_id=" + QString::number(conf_id));
	while(query.next())
	{
		ReportVariantConfiguration var_conf;
		var_conf.variant_type = VariantType::CNVS;

		//get CNV id
		CopyNumberVariant var = cnv(query.value("cnv_id").toInt());
		for (int i=0; i<cnvs.count(); ++i)
		{
			if (cnvs[i].hasSamePosition(var))
			{
				var_conf.variant_index = i;
			}
		}
		if (var_conf.variant_index==-1)
		{
			messages << "Could not find CNV '" + var.toString() + "' in given variant list!";
			continue;
		}

		var_conf.report_type = query.value("type").toString();
		var_conf.causal = query.value("causal").toBool();
		var_conf.classification = query.value("class").toString();
		var_conf.inheritance = query.value("inheritance").toString();
		var_conf.de_novo = query.value("de_novo").toBool();
		var_conf.mosaic = query.value("mosaic").toBool();
		var_conf.comp_het = query.value("compound_heterozygous").toBool();
		var_conf.exclude_artefact = query.value("exclude_artefact").toBool();
		var_conf.exclude_frequency = query.value("exclude_frequency").toBool();
		var_conf.exclude_phenotype = query.value("exclude_phenotype").toBool();
		var_conf.exclude_mechanism = query.value("exclude_mechanism").toBool();
		var_conf.exclude_other = query.value("exclude_other").toBool();
		var_conf.comments = query.value("comments").toString();
		var_conf.comments2 = query.value("comments2").toString();

		output.set(var_conf);
	}

	return output;
}

int NGSD::setReportConfig(const QString& processed_sample_id, const ReportConfiguration& config, const VariantList& variants, const CnvList& cnvs, QString user_name)
{
	//create report config (if missing)
	int id = reportConfigId(processed_sample_id);
	if (id!=-1)
	{
		//delete report config variants if it already exists
		SqlQuery query = getQuery();
		query.exec("DELETE FROM `report_configuration_variant` WHERE report_configuration_id=" + QString::number(id));
		query.exec("DELETE FROM `report_configuration_cnv` WHERE report_configuration_id=" + QString::number(id));

		//update report config
		query.exec("UPDATE `report_configuration` SET `last_edit_by`='" + QString::number(userId(user_name)) + "', `last_edit_date`=CURRENT_TIMESTAMP WHERE id=" + QString::number(id));
	}
	else
	{
		//insert new report config
		int user_id = userId(config.createdBy());

		SqlQuery query = getQuery();
		query.prepare("INSERT INTO `report_configuration`(`processed_sample_id`, `created_by`, `created_date`, `last_edit_by`, `last_edit_date`) VALUES (:0, :1, :2, :3, CURRENT_TIMESTAMP)");
		query.bindValue(0, processed_sample_id);
		query.bindValue(1, user_id);
		query.bindValue(2, config.createdAt());
		query.bindValue(3, user_id);
		query.exec();
		id = query.lastInsertId().toInt();
	}

	//store variant data
	SqlQuery query_var = getQuery();
	query_var.prepare("INSERT INTO `report_configuration_variant`(`report_configuration_id`, `variant_id`, `type`, `causal`, `inheritance`, `de_novo`, `mosaic`, `compound_heterozygous`, `exclude_artefact`, `exclude_frequency`, `exclude_phenotype`, `exclude_mechanism`, `exclude_other`, `comments`, `comments2`) VALUES (:0, :1, :2, :3, :4, :5, :6, :7, :8, :9, :10, :11, :12, :13, :14)");
	SqlQuery query_cnv = getQuery();
	query_cnv.prepare("INSERT INTO `report_configuration_cnv`(`report_configuration_id`, `cnv_id`, `type`, `causal`, `class`, `inheritance`, `de_novo`, `mosaic`, `compound_heterozygous`, `exclude_artefact`, `exclude_frequency`, `exclude_phenotype`, `exclude_mechanism`, `exclude_other`, `comments`, `comments2`) VALUES (:0, :1, :2, :3, :4, :5, :6, :7, :8, :9, :10, :11, :12, :13, :14, :15)");
	foreach(const ReportVariantConfiguration& var_conf, config.variantConfig())
	{
		if (var_conf.variant_type==VariantType::SNVS_INDELS)
		{
			//check variant index exists in variant list
			if (var_conf.variant_index<0 || var_conf.variant_index>=variants.count())
			{
				THROW(ProgrammingException, "Variant list does not contain variant with index '" + QString::number(var_conf.variant_index) + "' in NGSD::setReportConfig!");
			}

			//check that classification is not set (only used for CNVs)
			if (var_conf.classification!="n/a" && var_conf.classification!="")
			{
				THROW(ProgrammingException, "Report configuration for small variant '" + variants[var_conf.variant_index].toString() + "' set, but not supported!");
			}

			//get variant id (add variant if not in DB)
			const Variant& variant = variants[var_conf.variant_index];
			QString variant_id = variantId(variant, false);
			if (variant_id=="")
			{
				variant_id = addVariant(variant, variants);
			}

			query_var.bindValue(0, id);
			query_var.bindValue(1, variant_id);
			query_var.bindValue(2, var_conf.report_type);
			query_var.bindValue(3, var_conf.causal);
			query_var.bindValue(4, var_conf.inheritance);
			query_var.bindValue(5, var_conf.de_novo);
			query_var.bindValue(6, var_conf.mosaic);
			query_var.bindValue(7, var_conf.comp_het);
			query_var.bindValue(8, var_conf.exclude_artefact);
			query_var.bindValue(9, var_conf.exclude_frequency);
			query_var.bindValue(10, var_conf.exclude_phenotype);
			query_var.bindValue(11, var_conf.exclude_mechanism);
			query_var.bindValue(12, var_conf.exclude_other);
			query_var.bindValue(13, var_conf.comments.isEmpty() ? "" : var_conf.comments);
			query_var.bindValue(14, var_conf.comments2.isEmpty() ? "" : var_conf.comments2);

			query_var.exec();
		}
		else if (var_conf.variant_type==VariantType::CNVS)
		{
			//check CNV index exists in CNV list
			if (var_conf.variant_index<0 || var_conf.variant_index>=cnvs.count())
			{
				THROW(ProgrammingException, "CNV list does not contain CNV with index '" + QString::number(var_conf.variant_index) + "' in NGSD::setReportConfig!");
			}

			//check that report CNV callset exists
			QVariant callset_id = getValue("SELECT id FROM cnv_callset WHERE processed_sample_id=" + processed_sample_id, true);
			if (!callset_id.isValid())
			{
				THROW(ProgrammingException, "No CNV callset defined for processed sample with ID '" + processed_sample_id + "' in NGSD::setReportConfig!");
			}

			//get CNV id (add CNV if not in DB)
			const CopyNumberVariant& cnv = cnvs[var_conf.variant_index];
			QString cnv_id = cnvId(cnv, callset_id.toInt(), false);
			if (cnv_id=="")
			{
				cnv_id = addCnv(callset_id.toInt(), cnv, cnvs);
			}

			query_cnv.bindValue(0, id);
			query_cnv.bindValue(1, cnv_id);
			query_cnv.bindValue(2, var_conf.report_type);
			query_cnv.bindValue(3, var_conf.causal);
			query_cnv.bindValue(4, var_conf.classification); //only for CNVs
			query_cnv.bindValue(5, var_conf.inheritance);
			query_cnv.bindValue(6, var_conf.de_novo);
			query_cnv.bindValue(7, var_conf.mosaic);
			query_cnv.bindValue(8, var_conf.comp_het);
			query_cnv.bindValue(9, var_conf.exclude_artefact);
			query_cnv.bindValue(10, var_conf.exclude_frequency);
			query_cnv.bindValue(11, var_conf.exclude_phenotype);
			query_cnv.bindValue(12, var_conf.exclude_mechanism);
			query_cnv.bindValue(13, var_conf.exclude_other);
			query_cnv.bindValue(14, var_conf.comments.isEmpty() ? "" : var_conf.comments);
			query_cnv.bindValue(15, var_conf.comments2.isEmpty() ? "" : var_conf.comments2);

			query_cnv.exec();

		}
		else
		{
			THROW(NotImplementedException, "Storing of report config variants with type '" + QString::number((int)var_conf.variant_type) + "' not implemented!");
		}

	}

	return id;
}

void NGSD::deleteReportConfig(int id)
{
	QString rc_id = QString::number(id);

	//check that it exists
	bool rc_exists = getValue("SELECT id FROM `report_configuration` WHERE `id`=" + rc_id).isValid();
	if (!rc_exists)
	{
		THROW (ProgrammingException, "Cannot delete report configuration with id=" + rc_id + ", because it does not exist!");
	}

	//delete
	SqlQuery query = getQuery();
	query.exec("DELETE FROM `report_configuration_cnv` WHERE `report_configuration_id`=" + rc_id);
	query.exec("DELETE FROM `report_configuration_variant` WHERE `report_configuration_id`=" + rc_id);
	query.exec("DELETE FROM `report_configuration` WHERE `id`=" + rc_id);
}

void NGSD::setProcessedSampleQuality(const QString& processed_sample_id, const QString& quality)
{
	getQuery().exec("UPDATE processed_sample SET quality='" + quality + "' WHERE id='" + processed_sample_id + "'");
}

GeneInfo NGSD::geneInfo(QByteArray symbol)
{
	GeneInfo output;

	//get approved symbol
	symbol = symbol.trimmed();
	auto approved = geneToApprovedWithMessage(symbol);
	output.symbol = approved.first;
	output.symbol_notice = approved.second;
	SqlQuery query = getQuery();
	query.prepare("SELECT name FROM gene WHERE symbol=:0");
	query.bindValue(0, output.symbol);
	query.exec();
	if (query.size()==0)
	{
		output.name = "";
	}
	else
	{
		query.next();
		output.name = query.value(0).toString();
	}

	query.prepare("SELECT inheritance, gnomad_oe_syn, gnomad_oe_mis, gnomad_oe_lof, comments FROM geneinfo_germline WHERE symbol=:0");
	query.bindValue(0, output.symbol);
	query.exec();
	if (query.size()==0)
	{
		output.inheritance = "n/a";
		output.oe_syn = "n/a";
		output.oe_mis = "n/a";
		output.oe_lof = "n/a";
		output.comments = "";
	}
	else
	{
		query.next();
		output.inheritance = query.value(0).toString();
		output.oe_syn = query.value(1).isNull() ? "n/a" : QString::number(query.value(1).toDouble(), 'f', 2);
		output.oe_mis = query.value(2).isNull() ? "n/a" : QString::number(query.value(2).toDouble(), 'f', 2);
		output.oe_lof = query.value(3).isNull() ? "n/a" : QString::number(query.value(3).toDouble(), 'f', 2);
		output.comments = query.value(4).toString();
	}

	return output;
}

void NGSD::setGeneInfo(GeneInfo info)
{
	SqlQuery query = getQuery();
	query.prepare("INSERT INTO geneinfo_germline (symbol, inheritance, gnomad_oe_syn, gnomad_oe_mis, gnomad_oe_lof, comments) VALUES (:0, :1, NULL, NULL, NULL, :2) ON DUPLICATE KEY UPDATE inheritance=VALUES(inheritance), comments=VALUES(comments)");
	query.bindValue(0, info.symbol);
	query.bindValue(1, info.inheritance);
	query.bindValue(2, info.comments);
	query.exec();
}

QString AnalysisJob::runTimeAsString() const
{
	//determine start time
	QDateTime start;
	QDateTime end = QDateTime::currentDateTime();
	foreach(const AnalysisJobHistoryEntry& entry, history)
	{
		if (entry.status=="queued")
		{
			start = entry.time;
		}
		if (entry.status=="error" || entry.status=="finished" || entry.status=="cancel" || entry.status=="canceled")
		{
			end = entry.time;
		}
	}

	//calculate sec, min, hour
	double s = start.secsTo(end);
	double m = floor(s/60.0);
	s -= 60.0 * m;
	double h = floor(m/60.0);
	m -= 60.0 * h;

	QStringList parts;
	if (h>0) parts << QString::number(h, 'f', 0) + "h";
	if (h>0 || m>0) parts << QString::number(m, 'f', 0) + "m";
	parts << QString::number(s, 'f', 0) + "s";

	return parts.join(" ");
}

QString ReportConfigurationCreationData::toText() const
{
	QStringList output;
	output << "The NGSD contains a report configuration created by " + created_by + " at " + created_date + ".";
	if (last_edit_by!="") output << "It was last updated by " + last_edit_by + " at " + last_edit_date + ".";
	return output.join("\n");
}

QString TableFieldInfo::toString() const
{
	return "TableFieldInfo(" + name + "): index=" + QString::number(index) + "  type=" + typeToString(type) +" is_nullable=" + (is_nullable ? "yes" : "no") + " is_unsigned=" + (is_unsigned ? "yes" : "no") + " default_value: " + default_value + " is_primary_key=" + (is_primary_key ? "yes" : "no") + " fk_table=" + fk_table + " fk_field=" + fk_field;
}

QString TableFieldInfo::typeToString(TableFieldInfo::Type type)
{
	switch(type)
	{
		case BOOL:
			return "BOOL";
			break;
		case INT:
			return "INT";
			break;
		case FLOAT:
			return "FLOAT";
			break;
		case TEXT:
			return "TEXT";
			break;
		case VARCHAR:
			return "VARCHAR";
			break;
		case VARCHAR_PASSWORD:
			return "VARCHAR_PASSWORD";
			break;
		case ENUM:
			return"ENUM";
			break;
		case DATE:
			return "DATE";
			break;
		case DATETIME:
			return "DATETIME";
			break;
		case TIMESTAMP:
			return "TIMESTAMP";
			break;
		case FK:
			return "FK";
			break;
		default:
			THROW(NotImplementedException, "Unhandled type '" + QString::number(type) + "' in TableFieldInfo::typeToString!");
	}
}

QStringList NGSD::checkValue(const QString& table, const QString& field, const QString& value, bool check_unique) const
{
	QStringList errors;

	const TableFieldInfo& field_info = tableInfo(table).fieldInfo(field);

	switch(field_info.type)
	{
		case TableFieldInfo::INT:
			//check null
			if (value.isEmpty() && !field_info.is_nullable)
			{
				errors << "Cannot be empty!";
			}

			//check if numeric
			if (!value.isEmpty())
			{
				bool ok = true;
				int value_numeric = value.toInt(&ok);
				if (!ok)
				{
					errors << "Cannot be converted to a integer number!";
				}
				else if (field_info.is_unsigned && value_numeric<0)
				{
					errors << "Must not be negative!";
				}
			}
			break;

		case TableFieldInfo::FLOAT:
			//check null
			if (value.isEmpty() && !field_info.is_nullable)
			{
				errors << "Cannot be empty!";
			}

			//check if numeric
			if (!value.isEmpty())
			{
				bool ok = true;
				double value_numeric = value.toDouble(&ok);
				if (!ok)
				{
					errors << "Cannot be converted to a floating-point number!";
				}
				else if (field_info.is_unsigned && value_numeric<0)
				{
					errors << "Must not be negative!";
				}
			}
			break;

		case TableFieldInfo::DATE:
			//check null
			if (value.isEmpty() && !field_info.is_nullable)
			{
				errors << "Cannot be empty!";
			}

			//check if valid date
			if (!value.isEmpty())
			{
				QDate date = QDate::fromString(value, Qt::ISODate);
				if (!date.isValid())
				{
					errors << "Invalid format! The correct format is YYYY-MM-DD";
				}
			}
			break;

		case TableFieldInfo::VARCHAR:
			//check not empty
			if (!field_info.is_nullable && value.isEmpty())
			{
				errors << "Field must not be empty!";
			}

			//check length
			if (value.length()>field_info.type_constraints.max_length)
			{
				errors << "Maximum length is " + QString::number(field_info.type_constraints.max_length);
			}

			//check regexp
			if (!field_info.type_constraints.regexp.pattern().isEmpty())
			{
				//qDebug() << field_info.name << value << field_info.type_constraints.regexp.pattern();
				if (!value.contains(field_info.type_constraints.regexp))
				{
					errors << "Regular expression mismatch of value '" + value + "' (pattern=" + field_info.type_constraints.regexp.pattern() + ")";
				}
			}

			//check unique
			if (check_unique && field_info.is_unique)
			{
				SqlQuery q = getQuery();
				q.prepare("SELECT id FROM " + table + " WHERE " + field + "=:0");
				q.bindValue(0, value);
				q.exec();
				if (q.size()>0)
				{
					errors << "Value already present in database (this field is unique!)";
				}
			}
			break;

		case TableFieldInfo::VARCHAR_PASSWORD:
			//check not empty
			if (!field_info.is_nullable && value.isEmpty())
			{
				errors << "Field must not be empty!";
			}

			//check length
			if (value.length()>field_info.type_constraints.max_length)
			{
				errors << "Maximum length is " + QString::number(field_info.type_constraints.max_length);
			}
			if (value.length()<8)
			{
				errors << "Minimum length is 8";
			}

			//check composition
			{
				bool has_upper = false;
				bool has_lower = false;
				bool has_digit = false;
				bool has_symbol = false;
				foreach (const QChar& character, value)
				{
					if (character.isUpper())
					{
						has_upper = true;
					}
					else if (character.isLower())
					{
						has_lower = true;
					}
					else if (character.isDigit())
					{
						has_digit = true;
					}
					else
					{
						has_symbol = true;
					}
				}
				if (!has_lower || !has_upper || !has_digit || !has_symbol)
				{
					errors << "Must contains at least one letter of each class: upper-case, lower-case, digit, symbol";
				}
			}
			break;

		case TableFieldInfo::TEXT:
			//nothing to check
			break;

		case TableFieldInfo::BOOL:
			if (value!="0" && value!="1")
			{
				errors << "Can only be '0' or '1'!";
			}
			break;

		case TableFieldInfo::ENUM:
			//check null
			if (value.isEmpty())
			{
				if (!field_info.is_nullable)
				{
					errors << "Cannot be empty!";
				}
			}

			//values
			if (!value.isEmpty())
			{
				if (!field_info.type_constraints.valid_strings.contains(value))
				{
					errors << "Invalid value '" + value + "'. Valid are: '" + field_info.type_constraints.valid_strings.join("', '") + "'";
				}
			}
			break;

		case TableFieldInfo::DATETIME:
			//check null
			if (value.isEmpty() && !field_info.is_nullable)
			{
				errors << "Cannot be empty!";
			}

			//check if valid date and time
			if (!value.isEmpty())
			{
				QDateTime date = QDateTime::fromString(value, "YYYY-MM-DD HH:mm:SS");
				if (!date.isValid())
				{
					errors << "Invalid format! The correct format is YYYY-MM-DD HH:mm:SS";
				}
			}
			break;

		case TableFieldInfo::TIMESTAMP:
			//check null
			if (value.isEmpty() && !field_info.is_nullable)
			{
				errors << "Cannot be empty!";
			}

			//check if valid date and time
			if (!value.isEmpty())
			{
				QDateTime date = QDateTime::fromString(value, "YYYY-MM-DD HH:mm:SS");
				if (!date.isValid())
				{
					errors << "Invalid format! The correct format is YYYY-MM-DD HH:mm:SS";
				}
			}
			break;

		case TableFieldInfo::FK:
			//check null
			if (value.isEmpty() && !field_info.is_nullable)
			{
				errors << "Cannot be empty!";
			}

			//check valid foreign key
			if (!value.isEmpty())
			{
				QString name = getValue("SELECT " + field_info.fk_name_sql + " FROM " + field_info.fk_table + " WHERE " + field_info.fk_field + "=:0", true, value).toString();
				if (name.isEmpty())
				{
					errors << "Invalid foreign-key reference! Table " + field_info.fk_table + " does not contains a row with " + field_info.fk_field + "=" + value;
				}
			}
			break;

		default:
			THROW(ProgrammingException, "Unhandled table field type '" + QString::number(field_info.type) + "' in check of table/field " + table + "/" + field);
	}

	return errors;
}
