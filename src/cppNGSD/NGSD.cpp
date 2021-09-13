#include "NGSD.h"
#include "Exceptions.h"
#include "Helper.h"
#include "Log.h"
#include "Settings.h"
#include "ChromosomalIndex.h"
#include "NGSHelper.h"
#include "FilterCascade.h"
#include "LoginManager.h"
#include <QFileInfo>
#include <QPair>
#include <QSqlDriver>
#include <QSqlIndex>
#include <QSqlField>
#include <QSqlError>
#include <QJsonDocument>
#include <QJsonObject>
#include <QCryptographicHash>
#include <QDir>
#include "cmath"

NGSD::NGSD(bool test_db, bool hg38)
	: test_db_(test_db)
{
	db_.reset(new QSqlDatabase(QSqlDatabase::addDatabase("QMYSQL", "NGSD_" + Helper::randomString(20))));

	//connect to DB
	QString prefix = "ngsd";
	if (test_db_) prefix += "_test";
	if (hg38) prefix += "_hg38";
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

int NGSD::userId(QString user_name, bool only_active, bool throw_if_fails)
{
	// don't fail if user name is empty
	if (user_name == "")
	{
		if (throw_if_fails) THROW(DatabaseException, "Could not determine NGSD user ID for empty user name!")
		else return -1;
	}

	//check user exists
	bool ok = true;
	int user_id = getValue("SELECT id FROM user WHERE user_id=:0", true, user_name).toInt(&ok);
	if (!ok)
	{
		user_id = getValue("SELECT id FROM user WHERE name=:0", true, user_name).toInt(&ok);
	}
	if (!ok)
	{
		if (throw_if_fails) THROW(DatabaseException, "Could not determine NGSD user ID for user name '" + user_name + "!")
		else user_id = -1;
	}

	//check user is active
	if (only_active && !getValue("SELECT active FROM user WHERE id=" + QString::number(user_id), false).toBool())
	{
		if (throw_if_fails) THROW(DatabaseException, "User with user name '" + user_name + " is no longer active!")
		else user_id = -1;
	}

	return user_id;
}

QString NGSD::userName(int user_id)
{
	return getValue("SELECT name FROM user WHERE id=:0", false, QString::number(user_id)).toString();
}

QString NGSD::userLogin(int user_id)
{
	return getValue("SELECT user_id FROM user WHERE id=:0", false, QString::number(user_id)).toString();
}

QString NGSD::userEmail(int user_id)
{
	return getValue("SELECT email FROM user WHERE id=:0", false,  QString::number(user_id)).toString();
}

const QString& NGSD::passwordReplacement()
{
	static QString output = "********";

	return output;
}

QString NGSD::checkPassword(QString user_name, QString password, bool only_active)
{
	//check user id
	QString id = getValue("SELECT id FROM user WHERE user_id=:0", true, user_name).toString();
	if (id.isEmpty())
	{
		return "User '" + user_name + "' does not exist!";
	}

	//check user is active
	if (only_active)
	{
		QString active = getValue("SELECT active FROM user WHERE id=:0", false, id).toString();
		if (active=="0")
		{
			return "User '" + user_name + "' is no longer active!";
		}
	}

	//check password
	QString salt = getValue("SELECT salt FROM user WHERE id=:0", false, id).toString();
	if (salt.isEmpty()) salt = user_name; //needed for backward compatibility
	QByteArray hash = QCryptographicHash::hash((salt+password).toUtf8(), QCryptographicHash::Sha1).toHex();
	//qDebug() << user_name << salt << hash;
	if (hash!=getValue("SELECT password FROM user WHERE id=:0", false, id).toString())
	{
		return "Invalid password for user '" + user_name + "'!";
	}

	return "";
}

void NGSD::setPassword(int user_id, QString password)
{
	QString salt = Helper::randomString(40);
	QString hash = QCryptographicHash::hash((salt+password).toUtf8(), QCryptographicHash::Sha1).toHex();

	getQuery().exec("UPDATE user SET password='" + hash + "', salt='" + salt + "' WHERE id=" + QString::number(user_id));
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
			<< "psa.population as ancestry"
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
			<< "processed_sample ps LEFT JOIN sequencing_run r ON r.id=ps.sequencing_run_id LEFT JOIN diag_status ds ON ds.processed_sample_id=ps.id LEFT JOIN processed_sample_ancestry psa ON psa.processed_sample_id=ps.id"; //sequencing_run and diag_status are optional

	QStringList conditions;
	conditions	<< "ps.sample_id=s.id"
				<< "ps.processing_system_id=sys.id"
				<< "ps.project_id=p.id";

	//add filters (sample)
	if (p.s_name.trimmed()!="")
	{
		QStringList name_conditions;
		name_conditions << "s.name LIKE '%" + escapeForSql(p.s_name) + "%'";
		if (p.s_name_ext)
		{
			name_conditions << "s.name_external LIKE '%" + escapeForSql(p.s_name) + "%'";
		}
		if (p.s_name_comments)
		{
			name_conditions << "s.comment LIKE '%" + escapeForSql(p.s_name) + "%'";
			name_conditions << "ps.comment LIKE '%" + escapeForSql(p.s_name) + "%'";
		}
		conditions << "(" + name_conditions.join(" OR ") + ")";
	}
	if (p.s_species.trimmed()!="")
	{
		tables	<< "species sp";
		conditions	<< "sp.id=s.species_id"
					<< "sp.name='" + escapeForSql(p.s_species) + "'";
	}
	if (p.s_sender.trimmed()!="")
	{
		tables	<< "sender se";
		conditions	<< "se.id=s.sender_id"
					<< "se.name='" + escapeForSql(p.s_sender) + "'";
	}
	if (p.s_study.trimmed()!="")
	{
		tables	<< "study st";
		tables	<< "study_sample sts";
		conditions	<< "st.id=sts.study_id"
					<< "sts.processed_sample_id=ps.id"
					<< "st.name='" + escapeForSql(p.s_study) + "'";
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
	if (p.r_before.isValid())
	{
		conditions << "r.start_date<='" + p.r_before.toString(Qt::ISODate)+"'";
	}
	if (p.r_device_name.trimmed()!="")
	{
		tables << "device d";
		conditions << "d.id=r.device_id"
				   << "d.name LIKE '%" + escapeForSql(p.r_device_name) + "%'";
	}

	//add comments
	if (p.add_comments)
	{
		fields	<< "s.comment as comment_sample"
				<< "ps.comment as comment_processed_sample";
	}

	//add outcome
	if (p.add_outcome)
	{
		fields	<< "ds.outcome as outcome"
				<< "ds.comment as outcome_comment";
	}

	DBTable output = createTable("processed_sample", "SELECT " + fields.join(", ") + " FROM " + tables.join(", ") +" WHERE " + conditions.join(" AND ") + " ORDER BY s.name ASC, ps.process_id ASC");

	//add path
	if(!p.add_path.isEmpty())
	{
		QStringList new_col;
		for (int r=0; r<output.rowCount(); ++r)
		{
			new_col << processedSamplePath(output.row(r).id(), FileLocation::stringToType(p.add_path));
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

					QString entry = disease_query.value(1).toString().replace('\r', ' ').replace('\n', ' ');
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
			const QString& ps_id = output.row(r).id();
			report_conf_col << reportConfigSummaryText(ps_id);
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
	output.phenotypes = samplePhenotypes(sample_id);
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
	output.ancestry = getValue("SELECT `population` FROM `processed_sample_ancestry` WHERE `processed_sample_id`=:0", true, processed_sample_id).toString();

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

QStringList NGSD::sameSamples(QString sample_id, QString sample_type)
{
	QStringList valid_sample_types = getEnum("sample", "sample_type");
	if (!valid_sample_types.contains(sample_type))
	{
		THROW(ArgumentException, "Invalid sample type '" + sample_type + "'!");
	}
	QStringList all_same_samples;
	SqlQuery query = getQuery();
	query.exec("SELECT sample2_id FROM sample_relations WHERE relation='same sample' AND sample1_id='" + sample_id + "'");
	while (query.next())
	{
		all_same_samples.append(query.value(0).toString());
	}
	query.exec("SELECT sample1_id FROM sample_relations WHERE relation='same sample' AND sample2_id='" + sample_id + "'");
	while (query.next())
	{
		all_same_samples.append(query.value(0).toString());
	}

	QStringList filtered_same_samples;
	// filter same samples by type
	foreach(const QString& same_sample_id, all_same_samples)
	{
		if (getSampleData(same_sample_id).type == sample_type)
		{
			filtered_same_samples.append(same_sample_id);
		}
	}

	return filtered_same_samples;
}

void NGSD::setSampleDiseaseData(const QString& sample_id, const QString& disease_group, const QString& disease_status)
{
	getQuery().exec("UPDATE sample SET disease_group='" + disease_group + "', disease_status='" + disease_status + "' WHERE id='" + sample_id + "'");
}

PhenotypeList NGSD::samplePhenotypes(const QString& sample_id, bool throw_on_error)
{
	PhenotypeList output;

	QStringList hpo_ids = getValues("SELECT disease_info FROM sample_disease_info WHERE type='HPO term id' AND sample_id=" + sample_id);
	foreach(const QString& hpo_id, hpo_ids)
	{
		int id = phenotypeIdByAccession(hpo_id.toLatin1(), throw_on_error);
		if (id!=-1)
		{
			output << phenotype(id);
		}
	}

	return output;
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

ProcessingSystemData NGSD::getProcessingSystemData(int sys_id)
{
	ProcessingSystemData output;

	SqlQuery query = getQuery();
	query.exec("SELECT sys.name_manufacturer, sys.name_short, sys.type, sys.adapter1_p5, sys.adapter2_p7, sys.shotgun, sys.umi_type, g.build FROM processing_system sys, genome g WHERE sys.genome_id=g.id AND sys.id=" + QString::number(sys_id));
	query.next();

	output.name = query.value(0).toString();
	output.name_short = query.value(1).toString();
	output.type = query.value(2).toString();
	output.adapter1_p5 = query.value(3).toString();
	output.adapter2_p7 = query.value(4).toString();
	output.shotgun = query.value(5).toString()=="1";
	output.umi_type = query.value(6).toString();
	output.genome = query.value(7).toString();

	return output;
}

BedFile NGSD::processingSystemRegions(int sys_id)
{
	BedFile output;

	QString rel_path = getValue("SELECT target_file FROM processing_system WHERE id=" + QString::number(sys_id)).toString().trimmed();
	if (!rel_path.isEmpty())
	{
		output.load(getTargetFilePath() + rel_path);
	}

	return output;
}

BedFile NGSD::processingSystemAmplicons(int sys_id)
{
	BedFile output;

	QString rel_path = getValue("SELECT target_file FROM processing_system WHERE id=" + QString::number(sys_id)).toString().trimmed();
	if (!rel_path.isEmpty())
	{
		QString amplicon_file = getTargetFilePath() + rel_path.mid(0, rel_path.length() -4) + "_amplicons.bed";
		if (QFile::exists(amplicon_file))
		{
			output.load(amplicon_file);
		}
	}

	return output;
}

GeneSet NGSD::processingSystemGenes(int sys_id)
{
	GeneSet output;

	QString rel_path = getValue("SELECT target_file FROM processing_system WHERE id=" + QString::number(sys_id)).toString().trimmed();
	if (!rel_path.isEmpty())
	{
		QString gene_file = getTargetFilePath() + rel_path.mid(0, rel_path.length() -4) + "_genes.txt";

		if (QFile::exists(gene_file))
		{
			output = GeneSet::createFromFile(gene_file);
		}
	}

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
	query.prepare("SELECT CONCAT(s.name,'_',LPAD(ps.process_id,2,'0')), p.type, p.name, sys.name_short FROM processed_sample ps, sample s, project p, processing_system sys WHERE ps.processing_system_id=sys.id AND ps.sample_id=s.id AND ps.project_id=p.id AND ps.id=:0");
	query.bindValue(0, processed_sample_id);
	query.exec();
	if (query.size()==0) THROW(DatabaseException, "Processed sample with id '" + processed_sample_id + "' not found in NGSD!");
	query.next();

	//create sample folder
	QString output = Settings::path("projects_folder");
	QString ps_name = query.value(0).toString();
	QString p_type = query.value(1).toString();
	output += p_type;
	QString p_name = query.value(2).toString();
	output += QDir::separator() + p_name + QDir::separator() + "Sample_" + ps_name + QDir::separator();
	QString sys_name_short = query.value(3).toString();

	//append file name if requested
	if (type==PathType::BAM) output += ps_name + ".bam";
	else if (type==PathType::GSVAR) output += ps_name + ".GSvar";
	else if (type==PathType::VCF) output += ps_name + "_var_annotated.vcf.gz";
	else if (type==PathType::VCF_CF_DNA) output += ps_name + "_var.vcf";
	else if (type==PathType::LOWCOV_BED) output += ps_name + "_" + sys_name_short + "_lowcov.bed";
	else if (type==PathType::MANTA_EVIDENCE) output += "manta_evid/" + ps_name + "_manta_evidence.bam";
	else if (type==PathType::BAF) output += ps_name + "_bafs.igv";
	else if (type==PathType::STRUCTURAL_VARIANTS) output += ps_name + "_manta_var_structural.bedpe";
	else if (type==PathType::COPY_NUMBER_RAW_DATA) output += ps_name + "_cnvs_clincnv.seg";
	else if (type==PathType::COPY_NUMBER_CALLS) output += ps_name + "_cnvs_clincnv.tsv";
	else if (type==PathType::FUSIONS) output += ps_name + "_var_fusions.tsv";
	else if (type==PathType::MANTA_FUSIONS) output +=  ps_name + "_var_fusions_manta.bedpe";
	else if (type==PathType::VIRAL) output += ps_name + "_viral.tsv";
	else if (type==PathType::COUNTS) output += ps_name + "_counts.tsv";
	else if (type!=PathType::SAMPLE_FOLDER) THROW(ProgrammingException, "Unhandled PathType '" + FileLocation::typeToString(type) + "' in processedSamplePath!");

	return QFileInfo(output).absoluteFilePath();
}

QStringList NGSD::secondaryAnalyses(QString processed_sample_name, QString analysis_type)
{
	//init
	QString project_folder = Settings::path("projects_folder");
	QStringList project_types = getEnum("project", "type");

	//convert to platform-specific canonical path
	QStringList output = getValues("SELECT gsvar_file FROM secondary_analysis WHERE type='" + analysis_type + "' AND gsvar_file LIKE '%" + processed_sample_name + "%'");
	for (int i=0; i<output.count(); ++i)
	{
		QString file = output[i];

		foreach(QString project_type, project_types)
		{
			QStringList parts = file.split("/" + project_type + "/");
			if (parts.count()==2)
			{
				file = project_folder + project_type + QDir::separator() + parts[1];
				break;
			}
		}

		//convert to canonical path
		file = QFileInfo(file).absoluteFilePath();

		output[i] = file;
	}

	return output;
}

QString NGSD::addVariant(const Variant& variant, const VariantList& variant_list)
{
	SqlQuery query = getQuery(); //use binding (user input)
	query.prepare("INSERT INTO variant (chr, start, end, ref, obs, 1000g, gnomad, coding) VALUES (:0,:1,:2,:3,:4,:5,:6,:7)");
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
	idx = variant_list.annotationIndexByName("coding_and_splicing");
	query.bindValue(7, variant.annotations()[idx]);
	query.exec();

	return query.lastInsertId().toString();
}

QList<int> NGSD::addVariants(const VariantList& variant_list, double max_af, int& c_add, int& c_update)
{
	QList<int> output;

	//prepare queried
	SqlQuery q_id = getQuery();
	q_id.prepare("SELECT id, 1000g, gnomad, coding, cadd, spliceai FROM variant WHERE chr=:0 AND start=:1 AND end=:2 AND ref=:3 AND obs=:4");

	SqlQuery q_update = getQuery(); //use binding (user input)
	q_update.prepare("UPDATE variant SET 1000g=:0, gnomad=:1, coding=:2, cadd=:3, spliceai=:4 WHERE id=:5");

	SqlQuery q_insert = getQuery(); //use binding (user input)
	q_insert.prepare("INSERT IGNORE INTO variant (chr, start, end, ref, obs, 1000g, gnomad, coding, cadd, spliceai) VALUES (:0,:1,:2,:3,:4,:5,:6,:7,:8,:9)");

	//get annotated column indices
	int i_tg = variant_list.annotationIndexByName("1000g");
	int i_gnomad = variant_list.annotationIndexByName("gnomAD");
	int i_co_sp = variant_list.annotationIndexByName("coding_and_splicing");
	int i_cadd = variant_list.annotationIndexByName("CADD");
	int i_spliceai = variant_list.annotationIndexByName("SpliceAI");

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

		QByteArray cadd = variant.annotations()[i_cadd].trimmed();
		QByteArray spliceai = variant.annotations()[i_spliceai].trimmed();

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
				|| q_id.value(3).toByteArray()!=variant.annotations()[i_co_sp]
				|| q_id.value(4).toByteArray().toDouble()!=cadd.toDouble() //numeric comparison (NULL > "" > 0.0)
				|| q_id.value(5).toByteArray().toDouble()!=spliceai.toDouble() //numeric comparison (NULL > "" > 0.0)
				)
			{
				q_update.bindValue(0, tg.isEmpty() ? QVariant() : tg);
				q_update.bindValue(1, gnomad.isEmpty() ? QVariant() : gnomad);
				q_update.bindValue(2, variant.annotations()[i_co_sp]);
				q_update.bindValue(3, cadd.isEmpty() ? QVariant() : cadd);
				q_update.bindValue(4, spliceai.isEmpty() ? QVariant() : spliceai);
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
			q_insert.bindValue(7, variant.annotations()[i_co_sp]);
			q_insert.bindValue(8, cadd.isEmpty() ? QVariant() : cadd);
			q_insert.bindValue(9, spliceai.isEmpty() ? QVariant() : spliceai);
			q_insert.exec();
			++c_add;
			QVariant last_insert_id = q_insert.lastInsertId();
			if (last_insert_id.isValid())
			{
				output << last_insert_id.toInt();
			}
			else //the variant was inserted by another query between checking and inserting here. Thus the insert statement was ignored. In this case "lastInsertId()" does not return the actual ID > we need to get the variant id manually
			{
				output << variantId(variant).toInt();
			}
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

QPair<int, int> NGSD::variantCounts(const QString& variant_id, bool use_cached_data_from_variant_table)
{
	//get same sample information (cached)
	QHash<int, QList<int>>& same_samples = getCache().same_samples;
	if (same_samples.isEmpty())
	{
		SqlQuery query = getQuery();
		query.exec("SELECT sample1_id, sample2_id FROM sample_relations WHERE relation='same sample' OR relation='same patient'");
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

	if (use_cached_data_from_variant_table)
	{
		SqlQuery query = getQuery();
		query.exec("SELECT germline_het, germline_hom FROM variant WHERE id=" + variant_id);
		query.next();

		count_het = query.value(0).toInt();
		count_hom = query.value(1).toInt();
	}
	else
	{
		QSet<int> samples_done_het;
		QSet<int> samples_done_hom;
		SqlQuery query = getQuery();
		query.exec("SELECT ps.sample_id, dv.genotype FROM detected_variant dv, processed_sample ps WHERE dv.variant_id='" + variant_id + "' AND dv.processed_sample_id=ps.id");
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
	}

	return qMakePair(count_het, count_hom);
}

void NGSD::deleteSomaticVariants(QString t_ps_id, QString n_ps_id)
{
	deleteSomaticVariants(t_ps_id, n_ps_id, VariantType::SNVS_INDELS);
	deleteSomaticVariants(t_ps_id, n_ps_id, VariantType::CNVS);
}

void NGSD::deleteSomaticVariants(QString t_ps_id, QString n_ps_id, VariantType type)
{
	if(type==VariantType::SNVS_INDELS)
	{
		getQuery().exec("DELETE FROM detected_somatic_variant WHERE processed_sample_id_tumor=" + t_ps_id + " AND processed_sample_id_normal=" +n_ps_id);
	}
	else if(type == VariantType::CNVS)
	{
		QString callset_id = getValue("SELECT id FROM somatic_cnv_callset WHERE ps_tumor_id=" + t_ps_id + " AND ps_normal_id=" + n_ps_id).toString();
		if(callset_id != "")
		{
			getQuery().exec("DELETE FROM somatic_cnv WHERE somatic_cnv_callset_id=" + callset_id);
			getQuery().exec("DELETE FROM somatic_cnv_callset WHERE id=" + callset_id);
		}
	}
	else
	{
		THROW(NotImplementedException, "Deleting somatic variants of type '" + QString::number((int)type) + "' not implemented!");
	}
}

void NGSD::deleteVariants(const QString& ps_id)
{
	deleteVariants(ps_id, VariantType::SNVS_INDELS);
	deleteVariants(ps_id, VariantType::CNVS);
	deleteVariants(ps_id, VariantType::SVS);
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
	else if (type==VariantType::SVS)
	{
		QString callset_id = getValue("SELECT id FROM sv_callset WHERE processed_sample_id=" + ps_id).toString();
		if (callset_id!="")
		{
			getQuery().exec("DELETE FROM sv_deletion WHERE sv_callset_id='" + callset_id + "'");
			getQuery().exec("DELETE FROM sv_duplication WHERE sv_callset_id='" + callset_id + "'");
			getQuery().exec("DELETE FROM sv_insertion WHERE sv_callset_id='" + callset_id + "'");
			getQuery().exec("DELETE FROM sv_inversion WHERE sv_callset_id='" + callset_id + "'");
			getQuery().exec("DELETE FROM sv_translocation WHERE sv_callset_id='" + callset_id + "'");
			getQuery().exec("DELETE FROM sv_callset WHERE id='" + callset_id + "'");
		}
	}
	else
	{
		THROW(NotImplementedException, "Deleting variants of type '" + QString::number((int)type) + "' not implemented!");
	}
}

QString NGSD::somaticCnvId(const CopyNumberVariant &cnv, int callset_id, bool throw_if_fails)
{
	SqlQuery query = getQuery();
	query.prepare("SELECT id FROM somatic_cnv WHERE somatic_cnv_callset_id=:0 AND chr=:1 AND start=:2 AND end=:3");
	query.bindValue(0, callset_id);
	query.bindValue(1, cnv.chr().strNormalized(true));
	query.bindValue(2, cnv.start());
	query.bindValue(3, cnv.end());
	query.exec();
	if(!query.next())
	{
		if(throw_if_fails)
		{
			THROW(DatabaseException, "Somatic CNV " + cnv.toString() + " with somatic callset id '" + QString::number(callset_id) + "' not found in NGSD!");
		}
		else
		{
			return "";
		}
	}
	return query.value(0).toString();
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
			THROW(DatabaseException, "CNV " + cnv.toString() + " if callset with id '" + QString::number(callset_id) + "' not found in NGSD!");
		}
		else
		{
			return "";
		}
	}

	return query.value(0).toString();
}


CopyNumberVariant NGSD::somaticCnv(int cnv_id)
{
	SqlQuery query = getQuery();
	query.exec("SELECT * FROM somatic_cnv WHERE id='" + QString::number(cnv_id)  + "'");
	if(!query.next()) THROW(DatabaseException, "Somatic CNV with identifier '" + QString::number(cnv_id) + "' does not exist!");

	return CopyNumberVariant(query.value("chr").toByteArray(), query.value("start").toInt(), query.value("end").toInt());
}

CopyNumberVariant NGSD::cnv(int cnv_id)
{
	SqlQuery query = getQuery();
	query.exec("SELECT * FROM cnv WHERE id='" + QString::number(cnv_id) + "'");
	if (!query.next()) THROW(DatabaseException, "CNV with identifier '" + QString::number(cnv_id) + "' does not exist!");

	return CopyNumberVariant(query.value("chr").toByteArray(), query.value("start").toInt(), query.value("end").toInt());
}

QString NGSD::addSomaticCnv(int callset_id, const CopyNumberVariant &cnv, const CnvList &cnv_list, double max_ll)
{
	if(cnv_list.type() != CnvListType::CLINCNV_TUMOR_NORMAL_PAIR)
	{
		THROW(ProgrammingException, "NGSD::addSomaticCnv can only be used with tumor-normal ClinCNV data.");
	}

	QJsonObject quality_metrics;
	quality_metrics.insert("regions", QString::number(cnv.regions()));

	//Quality metrics to be included (determinend from columns in CNV file)
	const QList<QString> qc_metric_cols = {"major_CN_allele", "minor_CN_allele", "loglikelihood", "regions", "Ontarget_RD_CI_lower", "Ontarget_RD_CI_upper","Offtarget_RD_CI_lower",
								 "Offtarget_RD_CI_upper", "Lowmed_tumor_BAF", "Highmed_tumor_BAF", "BAF_qval_fdr", "Overall_qvalue"};

	for(int i=0; i<cnv_list.annotationHeaders().count(); ++i)
	{
		const QByteArray& col_name = cnv_list.annotationHeaders()[i];
		const QByteArray& entry = cnv.annotations()[i];

		if(qc_metric_cols.contains(col_name))
		{
			if(col_name == "loglikelihood")
			{
				if (max_ll>0.0 && Helper::toDouble(entry, "log-likelihood")<max_ll)
				{
					return "";
				}
			}
			quality_metrics.insert(QString(col_name), QString(entry));
		}
	}


	int tumor_cn = cnv.annotations()[cnv_list.annotationIndexByName("tumor_CN_change", true)].toInt();
	double clonality = cnv.annotations()[cnv_list.annotationIndexByName("tumor_clonality", true)].toDouble();
	double normal_cn = cnv.annotations()[cnv_list.annotationIndexByName("CN_change", true)].toDouble();

	//Insert data
	SqlQuery query = getQuery();
	query.prepare("INSERT INTO `somatic_cnv` (`somatic_cnv_callset_id`, `chr`, `start`, `end`, `cn`, `tumor_cn`, `tumor_clonality`, `quality_metrics`) VALUES (:0, :1, :2, :3, :4, :5, :6, :7)");

	query.bindValue(0, callset_id);
	query.bindValue(1, cnv.chr().strNormalized(true));
	query.bindValue(2, cnv.start());
	query.bindValue(3, cnv.end());
	query.bindValue(4, normal_cn);
	query.bindValue(5, tumor_cn);
	query.bindValue(6, clonality);
	QJsonDocument json_doc;
	json_doc.setObject(quality_metrics);
	query.bindValue(7, json_doc.toJson(QJsonDocument::Compact));
	query.exec();

	//return ID of inserted somatic CNV
	return query.lastInsertId().toString();

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
			THROW(ProgrammingException, "CNV caller type not handled in NGSD::addCnv");
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
	// skip SVs on special chr
	if (!sv.chr1().isNonSpecial() || !sv.chr2().isNonSpecial() )
	{
		THROW(ArgumentException, "Structural variants on special chromosomes can not be added to the NGSD!");
		return -1;
	}
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
		if (sv.chr1() != sv.chr2()) THROW(ArgumentException, "Invalid insertion position:'" + sv.position1() + ", " + sv.position2() + "'!");
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

		// determine position and upper CI:
		int pos = std::min(std::min(sv.start1(), sv.start2()), std::min(sv.end1(), sv.end2()));
		int ci_upper = std::max(std::max(sv.start1(), sv.start2()), std::max(sv.end1(), sv.end2())) - pos;
		// insert SV into the NGSD
		SqlQuery query = getQuery();
		query.prepare(QByteArray() + "INSERT INTO `sv_insertion` (`sv_callset_id`, `chr`, `pos`, `ci_upper`, `inserted_sequence`, "
					  + "`known_left`, `known_right`, `quality_metrics`) VALUES (:0, :1,  :2, :3, :4, :5, :6, :7)");
		query.bindValue(0, callset_id);
		query.bindValue(1, sv.chr1().strNormalized(true));
		query.bindValue(2, pos);
		query.bindValue(3, ci_upper);
		query.bindValue(4, inserted_sequence);
		query.bindValue(5, known_left);
		query.bindValue(6, known_right);
		query.bindValue(7, quality_metrics_string);		query.exec();

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
		THROW(ArgumentException, "Invalid structural variant type!");
		return -1;
	}

}

QString NGSD::svId(const BedpeLine& sv, int callset_id, const BedpeFile& svs, bool throw_if_fails)
{
	QString db_table_name = svTableName(sv.type());
	SqlQuery query = getQuery();
	if (sv.type() == StructuralVariantType::DEL || sv.type() == StructuralVariantType::DUP || sv.type() == StructuralVariantType::INV)
	{
		// get SV from NGSD
		query.exec("SELECT id FROM `" + db_table_name + "` WHERE `sv_callset_id`=" + QString::number(callset_id)
				   + " AND `chr`=\"" + sv.chr1().strNormalized(true) + "\""
				   + " AND `start_min`=" + QString::number(sv.start1())
				   + " AND `start_max`=" + QString::number(sv.end1())
				   + " AND `end_min`="+ QString::number(sv.start2())
				   + " AND `end_max`="+ QString::number(sv.end2()));
	}
	else if (sv.type() == StructuralVariantType::INS)
	{
		if (sv.chr1() != sv.chr2()) THROW(ArgumentException, "Invalid insertion position:'" + sv.position1() + ", " + sv.position2() + "'!");

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

		// determine filter for sequence
		QStringList sequence_filter;
		sequence_filter << ((inserted_sequence == "")? "AND `inserted_sequence` IS NULL": "AND `inserted_sequence`='" + inserted_sequence + "'");
		sequence_filter << ((known_left == "")? "AND `known_left` IS NULL": "AND `known_left`='" + known_left + "'");
		sequence_filter << ((known_right == "")? "AND `known_right` IS NULL": "AND `known_right`='" + known_right + "'");

		// determine position and upper CI:
		int pos = std::min(std::min(sv.start1(), sv.start2()), std::min(sv.end1(), sv.end2()));
		int ci_upper = std::max(std::max(sv.start1(), sv.start2()), std::max(sv.end1(), sv.end2())) - pos;

		// get SV from NGSD
		query.exec("SELECT id FROM `" + db_table_name + "` WHERE `sv_callset_id`=" + QString::number(callset_id)
				   + " AND `chr`=\"" + sv.chr1().strNormalized(true) + "\""
				   + " AND (`pos` - `ci_lower`) =" + QString::number(pos)
				   + " AND `ci_upper`=" + QString::number(ci_upper)
				   + " " + sequence_filter.join(" "));
	}
	else if (sv.type() == StructuralVariantType::BND)
	{
		// get SV from NGSD
		query.exec("SELECT id FROM `" + db_table_name + "` WHERE `sv_callset_id`=" + QString::number(callset_id)
				   + " AND `chr1`=\"" + sv.chr1().strNormalized(true) + "\""
				   + " AND `start1`=" + QString::number(sv.start1())
				   + " AND `end1`=" + QString::number(sv.end1())
				   + " AND `chr2`=\"" + sv.chr2().strNormalized(true) + "\""
				   + " AND `start2`=" + QString::number(sv.start2())
				   + " AND `end2`=" + QString::number(sv.end2()));
	}
	else
	{
		THROW(FileParseException, "Invalid structural variant type!");
	}

	query.next();

	// Throw error if multiple matches found
	if(query.size() > 1) THROW(DatabaseException, "Multiple matching SVs found in NGSD!");

	if(query.size() < 1)
	{
		if(!throw_if_fails) return "";

		THROW(DatabaseException, "SV " + BedpeFile::typeToString(sv.type()) + " at " + sv.positionRange() + " for callset with id '" + QString::number(callset_id) + "' not found in NGSD!");
	}

	return query.value("id").toString();

}

BedpeLine NGSD::structuralVariant(int sv_id, StructuralVariantType type, const BedpeFile& svs, bool no_annotation)
{
	BedpeLine sv;
	QList<QByteArray> annotations;

	int qual_idx = -1, filter_idx = -1, alt_a_idx = -1, info_a_idx = -1;
	if (!no_annotation)
	{
		annotations= QVector<QByteArray>(svs.annotationHeaders().size()).toList();

		// determine indices for annotations
		qual_idx = svs.annotationIndexByName("QUAL");
		filter_idx = svs.annotationIndexByName("FILTER");
		alt_a_idx = svs.annotationIndexByName("ALT_A");
		info_a_idx = svs.annotationIndexByName("INFO_A");
	}


	// get DEL, DUP or INV
	if (type == StructuralVariantType::DEL || type == StructuralVariantType::DUP || type == StructuralVariantType::INV)
	{
		// define pos varaibles
		Chromosome chr1, chr2;
		int start1, start2, end1, end2;

		// get correct sv table
		QByteArray table;
		switch (type)
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

		// get SV from the NGSD
		SqlQuery query = getQuery();
		query.exec("SELECT * FROM `" + table + "` WHERE id=" + QByteArray::number(sv_id));
		if (query.size() == 0 ) THROW(DatabaseException, "SV with id '" + QString::number(sv_id) + "'not found in table '" + table + "'!" );
		query.next();
		chr1 = Chromosome(query.value("chr").toByteArray());
		chr2 = Chromosome(query.value("chr").toByteArray());
		start1 = query.value("start_min").toInt();
		end1 = query.value("start_max").toInt();
		start2 = query.value("end_min").toInt();
		end2 = query.value("end_max").toInt();

		if (!no_annotation)
		{
			// parse quality & filter
			QJsonObject quality_metrics = QJsonDocument::fromJson(query.value("quality_metrics").toByteArray()).object();
			annotations[qual_idx] = quality_metrics.value("quality").toString().toUtf8();
			annotations[filter_idx] = quality_metrics.value("filter").toString().toUtf8();
		}

		// create SV
		sv = BedpeLine(chr1, start1, end1, chr2, start2, end2, type, annotations);
	}
	else if (type == StructuralVariantType::INS)
	{
		// get INS from the NGSD
		SqlQuery query = getQuery();
		query.exec("SELECT * FROM `sv_insertion` WHERE id=" + QByteArray::number(sv_id));
		if (query.size() == 0 ) THROW(DatabaseException, "SV with id '" + QString::number(sv_id) + "'not found in table 'sv_insertion'!" );
		query.next();
		Chromosome chr = Chromosome(query.value("chr").toByteArray());
		int pos = query.value("pos").toInt();
		int pos_upper = pos + query.value("ci_upper").toInt();

		if (!no_annotation)
		{
			// parse quality & filter
			QJsonObject quality_metrics = QJsonDocument::fromJson(query.value("quality_metrics").toByteArray()).object();
			annotations[qual_idx] = quality_metrics.value("quality").toString().toUtf8();
			annotations[filter_idx] = quality_metrics.value("filter").toString().toUtf8();

			// get inserted sequences:
			if (!query.value("inserted_sequence").isNull())
			{
				annotations[alt_a_idx] = query.value("inserted_sequence").toByteArray();
			}
			else
			{
				annotations[alt_a_idx] = "<INS>";
			}

			QByteArrayList partial_sequences;
			if (!query.value("known_left").isNull())
			{
				partial_sequences << "LEFT_SVINSSEQ=" + query.value("known_left").toByteArray();
			}
			if (!query.value("known_right").isNull())
			{
				partial_sequences << "RIGHT_SVINSSEQ=" + query.value("known_right").toByteArray();
			}
			annotations[info_a_idx] = partial_sequences.join(";");
		}

		// create SV
		sv = BedpeLine(chr, pos, pos_upper, chr, pos, pos, type, annotations);
	}
	else if (type == StructuralVariantType::BND)
	{
		// define pos varaibles
		Chromosome chr1, chr2;
		int start1, start2, end1, end2;

		// get SV from the NGSD
		SqlQuery query = getQuery();
		query.exec("SELECT * FROM `sv_translocation` WHERE id=" + QByteArray::number(sv_id));
		if (query.size() == 0 ) THROW(DatabaseException, "SV with id '" + QString::number(sv_id) + "'not found in table 'sv_translocation'!" );
		query.next();
		chr1 = Chromosome(query.value("chr1").toByteArray());
		chr2 = Chromosome(query.value("chr2").toByteArray());
		start1 = query.value("start1").toInt();
		end1 = query.value("end1").toInt();
		start2 = query.value("start2").toInt();
		end2 = query.value("end2").toInt();

		if (!no_annotation)
		{
			// parse quality & filter
			QJsonObject quality_metrics = QJsonDocument::fromJson(query.value("quality_metrics").toByteArray()).object();
			annotations[qual_idx] = quality_metrics.value("quality").toString().toUtf8();
			annotations[filter_idx] = quality_metrics.value("filter").toString().toUtf8();
		}

		// create SV
		sv = BedpeLine(chr1, start1, end1, chr2, start2, end2, type, annotations);
	}
	else
	{
		THROW(ArgumentException, "Invalid structural variant type!");
	}

	return sv;
}

QString NGSD::svTableName(StructuralVariantType type)
{
	switch (type)
	{
		case StructuralVariantType::DEL:
			return "sv_deletion";
		case StructuralVariantType::DUP:
			return "sv_duplication";
		case StructuralVariantType::INS:
			return "sv_insertion";
		case StructuralVariantType::INV:
			return "sv_inversion";
		case StructuralVariantType::BND:
			return "sv_translocation";
		default:
			THROW(ArgumentException, "Invalid structural variant type!");
			break;
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
			return QVariant(); //invalid
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

QList<int> NGSD::getValuesInt(const QString& query, QString bind_value) const
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

	QList<int> output;
	output.reserve(q.size());
	while(q.next())
	{
		output << q.value(0).toInt();
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
	QMap<QString, TableInfo>& table_infos = getCache().table_infos;

	//create if necessary
	if (!table_infos.contains(table))
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
			else if(type=="mediumtext") info.type = TableFieldInfo::TEXT;
			else if(type=="float") info.type = TableFieldInfo::FLOAT;
			else if(type=="date") info.type = TableFieldInfo::DATE;
			else if(type=="datetime") info.type = TableFieldInfo::DATETIME;
			else if(type=="timestamp") info.type = TableFieldInfo::TIMESTAMP;
			else if(type=="tinyint(1)") info.type = TableFieldInfo::BOOL;
			else if(type=="int" || type.startsWith("int(") || type.startsWith("tinyint(")) info.type = TableFieldInfo::INT;
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
				if (table=="sample" && info.name=="name") info.type_constraints.regexp = QRegularExpression("^[A-Za-z0-9-]*$");
				if (table=="mid" && info.name=="sequence") info.type_constraints.regexp = QRegularExpression("^[ACGT]*$");
				if (table=="processing_system" && info.name=="name_short") info.type_constraints.regexp = QRegularExpression("^[A-Za-z0-9_\\.-]*$");
				if (table=="processing_system" && info.name=="adapter1_p5") info.type_constraints.regexp = QRegularExpression("^[ACGTN]*$");
				if (table=="processing_system" && info.name=="adapter2_p7") info.type_constraints.regexp = QRegularExpression("^[ACGTN]*$");
				if (table=="processed_sample" && info.name=="lane") info.type_constraints.regexp = QRegularExpression("^[1-8](,[1-8])*$");
				if (table=="user" && info.name=="user_id") info.type_constraints.regexp = QRegularExpression("^[A-Za-z0-9_]+$");
				if (table=="study" && info.name=="name") info.type_constraints.regexp = QRegularExpression("^[A-Za-z0-9_ -]+$");
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
					else if (table=="variant_publication")
					{
						if (info.name=="sample_id")
						{
							info.fk_name_sql = "name";
						}
						else if (info.name=="user_id")
						{
							info.fk_name_sql = "name";
						}
					}
					else if (table=="preferred_transcripts")
					{
						if (info.name=="added_by")
						{
							info.fk_name_sql = "name";
						}
					}
					else if (table=="study_sample")
					{
						if (info.name=="study_id")
						{
							info.fk_name_sql = "name";
						}
						else if (info.name=="processed_sample_id")
						{
							info.fk_name_sql = "(SELECT CONCAT(s.name,'_',LPAD(ps.process_id,2,'0')) FROM sample s, processed_sample ps WHERE ps.id=processed_sample.id AND s.id=ps.sample_id)";
						}
					}
					else if (table=="somatic_gene_role")
					{
						if(info.name == "gene_id") info.fk_name_sql = "symbol";
					}
					{
						if (info.name=="sample1_id")
						{
							info.fk_name_sql = "name";
						}
						if (info.name=="sample2_id")
						{
							info.fk_name_sql = "name";
						}
						if (info.name=="user_id")
						{
							info.fk_name_sql = "name";
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
			if (table=="variant_publication" && info.name=="sample_id") info.label = "sample";
			if (table=="variant_publication" && info.name=="user_id") info.label = "published by";
			if (table=="preferred_transcripts" && info.name=="added_by") info.label = "added by";

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
				(table=="processed_sample" && info.name=="process_id") ||
				(table=="user" && info.name=="salt")
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
		table_infos.insert(table, output);
	}

	return table_infos[table];
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
			if  (field_info.fk_name_sql.isEmpty())
			{
				THROW(ProgrammingException, "Cannot create overview table for '" + table + "': Foreign key name SQL not defined for table '" + field_info.fk_table + "'");
			}
			replaceForeignKeyColumn(output, c, field_info.fk_table, field_info.fk_name_sql);
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
			while(column.count() < output.rowCount()) column << passwordReplacement();
			output.setColumn(c, column);
		}
	}

	//remove hidden columns (reverse order so that indices stay valid)
	for (int c=headers.count()-1; c>=0; --c)
	{
		const TableFieldInfo& field_info = table_info.fieldInfo(headers[c]);

		if (field_info.is_hidden)
		{
			output.takeColumn(c);
			headers.removeAt(c);
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

void NGSD::replaceForeignKeyColumn(DBTable& table, int c, QString fk_table, QString fk_name_sql)
{
	QHash<QString, QString> cache; //check query result to reduce the number of SQL queries (they are slow)

	QStringList column = table.extractColumn(c);
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
				QString fk_value = getValue("SELECT " + fk_name_sql + " FROM " + fk_table + " WHERE id=" + value).toString();
				column[r] = fk_value;
				cache[value] = fk_value;
			}
		}
	}

	table.setColumn(c, column);
}

void NGSD::init(QString password)
{
	//remove existing tables (or clear if possible - that's faster)
	QStringList tables = getValues("SHOW TABLES");
	if (!tables.isEmpty())
	{
		//check password for re-init of production DB
		if (!test_db_ && password!=Settings::string("ngsd_pass"))
		{
			THROW(DatabaseException, "Password provided for re-initialization of production database is incorrect!");
		}

		bool clear_only = false;
		if(test_db_)
		{
			QVariant ngsd_init_date = getValue("SELECT created FROM user WHERE user_id='init_date'");
			if (ngsd_init_date.toString()!="" && QFileInfo(":/resources/NGSD_schema.sql").lastModified()<ngsd_init_date.toDateTime())
			{
				clear_only = true;
			}
		}

		if (clear_only)
		{
			SqlQuery query = getQuery();
			query.exec("SET FOREIGN_KEY_CHECKS = 0;");
			foreach(QString table, tables)
			{
				query.exec("DELETE FROM " + table);
				query.exec("ALTER TABLE " + table  + " AUTO_INCREMENT = 1");
			}
			query.exec("SET FOREIGN_KEY_CHECKS = 1;");
		}
		else
		{
			SqlQuery query = getQuery();
			query.exec("SET FOREIGN_KEY_CHECKS = 0;");
			query.exec("DROP TABLE " + tables.join(","));
			query.exec("SET FOREIGN_KEY_CHECKS = 1;");
		}
	}

	//initilize
	executeQueriesFromFile(":/resources/NGSD_schema.sql");
	executeQueriesFromFile(":/resources/NGSD_initial_data.sql");
	if (test_db_)
	{
		getQuery().exec("INSERT INTO user VALUES (NULL, 'init_date', 'pass', 'user', 'some name','some_name@email.de', NOW(), NULL, 0, NULL)");
	}

	//clear cache
	clearCache();
}

QStringList NGSD::subPanelList(bool archived)
{
	return getValues("SELECT name FROM subpanels WHERE archived=" + QString(archived ? "1" : "0") + " ORDER BY name ASC");
}

BedFile NGSD::subpanelRegions(QString name)
{
	QByteArray roi = getValue("SELECT roi FROM subpanels WHERE name=:0", false, name).toByteArray();

	return BedFile::fromText(roi);
}

GeneSet NGSD::subpanelGenes(QString name)
{
	QByteArray genes = getValue("SELECT genes FROM subpanels WHERE name=:0", false, name).toByteArray();

	return GeneSet::createFromText(genes);
}

QList<CfdnaPanelInfo> NGSD::cfdnaPanelInfo(const QString& processed_sample_id, int processing_system_id)
{
	// get all cfDNA Panel for the given tumor id
	QList<CfdnaPanelInfo> cfdna_panels;
	SqlQuery query = getQuery();
	if(processing_system_id == -1)
	{
		query.prepare("SELECT id, tumor_id, cfdna_id, created_by, created_date, `processing_system_id` FROM cfdna_panels WHERE tumor_id=:0");
		query.bindValue(0, processed_sample_id);
	}
	else
	{
		query.prepare("SELECT id, tumor_id, cfdna_id, created_by, created_date, `processing_system_id` FROM cfdna_panels WHERE tumor_id=:0 AND `processing_system_id`=:1");
		query.bindValue(0, processed_sample_id);
		query.bindValue(1, processing_system_id);
	}

	query.exec();
	while(query.next())
	{
		bool ok;
		CfdnaPanelInfo panel;
		panel.id = query.value(0).toInt(&ok);
		if (!ok) THROW(DatabaseException, "Error parsing id in cfdna_panels!");
		panel.tumor_id = query.value(1).toInt(&ok);
		if (!ok) THROW(DatabaseException, "Error parsing tumor_id in cfdna_panels!");
		if (query.value(2) == QVariant())
		{
			panel.cfdna_id = -1;
		}
		else
		{
			panel.cfdna_id = query.value(2).toInt(&ok);
			if (!ok) THROW(DatabaseException, "Error parsing cfdna_id in cfdna_panels!");
		}
		panel.created_by = query.value(3).toInt(&ok);
		if (!ok) THROW(DatabaseException, "Error parsing created_by in cfdna_panels!");
		panel.created_date = query.value(4).toDate();
		panel.processing_system_id = query.value(5).toInt(&ok);
		if (!ok) THROW(DatabaseException, "Error parsing processing_system in cfdna_panels!");

		cfdna_panels.append(panel);
	}

	return cfdna_panels;
}

void NGSD::storeCfdnaPanel(const CfdnaPanelInfo& panel_info, const QByteArray& bed_content, const QByteArray& vcf_content)
{
	// get user id
	int user_id = panel_info.created_by;
	// get processing system
	int sys_id = panel_info.processing_system_id;

	SqlQuery query = getQuery();
	if (panel_info.id == -1)
	{
		query.prepare("INSERT INTO `cfdna_panels` (`tumor_id`, `created_by`, `created_date`, `processing_system_id`, `bed`, `vcf`) VALUES (:0, :1, :2, :3, :4, :5);");

	}
	else
	{
		query.prepare("UPDATE `cfdna_panels` SET `tumor_id`=:0, `created_by`=:1, `created_date`=:2, `processing_system_id`=:3, `bed`=:4, `vcf`=:5  WHERE `id`=:6");
		query.bindValue(6, panel_info.id);
	}

	// bind values
	query.bindValue(0, panel_info.tumor_id);
	query.bindValue(1, user_id);
	query.bindValue(2, panel_info.created_date);
	query.bindValue(3, sys_id);
	query.bindValue(4, bed_content);
	query.bindValue(5, vcf_content);

	query.exec();
}

BedFile NGSD::cfdnaPanelRegions(int id)
{
	return BedFile::fromText(getValue("SELECT bed FROM cfdna_panels WHERE id=:0", false, QString::number(id)).toString().toUtf8());
}

VcfFile NGSD::cfdnaPanelVcf(int id)
{
	VcfFile vcf;
	vcf.fromText(getValue("SELECT vcf FROM cfdna_panels WHERE id=:0", false, QString::number(id)).toString().toUtf8());
	return vcf;
}

BedFile NGSD::cfdnaPanelRemovedRegions(int id)
{
	return BedFile::fromText(getValue("SELECT `excluded_regions` FROM `cfdna_panels` WHERE id=:0", false, QString::number(id)).toString().toUtf8());
}

void NGSD::setCfdnaRemovedRegions(int id, const BedFile& removed_regions)
{
	SqlQuery query = getQuery();
	query.prepare("UPDATE `cfdna_panels` SET `excluded_regions`=:0 WHERE `id`=" + QString::number(id));
	QString bed_content = "##modified at " + QDate::currentDate().toString("dd.MM.yyyy").toUtf8() + " by " + LoginManager::userName().toUtf8() + "\n" + removed_regions.toText();
	query.bindValue(0, bed_content);
	query.exec();
}

QList<CfdnaGeneEntry> NGSD::cfdnaGenes()
{
	QList<CfdnaGeneEntry> genes;
	SqlQuery query = getQuery();
	query.exec("SELECT `gene_name`, `chr`, `start`, `end`, `date`, `bed` FROM cfdna_panel_genes");

	while (query.next())
	{
		CfdnaGeneEntry gene_entry;
		gene_entry.gene_name = query.value("gene_name").toString();
		gene_entry.chr = Chromosome(query.value("chr").toString());
		gene_entry.start = query.value("start").toInt();
		gene_entry.end = query.value("end").toInt();
		gene_entry.date = query.value("date").toDate();
		gene_entry.bed = BedFile::fromText(query.value("bed").toString().toUtf8());

		genes.append(gene_entry);
	}

	return genes;
}

VcfFile NGSD::getIdSnpsFromProcessingSystem(int sys_id, bool throw_on_fail)
{
	VcfFile vcf;
	vcf.sampleIDs().append("TUMOR");
	vcf.sampleIDs().append("NORMAL");

	ProcessingSystemData sys = NGSD().getProcessingSystemData(sys_id);

	// add INFO line to determine source
	InfoFormatLine id_source;
	id_source.id = "ID_Source";
	id_source.number = ".";
	id_source.type = "String";
	id_source.description = "Source of the ID SNPs (e.g. processing system short name or KASP).";
	vcf.vcfHeader().addInfoLine(id_source);

	//prepare info for VCF line
	QByteArrayList info;
	InfoIDToIdxPtr info_ptr = InfoIDToIdxPtr(new OrderedHash<QByteArray, int>);
	QByteArray key = "ID_Source";
	QByteArray value = sys.name_short.toUtf8();
	info.push_back(value);
	info_ptr->push_back(key, static_cast<unsigned char>(0));

	BedFile target_region = NGSD().processingSystemRegions(sys_id);

	QByteArrayList format_ids = QByteArrayList() << "GT";
	QByteArrayList sample_ids = QByteArrayList() << "TUMOR" << "NORMAL";
	QList<QByteArrayList> list_of_format_values;
	list_of_format_values.append(QByteArrayList() << "./.");
	list_of_format_values.append(QByteArrayList() << "./.");

	for (int i=0; i<target_region.count(); i++)
	{
		const BedLine& line = target_region[i];
		if (line.annotations().size() > 0)
		{
			//create variant
			QByteArrayList variant_info = line.annotations().at(0).split('>');
			if (variant_info.size() != 2)
			{
				if (throw_on_fail)
				{
					THROW(FileParseException, "Invalid variant information '" + line.annotations().at(0) + "' for region " + line.toString(true) + "!" );
				}
				return VcfFile();
			}
			VcfLinePtr vcf_ptr = QSharedPointer<VcfLine>(new VcfLine(line.chr(), line.start(), Sequence(variant_info.at(0)), QVector<Sequence>() << Sequence(variant_info.at(1)), format_ids,
																	 sample_ids, list_of_format_values));
			vcf_ptr->setInfo(info);
			vcf_ptr->setInfoIdToIdxPtr(info_ptr);
			vcf_ptr->setId(QByteArrayList() << "ID");
			vcf.vcfLines() << vcf_ptr;
		}
		else
		{
			if (throw_on_fail)
			{
				THROW(FileParseException, "Target region does not contain variant information for region " + line.toString(true) + "!" );
			}
			return VcfFile();
		}
	}

	return vcf;
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

int NGSD::getSomaticViccId(const Variant &variant)
{
	QString query = "SELECT id FROM somatic_vicc_interpretation WHERE variant_id = '" + variantId(variant, false) +"'";
	QVariant id = getValue(query, true);
	return id.isValid() ? id.toInt() : -1;
}


SomaticViccData NGSD::getSomaticViccData(const Variant& variant, bool throw_on_fail)
{
	QString variant_id = variantId(variant, throw_on_fail);
	if (variant_id=="")
	{
		return SomaticViccData();
	}

	SqlQuery query = getQuery();
	query.exec("SELECT null_mutation_in_tsg, known_oncogenic_aa, strong_cancerhotspot, oncogenic_funtional_studies, located_in_canerhotspot, absent_from_controls, protein_length_change, other_aa_known_oncogenic, weak_cancerhotspot, computational_evidence, mutation_in_gene_with_etiology, very_weak_cancerhotspot, very_high_maf, benign_functional_studies, high_maf, benign_computational_evidence, synonymous_mutation, comment, created_by, created_date, last_edit_by, last_edit_date FROM somatic_vicc_interpretation WHERE variant_id='" + variant_id + "'");
	if (query.size()==0)
	{
		if(throw_on_fail)
		{
			THROW(DatabaseException, "Cannot find somatic VICC data for variant " + variant.toString(true, 100, true));
		}
		else
		{
			return SomaticViccData();
		}
	}
	query.next();



	SomaticViccData out;

	auto varToState = [](const QVariant& var)
	{
		if(var.isNull()) return SomaticViccData::State::NOT_APPLICABLE;
		if(var.toBool()) return SomaticViccData::State::VICC_TRUE;
		return SomaticViccData::State::VICC_FALSE;
	};

	out.null_mutation_in_tsg = varToState(query.value(0));
	out.known_oncogenic_aa = varToState(query.value(1));
	out.strong_cancerhotspot = varToState(query.value(2));
	out.oncogenic_functional_studies = varToState(query.value(3));
	out.located_in_canerhotspot = varToState(query.value(4));
	out.absent_from_controls = varToState(query.value(5));
	out.protein_length_change = varToState(query.value(6));
	out.other_aa_known_oncogenic = varToState(query.value(7));
	out.weak_cancerhotspot = varToState(query.value(8));
	out.computational_evidence = varToState(query.value(9));
	out.mutation_in_gene_with_etiology = varToState(query.value(10));
	out.very_weak_cancerhotspot = varToState(query.value(11));
	out.very_high_maf = varToState(query.value(12));
	out.benign_functional_studies = varToState(query.value(13));
	out.high_maf = varToState(query.value(14));
	out.benign_computational_evidence = varToState(query.value(15));
	out.synonymous_mutation = varToState(query.value(16));

	out.comment = query.value(17).toString();
	//created_by, created_date, last_edit_by, last_edit_date

	out.created_by = userLogin(query.value(18).toInt());
	out.created_at = query.value(19).toDateTime();
	out.last_updated_by = userLogin( query.value(20).toInt() );
	out.last_updated_at = query.value(21).toDateTime();

	return out;
}

void NGSD::setSomaticViccData(const Variant& variant, const SomaticViccData& vicc_data, QString user_name)
{
	if(!vicc_data.isValid())
	{
		THROW(ArgumentException, "Cannot set somatic VICC data for variant " + variant.toString() + " because VICC data is invalid");
	}

	QString variant_id = variantId(variant);

	SqlQuery query = getQuery();

	//this lambda binds all values needed for both inserting and updating
	auto bind = [&query, vicc_data, user_name, this]()
	{
		auto stateToVar = [](SomaticViccData::State state)
		{
			if(state == SomaticViccData::State::VICC_TRUE) return QVariant(true);
			else if(state == SomaticViccData::State::VICC_FALSE) return QVariant(false);
			return QVariant(QVariant::Bool);
		};

		query.bindValue( 0 , stateToVar( vicc_data.null_mutation_in_tsg ) );
		query.bindValue( 1 , stateToVar( vicc_data.known_oncogenic_aa ) );
		query.bindValue( 2 , stateToVar( vicc_data.oncogenic_functional_studies) );
		query.bindValue( 3 , stateToVar( vicc_data.strong_cancerhotspot ) );
		query.bindValue( 4 , stateToVar( vicc_data.located_in_canerhotspot ) );
		query.bindValue( 5 , stateToVar( vicc_data.absent_from_controls ) );
		query.bindValue( 6 , stateToVar( vicc_data.protein_length_change ) );
		query.bindValue( 7 , stateToVar( vicc_data.other_aa_known_oncogenic ) );
		query.bindValue( 8 , stateToVar( vicc_data.weak_cancerhotspot ) );
		query.bindValue( 9 , stateToVar( vicc_data.computational_evidence ) );
		query.bindValue(10 , stateToVar( vicc_data.mutation_in_gene_with_etiology ) );
		query.bindValue(11 , stateToVar( vicc_data.very_weak_cancerhotspot ) );
		query.bindValue(12 , stateToVar( vicc_data.very_high_maf ) );
		query.bindValue(13 , stateToVar( vicc_data.benign_functional_studies ) );
		query.bindValue(14 , stateToVar( vicc_data.high_maf ) );
		query.bindValue(15 , stateToVar( vicc_data.benign_computational_evidence ) );
		query.bindValue(16 , stateToVar( vicc_data.synonymous_mutation ) );
		query.bindValue(17 , vicc_data.comment );
		query.bindValue(18 , userId(user_name) );
	};

	int vicc_id = getSomaticViccId(variant);
	if(vicc_id != -1) //update data set
	{
		query.prepare("UPDATE `somatic_vicc_interpretation` SET  `null_mutation_in_tsg`=:0, `known_oncogenic_aa`=:1, `oncogenic_funtional_studies`=:2, `strong_cancerhotspot`=:3, `located_in_canerhotspot`=:4,  `absent_from_controls`=:5, `protein_length_change`=:6, `other_aa_known_oncogenic`=:7, `weak_cancerhotspot`=:8, `computational_evidence`=:9, `mutation_in_gene_with_etiology`=:10, `very_weak_cancerhotspot`=:11, `very_high_maf`=:12, `benign_functional_studies`=:13, `high_maf`=:14, `benign_computational_evidence`=:15, `synonymous_mutation`=:16, `comment`=:17, `last_edit_by`=:18, `last_edit_date`= CURRENT_TIMESTAMP WHERE `id`=" + QByteArray::number(vicc_id) );
		bind();
		query.exec();
	}
	else //insert new data set
	{
		query.prepare("INSERT INTO `somatic_vicc_interpretation` (`null_mutation_in_tsg`, `known_oncogenic_aa`, `oncogenic_funtional_studies`, `strong_cancerhotspot`, `located_in_canerhotspot`,  `absent_from_controls`, `protein_length_change`, `other_aa_known_oncogenic`, `weak_cancerhotspot`, `computational_evidence`, `mutation_in_gene_with_etiology`, `very_weak_cancerhotspot`, `very_high_maf`, `benign_functional_studies`, `high_maf`, `benign_computational_evidence`, `synonymous_mutation`, `comment`, `last_edit_by`, `last_edit_date`, `created_by`, `created_date`, `variant_id`) VALUES (:0, :1, :2, :3, :4, :5, :6, :7, :8, :9, :10, :11, :12, :13, :14, :15, :16, :17, :18, CURRENT_TIMESTAMP, :19, CURRENT_TIMESTAMP, :20)");
		bind();
		query.bindValue(19, userId(user_name) );
		query.bindValue(20, variant_id);
		query.exec();
	}
}

int NGSD::getSomaticGeneRoleId(QByteArray gene_symbol)
{
	QString query ="SELECT somatic_gene_role.id FROM somatic_gene_role WHERE symbol = '" + geneToApproved(gene_symbol, true) + "'";
	QVariant id = getValue(query, true);
	return id.isValid() ? id.toInt() : -1;
}

SomaticGeneRole NGSD::getSomaticGeneRole(QByteArray gene, bool throw_on_fail)
{
	SqlQuery query = getQuery();

	//Initialize output without gene symbol (in case it fails and method shall not throw error)
	SomaticGeneRole out;


	int gene_role_id = getSomaticGeneRoleId(gene);
	if(gene_role_id == -1)
	{
		if(throw_on_fail)
		{
			THROW(DatabaseException, "There is no somatic gene role for gene symbol '" + gene + "' in the NGSD.") ;
		}
		else
		{
			return out; //return empty data
		}
	}

	query.prepare("SELECT gene_role, high_evidence, comment FROM somatic_gene_role WHERE somatic_gene_role.id = " + QByteArray::number(gene_role_id));
	query.exec();

	if(query.size() != 1)
	{
		if(throw_on_fail)
		{
			THROW(DatabaseException, "Could not or found multiple somatic gene roles for " + gene);
		}
		else
		{
			return out; //return empty data
		}
	}


	query.next();

	out.gene = gene;

	//set gene role
	if(query.value(0).toString() == "activating") out.role = SomaticGeneRole::Role::ACTIVATING;
	else if(query.value(0).toString() == "loss_of_function") out.role = SomaticGeneRole::Role::LOSS_OF_FUNCTION;
	else if(query.value(0).toString() == "ambiguous") out.role = SomaticGeneRole::Role::AMBIGUOUS;
	else THROW(DatabaseException, "Unknown gene role '" + query.value(0).toString() + "' in relation 'somatic_gene_role'.");

	//evidence
	out.high_evidence = query.value(1).toBool();

	out.comment = query.value(2).toString();


	return out;
}

void NGSD::setSomaticGeneRole(const SomaticGeneRole& gene_role)
{
	QByteArray symbol = geneToApproved(gene_role.gene);

	if(geneToApprovedID(symbol) == -1)
	{
		THROW(DatabaseException, "Could not find gene symbol '" + symbol + "' in the NGSD in NGSD::setSomaticGeneRole!");
	}

	int gene_role_id = getSomaticGeneRoleId(symbol);

	SqlQuery query = getQuery();

	if(gene_role_id != -1) //update existing
	{
		query.prepare("UPDATE `somatic_gene_role` SET  `gene_role`=:0, `high_evidence`=:1, `comment`=:2 WHERE `id` = " + QByteArray::number(gene_role_id));

		if(gene_role.role == SomaticGeneRole::Role::ACTIVATING) query.bindValue(0, "activating");
		else if(gene_role.role == SomaticGeneRole::Role::LOSS_OF_FUNCTION) query.bindValue(0, "loss_of_function");
		else query.bindValue(0, "ambiguous");

		query.bindValue(1, gene_role.high_evidence);
		if(!gene_role.comment.isEmpty()) query.bindValue(2, gene_role.comment);
		else query.bindValue(2, QVariant::String);

		query.exec();
	}
	else //insert new somatic gene role
	{
		query.prepare("INSERT INTO somatic_gene_role (symbol, gene_role, high_evidence, comment) VALUES (:0, :1, :2, :3)");
		query.bindValue(0, symbol);
		if(gene_role.role == SomaticGeneRole::Role::ACTIVATING) query.bindValue(1, "activating");
		else if(gene_role.role == SomaticGeneRole::Role::LOSS_OF_FUNCTION) query.bindValue(1, "loss_of_function");
		else query.bindValue(1, "ambiguous");

		query.bindValue(2, gene_role.high_evidence);
		if(!gene_role.comment.isEmpty()) query.bindValue(3, gene_role.comment);
		else query.bindValue(3, QVariant::String);

		query.exec();
	}

}

void NGSD::deleteSomaticGeneRole(QByteArray gene)
{
	int id = getSomaticGeneRoleId(gene);
	if(id == -1)
	{
		THROW(ProgrammingException, "Cannot delete somatic gene role for gene symbol '" + gene + "' because it does not exist in NGSD::deleteSomaticGeneRole");
	}

	SqlQuery query = getQuery();
	query.exec("DELETE FROM somatic_gene_role WHERE id = " + QByteArray::number(id));

}


void NGSD::addVariantPublication(QString filename, const Variant& variant, QString database, QString classification, QString details)
{
	QString s_id = sampleId(filename);
	QString v_id = variantId(variant);
	QString user_id = LoginManager::userIdAsString();

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

void NGSD::queueAnalysis(QString type, bool high_priority, QStringList args, QList<AnalysisJobSample> samples)
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
	query.exec("INSERT INTO `analysis_job_history`(`analysis_job_id`, `time`, `user_id`, `status`, `output`) VALUES (" + job_id + ",'" + Helper::dateTime("") + "'," + LoginManager::userIdAsString() + ",'queued', '')");
}

bool NGSD::cancelAnalysis(int job_id)
{
	//check if running or already canceled
	AnalysisJob job = analysisInfo(job_id, false);
	if (!job.isRunning()) return false;

	SqlQuery query = getQuery();
	query.exec("INSERT INTO `analysis_job_history`(`analysis_job_id`, `time`, `user_id`, `status`, `output`) VALUES (" + QString::number(job_id) + ",'" + Helper::dateTime("") + "'," + LoginManager::userIdAsString() + ",'cancel', '')");

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
	QString output = processedSamplePath(processedSampleId(job.samples[0].name), PathType::SAMPLE_FOLDER) + QDir::separator() + ".." + QDir::separator();

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
		if(job.samples.count() == 2) //Tumor-normal pair
		{
			output += "Somatic_";
			sample_sep = "-";
		}
		else if(job.samples.count() == 1) //Tumor only
		{
			output += "Sample_";
		}
		else
		{
			THROW(ProgrammingException, "Somatic analysis type with " + QByteArray::number( job.samples.count() ) + " samples!");
		}
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
	output += QDir::separator();

	return QFileInfo(output).absoluteFilePath();
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
		if(job.samples.count() == 2) output += job.samples[0].name + "-" + job.samples[1].name + ".GSvar"; //tumor-normal pair
		else if(job.samples.count() == 1) output += job.samples[0].name + ".GSvar"; //tumor only;
		else THROW(ProgrammingException, "Somatic analysis type with " + QByteArray::number( job.samples.count() ) + " samples!");
	}
	else
	{
		THROW(ProgrammingException, "Unknown analysis type '" + job.type + "'!");
	}

	return output;
}

int NGSD::addGap(const QString& ps_id, const Chromosome& chr, int start, int end, const QString& status)
{
	SqlQuery query = getQuery();
	query.prepare("INSERT INTO `gaps`(`chr`, `start`, `end`, `processed_sample_id`) VALUES (:0,:1,:2,:3)");
	query.bindValue(0, chr.strNormalized(true));
	query.bindValue(1, start);
	query.bindValue(2, end);
	query.bindValue(3, ps_id);
	query.exec();

	//set status and history
	int id = query.lastInsertId().toInt();
	updateGapStatus(id, status);

	return id;
}

int NGSD::gapId(const QString& ps_id, const Chromosome& chr, int start, int end, bool exact_match)
{
	if (exact_match)
	{
		QVariant id =  getValue("SELECT id FROM gaps WHERE processed_sample_id='" + ps_id + "' AND chr='" + chr.strNormalized(true) + "' AND start='" + QString::number(start) + "' AND end='" + QString::number(end) + "'", true);
		if (id.isValid()) return id.toInt();
	}
	else
	{
		SqlQuery query = getQuery();
		query.exec("SELECT id, chr, start, end FROM gaps WHERE processed_sample_id='" + ps_id +"'");
		while(query.next())
		{
			if (chr==query.value("chr").toString())
			{
				if (BasicStatistics::rangeOverlaps(start, end, query.value("start").toInt(), query.value("end").toInt()))
				{
					return query.value("id").toInt();
				}
			}
		}
	}

	return -1;
}

void NGSD::updateGapStatus(int id, const QString& status)
{
	//check gap exists
	QString id_str = QString::number(id);
	if(getValue("SELECT EXISTS(SELECT * FROM gaps WHERE id='" + id_str + "')").toInt()==0)
	{
		THROW(ArgumentException, "Gap with ID '" + id_str + "' does not exist!");
	}

	//prepare history string
	QString history = getValue("SELECT history FROM gaps WHERE id='" + id_str + "'").toString().trimmed();
	if (!history.isEmpty()) history += "\n";
	history += QDateTime::currentDateTime().toString("dd.MM.yyyy hh:mm:ss") + " - " + status +" (" + LoginManager::userName() + ")";

	SqlQuery query = getQuery();
	query.exec("UPDATE gaps SET status='"+status+"', history='" + history + "' WHERE id='" + id_str + "'");
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

QString NGSD::getTargetFilePath()
{
	return Settings::path("data_folder", false) + QDir::separator() + "enrichment" + QDir::separator();
}

void NGSD::updateQC(QString obo_file, bool debug)
{
	//init
	QStringList valid_types = getEnum("qc_terms", "type");

	//load terms
	OntologyTermCollection terms(obo_file, false);
	if (debug) qDebug() << "Terms parsed: " << terms.size();

	// database connection
	transaction();
	QSqlQuery query = getQuery();
	query.prepare("INSERT INTO qc_terms (qcml_id, name, description, type, obsolete) VALUES (:0, :1, :2, :3, :4) ON DUPLICATE KEY UPDATE name=VALUES(name), description=VALUES(description), type=VALUES(type), obsolete=VALUES(obsolete)");
	int c_terms_ngs = 0;
	int c_terms_valid_type = 0;
	for(int i=0; i<terms.size(); ++i)
	{
		const OntologyTerm& term = terms.get(i);

		//remove terms not for NGS
		if (!term.id().startsWith("QC:2")) continue;
		++c_terms_ngs;

		//remove QC terms of invalid types
		if (!valid_types.contains(term.type())) continue;
		++c_terms_valid_type;

		//insert (or update if already contained)
		if (debug) qDebug() << "IMPORTING:" << term.id() << term.name() << term.type() << term.isObsolete()  << term.definition();
		query.bindValue(0, term.id());
		query.bindValue(1, term.name());
		query.bindValue(2, term.definition());
		query.bindValue(3, term.type());
		query.bindValue(4, term.isObsolete());
		query.exec();
		if (debug) qDebug() << "  ID:" << query.lastInsertId();
	}
	commit();

	if (debug)
	{
		qDebug() << "Terms for NGS: " << c_terms_ngs;
		qDebug() << "Terms with valid types ("+valid_types.join(", ")+"): " << c_terms_valid_type;
	}
}

QHash<QString, QStringList> NGSD::checkMetaData(const QString& ps_id, const VariantList& variants, const CnvList& cnvs, const BedpeFile& svs)
{
	QHash<QString, QStringList> output;

	QString s_id = getValue("SELECT s.id FROM sample s, processed_sample ps WHERE ps.sample_id=s.id AND ps.id='" + ps_id + "'").toString();

	//check sample meta data
	SampleData sample_data = getSampleData(s_id);
	QString s_name = sample_data.name;
	if (sample_data.disease_group=="n/a") output[s_name] << "disease group unset!";
	if (sample_data.disease_status=="n/a") output[s_name] << "disease status unset!";

	//check report config variants
	bool causal_diagnostic_variant_present = false;
	int rc_id = reportConfigId(ps_id);
	if (rc_id!=-1)
	{
		QStringList report_config_messages;
		QSharedPointer<ReportConfiguration> report_config = reportConfig(rc_id, variants, cnvs, svs, report_config_messages);
		foreach(const ReportVariantConfiguration& var_conf, report_config->variantConfig())
		{
			if (var_conf.causal)
			{
				if (var_conf.report_type=="diagnostic variant")
				{
					causal_diagnostic_variant_present = true;

					//check classification
					if (var_conf.variant_type==VariantType::SNVS_INDELS)
					{
						if (var_conf.variant_index!=-1)
						{
							const Variant& var = variants[var_conf.variant_index];
							ClassificationInfo class_info = getClassification(var);
							if (class_info.classification=="" || class_info.classification=="n/a")
							{
								output[s_name] << "causal diagnostic " + variantTypeToString(var_conf.variant_type) + " has no classification!";
							}
						}
					}
					else if (var_conf.variant_type==VariantType::CNVS || var_conf.variant_type==VariantType::SVS)
					{
						if (var_conf.classification=="" || var_conf.classification=="n/a")
						{
							output[s_name] << "causal diagnostic " + variantTypeToString(var_conf.variant_type) + " has no classification!";
						}
					}
					else
					{
						THROW(ProgrammingException, "checkMetaData: Unhandled variant type!")
					}

					//check inheritance
					if (var_conf.inheritance=="" || var_conf.inheritance=="n/a")
					{
						output[s_name] << "causal diagnostic " + variantTypeToString(var_conf.variant_type) + " has no inheritance!";
					}
				}
				else
				{
					output[s_name] << "causal " + variantTypeToString(var_conf.variant_type) + ", but report type is not 'diagnostic variant'!";
				}
			}
		}
	}

	if (sample_data.disease_status=="Affected")
	{
		//HPO terms
		QList<SampleDiseaseInfo> disease_info = getSampleDiseaseInfo(s_id, "HPO term id");
		if (disease_info.isEmpty()) output[s_name] << "no HPO phenotype(s) set!";

		//diagnostic status
		DiagnosticStatusData diag_status = getDiagnosticStatus(ps_id);
		if (diag_status.outcome=="n/a") output[s_name] << "diagnostic status outcome unset!";

		//report config
		if (diag_status.outcome=="significant findings")
		{
			if (!causal_diagnostic_variant_present)
			{
				output[s_name] << "outcome 'significant findings', but no causal diagnostic variant in the report configuration!";
			}
		}
		else //affected, but no significant findings
		{
			if (causal_diagnostic_variant_present)
			{
				output[s_name] << "outcome not 'significant findings', but causal variant in the report configuration!";
			}
		}
	}
	else //not affected
	{
		//diagnostic status
		DiagnosticStatusData diag_status = getDiagnosticStatus(ps_id);
		if (diag_status.outcome=="n/a") output[s_name] << "diagnostic status outcome unset!";

		//report config
		if (causal_diagnostic_variant_present)
		{
			output[s_name] << "disease status not 'Affected', but causal variant in the report configuration!";
		}
	}

	//related samples
	QStringList related_sample_ids = relatedSamples(s_id, "parent-child");
	related_sample_ids << relatedSamples(s_id, "siblings");
	related_sample_ids << relatedSamples(s_id, "twins");
	related_sample_ids << relatedSamples(s_id, "twins (monozygotic)");
	related_sample_ids << relatedSamples(s_id, "cousins");
	foreach(QString related_sample_id, related_sample_ids)
	{
		//sample data
		SampleData sample_data = getSampleData(related_sample_id);
		if (sample_data.disease_group=="n/a") output[sample_data.name] << "disease group unset!";
		if (sample_data.disease_status=="n/a") output[sample_data.name] << "disease status unset!";

		//HPO terms
		if (sample_data.disease_status=="Affected")
		{
			QList<SampleDiseaseInfo> disease_info = getSampleDiseaseInfo(related_sample_id, "HPO term id");
			if (disease_info.isEmpty()) output[sample_data.name] << "no HPO phenotype(s) set! ";
		}
	}

	return output;
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
	return text.trimmed().replace("\"", "").replace("'", "''").replace(";", "").replace("\n", "");
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
	QString project_folder = Settings::path("projects_folder");
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

		QString folder = project_folder + p_type + QDir::separator() + query.value(2).toString() + QDir::separator() + "Sample_" + ps_name + QDir::separator();
		if (!QFile::exists(folder))
		{
			QString ps_id = query.value(4).toString();

			//check if merged
			bool merged = false;
			SqlQuery query2 = getQuery();
			query2.exec("SELECT CONCAT(s.name,'_',LPAD(ps.process_id,2,'0')), p.type, p.name FROM sample s, processed_sample ps, project p WHERE ps.sample_id=s.id AND ps.project_id=p.id AND s.id='" + query.value(3).toString()+"' AND ps.id!='" + ps_id + "'");
			while(query2.next())
			{
				QString folder2 = project_folder + query2.value(1).toString() + QDir::separator() + query2.value(2).toString() + QDir::separator() + "Sample_" + query2.value(0).toString() + QDir::separator();
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
	QMap<QString, QStringList>& cache = getCache().enum_values;
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

bool NGSD::transaction()
{
	if(!db_->driver()->hasFeature(QSqlDriver::Transactions))
	{
		Log::warn("transactions are not supported by the current driver! (" + db_->driverName() + ")");
	}

	if (db_->transaction()) return true;
	Log::warn("transactions: db_->transaction() failed!");
	return false;
}

bool NGSD::commit()
{
	if (db_->commit()) return true;
	Log::warn("transactions: db_->commit() failed!");
	return false;
}

bool NGSD::rollback()
{
	if (db_->rollback()) return true;
	Log::warn("db_->rollback() failed!");
	return false;
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

	//not cached => try to convert
	QMap<QByteArray, QByteArray>& mapping = getCache().non_approved_to_approved_gene_names;
	if (!mapping.contains(gene))
	{
		int gene_id = geneToApprovedID(gene);
		mapping[gene] = (gene_id!=-1) ? geneSymbol(gene_id) : "";
	}

	//return result
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
	q.exec("SELECT symbol FROM gene_alias WHERE gene_id='" + QByteArray::number(id) + "' AND type='synonym'");
	while(q.next())
	{
		output.insert(q.value(0).toByteArray());
	}

	return output;
}

PhenotypeList NGSD::phenotypes(const QByteArray& symbol)
{
	PhenotypeList output;

	SqlQuery query = getQuery();
	query.prepare("SELECT hpo_term_id FROM hpo_genes WHERE gene=:0");
	query.bindValue(0, symbol);
	query.exec();
	while(query.next())
	{
		output << phenotype(query.value(0).toInt());
	}
	output.sortByName();
	return output;
}

PhenotypeList NGSD::phenotypes(QStringList search_terms)
{
	//trim terms and remove empty terms
	std::for_each(search_terms.begin(), search_terms.end(), [](QString& term){ term = term.trimmed(); });
	search_terms.removeAll("");

	PhenotypeList list;

	if (search_terms.isEmpty()) //no terms => all phenotypes
	{
		SqlQuery query = getQuery();
		query.exec("SELECT id FROM hpo_term ORDER BY name ASC");
		while(query.next())
		{
			list << phenotype(query.value(0).toInt());
		}
	}
	else //search for terms (intersect results of all terms)
	{
		bool first = true;
		QSet<Phenotype> set;
		SqlQuery query = getQuery();
		query.prepare("SELECT id FROM hpo_term WHERE name LIKE :0 OR hpo_id LIKE :1 OR synonyms LIKE :2");
		foreach(const QString& term, search_terms)
		{
			query.bindValue(0, "%" + term + "%");
			query.bindValue(1, "%" + term + "%");
			query.bindValue(2, "%" + term + "%");
			query.exec();
			QSet<Phenotype> tmp;
			while(query.next())
			{
				tmp << phenotype(query.value(0).toInt());
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

		list << set;
		list.sortByName();
	}

	return list;
}

GeneSet NGSD::phenotypeToGenes(int id, bool recursive, bool ignore_non_phenotype_terms)
{
	//prepare ignored terms
	QSet<int> ignored_terms_ids;
	if (ignore_non_phenotype_terms)
	{
		int pheno_inh = phenotypeIdByAccession("HP:0000005"); //"Mode of inheritance"
		ignored_terms_ids << pheno_inh;
		foreach(const Phenotype& pheno, phenotypeChildTerms(pheno_inh, true))
		{
			ignored_terms_ids << phenotypeIdByAccession(pheno.accession());
		}
		int pheno_freq = phenotypeIdByAccession("HP:0040279"); //"Frequency"
		ignored_terms_ids << pheno_freq;
		foreach(const Phenotype& pheno, phenotypeChildTerms(pheno_freq, true))
		{
			ignored_terms_ids << phenotypeIdByAccession(pheno.accession());
		}
	}

	//create a list of phenotype database ids
	QList<int> pheno_ids;
	pheno_ids << id;
	if (recursive)
	{
		foreach(const Phenotype& pheno, phenotypeChildTerms(id, true))
		{
			pheno_ids << phenotypeIdByAccession(pheno.accession());
		}
	}

	//create output gene set
	GeneSet genes;
	SqlQuery pid2genes = getQuery();
	pid2genes.prepare("SELECT gene FROM hpo_genes WHERE hpo_term_id=:0");
	while (!pheno_ids.isEmpty())
	{
		int id = pheno_ids.takeLast();
		if (ignore_non_phenotype_terms && ignored_terms_ids.contains(id)) continue;

		pid2genes.bindValue(0, id);
		pid2genes.exec();
		while(pid2genes.next())
		{
			QByteArray gene = pid2genes.value(0).toByteArray();
			genes.insert(geneToApproved(gene, true));
		}

	}

	return genes;
}

PhenotypeList NGSD::phenotypeChildTerms(int term_id, bool recursive)
{
	PhenotypeList output;

	//prepare queries
	SqlQuery pid2children = getQuery();
	pid2children.prepare("SELECT child FROM hpo_parent WHERE parent=:0");

	//convert term ids to genes
	QList<int> term_ids;
	term_ids << term_id;
	while (!term_ids.isEmpty())
	{
		int id = term_ids.takeLast();

		pid2children.bindValue(0, id);
		pid2children.exec();
		while(pid2children.next())
		{
			int id_child = pid2children.value(0).toInt();
			output << phenotype(id_child);
			if (recursive)
			{
				term_ids << id_child;
			}
		}
	}

	return output;
}

QList<OmimInfo> NGSD::omimInfo(const QByteArray& symbol)
{
	QList<OmimInfo> output;

	//get matching ids
	QString symbol_approved = geneToApproved(symbol, true);
	QStringList omim_gene_ids = getValues("SELECT id FROM omim_gene WHERE gene=:0 OR gene='" + symbol_approved + "' ORDER BY mim", symbol);
	foreach(QString omim_gene_id, omim_gene_ids)
	{
		OmimInfo info;
		info.mim = getValue("SELECT mim FROM omim_gene WHERE id=" + omim_gene_id).toByteArray();
		info.gene_symbol = getValue("SELECT gene FROM omim_gene WHERE id=" + omim_gene_id).toByteArray();

		QRegExp mim_exp("[^0-9]([0-9]{6})[^0-9]");
		QStringList phenos = getValues("SELECT phenotype FROM omim_phenotype WHERE omim_gene_id=" + omim_gene_id + " ORDER BY phenotype ASC");
		foreach(QString pheno, phenos)
		{
			Phenotype tmp;

			tmp.setName(pheno.toLatin1());
			if (mim_exp.indexIn(pheno)!=-1)
			{
				tmp.setAccession(mim_exp.cap(1).toLatin1());
			}

			info.phenotypes << tmp;
		}

		output << info;
	}

	return output;
}

QString NGSD::omimPreferredPhenotype(const QByteArray& symbol, const QByteArray& disease_group)
{
	SqlQuery query = getQuery();
	query.prepare("SELECT phenotype_accession FROM omim_preferred_phenotype WHERE gene=:0 AND disease_group=:1");

	query.bindValue(0, symbol);
	query.bindValue(1, disease_group);
	query.exec();

	if (query.next())
	{
		return query.value(0).toString();
	}
	else
	{
		return "";
	}
}

const GeneSet& NGSD::approvedGeneNames()
{
	GeneSet& output = getCache().approved_gene_names;

	if (output.isEmpty())
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

QMap<QByteArray, QByteArrayList> NGSD::getPreferredTranscripts()
{
	QMap<QByteArray, QByteArrayList> output;

	SqlQuery query = getQuery();
	query.exec("SELECT g.symbol, pt.name FROM gene g, gene_transcript gt, preferred_transcripts pt WHERE g.id=gt.gene_id AND gt.name=pt.name");
	while(query.next())
	{
		QByteArray gene = query.value(0).toByteArray().trimmed();
		QByteArray transcript = query.value(1).toByteArray().trimmed();
		output[gene].append(transcript);
	}

	return output;
}

bool NGSD::addPreferredTranscript(QByteArray transcript_name)
{
	transcript_name = transcript_name.trimmed();

	//check if already present
	QVariant pt_id = getValue("SELECT id FROM preferred_transcripts WHERE name=:0", true, transcript_name);
	if (pt_id.isValid()) return false;

	//check if valid transcript name.
	QVariant gt_id = getValue("SELECT id FROM gene_transcript WHERE name=:0 AND source='ensembl'", true, transcript_name);
	if (!gt_id.isValid()) THROW(DatabaseException, "Invalid Ensembl transcript name '" + transcript_name + "' given in NGSD::addPreferredTranscript!");

	//insert
	SqlQuery query = getQuery();
	query.prepare("INSERT INTO `preferred_transcripts`(`name`, `added_by`, `added_date`) VALUES (:0,:1,NOW())");
	query.bindValue(0, transcript_name);
	query.bindValue(1, LoginManager::userIdAsString());
	query.exec();

	return true;
}

int NGSD::phenotypeIdByName(const QByteArray& name, bool throw_on_error)
{
	SqlQuery q = getQuery();
	q.prepare("SELECT id FROM hpo_term WHERE name=:0");
	q.bindValue(0, name);
	q.exec();

	if (!q.next())
	{
		if (throw_on_error)
		{
			THROW(DatabaseException, "Unknown HPO phenotype name '" + name + "'!");
		}
		else
		{
			return -1;
		}
	}

	return q.value(0).toInt();
}

int NGSD::phenotypeIdByAccession(const QByteArray& accession, bool throw_on_error)
{
	QHash<QByteArray, int>& cache = getCache().phenotypes_accession_to_id;

	//init cache
	if (cache.isEmpty())
	{
		SqlQuery q = getQuery();
		q.exec("SELECT hpo_id, id FROM hpo_term");
		while (q.next())
		{
			cache[q.value(0).toByteArray()] = q.value(1).toInt();
		}
	}

	//check phenotype is in cache
	if (!cache.contains(accession))
	{
		if (throw_on_error)
		{
			THROW(DatabaseException, "Unknown HPO phenotype accession '" + accession + "'!");
		}
		else
		{
			return -1;
		}
	}

	return cache[accession];
}

const Phenotype& NGSD::phenotype(int id)
{
	QHash<int, Phenotype>& cache = getCache().phenotypes_by_id;

	//init cache
	if (cache.isEmpty())
	{
		SqlQuery q = getQuery();
		q.exec("SELECT id, hpo_id, name FROM hpo_term");
		while (q.next())
		{
			cache[q.value(0).toInt()] = Phenotype(q.value(1).toByteArray(), q.value(2).toByteArray());
		}
	}

	//check phenotype is in cache
	if (!cache.contains(id))
	{
		THROW(DatabaseException, "HPO phenotype with id '" + QString::number(id) + "' not found in NGSD!");
	}

	return cache[id];
}

GeneSet NGSD::genesOverlapping(const Chromosome& chr, int start, int end, int extend)
{
	//init cache
	BedFile& bed = getCache().gene_regions;
	ChromosomalIndex<BedFile>& index = getCache().gene_regions_index;

	if (bed.isEmpty())
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
	//init cache
	BedFile& bed = getCache().gene_exons;
	ChromosomalIndex<BedFile>& index = getCache().gene_exons_index;

	if (bed.isEmpty())
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

	output.sort();
	if (!annotate_transcript_names) output.removeDuplicates();

	return output;
}

BedFile NGSD::genesToRegions(const GeneSet& genes, Transcript::SOURCE source, QString mode, bool fallback, bool annotate_transcript_names, QTextStream* messages)
{
	BedFile output;

	foreach(const QByteArray& gene, genes)
	{
		output.add(geneToRegions(gene, source, mode, fallback, annotate_transcript_names, messages));
	}

	output.sort();
	if (!annotate_transcript_names) output.removeDuplicates();

	return output;
}

int NGSD::transcriptId(QString name, bool throw_on_error)
{
	QVariant value = getValue("SELECT id FROM gene_transcript WHERE name=:0", true, name);
	if (!value.isValid() && name.contains('.')) //if not found, try without version number (if present)
	{
		value = getValue("SELECT id FROM gene_transcript WHERE name=:0", true, name.left(name.indexOf('.')));
	}
	if (!value.isValid())
	{
		if (!throw_on_error) return -1;
		THROW(DatabaseException, "No transcript with name '" + name + "' found in NGSD!");
	}
	return value.toInt();
}

QList<Transcript> NGSD::transcripts(int gene_id, Transcript::SOURCE source, bool coding_only)
{
	QList<Transcript> output;

	//get chromosome
	QString gene_id_str = QString::number(gene_id);

	//get transcripts
	SqlQuery query = getQuery();
	query.exec("SELECT id FROM gene_transcript WHERE gene_id=" + gene_id_str + " AND source='" + Transcript::sourceToString(source) + "' " + (coding_only ? "AND start_coding IS NOT NULL AND end_coding IS NOT NULL" : "") + " ORDER BY name");
	while(query.next())
	{
		output.push_back(transcript(query.value(0).toInt()));
	}

	return output;
}

Transcript NGSD::transcript(int id)
{
	QString id_str = QString::number(id);

	SqlQuery query = getQuery();
	query.exec("SELECT source, name, chromosome, start_coding, end_coding, strand FROM gene_transcript WHERE id=" + id_str);
	if (query.size()==0) THROW(DatabaseException, "Could not find transcript with identifer  '" + id_str + "' in NGSD!");
	query.next();

	//get base information
	Transcript transcript;
	transcript.setName(query.value(1).toByteArray());
	transcript.setSource(Transcript::stringToSource(query.value(0).toString()));
	transcript.setStrand(Transcript::stringToStrand(query.value(5).toByteArray()));

	//get exons
	BedFile regions;
	Chromosome chr = query.value(2).toByteArray();
	SqlQuery query2 = getQuery();
	query2.exec("SELECT start, end FROM gene_exon WHERE transcript_id=" + id_str + " ORDER BY start");
	while(query2.next())
	{
		int start = query2.value(0).toInt();
		int end = query2.value(1).toInt();
		regions.append(BedLine(chr, start, end));
	}

	int start_coding = query.value(3).isNull() ? 0 : query.value(3).toInt();
	int end_coding = query.value(4).isNull() ? 0 : query.value(4).toInt();
	if (transcript.strand()==Transcript::MINUS)
	{
		int tmp = start_coding;
		start_coding = end_coding;
		end_coding = tmp;
	}
	transcript.setRegions(regions, start_coding, end_coding);

	return transcript;
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
	auto max_it = std::max_element(list.begin(), list.end(), [](const Transcript& a, const Transcript& b){ return a.codingRegions().baseCount() < b.codingRegions().baseCount(); });
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

void NGSD::setDiagnosticStatus(const QString& processed_sample_id, DiagnosticStatusData status)
{
	//get current user ID
	QString user_id = LoginManager::userIdAsString();

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

QString NGSD::reportConfigSummaryText(const QString& processed_sample_id)
{
	QString output;

	QVariant rc_id = getValue("SELECT id FROM report_configuration WHERE processed_sample_id=:0", true, processed_sample_id);
	if (rc_id.isValid())
	{
		output = "exists";

		//find causal small variants
		{
			QStringList causal_ids = getValues("SELECT variant_id FROM report_configuration_variant WHERE causal='1' AND report_configuration_id=" + rc_id.toString());
			foreach(const QString& id, causal_ids)
			{
				Variant var = variant(id);
				QString genotype = getValue("SELECT genotype FROM detected_variant WHERE processed_sample_id='" + processed_sample_id + "' AND variant_id='" + id + "'").toString();
				QString genes = genesOverlapping(var.chr(), var.start(), var.end(), 5000).join(", ");
				QString var_class = getValue("SELECT class FROM variant_classification WHERE variant_id='" + id + "'").toString();
				output += ", causal variant: " + var.toString() + " (genotype:" + genotype + " genes:" + genes;
				if (var_class != "") output += " classification:" + var_class; // add classification, if exists
				output += ")";
			}
		}

		//find causal CNVs
		{
			QStringList causal_ids = getValues("SELECT cnv_id FROM report_configuration_cnv WHERE causal='1' AND report_configuration_id=" + rc_id.toString());
			foreach(const QString& id, causal_ids)
			{
				CopyNumberVariant var = cnv(id.toInt());
				QString cn = getValue("SELECT cn FROM cnv WHERE id='" + id + "'").toString();
				QString cnv_class = getValue("SELECT class FROM report_configuration_cnv WHERE cnv_id='" + id + "'", false).toString();
				output += ", causal CNV: " + var.toString() + " (cn:" + cn;
				if (cnv_class != "") output += " classification:" + cnv_class; // add classification, if exists
				output += ")";
			}
		}

		//find causal SVs
		{
			QStringList sv_id_columns = QStringList() << "sv_deletion_id" << "sv_duplication_id" << "sv_insertion_id" << "sv_inversion_id" << "sv_translocation_id";
			QList<StructuralVariantType> sv_types = {StructuralVariantType::DEL, StructuralVariantType::DUP, StructuralVariantType::INS, StructuralVariantType::INV, StructuralVariantType::BND};
			BedpeFile svs;
			for (int i = 0; i < sv_id_columns.size(); ++i)
			{
				QStringList causal_ids = getValues("SELECT " + sv_id_columns.at(i) + " FROM report_configuration_sv WHERE causal='1' AND report_configuration_id=" + rc_id.toString() + " AND " + sv_id_columns.at(i) + " IS NOT NULL");

				foreach(const QString& id, causal_ids)
				{
					BedpeLine var = structuralVariant(id.toInt(), sv_types.at(i), svs, true);
					QString sv_class = getValue("SELECT class FROM report_configuration_sv WHERE " + sv_id_columns[i] + "='" + id + "'", false).toString();
					output += ", causal SV: " + var.toString();
					if (sv_class != "") output += " (classification:" + sv_class + ")"; // add classification, if exists
				}
			}
		}
	}

	return output;
}

bool NGSD::reportConfigIsFinalized(int id)
{
	return getValue("SELECT id FROM `report_configuration` WHERE `id`=" + QString::number(id) + " AND finalized_by IS NOT NULL").isValid();
}

QSharedPointer<ReportConfiguration> NGSD::reportConfig(int conf_id, const VariantList& variants, const CnvList& cnvs, const BedpeFile& svs, QStringList& messages)
{
	QSharedPointer<ReportConfiguration> output = QSharedPointer<ReportConfiguration>(new ReportConfiguration());

	//load main object
	SqlQuery query = getQuery();
	query.exec("SELECT (SELECT name FROM user WHERE id=created_by) as created_by, created_date, (SELECT name FROM user WHERE id=last_edit_by) as last_edit_by, last_edit_date, (SELECT name FROM user WHERE id=finalized_by) as finalized_by, finalized_date FROM report_configuration WHERE id=" + QString::number(conf_id));
	query.next();
	output->setCreatedBy(query.value("created_by").toString());
	output->setCreatedAt(query.value("created_date").toDateTime());
	output->last_updated_by_ = query.value("last_edit_by").toString();
	output->last_updated_at_ = query.value("last_edit_date").toDateTime();
	output->finalized_by_ = query.value("finalized_by").toString();
	output->finalized_at_ = query.value("finalized_date").toDateTime();

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

		output->set(var_conf);
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

		output->set(var_conf);
	}

	// Skip report import if empty sv file is provided (Trio)
	if (svs.count() > 0)
	{
		//load SV data
		query.exec("SELECT * FROM report_configuration_sv WHERE report_configuration_id=" + QString::number(conf_id));
		while(query.next())
		{
			ReportVariantConfiguration var_conf;
			var_conf.variant_type = VariantType::SVS;

			//get SV id
			int sv_id;
			StructuralVariantType type;

			//determine SV type and id
			if(!query.value("sv_deletion_id").isNull())
			{
				type = StructuralVariantType::DEL;
				sv_id = query.value("sv_deletion_id").toInt();
			}
			else if(!query.value("sv_duplication_id").isNull())
			{
				type = StructuralVariantType::DUP;
				sv_id = query.value("sv_duplication_id").toInt();
			}
			else if(!query.value("sv_insertion_id").isNull())
			{
				type = StructuralVariantType::INS;
				sv_id = query.value("sv_insertion_id").toInt();
			}
			else if(!query.value("sv_inversion_id").isNull())
			{
				type = StructuralVariantType::INV;
				sv_id = query.value("sv_inversion_id").toInt();
			}
			else if(!query.value("sv_translocation_id").isNull())
			{
				type = StructuralVariantType::BND;
				sv_id = query.value("sv_translocation_id").toInt();
			}
			else
			{
				THROW(DatabaseException, "Report config entry does not contain a SV id!");
			}

			BedpeLine sv = structuralVariant(sv_id, type, svs);

			var_conf.variant_index = svs.findMatch(sv, true, false);
			if (var_conf.variant_index==-1)
			{
				messages << "Could not find SV '" + BedpeFile::typeToString(sv.type()) + " " + sv.positionRange() + "' in given variant list!";
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

			output->set(var_conf);
		}

	}

	return output;
}

int NGSD::setReportConfig(const QString& processed_sample_id, QSharedPointer<ReportConfiguration> config, const VariantList& variants, const CnvList& cnvs, const BedpeFile& svs)
{
	int id = reportConfigId(processed_sample_id);
	QString id_str = QString::number(id);

	//check that it is not finalized
	if (id!=-1)
	{
		if (reportConfigIsFinalized(id))
		{
			THROW (ProgrammingException, "Cannot update report configuration with id=" + id_str + ", because it is finalized!");
		}
	}

	try
	{
		transaction();

		if (id!=-1) //clear old report config
		{
			//delete report config variants if it already exists
			SqlQuery query = getQuery();
			query.exec("DELETE FROM `report_configuration_variant` WHERE report_configuration_id=" + id_str);
			query.exec("DELETE FROM `report_configuration_cnv` WHERE report_configuration_id=" + id_str);
			query.exec("DELETE FROM `report_configuration_sv` WHERE report_configuration_id=" + id_str);

			//update report config
			query.exec("UPDATE `report_configuration` SET `last_edit_by`='" + LoginManager::userIdAsString() + "', `last_edit_date`=CURRENT_TIMESTAMP WHERE id=" + id_str);
		}
		else //create report config (if missing)
		{
			//insert new report config
			int user_id = userId(config->createdBy());

			SqlQuery query = getQuery();
			query.prepare("INSERT INTO `report_configuration`(`processed_sample_id`, `created_by`, `created_date`, `last_edit_by`, `last_edit_date`) VALUES (:0, :1, :2, :3, CURRENT_TIMESTAMP)");
			query.bindValue(0, processed_sample_id);
			query.bindValue(1, user_id);
			query.bindValue(2, config->createdAt());
			query.bindValue(3, user_id);
			query.exec();
			id = query.lastInsertId().toInt();
		}

		//store variant data
		SqlQuery query_var = getQuery();
		query_var.prepare("INSERT INTO `report_configuration_variant`(`report_configuration_id`, `variant_id`, `type`, `causal`, `inheritance`, `de_novo`, `mosaic`, `compound_heterozygous`, `exclude_artefact`, `exclude_frequency`, `exclude_phenotype`, `exclude_mechanism`, `exclude_other`, `comments`, `comments2`) VALUES (:0, :1, :2, :3, :4, :5, :6, :7, :8, :9, :10, :11, :12, :13, :14)");
		SqlQuery query_cnv = getQuery();
		query_cnv.prepare("INSERT INTO `report_configuration_cnv`(`report_configuration_id`, `cnv_id`, `type`, `causal`, `class`, `inheritance`, `de_novo`, `mosaic`, `compound_heterozygous`, `exclude_artefact`, `exclude_frequency`, `exclude_phenotype`, `exclude_mechanism`, `exclude_other`, `comments`, `comments2`) VALUES (:0, :1, :2, :3, :4, :5, :6, :7, :8, :9, :10, :11, :12, :13, :14, :15)");
		SqlQuery query_sv = getQuery();
		query_sv.prepare("INSERT INTO `report_configuration_sv`(`report_configuration_id`, `sv_deletion_id`, `sv_duplication_id`, `sv_insertion_id`, `sv_inversion_id`, `sv_translocation_id`, `type`, `causal`, `class`, `inheritance`, `de_novo`, `mosaic`, `compound_heterozygous`, `exclude_artefact`, `exclude_frequency`, `exclude_phenotype`, `exclude_mechanism`, `exclude_other`, `comments`, `comments2`) VALUES (:0, :1, :2, :3, :4, :5, :6, :7, :8, :9, :10, :11, :12, :13, :14, :15, :16, :17, :18, :19)");
		foreach(const ReportVariantConfiguration& var_conf, config->variantConfig())
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
			else if (var_conf.variant_type==VariantType::SVS)
			{
				//check SV index exists in SV list
				if (var_conf.variant_index<0 || var_conf.variant_index>=svs.count())
				{
					THROW(ProgrammingException, "SV list does not contain SV with index '" + QString::number(var_conf.variant_index) + "' in NGSD::setReportConfig!");
				}

				//check that report SV callset exists
				QVariant callset_id = getValue("SELECT id FROM sv_callset WHERE processed_sample_id=" + processed_sample_id, true);
				if (!callset_id.isValid())
				{
					THROW(ProgrammingException, "No SV callset defined for processed sample with ID '" + processed_sample_id + "' in NGSD::setReportConfig!");
				}

				//get SV id and table (add SV if not in DB)
				const BedpeLine& sv = svs[var_conf.variant_index];
				QString sv_id = svId(sv, callset_id.toInt(), svs, false);
				if (sv_id == "")
				{
					sv_id = QByteArray::number(addSv(callset_id.toInt(), sv, svs));
				}

				//define SQL query
				query_sv.bindValue(0, id);
				query_sv.bindValue(1, QVariant(QVariant::String));
				query_sv.bindValue(2, QVariant(QVariant::String));
				query_sv.bindValue(3, QVariant(QVariant::String));
				query_sv.bindValue(4, QVariant(QVariant::String));
				query_sv.bindValue(5, QVariant(QVariant::String));
				query_sv.bindValue(6, var_conf.report_type);
				query_sv.bindValue(7, var_conf.causal);
				query_sv.bindValue(8, var_conf.classification);
				query_sv.bindValue(9, var_conf.inheritance);
				query_sv.bindValue(10, var_conf.de_novo);
				query_sv.bindValue(11, var_conf.mosaic);
				query_sv.bindValue(12, var_conf.comp_het);
				query_sv.bindValue(13, var_conf.exclude_artefact);
				query_sv.bindValue(14, var_conf.exclude_frequency);
				query_sv.bindValue(15, var_conf.exclude_phenotype);
				query_sv.bindValue(16, var_conf.exclude_mechanism);
				query_sv.bindValue(17, var_conf.exclude_other);
				query_sv.bindValue(18, var_conf.comments.isEmpty() ? "" : var_conf.comments);
				query_sv.bindValue(19, var_conf.comments2.isEmpty() ? "" : var_conf.comments2);

				// set SV id
				switch (sv.type())
				{
					case StructuralVariantType::DEL:
						query_sv.bindValue(1, sv_id);
						break;
					case StructuralVariantType::DUP:
						query_sv.bindValue(2, sv_id);
						break;
					case StructuralVariantType::INS:
						query_sv.bindValue(3, sv_id);
						break;
					case StructuralVariantType::INV:
						query_sv.bindValue(4, sv_id);
						break;
					case StructuralVariantType::BND:
						query_sv.bindValue(5, sv_id);
						break;
					default:
						THROW(ArgumentException, "Invalid structural variant type!")
						break;
				}

				query_sv.exec();

			}
			else
			{
				THROW(NotImplementedException, "Storing of report config variants with type '" + QString::number((int)var_conf.variant_type) + "' not implemented!");
			}
		}

		commit();
	}
	catch(...)
	{
		rollback();
		throw;
	}

	return id;
}

void NGSD::finalizeReportConfig(int id, int user_id)
{
	QString rc_id = QString::number(id);

	//check that report config exists
	bool rc_exists = getValue("SELECT id FROM `report_configuration` WHERE `id`=" + rc_id).isValid();
	if (!rc_exists)
	{
		THROW (ProgrammingException, "Cannot finalize report configuration with id=" + rc_id + ", because it does not exist!");
	}

	//check that report config is not finalized
	if (reportConfigIsFinalized(id))
	{
		THROW (ProgrammingException, "Cannot finalize report configuration with id=" + QString::number(id) + ", because it is finalized!");
	}

	//finalize it
	SqlQuery query = getQuery();
	query.exec("UPDATE `report_configuration` SET finalized_by='" + QString::number(user_id) + "', finalized_date=NOW() WHERE `id`=" + rc_id);
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

	//check that it is not finalized
	if (reportConfigIsFinalized(id))
	{
		THROW (ProgrammingException, "Cannot delete report configuration with id=" + rc_id + ", because it is finalized!");
	}

	//delete
	SqlQuery query = getQuery();
	query.exec("DELETE FROM `report_configuration_cnv` WHERE `report_configuration_id`=" + rc_id);
	query.exec("DELETE FROM `report_configuration_variant` WHERE `report_configuration_id`=" + rc_id);
	query.exec("DELETE FROM `report_configuration_sv` WHERE `report_configuration_id`=" + rc_id);
	query.exec("DELETE FROM `report_configuration` WHERE `id`=" + rc_id);
}

EvaluationSheetData NGSD::evaluationSheetData(const QString& processed_sample_id, bool throw_if_fails)
{
	EvaluationSheetData evaluation_sheet_data;
	SqlQuery query = getQuery();
	query.exec("SELECT * FROM evaluation_sheet_data WHERE processed_sample_id=" + processed_sample_id);
	if (query.next())
	{
		// parse result
		evaluation_sheet_data.ps_id = query.value("processed_sample_id").toString();
		evaluation_sheet_data.dna_rna = query.value("dna_rna_id").toString();
		evaluation_sheet_data.reviewer1 = userName(query.value("reviewer1").toInt());
		evaluation_sheet_data.review_date1 = query.value("review_date1").toDate();
		evaluation_sheet_data.reviewer2 = userName(query.value("reviewer2").toInt());
		evaluation_sheet_data.review_date2 = query.value("review_date2").toDate();

		evaluation_sheet_data.analysis_scope = query.value("analysis_scope").toString();
		evaluation_sheet_data.acmg_requested = query.value("acmg_requested").toBool();
		evaluation_sheet_data.acmg_noticeable = query.value("acmg_noticeable").toBool();
		evaluation_sheet_data.acmg_analyzed = query.value("acmg_analyzed").toBool();

		evaluation_sheet_data.filtered_by_freq_based_dominant = query.value("filtered_by_freq_based_dominant").toBool();
		evaluation_sheet_data.filtered_by_freq_based_recessive = query.value("filtered_by_freq_based_recessive").toBool();
		evaluation_sheet_data.filtered_by_mito =  query.value("filtered_by_mito").toBool();
		evaluation_sheet_data.filtered_by_x_chr = query.value("filtered_by_x_chr").toBool();
		evaluation_sheet_data.filtered_by_cnv = query.value("filtered_by_cnv").toBool();
		evaluation_sheet_data.filtered_by_svs = query.value("filtered_by_svs").toBool();
		evaluation_sheet_data.filtered_by_res = query.value("filtered_by_res").toBool();
		evaluation_sheet_data.filtered_by_mosaic = query.value("filtered_by_mosaic").toBool();
		evaluation_sheet_data.filtered_by_phenotype = query.value("filtered_by_phenotype").toBool();
		evaluation_sheet_data.filtered_by_multisample = query.value("filtered_by_multisample").toBool();
		evaluation_sheet_data.filtered_by_trio_stringent = query.value("filtered_by_trio_stringent").toBool();
		evaluation_sheet_data.filtered_by_trio_relaxed = query.value("filtered_by_trio_relaxed").toBool();
	}
	else
	{
		// error handling
		if (throw_if_fails)
		{
			THROW(DatabaseException, "No entry found in table 'evaluation_sheet_data' for processed sample id '" + processed_sample_id +"'!");
		}
	}
	return evaluation_sheet_data;
}

int NGSD::storeEvaluationSheetData(const EvaluationSheetData& evaluation_sheet_data, bool overwrite_existing_data)
{
	QVariant id = getValue("SELECT id FROM evaluation_sheet_data WHERE processed_sample_id=:0", true, evaluation_sheet_data.ps_id);
	if (!id.isNull() && (!overwrite_existing_data)) THROW(DatabaseException, "Evaluation sheet data for processed sample id '" + evaluation_sheet_data.ps_id + "' already exists in NGSD table!");

	// generate query
	QString query_string = QString("REPLACE INTO evaluation_sheet_data (processed_sample_id, dna_rna_id, reviewer1, review_date1, reviewer2, review_date2, analysis_scope, acmg_requested, ")
			+ "acmg_noticeable, acmg_analyzed, filtered_by_freq_based_dominant, filtered_by_freq_based_recessive, filtered_by_mito, filtered_by_x_chr, filtered_by_cnv, filtered_by_svs, "
			+ "filtered_by_res, filtered_by_mosaic, filtered_by_phenotype, filtered_by_multisample, filtered_by_trio_stringent, filtered_by_trio_relaxed) "
			+ "VALUES (:0, :1, :2, :3, :4, :5, :6, :7, :8, :9, :10, :11, :12, :13, :14, :15, :16, :17, :18, :19, :20, :21)";

	// prepare query
	SqlQuery query = getQuery();
	query.prepare(query_string);

	// bind values
	query.bindValue(0, evaluation_sheet_data.ps_id);
	query.bindValue(1, evaluation_sheet_data.dna_rna);
	query.bindValue(2, userId(evaluation_sheet_data.reviewer1));
	query.bindValue(3, evaluation_sheet_data.review_date1);
	query.bindValue(4, userId(evaluation_sheet_data.reviewer2));
	query.bindValue(5, evaluation_sheet_data.review_date2);
	query.bindValue(6, evaluation_sheet_data.analysis_scope);
	query.bindValue(7, evaluation_sheet_data.acmg_requested);
	query.bindValue(8, evaluation_sheet_data.acmg_noticeable);
	query.bindValue(9, evaluation_sheet_data.acmg_analyzed);
	query.bindValue(10, evaluation_sheet_data.filtered_by_freq_based_dominant);
	query.bindValue(11, evaluation_sheet_data.filtered_by_freq_based_recessive);
	query.bindValue(12, evaluation_sheet_data.filtered_by_mito);
	query.bindValue(13, evaluation_sheet_data.filtered_by_x_chr);
	query.bindValue(14, evaluation_sheet_data.filtered_by_cnv);
	query.bindValue(15, evaluation_sheet_data.filtered_by_svs);
	query.bindValue(16, evaluation_sheet_data.filtered_by_res);
	query.bindValue(17, evaluation_sheet_data.filtered_by_mosaic);
	query.bindValue(18, evaluation_sheet_data.filtered_by_phenotype);
	query.bindValue(19, evaluation_sheet_data.filtered_by_multisample);
	query.bindValue(20, evaluation_sheet_data.filtered_by_trio_stringent);
	query.bindValue(21, evaluation_sheet_data.filtered_by_trio_relaxed);

	// insert into NGSD
	query.exec();

	// return id
	return query.lastInsertId().toInt();
}

QStringList NGSD::relatedSamples(const QString& sample_id, const QString& relation)
{
	// check if relation is valid
	QString q_relation_type;
	if (relation != "")
	{
		QStringList allowed_relation = getEnum("sample_relations", "relation");
		if (!allowed_relation.contains(relation))
		{
			THROW(ArgumentException, "Invalid relation type '" + relation + "' given!");
		}
		q_relation_type = " AND relation='" + relation + "'";
	}

	QStringList related_sample_ids;

	SqlQuery query = getQuery();
	query.exec("SELECT sample1_id, sample2_id FROM sample_relations WHERE (sample1_id=" + sample_id + " OR sample2_id=" + sample_id + ")" + q_relation_type);

	while(query.next())
	{
		QString id1 = query.value("sample1_id").toString();
		QString id2 = query.value("sample2_id").toString();

		if ((id1 == sample_id) && (id2 != sample_id))
		{
			// related sample is index 2
			related_sample_ids << id2;
			continue;
		}
		if ((id1 != sample_id) && (id2 == sample_id))
		{
			// related sample is index 1
			related_sample_ids << id1;
			continue;
		}
		THROW(ProgrammingException, "Sample relation does not fit query!");
	}

	return related_sample_ids;
}

void NGSD::addSampleRelation(const SampleRelation& rel, bool error_if_already_present)
{
	QString query_ext = error_if_already_present ? "" : " ON DUPLICATE KEY UPDATE relation=VALUES(relation)";
	//skip samples that already have a relation in NGSD

	SqlQuery query = getQuery();
	query.prepare("INSERT INTO `sample_relations`(`sample1_id`, `relation`, `sample2_id`, `user_id`) VALUES (:0, :1, :2, "+QString::number(LoginManager::userId())+")" + query_ext);
	query.bindValue(0, sampleId(rel.sample1));
	query.bindValue(1, rel.relation);
	query.bindValue(2, sampleId(rel.sample2));
	query.exec();
}

SomaticReportConfigurationData NGSD::somaticReportConfigData(int id)
{
	SqlQuery query = getQuery();
	query.exec("SELECT created_by, created_date, (SELECT name FROM user WHERE id=last_edit_by) as last_edit_by, last_edit_date, mtb_xml_upload_date, target_file FROM somatic_report_configuration WHERE id=" + QString::number(id));
	query.next();

	SomaticReportConfigurationData output;
	output.created_by = userName(query.value("created_by").toInt());
	QDateTime created_date = query.value("created_date").toDateTime();
	output.created_date = created_date.isNull() ? "" : created_date.toString("dd.MM.yyyy hh:mm:ss");
	output.last_edit_by = query.value("last_edit_by").toString();
	QDateTime last_edit_date = query.value("last_edit_date").toDateTime();
	output.last_edit_date = last_edit_date.isNull() ? "" : last_edit_date.toString("dd.MM.yyyy hh:mm:ss");

	if(!query.value("target_file").isNull()) output.target_file = query.value("target_file").toString();
	else output.target_file = "";

	if( !query.value("mtb_xml_upload_date" ).isNull()) output.mtb_xml_upload_date = query.value("mtb_xml_upload_date").toDateTime().toString("dd.MM.yyyy hh:mm:ss");
	else output.mtb_xml_upload_date = "";

	return output;
}


int NGSD::somaticReportConfigId(QString t_ps_id, QString n_ps_id)
{
	//Identify report configuration using tumor and normal processed sample ids (and rna ps if available)
	QString query = "SELECT id FROM somatic_report_configuration WHERE ps_tumor_id='" +t_ps_id + "' AND ps_normal_id='" + n_ps_id + "'";
	QVariant id = getValue(query, true);
	return id.isValid() ? id.toInt() : -1;
}

int NGSD::setSomaticReportConfig(QString t_ps_id, QString n_ps_id, const SomaticReportConfiguration& config, const VariantList& snvs, const CnvList& cnvs, const VariantList& germl_snvs, QString user_name)
{
	int id = somaticReportConfigId(t_ps_id, n_ps_id);

	QString target_file = "";
	if(!config.targetRegionName().isEmpty())
	{
		target_file = QFileInfo(config.targetRegionName()).fileName(); //store filename without path
	}

	if(id != -1) //delete old report if id exists
	{
		//Delete somatic report configuration variants that are assigned to report
		SqlQuery query = getQuery();
		query.exec("DELETE FROM `somatic_report_configuration_variant` WHERE somatic_report_configuration_id=" + QByteArray::number(id));
		query.exec("DELETE FROM `somatic_report_configuration_cnv` WHERE somatic_report_configuration_id=" + QByteArray::number(id));
		query.exec("DELETE FROM `somatic_report_configuration_germl_var` WHERE somatic_report_configuration_id=" + QByteArray::number(id));


		//Update somatic report configuration: last_edit_by, last_edit_user and target_file
		query.prepare("UPDATE `somatic_report_configuration` SET `last_edit_by`= :0, `last_edit_date` = CURRENT_TIMESTAMP, `target_file`= :1, `tum_content_max_af` =:2, `tum_content_max_clonality` =:3, `tum_content_hist` =:4, `msi_status` =:5, `cnv_burden` =:6, `hrd_score` =:7, `hrd_statement` =:8, `cnv_loh_count` =:9, `cnv_tai_count` =:10, `cnv_lst_count` =:11, `tmb_ref_text` =:12, `quality` =:13, `fusions_detected`=:14, `cin_chr`=:15, `limitations` = :16, `filter` =:17 WHERE id=:18");
		query.bindValue(0, userId(user_name));
		if(target_file != "") query.bindValue(1, target_file);
		else query.bindValue(1, QVariant(QVariant::String));
		query.bindValue(2, config.tumContentByMaxSNV());
		query.bindValue(3, config.tumContentByClonality());
		query.bindValue(4, config.tumContentByHistological());
		query.bindValue(5, config.msiStatus());
		query.bindValue(6, config.cnvBurden());
		query.bindValue(7, config.hrdScore());

		if(getEnum("somatic_report_configuration", "hrd_statement").contains(config.hrdStatement())) query.bindValue(8, config.hrdStatement());
		else query.bindValue(8, QVariant(QVariant::String));


		query.bindValue(9, config.cnvLohCount());
		query.bindValue(10, config.cnvTaiCount());
		query.bindValue(11, config.cnvLstCount());

		query.bindValue(12, config.tmbReferenceText());

		if( getEnum("somatic_report_configuration", "quality").contains(config.quality()) ) query.bindValue(13, config.quality());
		else query.bindValue(13, QVariant(QVariant::String));

		query.bindValue(14, config.fusionsDetected());

		if(config.cinChromosomes().count() > 0)	query.bindValue( 15, config.cinChromosomes().join(',') );
		else query.bindValue( 15, QVariant(QVariant::String) );

		if( !config.limitations().isEmpty()) query.bindValue(16, config.limitations() );
		else query.bindValue( 16, QVariant(QVariant::String) );

		if( !config.filter().isEmpty() ) query.bindValue(17, config.filter());
		else query.bindValue( 17, QVariant(QVariant::String) );

		query.bindValue(18, id);
		query.exec();
	}
	else
	{
		SqlQuery query = getQuery();
		query.prepare("INSERT INTO `somatic_report_configuration` (`ps_tumor_id`, `ps_normal_id`, `created_by`, `created_date`, `last_edit_by`, `last_edit_date`, `target_file`, `tum_content_max_af`, `tum_content_max_clonality`, `tum_content_hist`, `msi_status`, `cnv_burden`, `hrd_score`, `hrd_statement`, `cnv_loh_count`, `cnv_tai_count`, `cnv_lst_count`, `tmb_ref_text`, `quality`, `fusions_detected`, `cin_chr`, `limitations`, `filter`) VALUES (:0, :1, :2, :3, :4, CURRENT_TIMESTAMP, :5, :6, :7, :8, :9, :10, :11, :12, :13, :14, :15, :16, :17, :18, :19, :20, :21)");

		query.bindValue(0, t_ps_id);
		query.bindValue(1, n_ps_id);
		query.bindValue(2, userId(config.createdBy()));
		query.bindValue(3, config.createdAt());
		query.bindValue(4, userId(user_name));
		if(target_file != "") query.bindValue(5, target_file);
		else query.bindValue(5, QVariant(QVariant::String));

		query.bindValue(6, config.tumContentByMaxSNV());
		query.bindValue(7, config.tumContentByClonality());
		query.bindValue(8, config.tumContentByHistological());

		query.bindValue(9, config.msiStatus());
		query.bindValue(10, config.cnvBurden());
		query.bindValue(11, config.hrdScore());

		if( getEnum("somatic_report_configuration", "hrd_statement").contains(config.hrdStatement()) ) query.bindValue(12, config.hrdStatement());
		else query.bindValue(12, QVariant(QVariant::String));

		query.bindValue(13, config.cnvLohCount());
		query.bindValue(14, config.cnvTaiCount());
		query.bindValue(15, config.cnvLstCount());


		query.bindValue(16, config.tmbReferenceText());

		if( getEnum("somatic_report_configuration", "quality").contains(config.quality()) ) query.bindValue(17, config.quality());
		else query.bindValue(17, QVariant(QVariant::String));

		query.bindValue(18, config.fusionsDetected());

		if(config.cinChromosomes().count() != 0) query.bindValue(19, config.cinChromosomes().join(','));
		else query.bindValue(19, QVariant(QVariant::String));

		if(!config.limitations().isEmpty()) query.bindValue(20, config.limitations());
		else query.bindValue(20, QVariant(QVariant::String));

		if( !config.filter().isEmpty() ) query.bindValue( 21, config.filter() );
		else query.bindValue( 21, QVariant(QVariant::String) );

		query.exec();
		id = query.lastInsertId().toInt();
	}

	//Store variants in NGSD
	SqlQuery query_var = getQuery();

	query_var.prepare("INSERT INTO `somatic_report_configuration_variant` (`somatic_report_configuration_id`, `variant_id`, `exclude_artefact`, `exclude_low_tumor_content`, `exclude_low_copy_number`, `exclude_high_baf_deviation`, `exclude_other_reason`, `include_variant_alteration`, `include_variant_description`, `comment`) VALUES (:0, :1, :2, :3, :4, :5, :6, :7, :8, :9)");

	SqlQuery query_cnv = getQuery();
	query_cnv.prepare("INSERT INTO `somatic_report_configuration_cnv` (`somatic_report_configuration_id`, `somatic_cnv_id`, `exclude_artefact`, `exclude_low_tumor_content`, `exclude_low_copy_number`, `exclude_high_baf_deviation`, `exclude_other_reason`, `comment`) VALUES (:0, :1, :2, :3, :4, :5, :6, :7)");


	for(const auto& var_conf : config.variantConfig())
	{
		if(var_conf.variant_type == VariantType::SNVS_INDELS)
		{
			//check whether indices exist in variant list
			if(var_conf.variant_index<0 || var_conf.variant_index >= snvs.count())
			{
				THROW(ProgrammingException, "Variant list does not contain variant with index '" + QByteArray::number(var_conf.variant_index) + "' in NGSD::setSomaticReportConfig!");
			}

			const Variant& variant = snvs[var_conf.variant_index];
			QString variant_id = variantId(variant, false);
			if(variant_id=="")
			{
				variant_id = addVariant(variant, snvs);
			}

			query_var.bindValue(0, id);
			query_var.bindValue(1, variant_id);
			query_var.bindValue(2, var_conf.exclude_artefact);
			query_var.bindValue(3, var_conf.exclude_low_tumor_content);
			query_var.bindValue(4, var_conf.exclude_low_copy_number);
			query_var.bindValue(5, var_conf.exclude_high_baf_deviation);
			query_var.bindValue(6, var_conf.exclude_other_reason);
			query_var.bindValue(7, var_conf.include_variant_alteration.trimmed().isEmpty() ? "" : var_conf.include_variant_alteration);
			query_var.bindValue(8, var_conf.include_variant_description.trimmed().isEmpty() ? "" : var_conf.include_variant_description);
			query_var.bindValue(9, var_conf.comment.trimmed().isEmpty() ? "" : var_conf.comment);

			query_var.exec();

		}
		else if(var_conf.variant_type == VariantType::CNVS)
		{
			if(var_conf.variant_index<0 || var_conf.variant_index > cnvs.count())
			{
				THROW(ProgrammingException, "Somatic CNV list does not contain CNV with index '" + QString::number(var_conf.variant_index) + "' in NGSD::setSomaticReportConfig!");
			}

			//check that report somatic CNV callset exists
			QVariant callset_id = getValue("SELECT id FROM somatic_cnv_callset WHERE ps_tumor_id='" + t_ps_id + "' AND ps_normal_id='" + n_ps_id + "'", true);
			if(!callset_id.isValid())
			{
				THROW(ProgrammingException, "No somatic CNV callset defined for tumor-normal processed sample ids " + t_ps_id + "-" + n_ps_id + "in NGSD::setSomaticReportConfig!");
			}

			const CopyNumberVariant& cnv = cnvs[var_conf.variant_index];
			QString cnv_id = somaticCnvId(cnv, callset_id.toInt(), false);

			if(cnv_id=="")
			{
				cnv_id = addSomaticCnv(callset_id.toInt(), cnv, cnvs);
			}

			query_cnv.bindValue(0, id);
			query_cnv.bindValue(1, cnv_id);
			query_cnv.bindValue(2, var_conf.exclude_artefact);
			query_cnv.bindValue(3, var_conf.exclude_low_tumor_content);
			query_cnv.bindValue(4, var_conf.exclude_low_copy_number);
			query_cnv.bindValue(5, var_conf.exclude_high_baf_deviation);
			query_cnv.bindValue(6, var_conf.exclude_other_reason);
			query_cnv.bindValue(7, var_conf.comment);

			query_cnv.exec();
		}
		else
		{
			THROW(NotImplementedException, "Storing of somatic report configuration variant with type '" + QByteArray::number((int)var_conf.variant_type) + "' not implemented!");
		}
	}

	if(germl_snvs.count() > 0)
	{
		SqlQuery query_germl_var = getQuery();

		query_germl_var.prepare("INSERT INTO `somatic_report_configuration_germl_var` (`somatic_report_configuration_id`, `variant_id`, `tum_freq`, `tum_depth`) VALUES (:0, :1, :2, :3)");

		for(const auto& var_conf : config.variantConfigGermline())
		{
			//check whether indices exist in variant list
			if(var_conf.variant_index<0 || var_conf.variant_index >= germl_snvs.count())
			{
				THROW(ProgrammingException, "Variant list does not contain variant with index '" + QByteArray::number(var_conf.variant_index) + "' in NGSD::setSomaticReportConfig!");
			}

			const Variant& variant = germl_snvs[var_conf.variant_index];

			QString variant_id = variantId(variant, false);
			if(variant_id=="")
			{
				variant_id = addVariant(variant, germl_snvs);
			}

			query_germl_var.bindValue(0, id);
			query_germl_var.bindValue(1, variant_id);


			if(BasicStatistics::isValidFloat(var_conf.tum_freq)) query_germl_var.bindValue(2, var_conf.tum_freq);
			else query_germl_var.bindValue(2, QVariant(QVariant::Double) );

			if(BasicStatistics::isValidFloat(var_conf.tum_depth)) query_germl_var.bindValue(3, var_conf.tum_depth);
			else query_germl_var.bindValue(3, QVariant(QVariant::Double) );

			query_germl_var.exec();
		}
	}

	return id;
}

void NGSD::deleteSomaticReportConfig(int id)
{
	QString report_conf_id = QString::number(id);

	bool exists = getValue("SELECT id FROM `somatic_report_configuration` WHERE `id`=" + report_conf_id).isValid();
	if(!exists)
	{
		THROW(DatabaseException, "Cannot delete somatic report configuration with id=" + report_conf_id + " because it does not exist!");
	}

	//Delete
	SqlQuery query = getQuery();
	query.exec("DELETE FROM `somatic_report_configuration_cnv` WHERE `somatic_report_configuration_id`=" + report_conf_id);
	query.exec("DELETE FROM `somatic_report_configuration_variant` WHERE `somatic_report_configuration_id`=" + report_conf_id);
	query.exec("DELETE FROM `somatic_report_configuration_germl_var` WHERE `somatic_report_configuration_id`=" + report_conf_id);
	query.exec("DELETE FROM `somatic_report_configuration` WHERE `id`=" + report_conf_id);
}

SomaticReportConfiguration NGSD::somaticReportConfig(QString t_ps_id, QString n_ps_id, const VariantList& snvs, const CnvList& cnvs,  const VariantList& germline_snvs, QStringList& messages)
{
	SomaticReportConfiguration output;

	int config_id = somaticReportConfigId(t_ps_id, n_ps_id);
	if(config_id == -1)
	{
		QString message = "Somatic report for the processed samples with the database ids " + t_ps_id + " (tumor) and " + n_ps_id + " (normal) does not exist!";
		THROW(DatabaseException, message);
	}

	SqlQuery query = getQuery();
	query.exec("SELECT u.name, r.* FROM somatic_report_configuration r, user u WHERE r.id=" + QByteArray::number(config_id) + " AND u.id = r.created_by");
	query.next();
	output.setCreatedBy(query.value("name").toString());
	output.setCreatedAt(query.value("created_date").toDateTime());
	output.setTargetRegionName(query.value("target_file").toString());

	output.setTumContentByMaxSNV(query.value("tum_content_max_af").toBool());
	output.setTumContentByClonality(query.value("tum_content_max_clonality").toBool());
	output.setTumContentByHistological(query.value("tum_content_hist").toBool());

	output.setMsiStatus(query.value("msi_status").toBool());
	output.setCnvBurden(query.value("cnv_burden").toBool());
	output.setHrdScore(query.value("hrd_score").toInt());

	output.setHrdStatement( query.value("hrd_statement").toString() );
	output.setCnvLohCount( query.value("cnv_loh_count").toInt() );
	output.setCnvLstCount( query.value("cnv_lst_count").toInt() );
	output.setCnvTaiCount( query.value("cnv_tai_count").toInt() );



	if(query.value("tmb_ref_text").isNull()) output.setTmbReferenceText("");
	else output.setTmbReferenceText(query.value("tmb_ref_text").toString());

	if(query.value("quality").isNull()) output.setQuality("");
	else output.setQuality(query.value("quality").toString());

	output.setFusionsDetected(query.value("fusions_detected").toBool());

	if(!query.value("cin_chr").isNull()) output.setCinChromosomes( query.value("cin_chr").toString().split(',') );

	if(!query.value("limitations").isNull()) output.setLimitations( query.value("limitations").toString() );

	if(!query.value("filter").isNull()) output.setFilter( query.value("filter").toString() );


	//Load SNVs
	//Resolve variants stored in NGSD and compare to those in VariantList snvs
	query.exec("SELECT * FROM somatic_report_configuration_variant WHERE somatic_report_configuration_id=" + QString::number(config_id));
	while(query.next())
	{
		SomaticReportVariantConfiguration var_conf;
		Variant var = variant(query.value("variant_id").toString()); //variant resolved by its ID
		for(int i=0; i<snvs.count(); ++i)
		{
			if(var==snvs[i])
			{
				var_conf.variant_index = i;
			}
		}
		if(var_conf.variant_index == -1)
		{
			messages << "Could not find somatic variant '" + var.toString() + "' in given variant list!";
		}

		var_conf.exclude_artefact = query.value("exclude_artefact").toBool();
		var_conf.exclude_low_tumor_content = query.value("exclude_low_tumor_content").toBool();
		var_conf.exclude_low_copy_number = query.value("exclude_low_copy_number").toBool();
		var_conf.exclude_high_baf_deviation = query.value("exclude_high_baf_deviation").toBool();
		var_conf.exclude_other_reason = query.value("exclude_other_reason").toBool();

		var_conf.include_variant_alteration = query.value("include_variant_alteration").toString();
		var_conf.include_variant_description = query.value("include_variant_description").toString();

		var_conf.comment = query.value("comment").toString();

		output.set(var_conf);
	}

	//Load Cnvs
	query.exec("SELECT * FROM somatic_report_configuration_cnv WHERE somatic_report_configuration_id=" + QString::number(config_id));
	while(query.next())
	{
		SomaticReportVariantConfiguration var_conf;
		var_conf.variant_type = VariantType::CNVS;

		CopyNumberVariant cnv = somaticCnv(query.value("somatic_cnv_id").toInt());
		for(int i=0; i< cnvs.count(); ++i)
		{
			if(cnvs[i].hasSamePosition(cnv))
			{
				var_conf.variant_index = i;
			}
		}
		if(var_conf.variant_index == -1)
		{
			messages << "Could not find somatic CNV '" + cnv.toString() + "' in given variant list!";
			continue;
		}

		var_conf.exclude_artefact = query.value("exclude_artefact").toBool();
		var_conf.exclude_low_tumor_content = query.value("exclude_low_tumor_content").toBool();
		var_conf.exclude_low_copy_number = query.value("exclude_low_copy_number").toBool();
		var_conf.exclude_high_baf_deviation = query.value("exclude_high_baf_deviation").toBool();
		var_conf.exclude_other_reason = query.value("exclude_other_reason").toBool();
		var_conf.comment = query.value("comment").toString();


		output.set(var_conf);
	}

	//Load germline SNVs related to tumor
	query.exec("SELECT * FROM somatic_report_configuration_germl_var WHERE somatic_report_configuration_id=" + QString::number(config_id));
	while(query.next())
	{
		SomaticReportGermlineVariantConfiguration var_conf;
		Variant var = variant(query.value("variant_id").toString()); //variant resolved by its id
		for(int i=0; i<germline_snvs.count(); ++i)
		{
			if(var == germline_snvs[i]) var_conf.variant_index = i;
		}
		if(var_conf.variant_index == -1)
		{
			messages << "Could not find germline variant '" + var.toString() + "' in given variant list!";
		}

		if(!query.value("tum_freq").isNull()) var_conf.tum_freq = query.value("tum_freq").toDouble();
		else var_conf.tum_freq = std::numeric_limits<double>::quiet_NaN();

		if(!query.value("tum_depth").isNull() ) var_conf.tum_depth = query.value("tum_depth").toDouble();
		else var_conf.tum_depth = std::numeric_limits<double>::quiet_NaN();

		output.setGermline(var_conf);
	}

	return output;
}

void NGSD::setSomaticMtbXmlUpload(int report_id)
{
	SqlQuery query = getQuery();
	query.prepare("UPDATE `somatic_report_configuration` SET `mtb_xml_upload_date`= CURRENT_TIMESTAMP WHERE id=:0");
	query.bindValue(0, report_id );
	query.exec();
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

	//get infos from 'gene' table
	QString gene_id;
	SqlQuery query = getQuery();
	query.exec("SELECT * FROM gene WHERE symbol='" + output.symbol + "'");
	if (query.size()==0)
	{
		output.name = "";
		output.hgnc_id = "";
		output.locus_group = "";
	}
	else
	{
		query.next();
		gene_id = query.value("id").toString();
		output.name = query.value("name").toString();
		output.hgnc_id = "HGNC:" + query.value("hgnc_id").toString();
		output.locus_group = query.value("type").toString();
	}

	//get infos from 'geneinfo_germline' table
	query.exec("SELECT inheritance, gnomad_oe_syn, gnomad_oe_mis, gnomad_oe_lof, comments FROM geneinfo_germline WHERE symbol='" + output.symbol + "'");
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

	//imprinting info
	const QMap<QByteArray, ImprintingInfo>& imprinting = NGSHelper::imprintingGenes();
	if (imprinting.contains(symbol))
	{
		output.imprinting_source_allele = imprinting[symbol].source_allele;
		output.imprinting_status = imprinting[symbol].status;
	}

	//pseudogene info
	if (!gene_id.isEmpty())
	{
		query.exec("SELECT g.symbol, gps.gene_name FROM gene_pseudogene_relation gps LEFT JOIN gene g ON gps.pseudogene_gene_id=g.id WHERE parent_gene_id="+gene_id);
		if (query.size()>0)
		{
			query.next();

			QString hgnc_symbol = query.value(0).toString().trimmed();
			output.pseudogenes << (hgnc_symbol.isEmpty() ? query.value(1).toString().split(';').at(1) : hgnc_symbol);
		}
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

QString SomaticReportConfigurationData::history() const
{
	QStringList output;
	output << "The report configuration was created by " + created_by + " on " + created_date + ".";
	if (last_edit_by!="") output << "The report configuration was last updated by " + last_edit_by + " on " + last_edit_date + ".";
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
			//check length
			if (value.length()<6)
			{
				errors << "Minimum length is 6";
			}
			if (value.length()>field_info.type_constraints.max_length)
			{
				errors << "Maximum length is " + QString::number(field_info.type_constraints.max_length);
			}

			//check composition
			{
				bool has_letter = false;
				bool has_number = false;
				foreach (const QChar& character, value)
				{
					if (character.isUpper() || character.isLower())
					{
						has_letter = true;
					}
					else if (character.isDigit())
					{
						has_number = true;
					}
				}
				if (!has_letter || !has_number)
				{
					errors << "Must contain at least one character of each class: letter, number";
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

QString NGSD::escapeText(QString text)
{
	QSqlField f;
	f.setType(QVariant::String);
	f.setValue(text);

	return db_->driver()->formatValue(f);
}


NGSD::Cache& NGSD::getCache()
{
	static Cache cache_instance;

	return cache_instance;
}

void NGSD::clearCache()
{
	Cache& cache_instance = getCache();

	cache_instance.table_infos.clear();
	cache_instance.same_samples.clear();
	cache_instance.approved_gene_names.clear();
	cache_instance.enum_values.clear();
	cache_instance.non_approved_to_approved_gene_names.clear();
	cache_instance.phenotypes_by_id.clear();
	cache_instance.phenotypes_accession_to_id.clear();

	cache_instance.gene_regions.clear();
	cache_instance.gene_regions_index.createIndex();

	cache_instance.gene_exons.clear();
	cache_instance.gene_exons_index.createIndex();
}


NGSD::Cache::Cache()
	: gene_regions()
	, gene_regions_index(gene_regions)
	, gene_exons()
	, gene_exons_index(gene_exons)
{
}
