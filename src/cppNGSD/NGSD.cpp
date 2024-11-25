#include "NGSD.h"
#include "Exceptions.h"
#include "Helper.h"
#include "Log.h"
#include "Settings.h"
#include "ChromosomalIndex.h"
#include "NGSHelper.h"
#include "FilterCascade.h"
#include "LoginManager.h"
#include "UserPermissionList.h"
#include "VariantImpact.h"
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
#include <QThread>
#include <ProxyDataService.h>
#include <QMap>
#include "cmath"
#include "QUuid"
#include "ClientHelper.h"
#include "PipelineSettings.h"


NGSD::NGSD(bool test_db, QString test_name_override)
	: test_db_(test_db)
{
	QString db_identifier = "NGSD_" + QUuid::createUuid().toString();
	db_.reset(new QSqlDatabase(QSqlDatabase::addDatabase("QMYSQL", db_identifier)));

	//connect to DB
	QString db_name;
	if (ClientHelper::isClientServerMode() && !ClientHelper::isRunningOnServer() && !test_db_) //get credentials from server in client-server mode
	{
		db_->setHostName(LoginManager::ngsdHostName());
		db_->setPort(LoginManager::ngsdPort());
		db_->setDatabaseName(LoginManager::ngsdName());
		db_->setUserName(LoginManager::ngsdUser());
		db_->setPassword(LoginManager::ngsdPassword());
		db_name = LoginManager::ngsdName();
	}
	else
	{
		QString prefix = "ngsd";
		if (test_db_) prefix += "_test";
		if (test_db_ && !test_name_override.isEmpty()) prefix = test_name_override;
		db_->setHostName(Settings::string(prefix + "_host"));
		db_->setPort(Settings::integer(prefix + "_port"));
		db_->setDatabaseName(Settings::string(prefix + "_name"));
		db_->setUserName(Settings::string(prefix + "_user"));
		db_->setPassword(Settings::string(prefix + "_pass"));
		db_name = prefix;
	}

	if (!db_->open())
	{
		THROW(DatabaseException, "Could not connect to NGSD database '" + db_name + "': " + db_->lastError().text());
	}
}

bool NGSD::isAvailable(bool test_db)
{
	if (!test_db && ClientHelper::isClientServerMode() && !ClientHelper::isRunningOnServer())
	{
		return true;
	}

	QString prefix = "ngsd";
	if (test_db) prefix += "_test";

	return Settings::contains(prefix+"_host") && Settings::contains(prefix+"_port") && Settings::contains(prefix+"_name") && Settings::contains(prefix+"_user") && Settings::contains(prefix+"_pass");
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

QString NGSD::getUserRole(int user_id)
{
	return getValue("SELECT user_role FROM user WHERE id='" + QString::number(user_id) + "'").toString().toLower();
}

bool NGSD::userRoleIn(QString user, QStringList roles)
{
	//check that role list contains only correct user role names
	QStringList valid_roles = getEnum("user", "user_role");
	foreach(const QString& role, roles)
	{
		if (!valid_roles.contains(role)) THROW (ProgrammingException, "Invalid role '" + role + "' given in NGSD::userRoleIn()!");
	}

	QString user_role = getValue("SELECT user_role FROM user WHERE user_id=:0", false, user).toString();
	return roles.contains(user_role);
}

bool NGSD::userCanAccess(int user_id, int ps_id)
{
	//access restricted only for user role 'user_restricted'
	if (getUserRole(user_id)!="user_restricted") return true;

	//get permission list
	QSet<int> ps_ids;
	SqlQuery query = getQuery();
	query.exec("SELECT * FROM user_permissions WHERE user_id=" + QString::number(user_id));
	while(query.next())
	{
		Permission permission = UserPermissionList::stringToType(query.value("permission").toString());
		QVariant data = query.value("data").toString();

		switch(permission)
		{
			case Permission::PROJECT:
				ps_ids += getValuesInt("SELECT id FROM processed_sample WHERE project_id=" + data.toString()).toSet();
				break;
			case Permission::PROJECT_TYPE:
				ps_ids += getValuesInt("SELECT ps.id FROM processed_sample ps, project p WHERE ps.project_id=p.id AND p.type='" + data.toString() + "'").toSet();
				break;
			case Permission::SAMPLE:
				ps_ids += getValuesInt("SELECT id FROM processed_sample WHERE sample_id=" + data.toString()).toSet();
				break;
			case Permission::STUDY:
				ps_ids += getValuesInt("SELECT processed_sample_id FROM study_sample WHERE study_id=" + data.toString()).toSet();
				break;
		}
	}

	return ps_ids.contains(ps_id);
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
			<< "s.disease_status as disease_status"
			<< "s.tissue as tissue";

	QStringList tables;
	tables	<< "sample s"
			<< "processing_system sys"
			<< "project p"
			<< "processed_sample ps LEFT JOIN sequencing_run r ON r.id=ps.sequencing_run_id LEFT JOIN diag_status ds ON ds.processed_sample_id=ps.id LEFT JOIN processed_sample_ancestry psa ON psa.processed_sample_id=ps.id LEFT JOIN user u ON ps.operator_id=u.id"; //sequencing_run, operator and diag_status are optional

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
	if (p.s_patient_identifier.trimmed()!="")
	{
		conditions << "s.patient_identifier='" + escapeForSql(p.s_patient_identifier) + "'";
	}
	if (p.s_species.trimmed()!="")
	{
		tables	<< "species sp";
		conditions	<< "sp.id=s.species_id"
					<< "sp.name='" + escapeForSql(p.s_species) + "'";
	}
	if (p.s_type.trimmed()!="")
	{
		conditions << "s.sample_type='" + escapeForSql(p.s_type) + "'";
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
	if (p.s_tissue.trimmed() != "")
	{
		conditions << "s.tissue='" + escapeForSql(p.s_tissue) + "'";
	}
	if (p.s_ancestry.trimmed() != "")
	{
		conditions << "psa.population='" + escapeForSql(p.s_ancestry) + "'";
	}
	if (!p.include_bad_quality_samples)
	{
		conditions << "ps.quality!='bad'";
	}
	if (!p.include_scheduled_for_resequencing_samples)
	{
		conditions << "ps.scheduled_for_resequencing='0'";
	}
	if (!p.include_tumor_samples)
	{
		conditions << "s.tumor='0'";
	}
	if (!p.include_germline_samples)
	{
		conditions << "s.tumor='1'";
	}
	if (!p.include_ffpe_samples)
	{
		conditions << "s.ffpe='0'";
	}
	if (!p.include_merged_samples)
	{
		conditions << "ps.id NOT IN (SELECT processed_sample_id FROM merged_processed_samples)";
	}
	if (p.only_with_small_variants)
	{
		conditions << "ps.id IN (SELECT DISTINCT processed_sample_id FROM small_variants_callset)";
	}
	if (!p.s_phenotypes.isEmpty())
	{
		tables	<< "sample_disease_info sdi";
		conditions	<< "s.id=sdi.sample_id";
		conditions	<< "sdi.type='HPO term id'";

		//create complete phenotype list with children
		QStringList accessions;
		foreach(const Phenotype& phenotype, p.s_phenotypes)
		{
			accessions << phenotype.accession();
			int phenotype_id = phenotypeIdByAccession(phenotype.accession());
			foreach(const Phenotype& child, phenotypeChildTerms(phenotype_id, true))
			{
				accessions << child.accession();
			}
		}
		accessions.removeDuplicates();
		conditions	<< "sdi.disease_info IN ('" + accessions.join("', '") + "')";
	}

	//add filters (project)
	if (p.p_name.trimmed()!="")
	{
		conditions << "p.name LIKE '%" + escapeForSql(p.p_name) + "%'";
	}
	if (p.p_type.trimmed()!="")
	{
		conditions << "p.type='" + escapeForSql(p.p_type) + "'";
	}
	if (!p.include_archived_projects)
	{
		conditions << "p.archived='0'";
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
	if (p.r_after.isValid())
	{
		conditions << "r.start_date>='" + p.r_after.toString(Qt::ISODate)+"'";
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

	//add dates
	if (p.add_dates)
	{
		fields << "s.year_of_birth as year_of_birth";
		fields << "s.received as received_date";
		fields << "s.sampling_date as sampling_date";
		fields << "s.order_date as order_date";
	}

	//add processed sample and sample quality
	if (p.add_qc)
	{
		fields << "s.quality as sample_quality"
			   << "ps.quality as processed_sample_quality";
	}
	DBTable output = createTable("processed_sample", "SELECT " + fields.join(", ") + " FROM " + tables.join(", ") +" WHERE " + conditions.join(" AND ") + " ORDER BY r.name ASC, s.name ASC, ps.process_id ASC");

	//remove duplicates
	QSet<int> done;
	for(int r=output.rowCount()-1; r>=0; --r) //reverse, so that all indices are valid
	{
		int ps_id = output.row(r).id().toInt();
		if (done.contains(ps_id))
		{
			output.removeRow(r);
		}
		else
		{
			done << ps_id;
		}
	}

	//filter by user access rights (for restricted users only)
	if (p.restricted_user!="")
	{
		int user_id = userId(p.restricted_user);

		for(int r=output.rowCount()-1; r>=0; --r) //reverse, so that all indices are valid
		{
			int ps_id = output.row(r).id().toInt();
			if (!userCanAccess(user_id, ps_id))
			{
				output.removeRow(r);
			}
		}
	}

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
		//reorder columns to put sample and processed sample quality at the start of the qc block:
		QStringList sample_quality = output.takeColumn(output.columnIndex("sample_quality"));
		output.addColumn(sample_quality, "sample_quality");
		QStringList ps_quality = output.takeColumn(output.columnIndex("processed_sample_quality"));
		output.addColumn(ps_quality, "processed_sample_quality");

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

	if (p.add_normal_sample)
	{
		QStringList new_col;
		for (int r=0; r<output.rowCount(); ++r)
		{
			const QString& ps_id = output.row(r).id();
			new_col << normalSample(ps_id);
		}
		output.addColumn(new_col, "normal_sample");
	}

	if (p.add_call_details)
	{
		QStringList small_caller;
		QStringList small_date;
		QStringList cnv_caller;
		QStringList cnv_date;
		QStringList sv_caller;
		QStringList sv_date;
		QStringList re_caller;
		QStringList re_date;

		for (int r=0; r<output.rowCount(); ++r)
		{
			VariantCallingInfo call_info = variantCallingInfo(output.row(r).id());
			small_caller << (call_info.small_caller+" "+call_info.small_caller_version).trimmed();
			small_date << call_info.small_call_date.trimmed();

			cnv_caller << (call_info.cnv_caller+" "+call_info.cnv_caller_version).trimmed();
			cnv_date << call_info.cnv_call_date.trimmed();

			sv_caller << (call_info.sv_caller+" "+call_info.sv_caller_version).trimmed();
			sv_date << call_info.sv_call_date.trimmed();

			re_caller << (call_info.re_caller+" "+call_info.re_caller_version).trimmed();
			re_date << call_info.re_call_date.trimmed();
		}

		output.addColumn(small_caller, "small_variants_caller");
		output.addColumn(small_date, "small_variants_call_date");
		output.addColumn(cnv_caller, "cvn_caller");
		output.addColumn(cnv_date, "cnv_call_date");
		output.addColumn(sv_caller, "sv_caller");
		output.addColumn(sv_date, "sv_call_date");
		output.addColumn(re_caller, "re_caller");
		output.addColumn(re_date, "re_call_date");
	}

	if (p.add_study_column)
	{
		QStringList new_col;
		for (int r=0; r<output.rowCount(); ++r)
		{
			const QString& ps_id = output.row(r).id();
			new_col << studies(ps_id).join(", ");
		}
		output.addColumn(new_col, "studies");
	}

	return output;
}

SampleData NGSD::getSampleData(const QString& sample_id)
{
	//execute query
	SqlQuery query = getQuery();
	query.exec("SELECT s.name, s.name_external, s.gender, s.quality, s.comment, s.disease_group, s.disease_status, s.tumor, s.ffpe, s.sample_type, s.sender_id, s.species_id, s.received, s.receiver_id, s.tissue, s.patient_identifier, s.year_of_birth, s.order_date, s.sampling_date FROM sample s WHERE id=" + sample_id);
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

	output.tissue = query.value(14).toString();
	output.patient_identifier = query.value(15).toString();
	QVariant year_of_birth = query.value(16);
	if (!year_of_birth.isNull())
	{
		output.year_of_birth = year_of_birth.toString();
	}

	QVariant order_date = query.value(17);
	if (!order_date.isNull())
	{
		output.order_date = order_date.toDate().toString("dd.MM.yyyy");
	}

	QVariant sampling_date = query.value(18);
	if (!sampling_date.isNull())
	{
		output.sampling_date = sampling_date.toDate().toString("dd.MM.yyyy");
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
	query.exec("SELECT CONCAT(s.name,'_',LPAD(ps.process_id,2,'0')) as ps_name, sys.name_manufacturer as sys_name, sys.type as sys_type, ps.quality, ps.comment, p.name as p_name, p.type as p_type, r.name as r_name, ps.normal_id, s.gender, ps.operator_id, ps.processing_input, ps.molarity, ps.processing_modus, ps.batch_number, ps.scheduled_for_resequencing FROM sample s, project p, processing_system sys, processed_sample ps LEFT JOIN sequencing_run r ON ps.sequencing_run_id=r.id WHERE ps.sample_id=s.id AND ps.project_id=p.id AND ps.processing_system_id=sys.id AND ps.id=" + processed_sample_id);
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
	output.project_type = query.value("p_type").toString().trimmed();
	output.run_name = query.value("r_name").toString().trimmed();
	output.sequencer_type = getValue("SELECT d.type FROM device d, sequencing_run r WHERE r.device_id=d.id AND r.name=:0", true, output.run_name).toString();
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
	output.processing_modus = query.value("processing_modus").toString().trimmed();
	output.batch_number = query.value("batch_number").toString().trimmed();
	output.processing_input = query.value("processing_input").toString().trimmed();
	output.molarity = query.value("molarity").toString().trimmed();
	output.ancestry = getValue("SELECT `population` FROM `processed_sample_ancestry` WHERE `processed_sample_id`=:0", true, processed_sample_id).toString();
	output.scheduled_for_resequencing = query.value("scheduled_for_resequencing").toBool();

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
	foreach(const SampleDiseaseInfo& entry, disease_info)
	{
		addSampleDiseaseInfo(sample_id, entry);
	}
}

void NGSD::addSampleDiseaseInfo(const QString& sample_id, const SampleDiseaseInfo& entry)
{
	SqlQuery query_insert = getQuery();
	query_insert.prepare("INSERT INTO sample_disease_info (`sample_id`, `disease_info`, `type`, `user_id`, `date`) VALUES (" + sample_id + ", :0, :1, :2, :3)");
	query_insert.bindValue(0, entry.disease_info);
	query_insert.bindValue(1, entry.type);
	query_insert.bindValue(2, userId(entry.user));
	query_insert.bindValue(3, entry.date.toString(Qt::ISODate));
	query_insert.exec();
}

QString NGSD::normalSample(const QString& processed_sample_id)
{
	QVariant value = getValue("SELECT normal_id FROM processed_sample WHERE id=" + processed_sample_id, true);
	if (value.isNull()) return "";

	return processedSampleName(value.toString());
}

const QSet<int>& NGSD::sameSamples(int sample_id, SameSampleMode mode)
{
	static QSet<int> empty_entry;
	QHash<int, QSet<int>>& same_samples = (mode == SameSampleMode::SAME_PATIENT)? getCache().same_patients : getCache().same_samples;

	//prepare iterative query
	SqlQuery query_iterative = getQuery();
	query_iterative.prepare(QString("SELECT sample1_id, sample2_id FROM sample_relations WHERE (relation='same sample'") + ((mode == SameSampleMode::SAME_PATIENT)? " OR relation='same patient')": ")") + " AND (sample1_id=:0 OR sample2_id=:0)");

	//init if empty
	if (same_samples.isEmpty())
	{
		//sample relation
		SqlQuery query = getQuery();
		query.exec(QString("SELECT sample1_id FROM sample_relations WHERE relation='same sample'") + ((mode == SameSampleMode::SAME_PATIENT)? " OR relation='same patient'": ""));
		while (query.next())
		{
			int sample1_id = query.value(0).toInt();

			//skip already checked samples
			if (same_samples.contains(sample1_id)) continue;

			//look-up iteratively and get the same-sample cluster
			QSet<int> cluster;
			cluster << sample1_id;
			int n_ids = 0;
			while(n_ids != cluster.size())
			{
				//store current set size
				n_ids = cluster.size();
				foreach (int id, cluster)
				{
					query_iterative.bindValue(0, id);
					query_iterative.exec();
					while (query_iterative.next())
					{
						cluster << query_iterative.value(0).toInt() << query_iterative.value(1).toInt();
					}
				}
			}
			//set same samples for all samples of cluster (exclude key itself)
			foreach (int id, cluster)
			{
				QSet<int> current_cluster = cluster;
				current_cluster.remove(id);
				same_samples[id] = current_cluster;
			}
		}

		if (mode == SameSampleMode::SAME_PATIENT)
		{
			//same patient identifier
			query.exec("SELECT id, patient_identifier FROM sample WHERE patient_identifier IS NOT NULL AND patient_identifier!=''");
			QHash<QString, QSet<int>> sample_ids_by_patient_id;
			while (query.next())
			{
				int sample_id = query.value(0).toInt();
				QString patient_identifier = query.value(1).toString().trimmed();
				if (patient_identifier.isEmpty()) continue;

				sample_ids_by_patient_id[patient_identifier] << sample_id;
			}

			foreach(QString patient_id, sample_ids_by_patient_id.keys())
			{
				QSet<int>& sample_ids = sample_ids_by_patient_id[patient_id];

				//skip all patient ids with only 1 linked sample id
				if (sample_ids.size() < 2) continue;

				//else: merge cluster
				QSet<int> combined_sample_ids;
				foreach (int s_id, sample_ids)
				{
					combined_sample_ids << s_id;
					combined_sample_ids += same_samples[s_id];
				}

				//update each sample in the cluster
				foreach (int id, combined_sample_ids)
				{
					QSet<int> current_cluster = combined_sample_ids;
					current_cluster.remove(id);
					same_samples[id] = current_cluster;
				}
			}
		}

	}

	if (same_samples.contains(sample_id))
	{
		return same_samples[sample_id];
	}
	else
	{
		return empty_entry;
	}
}

const QSet<int>& NGSD::relatedSamples(int sample_id)
{
	static QSet<int> empty_entry;
	QHash<int, QSet<int>>& related_samples = getCache().related_samples;

	//init if empty
	if (related_samples.isEmpty())
	{
		SqlQuery query = getQuery();
		query.exec("SELECT sample1_id, sample2_id FROM sample_relations");
		while (query.next())
		{
			int sample1_id = query.value(0).toInt();
			int sample2_id = query.value(1).toInt();
			related_samples[sample1_id] << sample2_id;
			related_samples[sample2_id] << sample1_id;
		}
	}

	if (related_samples.contains(sample_id))
	{
		return related_samples[sample_id];
	}
	else
	{
		return empty_entry;
	}
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
		int id = phenotypeIdByAccession(hpo_id.toUtf8(), throw_on_error);
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

QString NGSD::processingSystemRegionsFilePath(int sys_id)
{
	QString rel_path = getValue("SELECT target_file FROM processing_system WHERE id=" + QString::number(sys_id)).toString().trimmed();
	if (!rel_path.isEmpty())
	{
		return getTargetFilePath() + rel_path;
	}
	return "";
}

BedFile NGSD::processingSystemRegions(int sys_id, bool ignore_if_missing)
{
	BedFile output;

	QString regions_file = processingSystemRegionsFilePath(sys_id);
	if (regions_file.isEmpty())
	{
		if (!ignore_if_missing) THROW(FileAccessException, "Target region BED file of processing system '" + getProcessingSystemData(sys_id).name + "' requested but not set in NGSD!");
	}
	else
	{
		output.load(regions_file);
	}

	return output;
}

QString NGSD::processingSystemGenesFilePath(int sys_id)
{
	QString rel_path = getValue("SELECT target_file FROM processing_system WHERE id=" + QString::number(sys_id)).toString().trimmed();
	if (!rel_path.isEmpty())
	{
		return getTargetFilePath() + rel_path.mid(0, rel_path.length() -4) + "_genes.txt";
	}
	return "";
}

GeneSet NGSD::processingSystemGenes(int sys_id, bool ignore_if_missing)
{
	GeneSet output;

	QString gene_file = processingSystemGenesFilePath(sys_id);
	if (gene_file.isEmpty())
	{
		if (!ignore_if_missing) THROW(FileAccessException, "Gene file of processing system '" + getProcessingSystemData(sys_id).name + "' requested but not set in NGSD!");
	}
	else
	{
		output = GeneSet::createFromFile(gene_file);
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

void NGSD::removeInitData()
{
	getQuery().exec("DELETE FROM user WHERE user_id='admin'");
	getQuery().exec("DELETE FROM user WHERE user_id='genlab_import'");
	getQuery().exec("DELETE FROM user WHERE user_id='unknown'");
	getQuery().exec("DELETE FROM user WHERE user_id='init_date'"); //needing for test database

	getQuery().exec("DELETE FROM species WHERE name='human'");

	getQuery().exec("DELETE FROM genome WHERE build='GRCh37'");
	getQuery().exec("DELETE FROM genome WHERE build='GRCh38'");

	getQuery().exec("DELETE FROM repeat_expansion WHERE 1");
}

QString NGSD::projectFolder(QString type)
{
    //GSvar server: use megSAP settings
    if (ClientHelper::isRunningOnServer())
    {
        return PipelineSettings::projectFolder(type);
    }

	//current type-specific project folder settings
	if (Settings::contains("projects_folder_"+type))
	{
		   return Settings::path("projects_folder_"+type, true).trimmed() + QDir::separator();
	}

	//fallback to legacy project folder settings
	if (Settings::contains("projects_folder"))
	{
		return Settings::path("projects_folder", true).trimmed() + QDir::separator() + type + QDir::separator();
	}

	THROW(ProgrammingException, "Found no project folder entry in settings.ini file for project type '" + type + "'!");
}

QString NGSD::processedSamplePath(const QString& processed_sample_id, PathType type)
{
	//create query
	QString query_str ="SELECT CONCAT(s.name,'_',LPAD(ps.process_id,2,'0')), p.type, p.name, sys.name_short, ";
	bool is_client = ClientHelper::isClientServerMode() && !ClientHelper::isRunningOnServer();
//	is_client = (ClientHelper::isClientServerMode() && !ClientHelper::isRunningOnServer()) || !ClientHelper::isClientServerMode(); //activate for debug
	query_str += is_client ? "ps.folder_override_client, p.folder_override_client" : "ps.folder_override, p.folder_override";
	query_str += " FROM processed_sample ps, sample s, project p, processing_system sys WHERE ps.processing_system_id=sys.id AND ps.sample_id=s.id AND ps.project_id=p.id AND ps.id=:0";

	//execute query
	SqlQuery query = getQuery();
	query.prepare(query_str);
	query.bindValue(0, processed_sample_id);
	query.exec();
	if (query.size()==0) THROW(DatabaseException, "Processed sample with id '" + processed_sample_id + "' not found in NGSD!");
	query.next();

	//create sample folder
	QString ps_name = query.value(0).toString();
	QString ps_folder_override = query.value(4).toString();
	QString p_folder_override = query.value(5).toString();
	QString output;
	if (!ps_folder_override.isEmpty())
	{
		output = ps_folder_override;
		if (!output.endsWith(QDir::separator())) output += QDir::separator();
	}
	else if (!p_folder_override.isEmpty())
	{
		output = p_folder_override;
		if (!output.endsWith(QDir::separator())) output += QDir::separator();
		output += "Sample_" + ps_name + QDir::separator();
	}
	else
	{
		QString p_type = query.value(1).toString();
		output = projectFolder(p_type);
		QString p_name = query.value(2).toString();
		output += p_name + QDir::separator() + "Sample_" + ps_name + QDir::separator();
	}
	QString sys_name_short = query.value(3).toString();

	//append file name if requested
	if (type==PathType::BAM)
	{
		if (QFile::exists(output + ps_name + ".cram"))
		{
			output += ps_name + ".cram";
		}
		else
		{
			output += ps_name + ".bam";
		}
	}
	else if (type==PathType::GSVAR) output += ps_name + ".GSvar";
	else if (type==PathType::VCF) output += ps_name + "_var_annotated.vcf.gz";
	else if (type==PathType::VCF_CF_DNA) output += ps_name + "_var.vcf";
	else if (type==PathType::LOWCOV_BED) output += ps_name + "_" + sys_name_short + "_lowcov.bed";
	else if (type==PathType::MANTA_EVIDENCE) output += "manta_evid/" + ps_name + "_manta_evidence.bam";
	else if (type==PathType::BAF) output += ps_name + "_bafs.igv";
	else if (type==PathType::STRUCTURAL_VARIANTS)
	{
		if (QFile::exists(output + ps_name + "_var_structural_variants.bedpe"))
		{
			output += ps_name + "_var_structural_variants.bedpe";
		}
		else
		{
			//Fallback for old manta file name
			output += ps_name + "_manta_var_structural.bedpe";
		}
	}
	else if (type==PathType::COPY_NUMBER_RAW_DATA) output += ps_name + "_cnvs_clincnv.seg";
	else if (type==PathType::COPY_NUMBER_CALLS) output += ps_name + "_cnvs_clincnv.tsv";
	else if (type==PathType::FUSIONS) output += ps_name + "_fusions_arriba.tsv";
	else if (type==PathType::FUSIONS_PIC_DIR) output += ps_name + "_fusions_arriba_pics";
	else if (type==PathType::FUSIONS_BAM) output += ps_name + "_fusions_arriba.bam";
	else if (type==PathType::SPLICING_BED) output += ps_name + "_splicing.bed";
	else if (type==PathType::SPLICING_ANN) output += ps_name + "_splicing_annot.tsv";
	else if (type==PathType::MANTA_FUSIONS) output +=  ps_name + "_var_fusions_manta.bedpe";
	else if (type==PathType::VIRAL) output += ps_name + "_viral.tsv";
	else if (type==PathType::COUNTS) output += ps_name + "_counts.tsv";
	else if (type==PathType::EXPRESSION) output += ps_name + "_expr.tsv";
	else if (type==PathType::EXPRESSION_COHORT) output += ps_name + "_expr.cohort.tsv";
	else if (type==PathType::EXPRESSION_STATS) output += ps_name + "_expr.stats.tsv";
	else if (type==PathType::EXPRESSION_CORR) output += ps_name + "_expr.corr.txt";
	else if (type==PathType::EXPRESSION_EXON) output += ps_name + "_expr_exon.tsv";
	else if (type==PathType::MRD_CF_DNA) output += QString("umiVar") + QDir::separator() + ps_name + ".mrd";
	else if (type==PathType::HLA_GENOTYPER) output += ps_name + "_hla_genotyper.tsv";
	else if (type==PathType::REPEAT_EXPANSIONS)
	{
		if (QFile::exists(output + ps_name + "_repeats_expansionhunter.vcf"))
		{
			output += ps_name + "_repeats_expansionhunter.vcf";
		}
		else
		{
			//Fallback for general name used e.g. for longreads
			output += ps_name + "_repeats.vcf";
		}
	}
	else if (type==PathType::METHYLATION) output += ps_name + "_var_methylation.tsv";
	else if (type!=PathType::SAMPLE_FOLDER) THROW(ProgrammingException, "Unhandled PathType '" + FileLocation::typeToString(type) + "' in processedSamplePath!");

	return QFileInfo(output).absoluteFilePath();
}

QStringList NGSD::secondaryAnalyses(QString processed_sample_name, QString analysis_type)
{
	//init
	QStringList project_types = getEnum("project", "type");

	//convert to platform-specific canonical path
	QStringList output = getValues("SELECT gsvar_file FROM secondary_analysis WHERE type='" + analysis_type + "' AND gsvar_file LIKE '%" + processed_sample_name + "%'");
	for (int i=0; i<output.count(); ++i)
	{
		QString file = output[i];

		//get folder of secondary analysis
		QString gsvar_file = QFileInfo(file).fileName();
		QString secondary_folder_name = QFileInfo(file).dir().dirName();

		//get first sample (determines path)
		QStringList parts = secondary_folder_name.split("_");
		QString first_sample = parts[1] + "_" + parts[2];
		QDir sample_folder = QFileInfo(processedSamplePath(processedSampleId(first_sample), PathType::SAMPLE_FOLDER)).dir();
		QString project_folder = QFileInfo(sample_folder.absolutePath()).absolutePath();

		//concat path
		file = project_folder + QDir::separator() + secondary_folder_name + QDir::separator() + gsvar_file;

		//convert to canonical path
		file = QFileInfo(file).absoluteFilePath();

		qDebug() << "final path: " << file;

		output[i] = file;
	}

	return output;
}

QString NGSD::addVariant(const Variant& variant)
{
	SqlQuery query = getQuery(); //use binding (user input)
	query.prepare("INSERT INTO variant (chr, start, end, ref, obs, comment) VALUES (:0,:1,:2,:3,:4,:5)");
	query.bindValue(0, variant.chr().strNormalized(true));
	query.bindValue(1, variant.start());
	query.bindValue(2, variant.end());
	query.bindValue(3, variant.ref());
	query.bindValue(4, variant.obs());
	query.bindValue(5, "Added manually by "+Helper::userName()+" at "+ QDateTime::currentDateTime().toString("dd.MM.yyyy hh:mm:ss")+".");
	query.exec();

	return query.lastInsertId().toString();
}

QString NGSD::addVariant(const Variant& variant, const VariantList& variant_list)
{
	SqlQuery query = getQuery(); //use binding (user input)
	query.prepare("INSERT INTO variant (chr, start, end, ref, obs, gnomad, coding) VALUES (:0,:1,:2,:3,:4,:5,:6)");
	query.bindValue(0, variant.chr().strNormalized(true));
	query.bindValue(1, variant.start());
	query.bindValue(2, variant.end());
	query.bindValue(3, variant.ref());
	query.bindValue(4, variant.obs());
	int idx = variant_list.annotationIndexByName("gnomAD");
	QByteArray gnomad = variant.annotations()[idx].trimmed();
	if (gnomad.isEmpty() || gnomad=="n/a")
	{
		query.bindValue(5, QVariant());
	}
	else
	{
		query.bindValue(5, gnomad);
	}
	idx = variant_list.annotationIndexByName("coding_and_splicing");
	query.bindValue(6, variant.annotations()[idx]);
	query.exec();

	return query.lastInsertId().toString();
}

QList<int> NGSD::addVariants(const VariantList& variant_list, double max_af, int& c_add, int& c_update)
{
	QList<int> output;

	//prepare queried
	SqlQuery q_id = getQuery();
	q_id.prepare("SELECT id, gnomad, coding, cadd, spliceai FROM variant WHERE chr=:0 AND start=:1 AND end=:2 AND ref=:3 AND obs=:4");

	SqlQuery q_update = getQuery(); //use binding (user input)
	q_update.prepare("UPDATE variant SET gnomad=:0, coding=:1, cadd=:2, spliceai=:3 WHERE id=:4");

	SqlQuery q_insert = getQuery(); //use binding (user input)
	q_insert.prepare("INSERT INTO variant (chr, start, end, ref, obs, gnomad, coding, cadd, spliceai) VALUES (:0,:1,:2,:3,:4,:5,:6,:7,:8) ON DUPLICATE KEY UPDATE id=id");

	//get annotated column indices
	int i_gnomad = variant_list.annotationIndexByName("gnomAD");
	int i_co_sp = variant_list.annotationIndexByName("coding_and_splicing");
	int i_cadd = variant_list.annotationIndexByName("CADD");
	int i_spliceai = variant_list.annotationIndexByName("SpliceAI");
	int i_pubmed = variant_list.annotationIndexByName("PubMed", true, false);

	c_add = 0;
	c_update = 0;
	for (int i=0; i<variant_list.count(); ++i)
	{
		const Variant& variant = variant_list[i];

		//skip variants over 500 bases length - the unique index of the variant table does not work for those
		if (variant.ref().count()>MAX_VARIANT_SIZE || variant.obs().count()>MAX_VARIANT_SIZE)
		{
			output << -1;
			continue;
		}

		//skip variants with too high AF
		QByteArray gnomad = variant.annotations()[i_gnomad].trimmed();
		if (gnomad=="n/a") gnomad.clear();
		if (!gnomad.isEmpty() && gnomad.toDouble()>max_af)
		{
			output << -1;
			continue;
		}


		QByteArray cadd = variant.annotations()[i_cadd].trimmed();
		double spliceai = NGSHelper::maxSpliceAiScore(variant.annotations()[i_spliceai]);
		QByteArrayList pubmed_ids;
		if (i_pubmed > 0) pubmed_ids = variant.annotations()[i_pubmed].split(',');

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
			if (q_id.value(1).toByteArray().toDouble()!=gnomad.toDouble() //numeric comparison (NULL > "" > 0.0)
				|| q_id.value(2).toByteArray()!=variant.annotations()[i_co_sp]
				|| q_id.value(3).toByteArray().toDouble()!=cadd.toDouble() //numeric comparison (NULL > "" > 0.0)
				|| q_id.value(4).toByteArray().toDouble()!=std::max(0.0, spliceai) //numeric comparison (NULL > "" > 0.0); no SpliceAI leads to a score of -1, so we use max to set it to 0.
				)
			{
				q_update.bindValue(0, gnomad.isEmpty() ? QVariant() : gnomad);
				q_update.bindValue(1, variant.annotations()[i_co_sp]);
				q_update.bindValue(2, cadd.isEmpty() ? QVariant() : cadd);
				q_update.bindValue(3, spliceai<0 ? QVariant() : spliceai);
				q_update.bindValue(4, id);
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
			q_insert.bindValue(5, gnomad.isEmpty() ? QVariant() : gnomad);
			q_insert.bindValue(6, variant.annotations()[i_co_sp]);
			q_insert.bindValue(7, cadd.isEmpty() ? QVariant() : cadd);
			q_insert.bindValue(8, spliceai<0 ? QVariant() : spliceai);
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
		foreach (const QByteArray pubmed_id, pubmed_ids)
		{
			if (pubmed_id.isEmpty()) continue;
			addPubmedId(output.last(), pubmed_id);
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

GenotypeCounts NGSD::genotypeCountsCached(const QString &variant_id)
{
	SqlQuery query = getQuery();
	query.exec("SELECT germline_hom, germline_het, germline_mosaic FROM variant WHERE id=" + variant_id);
	query.next();

	return GenotypeCounts{query.value(0).toInt(), query.value(1).toInt(), query.value(2).toInt()};
}

GenotypeCounts NGSD::genotypeCounts(const QString& variant_id)
{
	int c_het = 0;
	int c_hom = 0;
	int c_mosaic = 0;

	QSet<int> samples_done_het;
	QSet<int> samples_done_hom;
	QSet<int> samples_done_mosaic;
	SqlQuery query = getQuery();
	query.exec("SELECT ps.sample_id, dv.genotype, dv.mosaic FROM detected_variant dv, processed_sample ps WHERE dv.variant_id='" + variant_id + "' AND dv.processed_sample_id=ps.id");
	while(query.next())
	{
		//use sample ID to prevent counting variants several times if a sample was sequenced more than once.
		int sample_id = query.value(0).toInt();
		QByteArray genotype = query.value(1).toByteArray();
		bool mosaic = query.value(2).toBool();

		if (genotype=="het")
		{
			if (!mosaic && !samples_done_het.contains(sample_id))
			{
				++c_het;

				samples_done_het << sample_id;
				samples_done_het.unite(sameSamples(sample_id, SameSampleMode::SAME_PATIENT));
			}
			if (mosaic && !samples_done_mosaic.contains(sample_id))
			{
				++c_mosaic;

				samples_done_mosaic << sample_id;
				samples_done_mosaic.unite(sameSamples(sample_id, SameSampleMode::SAME_PATIENT));
			}

		}
		if (genotype=="hom" && !samples_done_hom.contains(sample_id))
		{
			++c_hom;

			samples_done_hom << sample_id;
			samples_done_hom.unite(sameSamples(sample_id, SameSampleMode::SAME_PATIENT));
		}
	}

	return GenotypeCounts{c_hom, c_het, c_mosaic};
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
	deleteVariants(ps_id, VariantType::RES);
}

void NGSD::deleteVariants(const QString& ps_id, VariantType type)
{
	if (type==VariantType::SNVS_INDELS)
	{
		getQuery().exec("DELETE FROM small_variants_callset WHERE processed_sample_id='" + ps_id + "'");
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
	else if (type==VariantType::RES)
	{
		QString callset_id = getValue("SELECT id FROM re_callset WHERE processed_sample_id=" + ps_id).toString();
		getQuery().exec("DELETE FROM re_callset WHERE id='" + callset_id + "'");
		getQuery().exec("DELETE FROM repeat_expansion_genotype WHERE processed_sample_id=" + ps_id);
	}
	else
	{
		THROW(NotImplementedException, "Deleting variants of type '" + QString::number((int)type) + "' not implemented!");
	}
}

int NGSD::repeatExpansionId(const BedLine& region, const QString& repeat_unit, bool throw_if_fails)
{
	SqlQuery query = getQuery(); //use binding user input (safety)
	query.prepare("SELECT id FROM repeat_expansion WHERE region=:0 and repeat_unit=:1");
	query.bindValue(0, region.toString(true));
	query.bindValue(1, repeat_unit);
	query.exec();
	if (!query.next())
	{
		if (throw_if_fails)
		{
			THROW(DatabaseException, "Repeat expansion " + region.toString(true) + "/" + repeat_unit + " not found in NGSD!");
		}
		else
		{
			return -1;
		}
	}

	return query.value(0).toInt();
}

QString NGSD::repeatExpansionName(int id, bool throw_on_error)
{
	return getValue("SELECT name FROM repeat_expansion WHERE id="+QString::number(id), !throw_on_error).toString();
}

QString NGSD::repeatExpansionComments(int id)
{
	QStringList output = getValue("SELECT comments FROM repeat_expansion WHERE id="+QString::number(id)).toString().replace("<br>", "\n").trimmed().split("\n");
	for (int i=0; i<output.count(); ++i)
	{
		QString line = output[i].trimmed();
		if (line.startsWith('#') && line.endsWith('#'))
		{
			output[i] = "<b>" + line.mid(1, line.length()-2) + "</b>";
		}
	}

	return output.join("<br>");
}

int NGSD::repeatExpansionGenotypeId(int repeat_expansion_id, int processed_sample_id, bool throw_if_fails)
{
	QVariant id = getValue("SELECT id FROM repeat_expansion_genotype WHERE repeat_expansion_id='" + QString::number(repeat_expansion_id) + "' AND processed_sample_id='" + QString::number(processed_sample_id) + "'", true);

	if (!id.isValid())
	{
		if (throw_if_fails)
		{
			QString re = repeatExpansionName(repeat_expansion_id);
			QString ps = processedSampleName(QString::number(processed_sample_id));
			THROW(DatabaseException, "No repeat expansion genotype data found for repeat expansion '" + re + "' and processed sample '" + ps + "'");
		}
		else return -1;
	}

	return id.toInt();
}

RepeatLocus NGSD::repeatExpansionGenotype(int id)
{
	SqlQuery query = getQuery();
	query.prepare("SELECT re.region, re.repeat_unit, reg.allele1, reg.allele1 FROM repeat_expansion_genotype reg, repeat_expansion re WHERE re.id=reg.repeat_expansion_id AND reg.id=:0");
	query.bindValue(0, id);
	query.exec();

	if (!query.next()) THROW(DatabaseException, "Repeat expansion with identifier '" + QString::number(id) + "' does not exist!");

	RepeatLocus re;
	re.setRegion(BedLine::fromString(query.value(0).toString()));
	re.setUnit(query.value(1).toByteArray());
	re.setAllele1(query.value(2).toByteArray());
	re.setAllele2(query.value(3).toByteArray());

	return re;
}

void NGSD::addPubmedId(int variant_id, const QString& pubmed_id)
{
	SqlQuery query = getQuery();
	query.prepare("INSERT INTO `variant_literature` (`variant_id`, `pubmed`) VALUES (:0, :1) ON DUPLICATE KEY UPDATE id=id");
	query.bindValue(0, variant_id);
	query.bindValue(1, pubmed_id);
	query.exec();
}

QStringList NGSD::pubmedIds(const QString& variant_id)
{
	return getValues("SELECT `pubmed` FROM `variant_literature` WHERE `variant_id`=:0", variant_id);
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

ImportStatusGermline NGSD::importStatus(const QString& ps_id)
{
	ImportStatusGermline output;

	//small variants
	output.small_variants = getValue("SELECT COUNT(*) FROM detected_variant WHERE processed_sample_id='" + ps_id + "'").toInt();

	//CNVs
	QVariant cnv_callset_id = getValue("SELECT id FROM cnv_callset WHERE processed_sample_id='" + ps_id + "'", true);
	if (cnv_callset_id.isValid())
	{
		output.cnvs = getValue("SELECT COUNT(*) FROM cnv WHERE cnv_callset_id='" + cnv_callset_id.toString() + "'").toInt();
	}

	//SVs
	QVariant sv_callset_id = getValue("SELECT id FROM sv_callset WHERE processed_sample_id='" + ps_id + "'", true);
	if (sv_callset_id.isValid())
	{
		QString cs_id = sv_callset_id.toString();
		static QStringList tables{"sv_deletion","sv_duplication","sv_insertion","sv_inversion","sv_translocation"};
		foreach(const QString& table, tables)
		{
			output.svs += getValue("SELECT count(*) FROM " + table + " WHERE sv_callset_id='" + cs_id+ "'").toInt();
		}
	}

	//REs
	output.res = getValue("SELECT COUNT(*) FROM repeat_expansion_genotype WHERE processed_sample_id='" + ps_id + "'").toInt();

	//QC
	output.qc_terms = getValue("SELECT COUNT(*) FROM processed_sample_qc WHERE processed_sample_id='" + ps_id + "'").toInt();

	return output;
}

void NGSD::importGeneExpressionData(const QString& expression_data_file_path, const QString& ps_name, bool force, bool debug)
{
	QTextStream outstream(stdout);
	QTime timer;
	timer.start();
	//check ps_name
	QString ps_id = processedSampleId(ps_name);
	if(debug) outstream << "Processed sample: " << ps_name << endl;

	// check if already imported
	int n_prev_entries = getValue("SELECT COUNT(`id`) FROM `expression` WHERE `processed_sample_id`=:0", false, ps_id).toInt();
	if(debug) outstream << "Previously imported expression values: " << n_prev_entries << endl;

	if (!force && (n_prev_entries > 0))
	{
		THROW(DatabaseException, "Expression values for sample '" + ps_name + "' already imported and method called without '-force' parameter: Cannot import data!");
	}

	// start transaction
	transaction();

	// delete old entries
	if (n_prev_entries > 0)
	{
		SqlQuery query = getQuery();
		query.exec("DELETE FROM `expression` WHERE `processed_sample_id`='"+ps_id+"'");
		if(debug) outstream << QByteArray::number(n_prev_entries) + " previously imported expression values deleted." << endl;
	}

	//get ENSG -> id mapping
	QMap<QByteArray,QByteArray> gene_mapping = getEnsemblGeneMapping();

	//get id <-> gene mapping from expression gene table
	QMap<QByteArray,int>& gene2id = getCache().gene_expression_gene2id;
	if(gene2id.isEmpty()) initGeneExpressionCache();


	// prepare query
	SqlQuery query = getQuery();
	query.prepare("INSERT INTO `expression`(`processed_sample_id`, `symbol_id`, `tpm`, `raw`) VALUES ('" + ps_id + "', :0, :1, :2)");


	// open file and iterate over expression values
	TSVFileStream tsv_file(expression_data_file_path);
	int idx_ensg = tsv_file.colIndex("gene_id", true);
	int idx_tpm = tsv_file.colIndex("tpm", true);
	int idx_raw = tsv_file.colIndex("raw", true);
	int n_imported = 0;
	int n_skipped = 0;

	while (!tsv_file.atEnd())
	{
		QByteArrayList tsv_line = tsv_file.readLine();
		QByteArray ensg = tsv_line.at(idx_ensg);
		double tpm = Helper::toDouble(tsv_line.at(idx_tpm), "TPM value");
		double raw = Helper::toInt(tsv_line.at(idx_raw), "raw value");

		//skip ENSG ids which are not in the NGSD
		if (!gene_mapping.contains(ensg))
		{
			n_skipped++;
			continue;
		}

		//get gene symbol id in expression
		int symbol_id;
		QByteArray gene_symbol = gene_mapping.value(ensg);
		if (gene2id.contains(gene_symbol))
		{
			symbol_id = gene2id.value(gene_symbol);
		}
		else
		{
			//add symbol to helper table
			symbol_id = addGeneSymbolToExpressionTable(gene_symbol);
			//add the new key-value pair to the cache (avoid reconstruction or gene name clashes)
			gene2id.insert(gene_symbol, symbol_id);
		}

		// import value
		query.bindValue(0, symbol_id);
		query.bindValue(1, tpm);
		query.bindValue(2, raw);
		query.exec();
		n_imported++;
	}


	// commit
	commit();

	if(debug) outstream << "runtime: " << Helper::elapsedTime(timer) << endl;
	if(debug) outstream << QByteArray::number(n_imported) + " expression values imported into the NGSD." << endl;
	if(debug) outstream << QByteArray::number(n_skipped) + " expression values skipped." << endl;
}

int NGSD::addGeneSymbolToExpressionTable(const QByteArray& gene_symbol)
{
	//only add approved gene names
	int gene_id = geneId(gene_symbol);
	if ( gene_id == -1) THROW(DatabaseException, "'" + gene_symbol + "' is not an approved gene name and cannot be added to the NGSD table!");
	// prepare query
	SqlQuery query = getQuery();
	query.prepare("INSERT INTO `expression_gene`(`symbol`) VALUES (:0)");
	query.bindValue(0, gene_symbol);
	query.exec();
	return query.lastInsertId().toInt();
}


void NGSD::importExonExpressionData(const QString& expression_data_file_path, const QString& ps_name, bool force, bool debug)
{
	QTextStream outstream(stdout);
	QTime timer;
	timer.start();
	//check ps_name
	QString ps_id = processedSampleId(ps_name);
	if(debug) outstream << "Processed sample: " << ps_name << endl;

	// check if already imported
	int n_prev_entries = getValue("SELECT COUNT(`id`) FROM `expression_exon` WHERE `processed_sample_id`=:0", false, ps_id).toInt();
	if(debug) outstream << "Previously imported expression values: " << n_prev_entries << endl;

	if (!force && (n_prev_entries > 0))
	{
		THROW(DatabaseException, "Expression values for sample '" + ps_name + "' already imported and method called without '-force' parameter: Cannot import data!");
	}

	// start transaction
	transaction();

	// delete old entries
	if (n_prev_entries > 0)
	{
		SqlQuery query = getQuery();
		query.exec("DELETE FROM `expression_exon` WHERE `processed_sample_id`='"+ps_id+"'");
		if(debug) outstream << QByteArray::number(n_prev_entries) + " previously imported expression values deleted." << endl;
	}


	// prepare query
	SqlQuery query = getQuery();
	query.prepare("INSERT INTO `expression_exon`(`processed_sample_id`, `chr`, `start`, `end`, `raw`, `rpb`, `srpb`) VALUES ('" + ps_id + "', :0, :1, :2, :3, :4, :5)");


	// open file and iterate over expression values
	TSVFileStream tsv_file(expression_data_file_path);
	int idx_exon = tsv_file.colIndex("exon", true);
	int idx_raw = tsv_file.colIndex("raw", true);
	int idx_rpb = tsv_file.colIndex("rpb", true);
	int idx_srpb = tsv_file.colIndex("srpb", true);

	int n_imported = 0;
	int n_skipped = 0;
	int n_duplicates = 0;
	int line_idx = 0;


	//get all valid exons
	QSet<QByteArray> valid_exons;
	SqlQuery query_exon = getQuery();
	query_exon.exec("SELECT DISTINCT gt.chromosome, ge.start, ge.end FROM `gene_exon` ge INNER JOIN `gene_transcript` gt ON ge.transcript_id = gt.id;");

	while(query_exon.next())
	{
		BedLine exon = BedLine(Chromosome("chr" + query_exon.value(0).toString()), query_exon.value(1).toInt(), query_exon.value(2).toInt());
		valid_exons << exon.toString(true).toUtf8();
	}

	if(debug) outstream << QByteArray::number(valid_exons.size()) << " unique exons stored in the NGSD (" << Helper::elapsedTime(timer) << ") " << endl;




	QSet<QByteArray> imported_exons;
	while (!tsv_file.atEnd())
	{
		QByteArrayList tsv_line = tsv_file.readLine();
		BedLine exon = BedLine::fromString(tsv_line.at(idx_exon));


		if(imported_exons.contains(exon.toString(true).toUtf8()))
		{
			n_duplicates++;
			continue;
		}

		int raw = Helper::toInt(tsv_line.at(idx_raw), "raw value");
		double rpb = Helper::toDouble(tsv_line.at(idx_rpb), "rpb value");
		double srpb = Helper::toDouble(tsv_line.at(idx_srpb), "srpb value");

		//skip exons which are not in the NGSD
		if (!valid_exons.contains(exon.toString(true).toUtf8()))
		{
			n_skipped++;
			continue;
		}

		// import value
		query.bindValue(0, exon.chr().strNormalized(true));
		query.bindValue(1, exon.start());
		query.bindValue(2, exon.end());
		query.bindValue(3, raw);
		query.bindValue(4, rpb);
		query.bindValue(5, srpb);
		query.exec();
		n_imported++;
		imported_exons.insert(exon.toString(true).toUtf8());

		line_idx++;
		if(debug && (line_idx % 100000 == 0))
		{
			outstream << QByteArray::number(line_idx) << " lines parsed..." << endl;
		}
	}


	// commit
	commit();
	if(debug) outstream << "runtime: " << Helper::elapsedTime(timer) << endl;
	if(debug) outstream << QByteArray::number(n_imported) + " expression values imported into the NGSD." << endl;
	if(debug) outstream << QByteArray::number(n_skipped) + " expression values skipped (not in NGSD)." << endl;
	if(debug) outstream << QByteArray::number(n_duplicates) + " expression values skipped (duplicates)." << endl;
}


QMap<QByteArray, QByteArray> NGSD::getEnsemblGeneMapping()
{
	QMap<QByteArray, QByteArray> mapping;
	SqlQuery query = getQuery();
	query.exec("SELECT symbol, ensembl_id FROM gene WHERE ensembl_id IS NOT NULL");
	while(query.next())
	{
		mapping.insert(query.value(1).toByteArray(), query.value(0).toByteArray());
	}

	return mapping;
}

QMap<QByteArray, QByteArray> NGSD::getGeneEnsemblMapping()
{
	QMap<QByteArray, QByteArray> mapping;
	SqlQuery query = getQuery();
	query.exec("SELECT symbol, ensembl_id FROM gene WHERE ensembl_id IS NOT NULL");
	while(query.next())
	{
		mapping.insert(query.value(0).toByteArray(), query.value(1).toByteArray());
	}

	return mapping;
}

QMap<QByteArray, QByteArrayList> NGSD::getExonTranscriptMapping()
{
	QMap<QByteArray, QByteArrayList> mapping;
	SqlQuery query = getQuery();
	query.exec("SELECT gt.chromosome, ge.start, ge.end, gt.name FROM gene_exon ge INNER JOIN gene_transcript gt ON ge.transcript_id=gt.id");
	while(query.next())
	{
		QByteArray exon = "chr" + query.value(0).toByteArray() + ":" + query.value(1).toByteArray() + "-" + query.value(2).toByteArray();
		if(mapping.contains(exon))
		{
			mapping[exon].append(query.value(3).toByteArray());
		}
		else
		{
			mapping.insert(exon, QByteArrayList() << query.value(3).toByteArray());
		}
	}

	return mapping;
}

QVector<double> NGSD::getGeneExpressionValues(const QByteArray& gene, int sys_id, const QString& tissue_type, bool log2)
{
	QVector<double> expr_values;
	QByteArray gene_approved = geneToApproved(gene);
	auto gene2id = getGeneExpressionGene2IdMapping();
	int symbol_id = gene2id.value(gene_approved);
	if (gene_approved.isEmpty()) return expr_values;

	QStringList expr_values_str = getValues(QString() + "SELECT ev.tpm FROM `expression ev "
											  + "INNER JOIN `processed_sample` ps ON ev.processed_sample_id = ps.id "
											  + "INNER JOIN `sample` s ON ps.sample_id = s.id "
											  + "WHERE ps.processing_system_id = " + QByteArray::number(sys_id) + " AND s.tissue=:0 AND ev.symbol=" + QString::number(symbol_id), tissue_type);

	foreach (const QString& value, expr_values_str)
	{
		if(log2)
		{
			expr_values << std::log2(Helper::toDouble(value) + 1);
		}
		else
		{
			expr_values << Helper::toDouble(value);
		}

	}

	return expr_values;
}

QVector<double> NGSD::getGeneExpressionValues(const QByteArray& gene, QSet<int> cohort, bool log2)
{
	// debug
	QVector<int> cohort_sorted = cohort.toList().toVector();
	std::sort(cohort_sorted.begin(), cohort_sorted.end());

	return getGeneExpressionValues(gene, cohort_sorted, log2);
}

QVector<double> NGSD::getGeneExpressionValues(const QByteArray& gene, QVector<int> cohort, bool log2)
{
	QVector<double> expr_values;
	QByteArray gene_approved = geneToApproved(gene);
	auto gene2id = getGeneExpressionGene2IdMapping();
	int symbol_id = gene2id.value(gene_approved);
	if (gene_approved.isEmpty()) THROW(ArgumentException, "Can't convert gene '" + gene + "' to approved symbol!");

	SqlQuery query = getQuery();
	query.prepare("SELECT ev.tpm FROM `expression` ev WHERE ev.symbol_id=" + QString::number(symbol_id) + " AND ev.processed_sample_id=:0");

	foreach (int ps_id, cohort)
	{
		//execute query
		query.bindValue(0, ps_id);
		query.exec();

		//parse result
		if(query.size()==0)
		{
			//no expression value found
			expr_values << nan("");
		}
		else
		{
			query.next();
			if(log2)
			{
				expr_values << std::log2(query.value(0).toDouble() + 1);
			}
			else
			{
				expr_values << query.value(0).toDouble();
			}

		}
	}

	return expr_values;
}

QVector<double> NGSD::getExonExpressionValues(const BedLine& exon, QSet<int> cohort, bool log2)
{
	QVector<double> expr_values;

	QList<int> cohort_sorted = cohort.toList();
	std::sort(cohort_sorted.begin(), cohort_sorted.end());
	QStringList cohort_str;
	foreach (int i , cohort_sorted)
	{
		cohort_str << QString::number(i);
	}


	QStringList expr_values_str = getValues(QString() + "SELECT ee.srpb FROM `expression_exon` ee "
											+ "WHERE ee.chr='" + exon.chr().strNormalized(true) + "' "
											+ "AND ee.start=" + QByteArray::number(exon.start()) + " "
											+ "AND ee.end=" + QByteArray::number(exon.end()) + " "
											+ "AND ee.processed_sample_id IN (" + cohort_str.join(", ") +  ")");

	foreach (const QString& value, expr_values_str)
	{
		if(log2)
		{
			expr_values << std::log2(Helper::toDouble(value) + 1);
		}
		else
		{
			expr_values << Helper::toDouble(value);
		}

	}

	return expr_values;
}

QMap<QByteArray, double> NGSD::getGeneExpressionValuesOfSample(const QString& ps_id, bool allow_empty)
{
	QMap<QByteArray, double> expression_data;

	SqlQuery query = getQuery();
	auto id2gene = getGeneExpressionId2GeneMapping();
	query.prepare("SELECT symbol_id, tpm FROM expression WHERE processed_sample_id=:0");
	query.bindValue(0, ps_id);
	query.exec();

	if(!allow_empty && query.size() < 1) THROW(ArgumentException, "No expression data found for processed sample '" + processedSampleName(ps_id) + "'!");

	while(query.next())
	{
		expression_data.insert(id2gene.value(query.value(0).toInt()), query.value(1).toDouble());
	}
	return expression_data;
}

QMap<int, QByteArray> NGSD::getGeneExpressionId2GeneMapping()
{
	QMap<int, QByteArray>& id2gene = getCache().gene_expression_id2gene;
	if(id2gene.isEmpty()) initGeneExpressionCache();
	return id2gene;
}

QMap<QByteArray, int> NGSD::getGeneExpressionGene2IdMapping()
{
	QMap<QByteArray, int>& gene2id = getCache().gene_expression_gene2id;
	if(gene2id.isEmpty()) initGeneExpressionCache();
	return gene2id;
}

QMap<QByteArray, ExpressionStats> NGSD::calculateGeneExpressionStatistics(QSet<int>& cohort, QByteArray gene_symbol, bool debug)
{
	QTime timer;
	timer.start();

	QMap<QByteArray, ExpressionStats> gene_stats;

	QMap<int,QByteArray> id2gene = getGeneExpressionId2GeneMapping();
	QMap<QByteArray,int> gene2id = getGeneExpressionGene2IdMapping();

	//processed sample IDs as string list
	QStringList ps_ids_str;
	foreach (int id, cohort)
	{
		ps_ids_str << QString::number(id);
	}
	if(debug) qDebug() << "Cohort size: " << QString::number(cohort.size());

	//get expression data, ungrouped/long format
	SqlQuery q = getQuery();
	QString q_str;
	if (gene_symbol.isEmpty())
	{
		q_str = QString(
					"SELECT e.symbol_id, AVG(e.tpm), AVG(LOG2(e.tpm+1)), STD(LOG2(e.tpm+1)) FROM expression e "
					"WHERE e.processed_sample_id IN (" + ps_ids_str.join(", ") + ") "
					"GROUP BY e.symbol_id ORDER BY e.symbol_id;"
					);
	}
	else
	{
		//check if gene name is approved symbol
		int gene_id = geneId(gene_symbol);
		if (gene_id < 0 ) THROW(ArgumentException, "'" + gene_symbol + "' is not an approved gene symbol!");
		gene_symbol = geneSymbol(gene_id);
		if (!gene2id.contains(gene_symbol)) THROW(ArgumentException, "'" + gene_symbol + "' is not gene expression database!");

		q_str = QString(
					"SELECT e.symbol_id, AVG(e.tpm), AVG(LOG2(e.tpm+1)), STD(LOG2(e.tpm+1)) FROM expression e "
					"WHERE e.processed_sample_id IN (" + ps_ids_str.join(", ") + ") AND e.symbol_id=" + QString::number(gene2id.value(gene_symbol))
					);
	}


	if(debug) qDebug() << "Query SQL server: " << q_str;
	q.exec(q_str);
	if(debug) qDebug() << "Get expression data from SQL server: " << Helper::elapsedTime(timer);

	//parse results
	while (q.next())
	{
		QByteArray gene_symbol = id2gene.value(q.value(0).toInt());
		ExpressionStats stats;
		stats.mean = q.value(1).toDouble();
		stats.mean_log2 = q.value(2).toDouble();
		stats.stddev_log2 = q.value(3).toDouble();
		gene_stats.insert(gene_symbol, stats);

	}

	if(debug) qDebug() << "Statistics calculated: " << Helper::elapsedTime(timer);
	if(debug) qDebug() << "gene_stats: " << gene_stats.size();

	return gene_stats;
}

QMap<QByteArray, ExpressionStats> NGSD::calculateExonExpressionStatistics(QSet<int>& cohort, const BedLine& exon, bool debug)
{
	QTime timer;
	timer.start();

	QMap<QByteArray, ExpressionStats> exon_stats;

	//processed sample IDs as string list
	QStringList ps_ids_str;
	foreach (int id, cohort)
	{
		ps_ids_str << QString::number(id);
	}
	if(debug) qDebug() << "Cohort size: " << QString::number(cohort.size());

	//get expression data stats
	SqlQuery q = getQuery();
	QString q_str;
	if (exon.isValid())
	{
		// limit output to specific exon
		q_str = QString(
					"SELECT e.chr, e.start, e.end, AVG(e.srpb), AVG(LOG2(e.srpb+1)), STD(LOG2(e.srpb+1)) "
					"FROM expression_exon e "
					"WHERE e.processed_sample_id IN (" + ps_ids_str.join(", ") + ") "
					"AND e.chr='" + exon.chr().strNormalized(true) + "' "
					"AND e.start=" + QByteArray::number(exon.start()) + " "
					"AND e.end=" + QByteArray::number(exon.end()) + "; "
					);
	}
	else
	{
		// get expression values of all exons
		q_str = QString(
					"SELECT e.chr, e.start, e.end, AVG(e.srpb), AVG(LOG2(e.srpb+1)), STD(LOG2(e.srpb+1)) "
					"FROM expression_exon e "
					"WHERE e.processed_sample_id IN (" + ps_ids_str.join(", ") + ") "
					"GROUP BY e.chr, e.start, e.end ORDER BY e.chr ASC, e.start ASC, e.end ASC;"
					);
	}


	if(debug) qDebug() << "Query SQL server: " << q_str;
	q.exec(q_str);
	if(debug) qDebug() << "Get expression data from SQL server: " << Helper::elapsedTime(timer);

	//parse results
	while (q.next())
	{
		BedLine exon = BedLine(Chromosome(q.value(0).toByteArray()), q.value(1).toInt(), q.value(2).toInt());
		ExpressionStats stats;
		stats.mean = q.value(3).toDouble();
		stats.mean_log2 = q.value(4).toDouble();
		stats.stddev_log2 = q.value(5).toDouble();
		exon_stats.insert(exon.toString(true).toUtf8(), stats);

	}

	if(debug) qDebug() << "Statistics calculated: " << Helper::elapsedTime(timer);
	if(debug) qDebug() << "exon_stats: " << exon_stats.size();

	return exon_stats;
}


QMap<QByteArray, ExpressionStats> NGSD::calculateCohortExpressionStatistics(int sys_id, const QString& tissue_type, QSet<int>& cohort, const QString& project, const QString& ps_id,
																	  RnaCohortDeterminationStategy cohort_type, const QStringList& exclude_quality, bool debug)
{
	QTime timer;
	timer.start();

	//get cohort
	cohort = getRNACohort(sys_id, tissue_type, project, ps_id, cohort_type, "genes", exclude_quality, debug);

	QMap<QByteArray, ExpressionStats> expression_stats = calculateGeneExpressionStatistics(cohort);

	if(debug) qDebug() << "Get expression stats: " << Helper::elapsedTime(timer);
	return expression_stats;
}

QSet<int> NGSD::getRNACohort(int sys_id, const QString& tissue_type, const QString& project, const QString& ps_id, RnaCohortDeterminationStategy cohort_type, const QByteArray& mode, const QStringList& exclude_quality, bool debug)
{
	QTime timer;
	timer.start();
	QSet<int> cohort;

	//get all available ps ids with expression data
	QSet<int> all_ps_ids;
	if (mode == "genes")
	{
		all_ps_ids = getValuesInt("SELECT DISTINCT e.processed_sample_id FROM expression e").toSet();
	}
	else if (mode == "exons")
	{
		all_ps_ids = getValuesInt("SELECT DISTINCT e.processed_sample_id FROM expression_exon e").toSet();
	}
	else
	{
		THROW(ArgumentException, "Invalid mode '" + mode + "' given! Valid modes are 'genes' or 'exons'");
	}

	if(debug) qDebug() << "Get all psample ids with expression data: " << Helper::elapsedTime(timer);


	if ((cohort_type == RNA_COHORT_GERMLINE) || (cohort_type == RNA_COHORT_GERMLINE_PROJECT))
	{
		// check tissue
		if (!getEnum("sample", "tissue").contains(tissue_type))
		{
			THROW(ArgumentException, "'" +  tissue_type + "' is not a valid tissue type in the NGSD!")
		}

		QString query_string_cohort = QString(
					"SELECT ps.id "
					"FROM processed_sample ps "
					"         LEFT JOIN sample s on ps.sample_id = s.id "
					"WHERE ps.processing_system_id = " + QByteArray::number(sys_id) + " "
					"  AND s.tissue = '" + tissue_type + "' "
					);

		if (exclude_quality.size() > 0)
		{
			query_string_cohort.append(" AND ps.quality NOT IN ('" + exclude_quality.join("', '") + "')");
		}

		if (cohort_type == RNA_COHORT_GERMLINE_PROJECT)
		{
			int project_id = getValue("SELECT id FROM project WHERE name=:0", false, project).toInt();
			query_string_cohort.append("  AND ps.project_id = " + QByteArray::number(project_id));
		}

		cohort = getValuesInt(query_string_cohort).toSet();

	}
	else if (cohort_type == RNA_COHORT_SOMATIC)
	{
		QTime timer;
		timer.start();

		//check requirements:
		if(ps_id.trimmed().isEmpty()) THROW(ArgumentException, "Processed sample id required for somatic RNA cohort determination!");


		QString s_id = sampleId(processedSampleName(ps_id));
		int project_id = getValue("SELECT id FROM project WHERE name=:0", false, project).toInt();

		//get sample ids of related samples
		QSet<int> sample_ids = relatedSamples(Helper::toInt(s_id), "same sample", "DNA");
		sample_ids.insert(Helper::toInt(s_id));

		//get ICD10 and HPO of sample and related sample
		QSet<QString> icd10_disease_info;
		QSet<QString> hpo_disease_info;
		foreach (int id, sample_ids)
		{
			QList<SampleDiseaseInfo> icd10 = getSampleDiseaseInfo(QString::number(id), "ICD10 code");
			foreach (const SampleDiseaseInfo& info, icd10)
			{
				icd10_disease_info << info.disease_info;
			}
			QList<SampleDiseaseInfo> hpo = getSampleDiseaseInfo(QString::number(id), "HPO term id");
			foreach (const SampleDiseaseInfo& info, hpo)
			{
				hpo_disease_info << info.disease_info;
			}
		}
		if(debug) qDebug() << sample_ids;
		if(debug) qDebug() << "HPO: " << hpo_disease_info;
		if(debug) qDebug() << "ICD10: " << icd10_disease_info;

		if(icd10_disease_info.size() > 1) THROW(DatabaseException, "Sample " + processedSampleName(ps_id) + " contains more than 1 ICD10 code, cannot create sample cohort");
		if(hpo_disease_info.size() > 1) THROW(DatabaseException, "Sample " + processedSampleName(ps_id) + " contains more than 1 HPO term, cannot create sample cohort");
		if(icd10_disease_info.size() <1) THROW(DatabaseException, "Sample " + processedSampleName(ps_id) + " does not contain ICD10 code, cannot create sample cohort");
		if(hpo_disease_info.size() < 1) THROW(DatabaseException, "Sample " + processedSampleName(ps_id) + " does not contain HPO term, cannot create sample cohort");


		if(debug) qDebug() << "get disease info" << Helper::elapsedTime(timer);
		timer.restart();

		QString query_string_cohort = QString("SELECT DISTINCT ps.id FROM processed_sample ps LEFT JOIN sample s on ps.sample_id=s.id	"
											  "LEFT JOIN sample_relations sr ON s.id=sr.sample1_id OR s.id=sr.sample2_id "
											  "LEFT JOIN sample_disease_info sdi ON s.id=sdi.sample_id OR sr.sample1_id=sdi.sample_id OR sr.sample2_id=sdi.sample_id "
											  "WHERE ps.processing_system_id=" + QString::number(sys_id) + " "
											  "AND ps.project_id=" + QString::number(project_id) + " "
											  "AND ps.quality != 'bad' "
											  "AND (sr.relation='same sample' OR sr.relation IS NULL) "
											  "AND ((sdi.type='ICD10 code' AND sdi.disease_info='" + icd10_disease_info.toList().at(0) + "') "
											  "OR (sdi.type='HPO term id' AND sdi.disease_info='" + hpo_disease_info.toList().at(0) + "')) ");

		if (exclude_quality.size() > 0)
		{
			query_string_cohort.append("AND ps.quality NOT IN ('" + exclude_quality.join("', '") + "') ");
		}


		cohort = getValuesInt(query_string_cohort).toSet();


		if(debug) qDebug() << "get ps_ids of cohort (somatic)" << Helper::elapsedTime(timer);
		timer.restart();

		if(cohort.size() < 1) THROW(DatabaseException, "No matching samples for cohort found. Cannot create statistics.");
	}
	else
	{
		THROW(ArgumentException, "Invalid cohort type!");
	}

	//consider only ps_ids which have expression data
	cohort = cohort.intersect(all_ps_ids);
	if(debug) qDebug() << "Cohort size: " << cohort.size() << ": " << cohort;

	if(debug) qDebug() << "Get psample ids: " << Helper::elapsedTime(timer);

	return cohort;
}

CopyNumberVariant NGSD::cnv(int cnv_id)
{
	SqlQuery query = getQuery();
	query.exec("SELECT * FROM cnv WHERE id='" + QString::number(cnv_id) + "'");
	if (!query.next()) THROW(DatabaseException, "CNV with identifier '" + QString::number(cnv_id) + "' does not exist!");

	return CopyNumberVariant(query.value("chr").toByteArray(), query.value("start").toInt(), query.value("end").toInt());
}

QString NGSD::addSomaticCnv(int callset_id, const CopyNumberVariant &cnv, const CnvList &cnv_list, double min_ll)
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
				if (min_ll>0.0 && Helper::toDouble(entry, "log-likelihood")<min_ll)
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
		if (caller==CnvCallerType::CLINCNV)
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

QString NGSD::addSomaticSv(int callset_id, const BedpeLine& sv, const BedpeFile& svs)
{
	// skip SVs on special chr
	if (!sv.chr1().isNonSpecial() || !sv.chr2().isNonSpecial() )
	{
		THROW(ArgumentException, "Structural variants on special chromosomes can not be added to the NGSD!");
		return "";
	}
	// parse qc data
	QJsonObject quality_metrics;
	// get quality value
	quality_metrics.insert("quality", QString(sv.annotations()[svs.annotationIndexByName("SOMATICSCORE")].trimmed()));
	// get filter values
	quality_metrics.insert("filter", QString(sv.annotations()[svs.annotationIndexByName("FILTER")].trimmed()));
	QJsonDocument json_doc;
	json_doc.setObject(quality_metrics);
	QByteArray quality_metrics_string = json_doc.toJson(QJsonDocument::Compact);

	QByteArray table = somaticSvTableName(sv.type()).toLatin1();

	if (sv.type() == StructuralVariantType::DEL || sv.type() == StructuralVariantType::DUP || sv.type() == StructuralVariantType::INV)
	{
		// insert SV into the NGSD
		SqlQuery query = getQuery();
		query.prepare("INSERT INTO `" + table + "` (`somatic_sv_callset_id`, `chr`, `start_min`, `start_max`, `end_min`, `end_max`, `quality_metrics`) " +
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
		return query.lastInsertId().toString();

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
		query.prepare(QByteArray() + "INSERT INTO " + table + " (`somatic_sv_callset_id`, `chr`, `pos`, `ci_upper`, `inserted_sequence`, "
					  + "`known_left`, `known_right`, `quality_metrics`) VALUES (:0, :1,  :2, :3, :4, :5, :6, :7)");
		query.bindValue(0, callset_id);
		query.bindValue(1, sv.chr1().strNormalized(true));
		query.bindValue(2, pos);
		query.bindValue(3, ci_upper);
		query.bindValue(4, inserted_sequence);
		query.bindValue(5, known_left);
		query.bindValue(6, known_right);
		query.bindValue(7, quality_metrics_string);

		query.exec();

		//return insert ID
		return query.lastInsertId().toString();
	}
	else if (sv.type() == StructuralVariantType::BND)
	{
		// insert SV into the NGSD
		SqlQuery query = getQuery();
		query.prepare(QByteArray() + "INSERT INTO " + table + " (`somatic_sv_callset_id`, `chr1`, `start1`, `end1`, `chr2`, `start2`, `end2`, `quality_metrics`)"
					  + " VALUES (:0, :1,  :2, :3, :4, :5, :6, :7)");
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
		return query.lastInsertId().toString();
	}
	else
	{
		THROW(ArgumentException, "Invalid structural variant type!");
		return "";
	}
}
QString NGSD::somaticSvId(const BedpeLine& sv, int callset_id, const BedpeFile& svs, bool throw_if_fails)
{
	QString db_table_name = somaticSvTableName(sv.type());
	SqlQuery query = getQuery();
	if (sv.type() == StructuralVariantType::DEL || sv.type() == StructuralVariantType::DUP || sv.type() == StructuralVariantType::INV)
	{
		// get SV from NGSD
		query.exec("SELECT id FROM `" + db_table_name + "` WHERE `somatic_sv_callset_id`=" + QString::number(callset_id)
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
		query.exec("SELECT id FROM `" + db_table_name + "` WHERE `somatic_sv_callset_id`=" + QString::number(callset_id)
				   + " AND `chr`=\"" + sv.chr1().strNormalized(true) + "\""
				   + " AND (`pos` - `ci_lower`) =" + QString::number(pos)
				   + " AND `ci_upper`=" + QString::number(ci_upper)
				   + " " + sequence_filter.join(" "));
	}
	else if (sv.type() == StructuralVariantType::BND)
	{
		// get SV from NGSD
		query.exec("SELECT id FROM `" + db_table_name + "` WHERE `somatic_sv_callset_id`=" + QString::number(callset_id)
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

	// Throw error if multiple matches found
	if(query.size() > 1)
	{
		QStringList ids;
		while (query.next())
		{
			ids << query.value("id").toString();
		}
		THROW(DatabaseException, "Multiple matching SVs found in NGSD!\t(" + ids.join(",") + ")");
	}

	if(query.size() < 1)
	{
		if(!throw_if_fails) return "";

		THROW(DatabaseException, "SV " + BedpeFile::typeToString(sv.type()) + " at " + sv.positionRange() + " for callset with id '" + QString::number(callset_id) + "' not found in NGSD!");
	}

	query.next();
	return query.value("id").toString();
}

BedpeLine NGSD::somaticSv(QString sv_id, StructuralVariantType type, const BedpeFile& svs, bool no_annotation, int* callset_id)
{
	BedpeLine sv;
	QList<QByteArray> annotations;

	int qual_idx = -1, filter_idx = -1, alt_a_idx = -1, info_a_idx = -1;
	annotations = QVector<QByteArray>(svs.annotationHeaders().size()).toList();
	if (!no_annotation)
	{
		// determine indices for annotations
		qual_idx = svs.annotationIndexByName("SOMATICSCORE");
		filter_idx = svs.annotationIndexByName("FILTER");
		alt_a_idx = svs.annotationIndexByName("ALT_A");
		info_a_idx = svs.annotationIndexByName("INFO_A");
	}

	QString table = somaticSvTableName(type);

	// get DEL, DUP or INV
	if (type == StructuralVariantType::DEL || type == StructuralVariantType::DUP || type == StructuralVariantType::INV)
	{
		// define pos varaibles
		Chromosome chr1, chr2;
		int start1, start2, end1, end2;

		// get SV from the NGSD
		SqlQuery query = getQuery();
		query.exec("SELECT * FROM `" + table + "` WHERE id=" + sv_id);
		if (query.size() == 0 ) THROW(DatabaseException, "SV with id '" + sv_id + "' not found in table '" + table + "'!" );
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
		if (callset_id) *callset_id = query.value("sv_callset_id").toInt();
	}
	else if (type == StructuralVariantType::INS)
	{
		// get INS from the NGSD
		SqlQuery query = getQuery();
		query.exec("SELECT * FROM " + table +" WHERE id = " + sv_id);
		if (query.size() == 0 ) THROW(DatabaseException, "SV with id '" + sv_id + "' not found in table '" + table + "'!");
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
		if (callset_id) *callset_id = query.value("sv_callset_id").toInt();
	}
	else if (type == StructuralVariantType::BND)
	{
		// define pos varaibles
		Chromosome chr1, chr2;
		int start1, start2, end1, end2;

		// get SV from the NGSD
		SqlQuery query = getQuery();
		query.exec("SELECT * FROM " + table + " WHERE id = " + sv_id);
		if (query.size() == 0 ) THROW(DatabaseException, "SV with id '" + sv_id + "' not found in table '" + table + "'!" );
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
		if (callset_id) *callset_id = query.value("sv_callset_id").toInt();
	}
	else
	{
		THROW(ArgumentException, "Invalid structural variant type!");
	}

	return sv;
}

QString NGSD::somaticSvTableName(StructuralVariantType type)
{
	switch (type)
	{
		case StructuralVariantType::DEL:
			return "somatic_sv_deletion";
		case StructuralVariantType::DUP:
			return "somatic_sv_duplication";
		case StructuralVariantType::INS:
			return "somatic_sv_insertion";
		case StructuralVariantType::INV:
			return "somatic_sv_inversion";
		case StructuralVariantType::BND:
			return "somatic_sv_translocation";
		default:
			THROW(ArgumentException, "Invalid structural variant type!");
			break;
	}
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

	//get genotype
	int idx_format = svs.annotationIndexByName("FORMAT");
	QByteArray genotype;
	QByteArrayList format_keys = sv.annotations()[idx_format].split(':');
	QByteArrayList format_values = sv.annotations()[idx_format + 1].split(':');
	for (int i = 0; i < format_keys.size(); ++i)
	{
		if (format_keys.at(i) == "GT")
		{
			genotype = format_values.at(i).trimmed();
			if (genotype == "1/1")
			{
				genotype = "hom";
			}
			else
			{
				genotype = "het";
			}
			break;
		}
	}
	if (genotype.isEmpty()) THROW(FileParseException, "SV doesn't contain genotype information!");

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
		query.prepare("INSERT INTO `" + table + "` (`sv_callset_id`, `chr`, `start_min`, `start_max`, `end_min`, `end_max`, `genotype` , `quality_metrics`) " +
					  "VALUES (:0, :1,  :2, :3, :4, :5, :6, :7)");
		query.bindValue(0, callset_id);
		query.bindValue(1, sv.chr1().strNormalized(true));
		query.bindValue(2,  sv.start1());
		query.bindValue(3,  sv.end1());
		query.bindValue(4,  sv.start2());
		query.bindValue(5,  sv.end2());
		query.bindValue(6, genotype);
		query.bindValue(7, quality_metrics_string);

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
					  + "`known_left`, `known_right`, `genotype`, `quality_metrics`) VALUES (:0, :1,  :2, :3, :4, :5, :6, :7, :8)");
		query.bindValue(0, callset_id);
		query.bindValue(1, sv.chr1().strNormalized(true));
		query.bindValue(2, pos);
		query.bindValue(3, ci_upper);
		query.bindValue(4, inserted_sequence);
		query.bindValue(5, known_left);
		query.bindValue(6, known_right);
		query.bindValue(7, genotype);
		query.bindValue(8, quality_metrics_string);

		query.exec();

		//return insert ID
		return query.lastInsertId().toInt();
	}
	else if (sv.type() == StructuralVariantType::BND)
	{
		// insert SV into the NGSD
		SqlQuery query = getQuery();
		query.prepare(QByteArray() + "INSERT INTO `sv_translocation` (`sv_callset_id`, `chr1`, `start1`, `end1`, `chr2`, `start2`, `end2`, `genotype`, "
					  + "`quality_metrics`) VALUES (:0, :1,  :2, :3, :4, :5, :6, :7, :8)");
		query.bindValue(0, callset_id);
		query.bindValue(1, sv.chr1().strNormalized(true));
		query.bindValue(2, sv.start1());
		query.bindValue(3, sv.end1());
		query.bindValue(4, sv.chr2().strNormalized(true));
		query.bindValue(5, sv.start2());
		query.bindValue(6, sv.end2());
		query.bindValue(7, genotype);
		query.bindValue(8, quality_metrics_string);
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

	// Throw error if multiple matches found
	if(query.size() > 1)
	{
		QStringList ids;
		while (query.next())
		{
			ids << query.value("id").toString();
		}
		THROW(DatabaseException, "Multiple matching SVs found in NGSD!\t(" + ids.join(",") + ")");
	}

	if(query.size() < 1)
	{
		if(!throw_if_fails) return "";

		THROW(DatabaseException, "SV " + BedpeFile::typeToString(sv.type()) + " at " + sv.positionRange() + " for callset with id '" + QString::number(callset_id) + "' not found in NGSD!");
	}

	query.next();
	return query.value("id").toString();

}

BedpeLine NGSD::structuralVariant(int sv_id, StructuralVariantType type, const BedpeFile& svs, bool no_annotation, int* callset_id)
{
	BedpeLine sv;
	QList<QByteArray> annotations;

	int qual_idx = -1, filter_idx = -1, alt_a_idx = -1, info_a_idx = -1, format_idx = -1;
	annotations = QVector<QByteArray>(svs.annotationHeaders().size()).toList();
	format_idx = svs.annotationIndexByName("FORMAT");
	if (!no_annotation)
	{
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
		if (query.size() == 0 ) THROW(DatabaseException, "SV with id '" + QString::number(sv_id) + "' not found in table '" + table + "'!" );
		query.next();
		chr1 = Chromosome(query.value("chr").toByteArray());
		chr2 = Chromosome(query.value("chr").toByteArray());
		start1 = query.value("start_min").toInt();
		end1 = query.value("start_max").toInt();
		start2 = query.value("end_min").toInt();
		end2 = query.value("end_max").toInt();
		QByteArray genotype = query.value("genotype").toByteArray();
		if (genotype == "hom")
		{
			genotype = "1/1";
		}
		else if (genotype == "het")
		{
			genotype = "0/1";
		}
		else // 'n/a'
		{
			genotype = "./.";
		}
		annotations[format_idx] = "GT";
		annotations[format_idx + 1] = genotype;

		if (!no_annotation)
		{
			// parse quality & filter
			QJsonObject quality_metrics = QJsonDocument::fromJson(query.value("quality_metrics").toByteArray()).object();
			annotations[qual_idx] = quality_metrics.value("quality").toString().toUtf8();
			annotations[filter_idx] = quality_metrics.value("filter").toString().toUtf8();
		}

		// create SV
		sv = BedpeLine(chr1, start1, end1, chr2, start2, end2, type, annotations);
		if (callset_id) *callset_id = query.value("sv_callset_id").toInt();
	}
	else if (type == StructuralVariantType::INS)
	{
		// get INS from the NGSD
		SqlQuery query = getQuery();
		query.exec("SELECT * FROM `sv_insertion` WHERE id = " + QByteArray::number(sv_id));
		if (query.size() == 0 ) THROW(DatabaseException, "SV with id '" + QString::number(sv_id) + "' not found in table 'sv_insertion'!" );
		query.next();
		Chromosome chr = Chromosome(query.value("chr").toByteArray());
		int pos = query.value("pos").toInt();
		int pos_upper = pos + query.value("ci_upper").toInt();
		QByteArray genotype = query.value("genotype").toByteArray();
		if (genotype == "hom")
		{
			genotype = "1/1";
		}
		else if (genotype == "het")
		{
			genotype = "0/1";
		}
		else // 'n/a'
		{
			genotype = "./.";
		}
		annotations[format_idx] = "GT";
		annotations[format_idx + 1] = genotype;

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
		if (callset_id) *callset_id = query.value("sv_callset_id").toInt();
	}
	else if (type == StructuralVariantType::BND)
	{
		// define pos varaibles
		Chromosome chr1, chr2;
		int start1, start2, end1, end2;

		// get SV from the NGSD
		SqlQuery query = getQuery();
		query.exec("SELECT * FROM `sv_translocation` WHERE id = " + QByteArray::number(sv_id));
		if (query.size() == 0 ) THROW(DatabaseException, "SV with id '" + QString::number(sv_id) + "' not found in table 'sv_translocation'!" );
		query.next();
		chr1 = Chromosome(query.value("chr1").toByteArray());
		chr2 = Chromosome(query.value("chr2").toByteArray());
		start1 = query.value("start1").toInt();
		end1 = query.value("end1").toInt();
		start2 = query.value("start2").toInt();
		end2 = query.value("end2").toInt();
		QByteArray genotype = query.value("genotype").toByteArray();
		if (genotype == "hom")
		{
			genotype = "1/1";
		}
		else if (genotype == "het")
		{
			genotype = "0/1";
		}
		else // 'n/a'
		{
			genotype = "./.";
		}
		annotations[format_idx] = "GT";
		annotations[format_idx + 1] = genotype;

		if (!no_annotation)
		{
			// parse quality & filter
			QJsonObject quality_metrics = QJsonDocument::fromJson(query.value("quality_metrics").toByteArray()).object();
			annotations[qual_idx] = quality_metrics.value("quality").toString().toUtf8();
			annotations[filter_idx] = quality_metrics.value("filter").toString().toUtf8();
		}

		// create SV
		sv = BedpeLine(chr1, start1, end1, chr2, start2, end2, type, annotations);
		if (callset_id) *callset_id = query.value("sv_callset_id").toInt();
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
		QVariant value = q.value(0);
		if (!value.isNull())
		{
			output << value.toInt();
		}
	}
	return output;
}

QVector<double> NGSD::getValuesDouble(const QString& query, QString bind_value) const
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

	QVector<double> output;
	output.reserve(q.size());
	while(q.next())
	{
		QVariant value = q.value(0);
		if (!value.isNull())
		{
			output << value.toDouble();
		}
	}
	return output;
}

void NGSD::executeQueriesFromFile(QString filename)
{
	QStringList lines = Helper::loadTextFile(filename, true);
	QString query = "";
	foreach(const QString& line, lines)
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

bool NGSD::isProductionDb() const
{
	//no table 'db_info' > no production database
	if (!tables().contains("db_info")) return false;

	//no 'is_production' entry > no production database
	SqlQuery query = getQuery();
	query.exec("SELECT value FROM db_info WHERE name = 'is_production'");
	if (!query.next()) return false;

	QString is_production = query.value(0).toString().trimmed().toLower();
	if (is_production!="true" && is_production!="false") THROW(DatabaseException, "Entry 'is_production' in table 'db_info' contains invalid value '" + is_production + "'! Valid are 'true' or 'false'.");

	return is_production=="true";
}

QStringList NGSD::tables() const
{
	return db_->driver()->tables(QSql::Tables);
}

const TableInfo& NGSD::tableInfo(const QString& table, bool use_cache) const
{
	QMap<QString, TableInfo>& table_infos = getCache().table_infos;

	//create if necessary
	if (!table_infos.contains(table) || !use_cache)
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
			else if (type.startsWith("set("))
			{
				info.type = TableFieldInfo::SET;
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
				if (table=="project" && info.name=="name") info.type_constraints.regexp = QRegularExpression("^[A-Za-z0-9_-]*$");
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
					else if (table=="sample_relations")
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
					else if (table=="sample_disease_info")
					{
						if (info.name=="sample_id")
						{
							info.fk_name_sql = "name";
						}
						if (info.name=="user_id")
						{
							info.fk_name_sql = "name";
						}
					}
					else if (table=="user_permissions")
					{
						if (info.name=="user_id")
						{
							info.fk_name_sql = "name";
						}
					}
					else if (table=="somatic_pathway_gene")
					{
						if (info.name=="pathway_id")
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
			QString value_as_string = value.isNull() ? ""  : value.toString();
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
		if ((!test_db_ || isProductionDb()) && password!=Settings::string("ngsd_pass"))
		{
			THROW(DatabaseException, "Password provided for re-initialization of production database is incorrect!");
		}

		//check if we delete or clear the existing tables (clearing is a lot faster on Windows)
		bool clear_only = false;
		if(test_db_ && tables.contains("db_info"))
		{
			QString ngsd_init_date = getValue("SELECT value FROM db_info WHERE name = 'init_timestamp'", true).toString().trimmed();
			if (!ngsd_init_date.isEmpty() && QFileInfo(":/resources/NGSD_schema.sql").lastModified()<QDateTime::fromString(ngsd_init_date, Qt::ISODate))
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
	getQuery().exec("INSERT INTO db_info SET name='init_timestamp', value='" + QDateTime::currentDateTime().toString(Qt::ISODate) + "'"); //used to speed up tests by clearing tables only instead of dropping them
	getQuery().exec("INSERT INTO db_info SET name='is_production', value='" + QString(test_db_ ? "false" : "true") + "'");

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

void NGSD::setCfdnaRemovedRegions(int id, BedFile removed_regions)
{
	removed_regions.clearHeaders();
	removed_regions.clearAnnotations();

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

VcfFile NGSD::getIdSnpsFromProcessingSystem(int sys_id, const BedFile& target_region, bool tumor_only, bool throw_on_fail)
{
	VcfFile vcf;

	ProcessingSystemData sys = getProcessingSystemData(sys_id);

	// add INFO line to determine source
	InfoFormatLine id_source;
	id_source.id = "ID_Source";
	id_source.number = ".";
	id_source.type = "String";
	id_source.description = "Source of the ID SNPs (e.g. processing system short name or KASP).";
	vcf.vcfHeader().addInfoLine(id_source);

	//prepare info for VCF line
	QByteArrayList info_keys;
	info_keys << "ID_Source";
	QByteArrayList info;
	info.push_back(sys.name_short.toUtf8());

	QByteArrayList format_ids = QByteArrayList() << "GT";
	QByteArrayList sample_ids = QByteArrayList() << "TUMOR";
	if(!tumor_only) sample_ids << "NORMAL";
	QList<QByteArrayList> list_of_format_values;
	list_of_format_values.append(QByteArrayList() << "./.");
	if(!tumor_only) list_of_format_values.append(QByteArrayList() << "./.");

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
			VcfLine vcf_line(line.chr(), line.start(), variant_info.at(0), QList<Sequence>() << variant_info.at(1), format_ids, sample_ids, list_of_format_values);
			vcf_line.setInfo(info_keys, info);
			vcf_line.setId(QByteArrayList() << "ID");
			vcf.append(vcf_line);
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
	q.exec("SELECT n.name, nm.value, n.description, n.qcml_id, n.type FROM processed_sample_qc as nm, qc_terms as n WHERE nm.processed_sample_id='" + processed_sample_id + "' AND nm.qc_terms_id=n.id AND n.obsolete=0");
	QCCollection output;
	while(q.next())
	{
		QString name = q.value(0).toString();
		QString value = q.value(1).toString();
		QString desc = q.value(2).toString();
		QString qcml_id = q.value(3).toString();
		QString type = q.value(4).toString();
		if (type=="int")
		{
			bool ok = false;
			output.insert(QCValue(name, value.toLongLong(&ok), desc, qcml_id));
			if (!ok) THROW(DatabaseException, "Could not convert QC value '" + value + "'  ("+ qcml_id + " - " + name + ") to " + type);
		}
		else if (type=="float")
		{
			bool ok = false;
			output.insert(QCValue(name, value.toDouble(&ok), desc, qcml_id));
			if (!ok) THROW(DatabaseException, "Could not convert QC value '" + value + "'  ("+ qcml_id + " - " + name + ") to " + type);
		}
		else if (type=="string")
		{
			output.insert(QCValue(name, value, desc, qcml_id));
		}
		else THROW(ProgrammingException, "Invalid QC term type '" + type + "' in getQCData!");
	}

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

KaspData NGSD::kaspData(const QString& processed_sample_id)
{
	SqlQuery query = getQuery();
	query.exec("SELECT * FROM kasp_status WHERE processed_sample_id='" + processed_sample_id + "'");

	//no KASP in NGSD
	if (!query.next())
	{
		THROW(DatabaseException, "No KASP result found for " + processedSampleName(processed_sample_id));
	}

	//invalid KASP in NGSD
	bool ok = false;
	double random_error_prob = query.value("random_error_prob").toDouble(&ok);
	if (!ok || random_error_prob<0 || random_error_prob>1)
	{
		THROW(DatabaseException, "Invalid KASP result found for " + processedSampleName(processed_sample_id) + ". Random error probabilty is '" + query.value("random_error_prob").toString() + "'!");
	}

	//create output
	KaspData output;
	output.random_error_prob = random_error_prob;
	output.snps_evaluated = query.value("snps_evaluated").toInt();
	output.snps_match = query.value("snps_match").toInt();
	QVariant tmp = query.value("calculated_date");
	output.calculated_date = tmp.isNull() ? "" : tmp.toDateTime().toString(Qt::ISODate).replace("T", " ");
	tmp = query.value("calculated_by");
	output.calculated_by = tmp.isNull() ? "" : getValue("SELECT name FROM user WHERE id=" + tmp.toString()).toString();

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

QMap<QString, ClassificationInfo> NGSD::getAllClassifications()
{
	QMap<QString, ClassificationInfo> results;

	SqlQuery query = getQuery();
	query.exec("SELECT v.chr, v.start, v.end, v.ref, v.obs, vc.class, vc.comment FROM variant_classification as vc LEFT JOIN variant as v ON v.id=vc.variant_id");

	if (query.size() == 0) return results;

	while(query.next())
	{
		QString key = query.value(0).toString() + ":" + query.value(1).toString() + "-" + query.value(2).toString() + " " + query.value(3).toString() + ">" + query.value(4).toString();
		ClassificationInfo classification;
		classification.classification = query.value(5).toString();
		classification.comments = query.value(6).toString();
		results.insert(key, classification);
	}

	return results;
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
			THROW(DatabaseException, "Cannot find somatic VICC data for variant " + variant.toString(' ', 100, true));
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
		query.bindValue(17 , SomaticVariantInterpreter::viccScoreAsString(vicc_data)); //set classification by interpreter to make sure it fits with the states
		query.bindValue(18 , vicc_data.comment );
		query.bindValue(19 , userId(user_name) );

	};

	int vicc_id = getSomaticViccId(variant);
	if(vicc_id != -1) //update data set
	{
		query.prepare("UPDATE `somatic_vicc_interpretation` SET  `null_mutation_in_tsg`=:0, `known_oncogenic_aa`=:1, `oncogenic_funtional_studies`=:2, `strong_cancerhotspot`=:3, `located_in_canerhotspot`=:4,  `absent_from_controls`=:5, `protein_length_change`=:6, `other_aa_known_oncogenic`=:7, `weak_cancerhotspot`=:8, `computational_evidence`=:9, `mutation_in_gene_with_etiology`=:10, `very_weak_cancerhotspot`=:11, `very_high_maf`=:12, `benign_functional_studies`=:13, `high_maf`=:14, `benign_computational_evidence`=:15, `synonymous_mutation`=:16, `classification`=:17, `comment`=:18, `last_edit_by`=:19, `last_edit_date`= CURRENT_TIMESTAMP WHERE `id`=" + QByteArray::number(vicc_id));
		bind();
		query.exec();
	}
	else //insert new data set
	{
		query.prepare("INSERT INTO `somatic_vicc_interpretation` (`null_mutation_in_tsg`, `known_oncogenic_aa`, `oncogenic_funtional_studies`, `strong_cancerhotspot`, `located_in_canerhotspot`,  `absent_from_controls`, `protein_length_change`, `other_aa_known_oncogenic`, `weak_cancerhotspot`, `computational_evidence`, `mutation_in_gene_with_etiology`, `very_weak_cancerhotspot`, `very_high_maf`, `benign_functional_studies`, `high_maf`, `benign_computational_evidence`, `synonymous_mutation`, `classification`, `comment`, `last_edit_by`, `last_edit_date`, `created_by`, `created_date`, `variant_id`) VALUES (:0, :1, :2, :3, :4, :5, :6, :7, :8, :9, :10, :11, :12, :13, :14, :15, :16, :17, :18, :19, CURRENT_TIMESTAMP, :20, CURRENT_TIMESTAMP, :21)");
		bind();
		query.bindValue(20, userId(user_name) );
		query.bindValue(21, variant_id);
		query.exec();
	}
}


QByteArrayList NGSD::getSomaticPathways()
{
	QByteArrayList output;

	foreach(const QString& name, getValues("SELECT name FROM somatic_pathway sp ORDER BY name ASC"))
	{
		output << name.toUtf8();
	}

	return output;
}

QByteArrayList NGSD::getSomaticPathways(QByteArray gene_symbol)
{
	QByteArrayList output;

	gene_symbol = geneToApproved(gene_symbol, true);
	foreach(const QString& name, getValues("SELECT sp.name FROM somatic_pathway_gene sgp, somatic_pathway sp WHERE sgp.pathway_id=sp.id AND sgp.symbol=:0 ORDER BY sgp.symbol ASC", gene_symbol))
	{
		output << name.toUtf8();
	}

	return output;
}

GeneSet NGSD::getSomaticPathwayGenes(QByteArray pathway_name)
{
	GeneSet output;

	foreach(const QString& gene, getValues("SELECT sgp.symbol FROM somatic_pathway_gene sgp, somatic_pathway sp WHERE sgp.pathway_id=sp.id AND sp.name=:0", pathway_name))
	{
		output << gene.toUtf8();
	}

	return output;
}


int NGSD::getSomaticGeneRoleId(QByteArray gene_symbol)
{
	QVariant id = getValue("SELECT id FROM somatic_gene_role WHERE symbol = '" + geneToApproved(gene_symbol, true) + "'", true);
	return id.isValid() ? id.toInt() : -1;
}

SomaticGeneRole NGSD::getSomaticGeneRole(QByteArray gene, bool throw_on_fail)
{
	int gene_role_id = getSomaticGeneRoleId(gene);
	if(gene_role_id == -1)
	{
		if(throw_on_fail)
		{
			THROW(DatabaseException, "There is no somatic gene role for gene symbol '" + gene + "' in the NGSD.") ;
		}
		else
		{
			return SomaticGeneRole(); //return invalid data
		}
	}

	SqlQuery query = getQuery();
	query.exec("SELECT gene_role, high_evidence, comment FROM somatic_gene_role WHERE somatic_gene_role.id = " + QByteArray::number(gene_role_id));
	query.next();

	SomaticGeneRole out;
	out.gene = gene;

	QString role = query.value(0).toString();
	if(role == "activating") out.role = SomaticGeneRole::Role::ACTIVATING;
	else if(role == "loss_of_function") out.role = SomaticGeneRole::Role::LOSS_OF_FUNCTION;
	else if(role == "ambiguous") out.role = SomaticGeneRole::Role::AMBIGUOUS;
	else THROW(DatabaseException, "Unknown gene role '" + role + "' in relation 'somatic_gene_role'.");

	out.high_evidence = query.value(1).toBool();
	out.comment = query.value(2).toString();

	return out;
}

void NGSD::setSomaticGeneRole(const SomaticGeneRole& gene_role)
{
	QByteArray symbol = geneToApproved(gene_role.gene);

	if(geneId(symbol) == -1)
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


int NGSD::addVariantPublication(QString filename, const Variant& variant, QString database, QString classification, QString details, int user_id)
{
	QString s_id = sampleId(filename);
	QString v_id = variantId(variant);
	if (user_id < 0) user_id = LoginManager::userId();

	//insert
	SqlQuery query = getQuery();
	query.prepare("INSERT INTO variant_publication (sample_id, variant_id, variant_table, db, class, details, user_id) VALUES (:0, :1, :2, :3, :4, :5, :6)");
	query.bindValue(0, s_id);
	query.bindValue(1, v_id);
	query.bindValue(2, "variant");
	query.bindValue(3, database);
	query.bindValue(4, classification);
	query.bindValue(5, details);
	query.bindValue(6, user_id);
	query.exec();

	return query.lastInsertId().toInt();
}

int NGSD::addVariantPublication(QString processed_sample, const CopyNumberVariant& cnv, QString database, QString classification, QString details, int user_id)
{
	QString s_id = sampleId(processed_sample, true);
	QString ps_id = processedSampleId(processed_sample, true);
	QString callset_id = getValue("SELECT id FROM cnv_callset WHERE processed_sample_id=:0", false, ps_id).toString();
	QString cnv_id = cnvId(cnv, callset_id.toInt(), true);
	if (user_id < 0) user_id = LoginManager::userId();

	//insert
	SqlQuery query = getQuery();
	query.prepare("INSERT INTO variant_publication (sample_id, variant_id, variant_table, db, class, details, user_id) VALUES (:0, :1, :2, :3, :4, :5, :6)");
	query.bindValue(0, s_id);
	query.bindValue(1, cnv_id);
	query.bindValue(2, "cnv");
	query.bindValue(3, database);
	query.bindValue(4, classification);
	query.bindValue(5, details);
	query.bindValue(6, user_id);
	query.exec();

	return query.lastInsertId().toInt();
}

int NGSD::addVariantPublication(QString processed_sample, const BedpeLine& sv, const BedpeFile& svs, QString database, QString classification, QString details, int user_id)
{
	QString s_id = sampleId(processed_sample, true);
	QString ps_id = processedSampleId(processed_sample, true);
	QString callset_id = getValue("SELECT id FROM sv_callset WHERE processed_sample_id=:0", false, ps_id).toString();
	QString sv_id = svId(sv, callset_id.toInt(), svs, true);
	if (user_id < 0) user_id = LoginManager::userId();

	//insert
	SqlQuery query = getQuery();
	query.prepare("INSERT INTO variant_publication (sample_id, variant_id, variant_table, db, class, details, user_id) VALUES (:0, :1, :2, :3, :4, :5, :6)");
	query.bindValue(0, s_id);
	query.bindValue(1, sv_id);
	query.bindValue(2, svTableName(sv.type()));
	query.bindValue(3, database);
	query.bindValue(4, classification);
	query.bindValue(5, details);
	query.bindValue(6, user_id);
	query.exec();

	return query.lastInsertId().toInt();
}

int NGSD::addManualVariantPublication(QString sample_name, QString database, QString classification, QString details, int user_id)
{
	QString s_id = sampleId(sample_name);
	int v_id = -1;
	if (user_id < 0) user_id = LoginManager::userId();

	//insert
	SqlQuery query = getQuery();
	query.prepare("INSERT INTO variant_publication (sample_id, variant_id, variant_table, db, class, details, user_id) VALUES (:0, :1, :2, :3, :4, :5, :6)");
	query.bindValue(0, s_id);
	query.bindValue(1, v_id);
	query.bindValue(2, "none");
	query.bindValue(3, database);
	query.bindValue(4, classification);
	query.bindValue(5, details);
	query.bindValue(6, user_id);
	query.exec();

	return query.lastInsertId().toInt();
}

QString NGSD::getVariantPublication(QString filename, const Variant& variant)
{
	QString s_id = sampleId(filename);
	QString v_id = variantId(variant);

	//select
	SqlQuery query = getQuery();
	query.exec("SELECT vp.variant_table, vp.db, vp.class, vp.details, vp.date, vp.result, u.name FROM variant_publication vp LEFT JOIN user u on vp.user_id=u.id WHERE sample_id=" + s_id
			   + " AND variant_table='variant' AND variant_id=" + v_id);

	//create output
	QStringList output;
	while (query.next())
	{
		output <<  "table: " + query.value("variant_table").toString() + " db: " + query.value("db").toString() + " class: " + query.value("class").toString() + " user: " + query.value("name").toString()
				   + " date: " + query.value("date").toString().replace("T", " ") + "\n  " + query.value("details").toString().replace(";", "\n  ").replace("=", ": ")
				   + "\nresult: " + query.value("result").toString().replace(";-", "\n    - ").replace(";", ", ");
	}

	return output.join("\n");
}

QString NGSD::getVariantPublication(QString filename, const CopyNumberVariant& cnv)
{
	QString s_id = sampleId(filename);

	//get all matching cnvs from all callsets
	QStringList cnv_ids;
	QList<int> ps_ids = getValuesInt("SELECT id FROM processed_sample WHERE sample_id=:0", s_id);

	foreach (int ps_id, ps_ids)
	{
		QString callset_id = getValue("SELECT id FROM cnv_callset WHERE processed_sample_id=:0", true, QString::number(ps_id)).toString();
		if (callset_id.isEmpty()) continue;

		QString cnv_id = cnvId(cnv, callset_id.toInt(), false);
		if (cnv_id.isEmpty()) continue;

		cnv_ids.append(cnv_id);
	}

	if (cnv_ids.size() == 0) return "";

	//select
	SqlQuery query = getQuery();
	query.exec("SELECT vp.variant_table, vp.db, vp.class, vp.details, vp.date, vp.result, u.name FROM variant_publication vp LEFT JOIN user u on vp.user_id=u.id WHERE sample_id=" + s_id
			   + " AND variant_table='cnv' AND variant_id IN (" + cnv_ids.join(", ") + ")");

	//create output
	QStringList output;
	while (query.next())
	{
		output <<  "table: " + query.value("variant_table").toString() + " db: " + query.value("db").toString() + " class: " + query.value("class").toString() + " user: " + query.value("name").toString()
				   + " date: " + query.value("date").toString().replace("T", " ") + "\n  " + query.value("details").toString().replace(";", "\n  ").replace("=", ": ")
				   + "\nresult: " + query.value("result").toString().replace(";-", "\n    - ").replace(";", ", ");
	}

	return output.join("\n");
}

QString NGSD::getVariantPublication(QString filename, const BedpeLine& sv, const BedpeFile& svs)
{
	QString s_id = sampleId(filename);

	//get all matching svs from all callsets
	QStringList sv_ids;
	QList<int> ps_ids = getValuesInt("SELECT id FROM processed_sample WHERE sample_id=:0", s_id);

	foreach (int ps_id, ps_ids)
	{
		QString callset_id = getValue("SELECT id FROM sv_callset WHERE processed_sample_id=:0", true, QString::number(ps_id)).toString();
		if (callset_id.isEmpty()) continue;

		QString sv_id = svId(sv, callset_id.toInt(), svs, false);
		if (sv_id.isEmpty()) continue;

		sv_ids.append(sv_id);
	}

	if (sv_ids.size() == 0) return "";

	//select
	SqlQuery query = getQuery();
	query.exec("SELECT vp.variant_table, vp.db, vp.class, vp.details, vp.date, vp.result, u.name FROM variant_publication vp LEFT JOIN user u on vp.user_id=u.id WHERE sample_id=" + s_id
			   + " AND variant_table='" + svTableName(sv.type()) + "' AND variant_id IN (" + sv_ids.join(", ") + ")");

	//create output
	QStringList output;
	while (query.next())
	{
		output <<  "table: " + query.value("variant_table").toString() + " db: " + query.value("db").toString() + " class: " + query.value("class").toString() + " user: " + query.value("name").toString()
				   + " date: " + query.value("date").toString().replace("T", " ") + "\n  " + query.value("details").toString().replace(";", "\n  ").replace("=", ": ")
				   + "\nresult: " + query.value("result").toString().replace(";-", "\n    - ").replace(";", ", ");
	}

	return output.join("\n");
}

void NGSD::updateVariantPublicationResult(int variant_publication_id, QString result)
{
	// check if given id is valid
	if (getValue("SELECT COUNT(id) FROM variant_publication WHERE id=:0", false, QString::number(variant_publication_id)).toInt() != 1)
	{
		THROW(DatabaseException, "Given variant publication id '" + QString::number(variant_publication_id) + "' is not valid!");
	}
	SqlQuery query = getQuery();
	query.prepare("UPDATE variant_publication SET result=:0 WHERE id=:1");
	query.bindValue(0, result);
	query.bindValue(1, variant_publication_id);
	query.exec();
}

void NGSD::flagVariantPublicationAsReplaced(int variant_publication_id)
{
	// check if given id is valid
	if (getValue("SELECT COUNT(id) FROM variant_publication WHERE id=:0", false, QString::number(variant_publication_id)).toInt() != 1)
	{
		THROW(DatabaseException, "Given variant publication id '" + QString::number(variant_publication_id) + "' is not valid!");
	}
	//get linked id
	QString linked_id = getValue("SELECT linked_id FROM variant_publication WHERE linked_id IS NOT NULL AND id=:0", true, QString::number(variant_publication_id)).toString();
	SqlQuery query = getQuery();
	query.prepare("UPDATE variant_publication SET replaced=1 WHERE id=:0");
	query.bindValue(0, variant_publication_id);
	query.exec();
	if (linked_id != "")
	{
		query.bindValue(0, linked_id);
		query.exec();
	}
}

void NGSD::linkVariantPublications(int variant_publication_id1, int variant_publication_id2)
{
	SqlQuery query = getQuery();
	query.prepare("UPDATE variant_publication SET linked_id=:0 WHERE id=:1");
	query.bindValue(0, variant_publication_id1);
	query.bindValue(1, variant_publication_id2);
	query.exec();
	query.bindValue(0, variant_publication_id2);
	query.bindValue(1, variant_publication_id1);
	query.exec();
}

QPair<int, int> NGSD::updateClinvarSubmissionStatus(bool test_run)
{
	SqlQuery query = getQuery();

	query.exec("SELECT id, details, result FROM variant_publication WHERE db='ClinVar'");

	int n_var_checked = 0;
	int n_var_updated = 0;

	while(query.next())
	{
		int vp_id = query.value("id").toInt();
		QString details = query.value("details").toString();
		QString result = query.value("result").toString();

		// skip publications without submission id
		if (!details.contains("submission_id=SUB")) continue;

		// skip publications which are already processed
		if (result.startsWith("processed")) continue;
		if (result.startsWith("error")) continue;

		//extract submission id
		QString submission_id;
		QString stable_id;

		//special handling of deletions
		if(result.startsWith("deleted;"))
		{
			bool skip = false;
			foreach (QString info, result.split(";"))
			{
				if(info.startsWith("SUB")) submission_id = info.trimmed();
				if(info.startsWith("SCV")) stable_id = info.trimmed();
				if(info.trimmed() == "processed") skip  = true;

			}
			if (submission_id.isEmpty())
			{
				THROW(ArgumentException, "'result' column doesn't contain submission id!")
			}

			//skip already deleted
			if(skip) continue;

			ClinvarSubmissionStatus submission_status = getSubmissionStatus(submission_id, test_run);
			n_var_checked++;
			if (submission_status.status != result.split(";").at(2)) n_var_updated++;
			result = QStringList{"deleted", stable_id, submission_id, submission_status.status, submission_status.comment}.join(";");
			updateVariantPublicationResult(vp_id, result);

		}
		else
		{
			foreach (const QString& key_value_pair, details.split(';'))
			{
				if (key_value_pair.startsWith("submission_id=SUB"))
				{
					submission_id = key_value_pair.split("=").at(1).trimmed();
					break;
				}
			}
			if (submission_id.isEmpty())
			{
				THROW(ArgumentException, "'details' column doesn't contain submission id!")
			}

			ClinvarSubmissionStatus submission_status = getSubmissionStatus(submission_id, test_run);
			n_var_checked++;

			//update db if neccessary
			if (!result.startsWith(submission_status.status))
			{
				//update NGSD
				result = submission_status.status;

				if (submission_status.status == "processed")
				{
					result += ";" + submission_status.stable_id;
				}
				else if (submission_status.status == "error")
				{
					result += ";" + submission_status.comment;
				}

				//update result info in the NGSD
				updateVariantPublicationResult(vp_id, result);
				n_var_updated++;
			}
		}
	}

	return QPair<int,int>(n_var_checked, n_var_updated);
}

ClinvarSubmissionStatus NGSD::getSubmissionStatus(const QString& submission_id, bool test_run)
{
	//switch on/off testing
	if(test_run) qDebug() << "Test run enabled!";
	const QString api_url = (test_run)? "https://submit.ncbi.nlm.nih.gov/apitest/v1/submissions/" : "https://submit.ncbi.nlm.nih.gov/api/v1/submissions/";


	// read API key
	QByteArray api_key = Settings::string("clinvar_api_key").trimmed().toUtf8();
	if (api_key.isEmpty()) THROW(FileParseException, "Settings INI file does not contain ClinVar API key!");

	ClinvarSubmissionStatus submission_status;
	HttpRequestHandler request_handler(ProxyDataService::getProxy());

	try
	{

		//add headers
		HttpHeaders add_headers;
		add_headers.insert("Content-Type", "application/json");
		add_headers.insert("SP-API-KEY", api_key);

		//get request
        QByteArray reply = request_handler.get(api_url + submission_id.toUpper() + "/actions/", add_headers).body;
		qDebug() << api_url + submission_id.toUpper() + "/actions/";
		// parse response
		QJsonObject response = QJsonDocument::fromJson(reply).object();

		//extract status
		QJsonArray actions = response.value("actions").toArray();
		submission_status.status = actions.at(0).toObject().value("status").toString();

		if (submission_status.status == "processed" || submission_status.status == "error")
		{
			//get summary file and extract stable id or error message
			QString report_summary_file = actions.at(0).toObject().value("responses").toArray().at(0).toObject().value("files").toArray().at(0).toObject().value("url").toString();
            QByteArray summary_reply = request_handler.get(report_summary_file).body;
			QJsonDocument summary_response = QJsonDocument::fromJson(summary_reply);

			if (submission_status.status == "processed")
			{
				// get stable id
				submission_status.stable_id = summary_response.object().value("submissions").toArray().at(0).toObject().value("identifiers").toObject().value("clinvarAccession").toString();
			}
			if (submission_status.status == "error")
			{
				// get error message
				QJsonArray errors = summary_response.object().value("submissions").toArray().at(0).toObject().value("errors").toArray();
				QStringList error_messages;
				foreach (const QJsonValue& error, errors)
				{
					error_messages << error.toObject().value("output").toObject().value("errors").toArray().at(0).toObject().value("userMessage").toString();
				}
				submission_status.comment = error_messages.join("\n");
			}
		}

		return submission_status;



	}
	catch(Exception e)
	{
		THROW(ArgumentException, "Status check failed for submission " + submission_id + " (" + e.message() + ")!");
		return ClinvarSubmissionStatus();
	}
}


QString NGSD::comment(const Variant& variant)
{
	return getValue("SELECT comment FROM variant WHERE id='" + variantId(variant) + "'").toString();
}

int NGSD::lastAnalysisOf(const QString& processed_sample_id)
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

FileInfo NGSD::analysisJobLatestLogInfo(int job_id)
{
	FileInfo output;
	QString folder = analysisJobFolder(job_id);
	if (QFile::exists(folder))
	{
		QStringList files = Helper::findFiles(folder, "*.log", false);
		if (!files.isEmpty())
		{
			foreach(QString file, files)
			{
				QFileInfo file_info(file);
				QDateTime mod_time = file_info.lastModified();
				if (output.last_modiefied.isNull() || mod_time>output.last_modiefied)
				{
					output.file_name = file_info.fileName();
					output.file_name_with_path = file_info.filePath();
					output.created = file_info.created();
					output.last_modiefied = mod_time;
				}
			}
		}
	}
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

void NGSD::addAnalysisHistoryEntry(int job_id, QString status, QByteArrayList output)
{
	//check status
	QStringList status_valid = getEnum("analysis_job_history", "status");
	if (!status_valid.contains(status)) THROW(ProgrammingException, "Invalid analysis job history status '" + status + "!");

	//add status
	SqlQuery query = getQuery();
	query.prepare("INSERT INTO analysis_job_history (analysis_job_id, status, output) VALUES (:0, :1, :2)");
	query.bindValue(0, job_id);
	query.bindValue(1, status);
	QString output_str = output.join("\n");
	if (output_str.length()>65000)
	{
		output_str = (output_str.left(32500) + "\n\n ... (truncated) ...\n\n" + output_str.right(32500));
	}
	query.bindValue(2, output_str);
	query.exec();
}

int NGSD::addGap(int ps_id, const Chromosome& chr, int start, int end, const QString& status)
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

int NGSD::gapId(int ps_id, const Chromosome& chr, int start, int end)
{
	QVariant id =  getValue("SELECT id FROM gaps WHERE processed_sample_id='" + QString::number(ps_id) + "' AND chr='" + chr.strNormalized(true) + "' AND start='" + QString::number(start) + "' AND end='" + QString::number(end) + "'", true);
	if (id.isValid()) return id.toInt();

	return -1;
}

void NGSD::updateGapStatus(int id, const QString& status)
{
	QString id_str = QString::number(id);

	//check gap exists
	if(!rowExists("gaps", id))
	{
		THROW(ArgumentException, "Gap with ID '" + id_str + "' does not exist!");
	}

	//only update if necessary
	QString status_old = getValue("SELECT status FROM gaps WHERE id='" + id_str + "'").toString();
	if (status==status_old) return;

	//prepare history string
	QString history = getValue("SELECT history FROM gaps WHERE id='" + id_str + "'").toString().trimmed();
	if (!history.isEmpty()) history += "\n";
	history += QDateTime::currentDateTime().toString("dd.MM.yyyy hh:mm:ss") + " - " + status +" (" + LoginManager::userName() + ")";

	//update
	SqlQuery query = getQuery();
	query.exec("UPDATE gaps SET status='"+status+"', history='" + history + "' WHERE id='" + id_str + "'");
}

void NGSD::addGapComment(int id, const QString& comment)
{
	QString id_str = QString::number(id);

	//check gap exists
	if(!rowExists("gaps", id))
	{
		THROW(ArgumentException, "Gap with ID '" + id_str + "' does not exist!");
	}

	//prepare history string
	QString history = getValue("SELECT history FROM gaps WHERE id='" + id_str + "'").toString().trimmed();
	if (!history.isEmpty()) history += "\n";
	history += QDateTime::currentDateTime().toString("dd.MM.yyyy hh:mm:ss") + " - comment (" + LoginManager::userName() + "): " + comment;

	//update (binding becaues it's user input)
	SqlQuery query = getQuery();
	query.prepare("UPDATE gaps SET history=:0 WHERE id='" + id_str + "'");
	query.bindValue(0, history);
	query.exec();
}

VariantCallingInfo NGSD::variantCallingInfo(QString ps_id)
{
	VariantCallingInfo output;

	SqlQuery query = getQuery();

	//small variants
	query.exec("SELECT caller, caller_version, call_date FROM small_variants_callset WHERE processed_sample_id="+ps_id);
	if (query.next())
	{
		output.small_caller = query.value(0).toString().trimmed();
		output.small_caller_version = query.value(1).toString().trimmed();
		output.small_call_date = (query.value(2).isNull() ? "" : query.value(2).toDate().toString(Qt::ISODate));
	}

	//CNVs
	query.exec("SELECT caller, caller_version, call_date FROM cnv_callset WHERE processed_sample_id="+ps_id);
	if (query.next())
	{
		output.cnv_caller = query.value(0).toString().trimmed();
		output.cnv_caller_version = query.value(1).toString().trimmed();
		output.cnv_call_date = (query.value(2).isNull() ? "" : query.value(2).toDate().toString(Qt::ISODate));
	}

	//SVs
	query.exec("SELECT caller, caller_version, call_date FROM sv_callset WHERE processed_sample_id="+ps_id);
	if (query.next())
	{
		output.sv_caller = query.value(0).toString().trimmed();
		output.sv_caller_version = query.value(1).toString().trimmed();
		output.sv_call_date = (query.value(2).isNull() ? "" : query.value(2).toDate().toString(Qt::ISODate));
	}

	//REs
	query.exec("SELECT caller, caller_version, call_date FROM re_callset WHERE processed_sample_id="+ps_id);
	if (query.next())
	{
		output.re_caller = query.value(0).toString().trimmed();
		output.re_caller_version = query.value(1).toString().trimmed();
		output.re_call_date = (query.value(2).isNull() ? "" : query.value(2).toDate().toString(Qt::ISODate));
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
		double metric_numeric = metric_string.toDouble(&ok);
		if (ok && BasicStatistics::isValidFloat(metric_numeric)) output << metric_numeric;
	}

	return output;
}

QString NGSD::getTargetFilePath()
{
	if (ClientHelper::isRunningOnServer())
	{
		return PipelineSettings::dataFolder() + QDir::separator() + "enrichment" + QDir::separator();
	}

	return Settings::string("data_folder") + QDir::separator() + "enrichment" + QDir::separator();
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
		if (debug) qDebug() << "  Last Error:" << query.lastError();
		if (debug) qDebug() << "  ID:" << query.lastInsertId();
	}
	commit();

	if (debug)
	{
		qDebug() << "Terms for NGS: " << c_terms_ngs;
		qDebug() << "Terms with valid types ("+valid_types.join(", ")+"): " << c_terms_valid_type;
	}
}

QHash<QString, QStringList> NGSD::checkMetaData(const QString& ps_id, const VariantList& variants, const CnvList& cnvs, const BedpeFile& svs, const RepeatLocusList& res)
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
		QSharedPointer<ReportConfiguration> report_config = reportConfig(rc_id, variants, cnvs, svs, res);
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
					else if (var_conf.variant_type==VariantType::RES)
					{
						//nothing to check
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

		if(report_config->otherCausalVariant().isValid())
		{
			causal_diagnostic_variant_present = true;
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
	int s_id_int = s_id.toInt();
	QSet<int> related_sample_ids = relatedSamples(s_id_int, "parent-child");
	related_sample_ids.unite(relatedSamples(s_id_int, "siblings"));
	related_sample_ids.unite(relatedSamples(s_id_int, "twins"));
	related_sample_ids.unite(relatedSamples(s_id_int, "twins (monozygotic)"));
	related_sample_ids.unite(relatedSamples(s_id_int, "cousins"));
	foreach(int related_sample_id, related_sample_ids)
	{
		//sample data
		SampleData sample_data = getSampleData(QString::number(related_sample_id));
		if (sample_data.disease_group=="n/a") output[sample_data.name] << "disease group unset!";
		if (sample_data.disease_status=="n/a") output[sample_data.name] << "disease status unset!";

		//HPO terms
		if (sample_data.disease_status=="Affected")
		{
			QList<SampleDiseaseInfo> disease_info = getSampleDiseaseInfo(QString::number(related_sample_id), "HPO term id");
			if (disease_info.isEmpty()) output[sample_data.name] << "no HPO phenotype(s) set! ";
		}
	}

	return output;
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

QString NGSD::createSampleSheet(int run_id, QStringList& warnings)
{
	QStringList sample_sheet;

	QString sw_version = Settings::string("nova_seq_x_sw_version");
	QString app_version = Settings::string("nova_seq_x_app_version");
	QString keep_fastq = Settings::boolean("nova_seq_x_keep_fastq")?"true":"false";
	QString fastq_compression_format = "dragen"; //can be "gzip" or "dragen"
	int barcode_mismatch_index1 = 1;
	int barcode_mismatch_index2 = 1;

	//get info from db
	SqlQuery query = getQuery();
	query.exec("SELECT r.*, d.name d_name, d.type d_type FROM sequencing_run r, device d WHERE r.device_id=d.id AND r.id='" + QString::number(run_id) + "'");
	query.next();
	QString run_name = query.value("name").toString();
	QStringList recipe = query.value("recipe").toString().split("+");
	if (recipe.size() != 4)
	{
		THROW(ArgumentException, "Invalid recipe '" + query.value("recipe").toString() + "' provided! It has to contain 4 read lengths (forward, index1, index2, reverse), divided by '+'.");
	}
	int forward_read_length = Helper::toInt(recipe.at(0), "forward read");
	int index1_read_length = Helper::toInt(recipe.at(1), "index1");
	int index2_read_length = Helper::toInt(recipe.at(2), "index2");
	int reverse_read_length = Helper::toInt(recipe.at(3), "reverse read");
	QString flowcell_type = query.value("flowcell_type").toString();


	//create header
	sample_sheet.append("[Header],");
	sample_sheet.append("FileFormatVersion,2");
	if (run_name.startsWith("#")) run_name.remove(0,1);
	sample_sheet.append("RunName," + run_name);
	sample_sheet.append("InstrumentPlatform,NovaSeqXSeries");
	sample_sheet.append("InstrumentType," + query.value("d_type").toString());
	sample_sheet.append("IndexOrientation,Forward");
	sample_sheet.append("");

	//create read info
	sample_sheet.append("[Reads]");
	sample_sheet.append("Read1Cycles," + QString::number(forward_read_length));
	sample_sheet.append("Read2Cycles," + QString::number(reverse_read_length));
	sample_sheet.append("Index1Cycles,"+ QString::number(index1_read_length));
	sample_sheet.append("Index2Cycles,"+ QString::number(index2_read_length));
	sample_sheet.append("");

	//get sample info
	QSet<QString> adapter_sequences_read1;
	QSet<QString> adapter_sequences_read2;
	QSet<int> used_lanes; //helper var to track if all lanes are used
	QStringList bcl_convert;
	QStringList germline_analysis;
	QStringList enrichment_analysis;
	QStringList rna_analysis;
	bool mid1_chopped = false;
	bool mid2_chopped = false;

	query.exec("SELECT ps.id, ps.lane, CONCAT(s.name,'_',LPAD(ps.process_id,2,'0')) as ps_name, s.tumor, s.sample_type, (SELECT sequence FROM mid WHERE id=ps.mid1_i7) as mid1,"
			   " (SELECT sequence FROM mid WHERE id=ps.mid2_i5) as mid2, (SELECT name_short FROM processing_system WHERE id=ps.processing_system_id) as system_name,"
			   " (SELECT type FROM processing_system WHERE id=ps.processing_system_id) as system_type, (SELECT name FROM project WHERE id=ps.project_id) as project "
			   " FROM processed_sample ps, sample s WHERE ps.sample_id=s.id AND ps.sequencing_run_id='" + QString::number(run_id) + "' ORDER BY ps.lane ASC, ps.id");
	while (query.next())
	{
		QStringList lanes = query.value("lane").toString().split(",");
		QString ps_name = query.value("ps_name").toString();
		Sequence mid1 = Sequence(query.value("mid1").toByteArray()).trimmed();
		Sequence mid2 = Sequence(query.value("mid2").toByteArray()).trimmed();
		QString sample_type = query.value("sample_type").toString();
		QByteArray system_type = query.value("system_type").toByteArray();
		QByteArray system_name = query.value("system_name").toByteArray();

		//cut MIDs to fit recipe
		if (mid1.length() > index1_read_length)
		{
			mid1 = mid1.chopped(index1_read_length);
			mid1_chopped = true;
		}
		if (mid2.length() > index2_read_length)
		{
			mid2 = mid2.chopped(index2_read_length);
			mid2_chopped = true;
		}

		//get adapter sequence
		ProcessingSystemData sys_info = getProcessingSystemData(processingSystemId(system_name));
		if (!sys_info.adapter1_p5.trimmed().isEmpty()) adapter_sequences_read1.insert(sys_info.adapter1_p5);
		if (!sys_info.adapter2_p7.trimmed().isEmpty()) adapter_sequences_read2.insert(sys_info.adapter2_p7);


		if (sample_type == "DNA" || sample_type == "cfDNA")
		{
			if (system_type == "WGS")
			{
				germline_analysis.append(ps_name);
			}
			else if (system_type == "WES")
			{
				enrichment_analysis.append(ps_name + "," + system_name + ".bed");
			}
		}
		else if (sample_type == "RNA")
		{
			rna_analysis.append(ps_name);
		}
		else
		{
			THROW(ArgumentException, "Invalid sample type '" + sample_type + "'!");
		}

		//create line for BCLConvert
		foreach (const QString& lane, lanes)
		{
			int umi_length = 0;
			QStringList line;
			line.append(lane);
			line.append(ps_name);
			line.append(mid1);
			line.append(mid2);

			//track lane
			used_lanes.insert(Helper::toInt(lane, "Sequencing lane", ps_name));

			QString override_cycles;
			// forward read
			override_cycles = "Y" + QString::number(forward_read_length) + ";";
			// index1
			override_cycles += "I" + QString::number(mid1.length());
			if(sys_info.umi_type == "IDT-UDI-UMI")
			{
				//add UMIs:
				override_cycles += "U11";
				umi_length = 11;
			}
			else if ((sys_info.umi_type == "IDT-xGen-Prism") || (sys_info.umi_type == "Twist"))
			{
				qDebug() << "UMI processing will be done in megSAP";
			}
			else if (sys_info.umi_type != "n/a")
			{
				THROW(NotImplementedException, "Unsupported UMI type '" + sys_info.umi_type + "!");
			}
			if (index1_read_length - (mid1.length() + umi_length) < 0) THROW(ArgumentException, "Index1 (+ UMI) read longer than seqeuncing length!")
			if (index1_read_length - (mid1.length() + umi_length) > 0) override_cycles += "N" + QString::number(index1_read_length - mid1.length());
			override_cycles += ";";
			//index2
			if (index2_read_length - mid2.length() < 0) THROW(ArgumentException, "Index2 read longer than seqeuncing length!")
			if (index2_read_length - mid2.length() > 0) override_cycles += "N" + QString::number(index2_read_length - mid2.length());
			override_cycles += "I" + QString::number(mid2.length()) + ";";
			// reverse read
			override_cycles += "Y" + QString::number(reverse_read_length);
			line.append(override_cycles);

			line.append(QString::number(barcode_mismatch_index1));
			line.append(QString::number(barcode_mismatch_index2));

			bcl_convert.append(line.join(","));
		}
	}

	// check if all lanes are used
	if ((flowcell_type == "Illumina NovaSeqX 25B") || (flowcell_type == "Illumina NovaSeqX 10B"))
	{
		if (used_lanes.size() != 8) warnings << "WARNING: The number of lanes covered by samples (" + QString::number(used_lanes.size()) + ") and the number of lanes on the flow cell (8) does not match!";
	}
	else if(used_lanes.size() != 2) //"Illumina NovaSeqX 1.5B"
	{
		warnings << "WARNING: The number of lanes covered by samples (" + QString::number(used_lanes.size()) + ") and the number of lanes on the flow cell (2) does not match!";
	}

	//BCLConvert
	sample_sheet.append("[BCLConvert_Settings]");
	sample_sheet.append("SoftwareVersion,"  + sw_version);

	//sort adapter to make it testable
	QStringList adapter_sequences_read1_list = adapter_sequences_read1.toList();
	adapter_sequences_read1_list.sort();
	if (adapter_sequences_read1_list.length() > 0) sample_sheet.append("AdapterRead1," + adapter_sequences_read1_list.join("+"));
	else warnings << "WARNING: No adapter for read 1 provided! Adapter trimming will not work.";
	QStringList adapter_sequences_read2_list = adapter_sequences_read2.toList();
	adapter_sequences_read2_list.sort();
	if (adapter_sequences_read2_list.length() > 0) sample_sheet.append("AdapterRead2," + adapter_sequences_read2_list.join("+"));
	else warnings << "WARNING: No adapter for read 2 provided! Adapter trimming will not work.";

	sample_sheet.append("FastqCompressionFormat," +fastq_compression_format);
	sample_sheet.append("");
	sample_sheet.append("[BCLConvert_Data]");
	sample_sheet.append("Lane,Sample_ID,Index,Index2,OverrideCycles,BarcodeMismatchesIndex1,BarcodeMismatchesIndex2");
	sample_sheet.append(bcl_convert);
	sample_sheet.append("");

	// add warning if MIDs are chopped
	if (mid1_chopped) warnings << "WARNING: At least one Sample has a i7 MID which is longer than recipe. It will be shorted according to recipe.";
	if (mid2_chopped) warnings << "WARNING: At least one Sample has a i5 MID which is longer than recipe. It will be shorted according to recipe.";


	//DRAGEN Germline
	if (germline_analysis.size() > 0)
	{
		sample_sheet.append("[DragenGermline_Settings]");
		sample_sheet.append("SoftwareVersion," + sw_version);
		sample_sheet.append("AppVersion," + app_version);
		sample_sheet.append("KeepFastq," + keep_fastq);
		sample_sheet.append("MapAlignOutFormat,cram");
		sample_sheet.append("ReferenceGenomeDir,GRCh38");
		sample_sheet.append("VariantCallingMode,AllVariantCallers");
		sample_sheet.append("");
		sample_sheet.append("[DragenGermline_Data]");
		sample_sheet.append("Sample_ID");
		sample_sheet.append(germline_analysis);
		sample_sheet.append("");
	}

	//DRAGEN Enrichment
	if (enrichment_analysis.size() > 0)
	{
		sample_sheet.append("[DragenEnrichment_Settings]");
		sample_sheet.append("SoftwareVersion," + sw_version);
		sample_sheet.append("AppVersion," + app_version);
		sample_sheet.append("KeepFastq," + keep_fastq);
		sample_sheet.append("MapAlignOutFormat,cram");
		sample_sheet.append("ReferenceGenomeDir,GRCh38");
		sample_sheet.append("GermlineOrSomatic,germline");
		sample_sheet.append("VariantCallingMode,AllVariantCallers");
		sample_sheet.append("");
		sample_sheet.append("[DragenEnrichment_Data]");
		sample_sheet.append("Sample_ID,BedFile");
		sample_sheet.append(enrichment_analysis);
		sample_sheet.append("");
	}
//disabled until further testing
/*
	//DRAGEN RNA
	if (rna_analysis.size() > 0)
	{
		sample_sheet.append("[DragenRNA_Settings]");
		sample_sheet.append("SoftwareVersion," + sw_version);
		sample_sheet.append("AppVersion," + app_version);
		sample_sheet.append("KeepFastq," + keep_fastq);
		sample_sheet.append("MapAlignOutFormat,bam");
		sample_sheet.append("GermlineOrSomatic,germline");
		sample_sheet.append("VariantCallingMode,AllVariantCallers");
		sample_sheet.append("");
		sample_sheet.append("[DragenRNA_Data]");
		sample_sheet.append("Sample_ID");
		sample_sheet.append(rna_analysis);
		sample_sheet.append("");
	}
*/

	return sample_sheet.join("\n");
}

QStringList NGSD::studies(const QString& processed_sample_id)
{
	QStringList output = getValues("SELECT s.name FROM study s, study_sample ss WHERE s.id=ss.study_id AND ss.processed_sample_id=" + processed_sample_id);

	output.sort();

	return output;
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
		if (type.startsWith("enum("))
		{
			type = type.mid(6,type.length()-8);
			cache[hash] = type.split("','");
		}
		else if (type.startsWith("set("))
		{
			type = type.mid(5,type.length()-7);
			cache[hash] = type.split("','");
		}
		else
		{
			THROW(ProgrammingException, "Could not determine enum values of column '"+column+"' in table '"+table+"'! Column type doesn't start with 'enum' or 'set'. Type: " + type);
		}
		return cache[hash];
	}

	THROW(ProgrammingException, "Could not determine enum values of column '"+column+"' in table '"+table+"'!");
}


bool NGSD::tableExists(QString table, bool throw_error_if_not_existing) const
{
	SqlQuery query = getQuery();
	query.exec("SHOW TABLES LIKE '" + table + "'");
	if (query.size()==0)
	{
		if (throw_error_if_not_existing) THROW(DatabaseException, "Table '" + table + "' does not exist!");

		return false;
	}

	return true;
}

bool NGSD::rowExists(QString table, int id) const
{
	if (!tableInfo(table).fieldExists("id"))
	{
		THROW(DatabaseException, "Table '" + table + "' has no column 'id'!");
	}

	SqlQuery query = getQuery();
	query.exec("SELECT id FROM " + table  + " WHERE id=" + QString::number(id));
	return query.size()==1;
}

bool NGSD::tableEmpty(QString table) const
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
		Log::warn("transactions are not supported by the current driver: (" + db_->driverName() + ")");
	}

	if (!db_->transaction())
	{
		Log::warn("Starting transactions failed: " + db_->lastError().text());
		return false;
	}
	return true;
}

bool NGSD::commit()
{
	if (!db_->commit())
	{
		Log::warn("Committing transactions failed: " + db_->lastError().text());
		return false;
	}
	return true;
}

bool NGSD::rollback()
{
	if (!db_->rollback())
	{
		Log::warn("Transaction rollback failed: " + db_->lastError().text());
		return false;
	}
	return true;
}

int NGSD::geneId(const QByteArray& gene)
{
	QHash<QByteArray, int>& gene2id = getCache().gene2id;

	//check cache first
	int cache_id = gene2id.value(gene, -1);
	if (cache_id!=-1)
	{
		return cache_id;
	}

	//approved
	if (approvedGeneNames().contains(gene))
	{
		int gene_id = getValue("SELECT id FROM gene WHERE symbol='" + gene + "'").toInt();
		gene2id.insert(gene, gene_id);
		return gene_id;
	}

	//previous
	SqlQuery q_prev = getQuery();
	q_prev.prepare("SELECT g.id FROM gene g, gene_alias ga WHERE g.id=ga.gene_id AND ga.symbol=:0 AND ga.type='previous'");
	q_prev.bindValue(0, gene);
	q_prev.exec();
	if (q_prev.size()==1)
	{
		q_prev.next();
		int gene_id = q_prev.value(0).toInt();
		gene2id.insert(gene, gene_id);
		return gene_id;
	}
	else if(q_prev.size()>1)
	{
		gene2id.insert(gene, -1);
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
		int gene_id = q_syn.value(0).toInt();
		gene2id.insert(gene, gene_id);
		return gene_id;
	}

	gene2id.insert(gene, -1);
	return -1;
}

int NGSD::geneIdOfTranscript(const QByteArray& name, bool throw_on_error, GenomeBuild build)
{
	//Ensembl / CCDS
	int trans_id = transcriptId(name, false);
	if (trans_id!=-1)
	{
		return getValue("SELECT gene_id FROM gene_transcript WHERE id=:0", false, QString::number(trans_id)).toInt();
	}

	//RefSeq (via our transcript mapping)
	QByteArray name_nover = name;
	if (name_nover.contains('.')) name_nover = name_nover.left(name_nover.indexOf('.'));
	const QMap<QByteArray, QByteArrayList>& matches = NGSHelper::transcriptMatches(build);
	foreach(QByteArray match, matches.value(name_nover))
	{
		match = match.trimmed();
		if (match.startsWith("ENST"))
		{
			trans_id = transcriptId(match, false);
			if (trans_id!=-1)
			{
				return getValue("SELECT gene_id FROM gene_transcript WHERE id=:0", false, QString::number(trans_id)).toInt();
			}
		}
	}

	//not found
	if (throw_on_error)
	{
		THROW(DatabaseException, "No transcript with name '" + name + "' found in NGSD!");
	}

	return -1; //invalid
}

QByteArray NGSD::geneSymbol(int id)
{
	return getValue("SELECT symbol FROM gene WHERE id=" + QString::number(id), false).toByteArray();
}

QByteArray NGSD::geneHgncId(int id)
{
	return "HGNC:" + getValue("SELECT hgnc_id FROM gene WHERE id=" + QString::number(id), false).toByteArray();
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
		int gene_id = geneId(gene);
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
	Helper::trim(search_terms);
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

GeneSet NGSD::phenotypeToGenesbySourceAndEvidence(int id, QSet<PhenotypeSource> allowed_sources, QSet<PhenotypeEvidenceLevel> allowed_evidences, bool recursive, bool ignore_non_phenotype_terms)
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
		QString query = QString("SELECT gene FROM hpo_genes WHERE hpo_term_id=%1").arg(id);

		if (allowed_sources.size() > 0 && allowed_sources.count() < Phenotype::allSourceValues().count())
		{
			query += " and (";
			foreach (PhenotypeSource s, allowed_sources)
			{
				query += "details like \"%" + Phenotype::sourceToString(s) + "%\" or ";
			}
			query.chop(4);
			query.append(")");
		}

		if (allowed_evidences.size() > 0 && allowed_evidences.count() < Phenotype::allEvidenceValues(false).count())
		{
			query += " and (";

			foreach (PhenotypeEvidenceLevel e, allowed_evidences)
			{
				query += "evidence= \"" + Phenotype::evidenceToString(e) + "\" or ";
			}
			query.chop(4);
			query.append(")");
		}
		//pid2genes.bindValue(0, id);
		pid2genes.exec(query);

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

PhenotypeList NGSD::phenotypeParentTerms(int term_id, bool recursive)
{
	PhenotypeList output;

	//prepare queries
	SqlQuery pid2paranets = getQuery();
	pid2paranets.prepare("SELECT parent FROM hpo_parent WHERE child=:0");

	//convert term ids to genes
	QList<int> term_ids;
	term_ids << term_id;
	while (!term_ids.isEmpty())
	{
		int id = term_ids.takeLast();

		pid2paranets.bindValue(0, id);
		pid2paranets.exec();
		while(pid2paranets.next())
		{
			int id_child = pid2paranets.value(0).toInt();
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
	foreach(const QString& omim_gene_id, omim_gene_ids)
	{
		OmimInfo info;
		info.mim = getValue("SELECT mim FROM omim_gene WHERE id=" + omim_gene_id).toByteArray();
		info.gene_symbol = getValue("SELECT gene FROM omim_gene WHERE id=" + omim_gene_id).toByteArray();

		QRegExp mim_exp("[^0-9]([0-9]{6})[^0-9]");
		QStringList phenos = getValues("SELECT phenotype FROM omim_phenotype WHERE omim_gene_id=" + omim_gene_id + " ORDER BY phenotype ASC");
		foreach(const QString& pheno, phenos)
		{
			Phenotype tmp;

			tmp.setName(pheno.toUtf8());
			if (mim_exp.indexIn(pheno)!=-1)
			{
				tmp.setAccession(mim_exp.cap(1).toUtf8());
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

QString NGSD::sampleName(const QString& s_id, bool throw_if_fails)
{
	SqlQuery query = getQuery();
	query.prepare("SELECT name FROM sample WHERE id=:0");
	query.bindValue(0, s_id);
	query.exec();
	if (query.size()==0)
	{
		if(throw_if_fails)
		{
			THROW(DatabaseException, "Sample with ID '" + s_id + "' not found in NGSD!");
		}
		else
		{
			return "";
		}
	}
	query.next();
	return query.value(0).toString();
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
	TranscriptList& cache = getCache().gene_transcripts;
	if (cache.isEmpty()) initTranscriptCache();
	ChromosomalIndex<TranscriptList>& index = getCache().gene_transcripts_index;

	//create gene list
	GeneSet genes;
	QVector<int> matches = index.matchingIndices(chr, start-extend, end+extend);
	foreach(int i, matches)
	{
		genes << cache[i].gene();
	}
	return genes;
}

GeneSet NGSD::genesOverlappingByExon(const Chromosome& chr, int start, int end, int extend)
{
	TranscriptList& cache = getCache().gene_transcripts;
	if (cache.isEmpty()) initTranscriptCache();
	ChromosomalIndex<TranscriptList>& index = getCache().gene_transcripts_index;

	start -= extend;
	end += extend;

	GeneSet output;
	QVector<int> indices = index.matchingIndices(chr, start, end);
	foreach(int index, indices)
	{
		const Transcript& trans = cache[index];
		if (output.contains(trans.gene())) continue;

		for (int i=0; i<trans.regions().count();  ++i)
		{
			const BedLine& line = trans.regions()[i];
			if (line.overlapsWith(chr, start, end))
			{
				output << trans.gene();
				break;
			}
		}
	}

	return output;
}

BedFile NGSD::geneToRegions(const QByteArray& gene, Transcript::SOURCE source, QString mode, bool fallback, bool annotate_transcript_names, QTextStream* messages)
{
	//check mode
	QStringList valid_modes;
	valid_modes << "gene" << "exon";
	if (!valid_modes.contains(mode))
	{
		THROW(ArgumentException, "Invalid mode '" + mode + "'. Valid modes are: " + valid_modes.join(", ") + ".");
	}

	//process input data
	BedFile output;

	//get approved gene id
	int id = geneId(gene);
	if (id==-1)
	{
		if (messages) *messages << "Gene name '" << gene << "' is no HGNC-approved symbol. Skipping it!" << endl;
		return output;
	}

	//prepare annotations
	QByteArrayList annos;
	annos << geneSymbol(id);

	QList<Transcript::SOURCE> sources;
	sources << source;
	if (fallback) sources << (source==Transcript::ENSEMBL ? Transcript::CCDS : Transcript::ENSEMBL);
	foreach(Transcript::SOURCE current_source, sources)
	{
		TranscriptList transcript_list = transcripts(id, current_source, false);
		foreach(const Transcript& trans, transcript_list)
		{
			if (annotate_transcript_names)
			{
				annos.clear();
				annos << trans.gene() + " " + trans.nameWithVersion();
			}

			if (mode=="gene")
			{
				output.append(BedLine(trans.chr(), trans.start(), trans.end(), annos));
			}
			else
			{
				const BedFile& regions = trans.isCoding() ? trans.codingRegions() : trans.regions();
				for(int i=0; i<regions.count(); ++i)
				{
					const BedLine& line = regions[i];
					output.append(BedLine(line.chr(), line.start(), line.end(), annos));
				}
			}
		}

		//no fallback in case we found the gene in the primary source database
		if (current_source==source && !output.isEmpty()) break;
	}


	if (output.isEmpty() && messages!=nullptr)
	{
		*messages << "No transcripts found for gene '" + gene + "'. Skipping it!" << endl;
	}

	if (!output.isSorted()) output.sort();
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

	if (!output.isSorted()) output.sort();
	if (!annotate_transcript_names) output.removeDuplicates();

	return output;
}

BedFile NGSD::transcriptToRegions(const QByteArray& name, QString mode)
{
	//check mode
	QStringList valid_modes;
	valid_modes << "gene" << "exon";
	if (!valid_modes.contains(mode))
	{
		THROW(ArgumentException, "Invalid mode '" + mode + "'. Valid modes are: " + valid_modes.join(", ") + ".");
	}

	//get transcript id
	int id = transcriptId(name, false);
	if (id==-1)
	{
		THROW(ArgumentException, "Transcript '" + name + "' not found in NGSD.");
	}

	//get transcript
	const Transcript& trans = transcript(id);

	//prepare annotations
	QByteArrayList annos;
	annos << (trans.gene() + " " + trans.nameWithVersion());

	//create output
	BedFile output;
	if (mode=="gene")
	{
		output.append(BedLine(trans.chr(), trans.start(), trans.end(), annos));
	}
	else
	{
		const BedFile& regions = trans.isCoding() ? trans.codingRegions() : trans.regions();
		for(int i=0; i<regions.count(); ++i)
		{
			const BedLine& line = regions[i];
			output.append(BedLine(line.chr(), line.start(), line.end(), annos));
		}
	}

	if (!output.isSorted()) output.sort();

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

TranscriptList NGSD::transcripts(int gene_id, Transcript::SOURCE source, bool coding_only)
{
	TranscriptList& cache = getCache().gene_transcripts;
	if (cache.isEmpty()) initTranscriptCache();
	QHash<QByteArray, QSet<int>>& gene2indices = getCache().gene_transcripts_symbol2indices;

	TranscriptList output;

	QByteArray gene = geneSymbol(gene_id);
	foreach(int index, gene2indices[gene])
	{
		const Transcript& trans = cache[index];
		if (trans.source()!=source) continue;
		if (coding_only && !trans.isCoding()) continue;

		output << trans;
	}

	output.sortByPosition();

	return output;
}

TranscriptList NGSD::transcriptsOverlapping(const Chromosome& chr, int start, int end, int extend, Transcript::SOURCE source)
{
	TranscriptList& cache = getCache().gene_transcripts;
	if (cache.isEmpty()) initTranscriptCache();
	ChromosomalIndex<TranscriptList>& index = getCache().gene_transcripts_index;

	//create gene list
	TranscriptList output;
	QVector<int> matches = index.matchingIndices(chr, start-extend, end+extend);
	foreach(int i, matches)
	{
		if (cache[i].source()==source)
		{
			output << cache[i];
		}
	}
	return output;
}

Transcript NGSD::bestTranscript(int gene_id, const QList<VariantTranscript> var_transcripts, int *return_quality)
{
	TranscriptList list = transcripts(gene_id, Transcript::ENSEMBL, false);

	//preferred
	list.sortByCodingBases();
	TranscriptList list_lvl;

	foreach(const Transcript& t, list)
	{

		if (t.isPreferredTranscript()) list_lvl.append(t);
	}

	if (list_lvl.count() > 0)
	{
		if (return_quality != nullptr) *return_quality = 5;
		return highestImpactTranscript(list_lvl, var_transcripts);
	}


	//MANE select
	foreach(const Transcript& t, list)
	{
		if (t.isManeSelectTranscript()) list_lvl.append(t);
	}

	if (list_lvl.count() > 0)
	{
		if (return_quality != nullptr) *return_quality = 4;
		return highestImpactTranscript(list_lvl, var_transcripts);
	}

	//MANE plus clinical
	//not necessary because each gene with MANE plus clinical also has a MANE select transcript

	//Ensembl canonical
	foreach(const Transcript& t, list)
	{

		if (t.isEnsemblCanonicalTranscript()) list_lvl.append(t);
	}

	if (list_lvl.count() > 0)
	{
		if (return_quality != nullptr) *return_quality = 3;
		return highestImpactTranscript(list_lvl, var_transcripts);
	}

	//longest coding
	foreach(const Transcript& t, list)
	{
		if (t.isCoding()) list_lvl.append(t);
	}

	if (list_lvl.count() > 0)
	{
		if (return_quality != nullptr) *return_quality = 2;
		return highestImpactTranscript(list_lvl, var_transcripts);
	}

	//longest
	list.sortByBases();
	foreach(const Transcript& t, list)
	{
		if (return_quality != nullptr) *return_quality = 1;
		return t;
	}

	if (return_quality != nullptr) *return_quality = -1;
	return Transcript();
}

Transcript NGSD::highestImpactTranscript(TranscriptList transcripts, const QList<VariantTranscript> var_transcripts)
{
	if (transcripts.count() == 0) return Transcript();

	if (var_transcripts.isEmpty() || transcripts.count() == 1)
	{
		return transcripts[0];
	}
	else
	{
		VariantImpact current_impact = VariantImpact::MODIFIER;
		Transcript current_transcript;

		foreach (VariantTranscript var_t, var_transcripts)
		{
			if (transcripts.contains(var_t.idWithoutVersion()) && (! current_transcript.isValid() || lowerImpactThan(current_impact, var_t.impact)))
			{
				current_impact = var_t.impact;
				current_transcript = transcripts.getTranscript(var_t.idWithoutVersion());
			}
		}

		if (current_transcript.isValid())
		{
			return current_transcript;
		}
		else
		{
			return transcripts[0];
		}
	}
}

TranscriptList NGSD::relevantTranscripts(int gene_id)
{
	TranscriptList output;

	Transcript best_trans = bestTranscript(gene_id);
	if (best_trans.isValid()) output << best_trans;

	foreach(const Transcript& t, transcripts(gene_id, Transcript::ENSEMBL, false))
	{
		if (t.isPreferredTranscript() || t.isManeSelectTranscript() || t.isManePlusClinicalTranscript() || t.isEnsemblCanonicalTranscript())
		{
			if (!output.contains(t)) output << t;
		}
	}

	return output;
}

const TranscriptList& NGSD::transcripts()
{
	TranscriptList& cache = getCache().gene_transcripts;
	if (cache.isEmpty()) initTranscriptCache();

	return cache;
}

const Transcript& NGSD::transcript(int id)
{
	TranscriptList& cache = getCache().gene_transcripts;
	if (cache.isEmpty()) initTranscriptCache();

	//check transcript is in cache, i.e. in NGSD
	int index = getCache().gene_transcripts_id2index.value(id, -1);
	if (index==-1) THROW(DatabaseException, "Could not find transcript with identifer '" + QString::number(id) + "' in NGSD!");

	return cache[index];
}

Transcript NGSD::longestCodingTranscript(int gene_id, Transcript::SOURCE source, bool fallback_alt_source, bool fallback_noncoding)
{
	TranscriptList list = transcripts(gene_id, source, true);

	//fallback (non-coding)
	if (list.isEmpty() && fallback_noncoding)
	{
		list = transcripts(gene_id, source, false);
	}

	//fallback alternative source
	Transcript::SOURCE alt_source = (source==Transcript::CCDS) ? Transcript::ENSEMBL : Transcript::CCDS;
	if (list.isEmpty() && fallback_alt_source)
	{
		list = transcripts(gene_id, alt_source, true);
	}
	if (list.isEmpty() && fallback_alt_source && fallback_noncoding)
	{
		list = transcripts(gene_id, alt_source, false);
	}

	if (list.isEmpty()) return Transcript();

	//get longest transcript
	list.sortByCodingBases();
	return list.first();
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

QString NGSD::reportConfigSummaryText(const QString& processed_sample_id, bool add_users)
{
	QString output;

	int rc_id = reportConfigId(processed_sample_id);
	if (rc_id!=-1)
	{
		output = "exists";

		//find causal small variants
		{
			SqlQuery query = getQuery();
			query.exec("SELECT * FROM report_configuration_variant WHERE causal='1' AND report_configuration_id=" + QString::number(rc_id));
			while(query.next())
			{
				QString var_id = query.value("variant_id").toString();
				Variant var = variant(var_id);
				QString genotype = getValue("SELECT genotype FROM detected_variant WHERE processed_sample_id='" + processed_sample_id + "' AND variant_id='" + var_id + "'").toString();

				//manual curation
				QString manual_var = query.value("manual_var").toString().trimmed();
				if (!manual_var.isEmpty())
				{
					var = Variant::fromString(manual_var);
				}
				QString manual_genotype = query.value("manual_genotype").toString().trimmed();
				if (!manual_genotype.isEmpty())
				{
					genotype = query.value("manual_genotype").toString().trimmed();
				}

				QString genes = genesOverlapping(var.chr(), var.start(), var.end(), 5000).join(", ");

				QString var_class = getValue("SELECT class FROM variant_classification WHERE variant_id='" + var_id + "'").toString();

				//get output
				output += ", causal variant: " + var.toString() + " (genotype:" + genotype + " genes:" + genes;
				if (var_class != "") output += " classification:" + var_class; // add classification, if exists
				output += ")";
			}
		}

		//find causal CNVs
		{
			SqlQuery query = getQuery();
			query.exec("SELECT *, (manual_cn IS NULL) as manual_cn_is_null FROM report_configuration_cnv WHERE causal='1' AND report_configuration_id=" + QString::number(rc_id));
			while(query.next())
			{
				QString cnv_id = query.value("cnv_id").toString();
				CopyNumberVariant var = cnv(cnv_id.toInt());
				QString cn = getValue("SELECT cn FROM cnv WHERE id='" + cnv_id + "'").toString();

				//manual curation
				QVariant manual_start = query.value("manual_start");
				if (!manual_start.isNull()) var.setStart(manual_start.toInt());
				QVariant manual_end = query.value("manual_end");
				if (!manual_end.isNull()) var.setEnd(manual_end.toInt());
				if (!query.value("manual_cn_is_null").toBool() && query.value("manual_cn").toInt()>=0)
				{
					cn = query.value("manual_cn").toString();
				}

				QString cnv_class = query.value("class").toString();
				output += ", causal CNV: " + var.toString() + " (cn:" + cn;
				if (cnv_class != "") output += " classification:" + cnv_class; // add classification, if exists
				output += ")";
			}
		}

		//find causal SVs
		{
			SqlQuery query = getQuery();
			query.exec("SELECT * FROM report_configuration_sv WHERE causal='1' AND report_configuration_id=" + QString::number(rc_id));
			while(query.next())
			{
				//determine type and ID in type-specific table of SV
				StructuralVariantType type = StructuralVariantType::UNKNOWN;
				int sv_id = -1;
				if (query.value("sv_deletion_id").toInt()!=0)
				{
					type = StructuralVariantType::DEL;
					sv_id = query.value("sv_deletion_id").toInt();
				}
				if (query.value("sv_duplication_id").toInt()!=0)
				{
					type = StructuralVariantType::DUP;
					sv_id = query.value("sv_duplication_id").toInt();
				}
				if (query.value("sv_insertion_id").toInt()!=0)
				{
					type = StructuralVariantType::INS;
					sv_id = query.value("sv_insertion_id").toInt();
				}
				if (query.value("sv_inversion_id").toInt()!=0)
				{
					type = StructuralVariantType::INV;
					sv_id = query.value("sv_inversion_id").toInt();
				}
				if (query.value("sv_translocation_id").toInt()!=0)
				{
					type = StructuralVariantType::BND;
					sv_id = query.value("sv_translocation_id").toInt();
				}

				//get variant and genotype
				BedpeFile svs;
				svs.setAnnotationHeaders(QList<QByteArray>() << "FORMAT" << processedSampleName(processed_sample_id).toUtf8()); //FROMAT column that will contain the genotype
				BedpeLine var = structuralVariant(sv_id, type, svs, true);
				QString genotype = var.annotations()[1];
				if (genotype=="1/1") genotype = "hom";
				if (genotype=="0/1") genotype = "het";

				//manual curation
				QVariant manual_start = query.value("manual_start");
				if (!manual_start.isNull())
				{
					int coord = manual_start.toInt();
					if (type==StructuralVariantType::BND)
					{
						var.setStart1(coord);
					}
					else
					{
						var.setStart1(coord);
						var.setEnd1(coord);
					}
				}
				QVariant manual_end = query.value("manual_end");
				if (!manual_end.isNull())
				{
					int coord = manual_end.toInt();
					if (type==StructuralVariantType::BND)
					{
						var.setEnd1(coord);
					}
					else
					{
						var.setStart2(coord);
						var.setEnd2(coord);
					}
				}
				QVariant manual_genotype = query.value("manual_genotype");
				if (!manual_genotype.isNull())
				{
					genotype = manual_genotype.toString();
				}
				if (type==StructuralVariantType::BND)
				{
					QVariant manual_start2 = query.value("manual_start_bnd");
					if (!manual_start2.isNull())
					{
						var.setStart2(manual_start2.toInt());
					}
					QVariant manual_end2 = query.value("manual_end_bnd");
					if (!manual_end2.isNull())
					{
						var.setEnd2(manual_end2.toInt());
					}
				}

				//create output
				output += ", causal SV: " + var.toString() + " (genotype: " + genotype;
				QString sv_class = query.value("class").toString().trimmed();
				if (sv_class!="n/a") output += ", classification:" + sv_class;
				output += ")";
			}
		}

		//find causal REs
		{
			SqlQuery query = getQuery();
			query.exec("SELECT re.name, reg.allele1, reg.allele2, rcr.manual_allele1, rcr.manual_allele2 FROM report_configuration_re rcr, repeat_expansion_genotype reg, repeat_expansion re WHERE re.id=reg.repeat_expansion_id AND reg.id=rcr.repeat_expansion_genotype_id AND rcr.causal='1' AND rcr.report_configuration_id=" + QString::number(rc_id));
			while(query.next())
			{
				//gene lengths
				QString a1 = query.value("allele1").toString();
				QString a2 = query.value("allele2").toString();

				//manual curation
				QVariant a1_manual = query.value("manual_allele1");
				if (!a1_manual.isNull()) a1 = a1_manual.toString();
				QVariant a2_manual = query.value("manual_allele2");
				if (!a2_manual.isNull()) a2 = a2_manual.toString();

				output += ", causal RE: " + query.value("name").toString() + " (length:" + a1;
				if (!a2.isEmpty()) output += "/" + a2;
				output += ")";
			}
		}

		//find other causal variants
		SqlQuery query = getQuery();
		query.exec("SELECT * FROM report_configuration_other_causal_variant WHERE report_configuration_id=" + QString::number(rc_id));
		if(query.next())
		{
			output += ", causal " + query.value("type").toString() + ": " + query.value("coordinates").toString() + " (genes: " + query.value("gene").toString() + ")";
		}

		//users
		if (add_users)
		{
			QStringList user_output;
			QString user = getValue("SELECT u.name FROM user u, report_configuration rc WHERE rc.created_by=u.id AND rc.id="+QString::number(rc_id), true).toString();
			if (!user.isEmpty())
			{
				user_output << ("created by: " + user);
			}
			QString user2 = getValue("SELECT u.name FROM user u, report_configuration rc WHERE rc.last_edit_by=u.id AND rc.id="+QString::number(rc_id), true).toString();
			if (!user2.isEmpty() && user!=user2)
			{
				user_output << ("last edited by: " + user2);
			}
			if (!user_output.isEmpty())
			{
				output += " [" + user_output.join(", ") + "]";
			}
		}
	}

	return output;
}

bool NGSD::reportConfigIsFinalized(int id)
{
	return getValue("SELECT id FROM `report_configuration` WHERE `id`=" + QString::number(id) + " AND finalized_by IS NOT NULL").isValid();
}

QSharedPointer<ReportConfiguration> NGSD::reportConfig(int conf_id, const VariantList& variants, const CnvList& cnvs, const BedpeFile& svs, const RepeatLocusList& res)
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

		//skip variants that are not found, e.g. when trios and single sample analysis are used alternatingly
		if (var_conf.variant_index==-1) continue;

		var_conf.id = query.value("id").toInt();
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
		var_conf.rna_info = query.value("rna_info").toString();
		var_conf.manual_var = query.value("manual_var").toString();
		var_conf.manual_genotype = query.value("manual_genotype").toString();

		output->set(var_conf);
	}

	//load CNV data
	query.exec("SELECT *, (manual_cn IS NULL) as manual_cn_is_null FROM report_configuration_cnv WHERE report_configuration_id=" + QString::number(conf_id));
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

		//skip variants that are not found, e.g. when trios and single sample analysis are used alternatingly
		if (var_conf.variant_index==-1) continue;

		var_conf.id = query.value("id").toInt();
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
		var_conf.rna_info = query.value("rna_info").toString();
		if (query.value("manual_start").toInt()>0)
		{
			var_conf.manual_cnv_start = query.value("manual_start").toString();
		}
		if (query.value("manual_end").toInt()>0)
		{
			var_conf.manual_cnv_end = query.value("manual_end").toString();
		}
		if (!query.value("manual_cn_is_null").toBool() && query.value("manual_cn").toInt()>=0) //special handling for int that is NULL (not correctly handled by Qt)
		{
			var_conf.manual_cnv_cn = query.value("manual_cn").toString();
		}
		var_conf.manual_cnv_hgvs_type = query.value("manual_hgvs_type").toString();
		var_conf.manual_cnv_hgvs_suffix = query.value("manual_hgvs_suffix").toString();

		output->set(var_conf);
	}

	// Skip report import if empty sv file is provided (Trio)
	if (!svs.isEmpty())
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

			//skip variants that are not found, e.g. when trios and single sample analysis are used alternatingly
			var_conf.variant_index = svs.findMatch(sv, true, false);
			if (var_conf.variant_index==-1) continue;

			var_conf.id = query.value("id").toInt();
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
			var_conf.rna_info = query.value("rna_info").toString();
			if (query.value("manual_start").toInt()>0)
			{
				var_conf.manual_sv_start = query.value("manual_start").toString();
			}
			if (query.value("manual_end").toInt()>0)
			{
				var_conf.manual_sv_end = query.value("manual_end").toString();
			}
			var_conf.manual_sv_genotype = query.value("manual_genotype").toString();
			var_conf.manual_sv_hgvs_type = query.value("manual_hgvs_type").toString();
			var_conf.manual_sv_hgvs_suffix = query.value("manual_hgvs_suffix").toString();
			if (query.value("manual_start_bnd").toInt()>0)
			{
				var_conf.manual_sv_start_bnd = query.value("manual_start_bnd").toString();
			}
			if (query.value("manual_end_bnd").toInt()>0)
			{
				var_conf.manual_sv_end_bnd = query.value("manual_end_bnd").toString();
			}
			var_conf.manual_sv_hgvs_type_bnd = query.value("manual_hgvs_type_bnd").toString();
			var_conf.manual_sv_hgvs_suffix_bnd = query.value("manual_hgvs_suffix_bnd").toString();

			output->set(var_conf);
		}

	}

	// Skip report import if empty sv file is provided (Trio)
	if (!res.isEmpty())
	{
		//load RE data
		query.exec("SELECT * FROM report_configuration_re WHERE report_configuration_id=" + QString::number(conf_id));
		while(query.next())
		{
			ReportVariantConfiguration var_conf;
			var_conf.variant_type = VariantType::RES;

			//get CNV id
			RepeatLocus re = repeatExpansionGenotype(query.value("repeat_expansion_genotype_id").toInt());
			for (int i=0; i<res.count(); ++i)
			{
				if (res[i].sameRegionAndLocus(re))
				{
					var_conf.variant_index = i;
				}
			}

			//skip variants that are not found, e.g. when trios and single sample analysis are used alternatingly
			if (var_conf.variant_index==-1) continue;

			var_conf.id = query.value("id").toInt();
			var_conf.report_type = query.value("type").toString();
			var_conf.causal = query.value("causal").toBool();
			var_conf.inheritance = query.value("inheritance").toString();
			var_conf.de_novo = query.value("de_novo").toBool();
			var_conf.mosaic = query.value("mosaic").toBool();
			var_conf.comp_het = query.value("compound_heterozygous").toBool();
			var_conf.exclude_artefact = query.value("exclude_artefact").toBool();
			var_conf.exclude_phenotype = query.value("exclude_phenotype").toBool();
			var_conf.exclude_other = query.value("exclude_other").toBool();
			var_conf.comments = query.value("comments").toString();
			var_conf.comments2 = query.value("comments2").toString();
			if (!query.value("manual_allele1").isNull())
			{
				var_conf.manual_re_allele1 = query.value("manual_allele1").toString();
			}
			if (!query.value("manual_allele2").isNull())
			{
				var_conf.manual_re_allele2 = query.value("manual_allele2").toString();
			}

			output->set(var_conf);
		}
	}

	//load other causal variant
	query.exec("SELECT * FROM report_configuration_other_causal_variant WHERE report_configuration_id=" + QString::number(conf_id));
	if(query.next())
	{
		OtherCausalVariant causal_variant;
		causal_variant.id = query.value("id").toInt();
		causal_variant.coordinates = query.value("coordinates").toString();
		causal_variant.gene = query.value("gene").toString();
		causal_variant.type = query.value("type").toString();
		causal_variant.inheritance = query.value("inheritance").toString();
		causal_variant.comment = query.value("comment").toString();
		causal_variant.comment_reviewer1 = query.value("comment_reviewer1").toString();
		causal_variant.comment_reviewer2 = query.value("comment_reviewer2").toString();
		output->setOtherCausalVariant(causal_variant);
	}
	return output;
}

int NGSD::setReportConfig(const QString& processed_sample_id, QSharedPointer<ReportConfiguration> config, const VariantList& variants, const CnvList& cnvs, const BedpeFile& svs, const RepeatLocusList& res)
{
	int report_config_id = reportConfigId(processed_sample_id);
	QString report_config_id_str = QString::number(report_config_id);

	//check that it is not finalized
	if (report_config_id!=-1 && reportConfigIsFinalized(report_config_id))
	{
		WARNING(ProgrammingException, "Cannot update report configuration with id=" + report_config_id_str + " because it is finalized!");
	}

	try
	{
		transaction();

		if (report_config_id!=-1) //clear old report config
		{
			//delete report config variants if it already exists
			SqlQuery query = getQuery();
			query.exec("DELETE FROM `report_configuration_other_causal_variant` WHERE report_configuration_id=" + report_config_id_str);

			//update report config
			query.exec("UPDATE `report_configuration` SET `last_edit_by`='" + LoginManager::userIdAsString() + "', `last_edit_date`=CURRENT_TIMESTAMP WHERE id=" + report_config_id_str);
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
			report_config_id = query.lastInsertId().toInt();
		}

		//store variant data
		SqlQuery query_new_var = getQuery();
		query_new_var.prepare("INSERT INTO `report_configuration_variant`(`report_configuration_id`, `variant_id`, `type`, `causal`, `inheritance`, `de_novo`, `mosaic`, `compound_heterozygous`, `exclude_artefact`, `exclude_frequency`, `exclude_phenotype`, `exclude_mechanism`, `exclude_other`, `comments`, `comments2`, `rna_info`, `manual_var`, `manual_genotype`) VALUES (:0, :1, :2, :3, :4, :5, :6, :7, :8, :9, :10, :11, :12, :13, :14, :15, :16, :17)");
		SqlQuery query_update_var = getQuery();
		query_update_var.prepare("UPDATE `report_configuration_variant` SET `report_configuration_id`=:0, `variant_id`=:1, `type`=:2, `causal`=:3, `inheritance`=:4, `de_novo`=:5, `mosaic`=:6, `compound_heterozygous`=:7, `exclude_artefact`=:8, `exclude_frequency`=:9, `exclude_phenotype`=:10, `exclude_mechanism`=:11, `exclude_other`=:12, `comments`=:13, `comments2`=:14, `rna_info`=:15, `manual_var`=:16, `manual_genotype`=:17 WHERE `id`=:18");
		SqlQuery query_new_cnv = getQuery();
		query_new_cnv.prepare("INSERT INTO `report_configuration_cnv`(`report_configuration_id`, `cnv_id`, `type`, `causal`, `class`, `inheritance`, `de_novo`, `mosaic`, `compound_heterozygous`, `exclude_artefact`, `exclude_frequency`, `exclude_phenotype`, `exclude_mechanism`, `exclude_other`, `comments`, `comments2`, `rna_info`, `manual_start`, `manual_end`, `manual_cn`, `manual_hgvs_type`, `manual_hgvs_suffix`) VALUES (:0, :1, :2, :3, :4, :5, :6, :7, :8, :9, :10, :11, :12, :13, :14, :15, :16, :17, :18, :19, :20, :21)");
		SqlQuery query_update_cnv = getQuery();
		query_update_cnv.prepare("UPDATE `report_configuration_cnv` SET `report_configuration_id`=:0, `cnv_id`=:1, `type`=:2, `causal`=:3, `class`=:4, `inheritance`=:5, `de_novo`=:6, `mosaic`=:7, `compound_heterozygous`=:8, `exclude_artefact`=:9, `exclude_frequency`=:10, `exclude_phenotype`=:11, `exclude_mechanism`=:12, `exclude_other`=:13, `comments`=:14, `comments2`=:15, `rna_info`=:16, `manual_start`=:17, `manual_end`=:18, `manual_cn`=:19, `manual_hgvs_type`=:20, `manual_hgvs_suffix`=:21 WHERE `id`=:22");
		SqlQuery query_new_sv = getQuery();
		query_new_sv.prepare("INSERT INTO `report_configuration_sv`(`report_configuration_id`, `sv_deletion_id`, `sv_duplication_id`, `sv_insertion_id`, `sv_inversion_id`, `sv_translocation_id`, `type`, `causal`, `class`, `inheritance`, `de_novo`, `mosaic`, `compound_heterozygous`, `exclude_artefact`, `exclude_frequency`, `exclude_phenotype`, `exclude_mechanism`, `exclude_other`, `comments`, `comments2`, `rna_info`, `manual_start`, `manual_end`, `manual_genotype`, `manual_start_bnd`, `manual_end_bnd`, `manual_hgvs_type`, `manual_hgvs_suffix`, `manual_hgvs_type_bnd`, `manual_hgvs_suffix_bnd`) VALUES (:0, :1, :2, :3, :4, :5, :6, :7, :8, :9, :10, :11, :12, :13, :14, :15, :16, :17, :18, :19, :20, :21, :22, :23, :24, :25, :26, :27, :28, :29)");
		SqlQuery query_update_sv = getQuery();
		query_update_sv.prepare("UPDATE `report_configuration_sv` SET `report_configuration_id`=:0, `sv_deletion_id`=:1, `sv_duplication_id`=:2, `sv_insertion_id`=:3, `sv_inversion_id`=:4, `sv_translocation_id`=:5, `type`=:6, `causal`=:7, `class`=:8, `inheritance`=:9, `de_novo`=:10, `mosaic`=:11, `compound_heterozygous`=:12, `exclude_artefact`=:13, `exclude_frequency`=:14, `exclude_phenotype`=:15, `exclude_mechanism`=:16, `exclude_other`=:17, `comments`=:18, `comments2`=:19, `rna_info`=:20, `manual_start`=:21, `manual_end`=:22, `manual_genotype`=:23, `manual_start_bnd`=:24, `manual_end_bnd`=:25, `manual_hgvs_type`=:26, `manual_hgvs_suffix`=:27 , `manual_hgvs_type_bnd`=:28, `manual_hgvs_suffix_bnd`=:29 WHERE `id`=:30");
		SqlQuery query_new_re = getQuery();
		query_new_re.prepare("INSERT INTO `report_configuration_re`(`report_configuration_id`, `repeat_expansion_genotype_id`, `type`, `causal`, `inheritance`, `de_novo`, `mosaic`, `compound_heterozygous`, `exclude_artefact`, `exclude_phenotype`, `exclude_other`, `comments`, `comments2`, `manual_allele1`, `manual_allele2`) VALUES (:0, :1, :2, :3, :4, :5, :6, :7, :8, :9, :10, :11, :12, :13, :14)");
		SqlQuery query_update_re = getQuery();
		query_update_re.prepare("UPDATE `report_configuration_re` SET `report_configuration_id`=:0, `repeat_expansion_genotype_id`=:1, `type`=:2, `causal`=:3, `inheritance`=:4, `de_novo`=:5, `mosaic`=:6, `compound_heterozygous`=:7, `exclude_artefact`=:8, `exclude_phenotype`=:9, `exclude_other`=:10, `comments`=:11, `comments2`=:12, `manual_allele1`=:13, `manual_allele2`=:14 WHERE `id`=:15");
		SqlQuery query = getQuery();

		QList<ReportVariantConfiguration> rvc_to_update;

		foreach(ReportVariantConfiguration var_conf, config->variantConfig())
		{
			// remove variant from report config
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

				//check if update or new import
				if (var_conf.id < 0)
				{
					//check if variant-report config combination is already imported
					QString var_conf_id_str = getValue("SELECT id FROM `report_configuration_variant` WHERE `report_configuration_id`=" + QString::number(report_config_id)
												   + " AND `variant_id`=:0", true, variant_id).toString();
					if (!var_conf_id_str.isEmpty())
					{
						// report-variant combination is already imported -> update
						var_conf.id = var_conf_id_str.toInt();
						rvc_to_update.append(var_conf);
						query = query_update_var;
					}
					else
					{
						//actual new variant
						query = query_new_var;
					}
				}
				else
				{
					//update
					query = query_update_var;
				}

				query.bindValue(0, report_config_id);
				query.bindValue(1, variant_id);
				query.bindValue(2, var_conf.report_type);
				query.bindValue(3, var_conf.causal);
				query.bindValue(4, var_conf.inheritance);
				query.bindValue(5, var_conf.de_novo);
				query.bindValue(6, var_conf.mosaic);
				query.bindValue(7, var_conf.comp_het);
				query.bindValue(8, var_conf.exclude_artefact);
				query.bindValue(9, var_conf.exclude_frequency);
				query.bindValue(10, var_conf.exclude_phenotype);
				query.bindValue(11, var_conf.exclude_mechanism);
				query.bindValue(12, var_conf.exclude_other);
				query.bindValue(13, var_conf.comments.isEmpty() ? "" : var_conf.comments);
				query.bindValue(14, var_conf.comments2.isEmpty() ? "" : var_conf.comments2);
				query.bindValue(15, var_conf.rna_info.isEmpty() ? "n/a" : var_conf.rna_info);
				query.bindValue(16, var_conf.manual_var);
				query.bindValue(17, var_conf.manualVarGenoIsValid() ? var_conf.manual_genotype : QVariant());

				if (var_conf.id < 0)
				{
					//new variant
					query.exec();
					var_conf.id = query.lastInsertId().toInt();
					rvc_to_update.append(var_conf);
				}
				else
				{
					//update
					query.bindValue(18, var_conf.id);
					query.exec();
				}

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

				//check if update or new import
				if (var_conf.id < 0)
				{
					//check if cnv-report config combination is already imported
					QString var_conf_id_str = getValue("SELECT id FROM `report_configuration_cnv` WHERE `report_configuration_id`=" + QString::number(report_config_id)
												   + " AND `cnv_id`=:0", true, cnv_id).toString();
					if (!var_conf_id_str.isEmpty())
					{
						// report-cnv combination is already imported -> update
						var_conf.id = var_conf_id_str.toInt();
						rvc_to_update.append(var_conf);
						query = query_update_cnv;
					}
					else
					{
						//actual new variant
						query = query_new_cnv;
					}
				}
				else
				{
					//update
					query = query_update_cnv;
				}

				query.bindValue(0, report_config_id);
				query.bindValue(1, cnv_id);
				query.bindValue(2, var_conf.report_type);
				query.bindValue(3, var_conf.causal);
				query.bindValue(4, var_conf.classification); //only for CNVs
				query.bindValue(5, var_conf.inheritance);
				query.bindValue(6, var_conf.de_novo);
				query.bindValue(7, var_conf.mosaic);
				query.bindValue(8, var_conf.comp_het);
				query.bindValue(9, var_conf.exclude_artefact);
				query.bindValue(10, var_conf.exclude_frequency);
				query.bindValue(11, var_conf.exclude_phenotype);
				query.bindValue(12, var_conf.exclude_mechanism);
				query.bindValue(13, var_conf.exclude_other);
				query.bindValue(14, var_conf.comments.isEmpty() ? "" : var_conf.comments);
				query.bindValue(15, var_conf.comments2.isEmpty() ? "" : var_conf.comments2);
				query.bindValue(16, var_conf.rna_info.isEmpty() ? "n/a" : var_conf.rna_info);
				query.bindValue(17, var_conf.manualCnvStartIsValid() ? var_conf.manual_cnv_start : QVariant());
				query.bindValue(18, var_conf.manualCnvEndIsValid() ? var_conf.manual_cnv_end : QVariant());
				query.bindValue(19, var_conf.manualCnvCnIsValid() ? var_conf.manual_cnv_cn : QVariant());
				query.bindValue(20, var_conf.manual_cnv_hgvs_type);
				query.bindValue(21, var_conf.manual_cnv_hgvs_suffix);

				if (var_conf.id < 0)
				{
					//new variant
					query.exec();
					var_conf.id = query.lastInsertId().toInt();
					rvc_to_update.append(var_conf);
				}
				else
				{
					//update
					query.bindValue(22, var_conf.id);
					query.exec();
				}

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

				//check if update or new import
				if (var_conf.id < 0)
				{
					//check if sv-report config combination is already imported
					QString var_conf_id_str = getValue("SELECT id FROM `report_configuration_sv` WHERE `report_configuration_id`=" + QString::number(report_config_id)
												   + " AND `" + svTableName(sv.type()) + "_id`=:0", true, sv_id).toString();
					if (!var_conf_id_str.isEmpty())
					{
						// report-sv combination is already imported -> update
						var_conf.id = var_conf_id_str.toInt();
						rvc_to_update.append(var_conf);
						query = query_update_sv;
					}
					else
					{
						//actual new variant
						query = query_new_sv;
					}
				}
				else
				{
					//update
					query = query_update_sv;
				}

				//define SQL query
				query.bindValue(0, report_config_id);
				query.bindValue(1, QVariant(QVariant::String));
				query.bindValue(2, QVariant(QVariant::String));
				query.bindValue(3, QVariant(QVariant::String));
				query.bindValue(4, QVariant(QVariant::String));
				query.bindValue(5, QVariant(QVariant::String));
				query.bindValue(6, var_conf.report_type);
				query.bindValue(7, var_conf.causal);
				query.bindValue(8, var_conf.classification);
				query.bindValue(9, var_conf.inheritance);
				query.bindValue(10, var_conf.de_novo);
				query.bindValue(11, var_conf.mosaic);
				query.bindValue(12, var_conf.comp_het);
				query.bindValue(13, var_conf.exclude_artefact);
				query.bindValue(14, var_conf.exclude_frequency);
				query.bindValue(15, var_conf.exclude_phenotype);
				query.bindValue(16, var_conf.exclude_mechanism);
				query.bindValue(17, var_conf.exclude_other);
				query.bindValue(18, var_conf.comments.isEmpty() ? "" : var_conf.comments);
				query.bindValue(19, var_conf.comments2.isEmpty() ? "" : var_conf.comments2);
				query.bindValue(20, var_conf.rna_info.isEmpty() ? "n/a" : var_conf.rna_info);
				query.bindValue(21, var_conf.manualSvStartIsValid() ? var_conf.manual_sv_start : QVariant());
				query.bindValue(22, var_conf.manualSvEndIsValid() ? var_conf.manual_sv_end : QVariant());
				query.bindValue(23, var_conf.manualSvGenoIsValid() ? var_conf.manual_sv_genotype : QVariant());
				query.bindValue(24, var_conf.manualSvStartBndIsValid() ? var_conf.manual_sv_start_bnd : QVariant());
				query.bindValue(25, var_conf.manualSvEndBndIsValid() ? var_conf.manual_sv_end_bnd : QVariant());
				query.bindValue(26, var_conf.manual_sv_hgvs_type);
				query.bindValue(27, var_conf.manual_sv_hgvs_suffix);
				query.bindValue(28, var_conf.manual_sv_hgvs_type_bnd);
				query.bindValue(29, var_conf.manual_sv_hgvs_suffix_bnd);

				// set SV id
				switch (sv.type())
				{
					case StructuralVariantType::DEL:
						query.bindValue(1, sv_id);
						break;
					case StructuralVariantType::DUP:
						query.bindValue(2, sv_id);
						break;
					case StructuralVariantType::INS:
						query.bindValue(3, sv_id);
						break;
					case StructuralVariantType::INV:
						query.bindValue(4, sv_id);
						break;
					case StructuralVariantType::BND:
						query.bindValue(5, sv_id);
						break;
					default:
						THROW(ArgumentException, "Invalid structural variant type!")
						break;
				}

				if (var_conf.id < 0)
				{
					//new variant
					query.exec();
					var_conf.id = query.lastInsertId().toInt();
					rvc_to_update.append(var_conf);
				}
				else
				{
					//update
					query.bindValue(30, var_conf.id);
					query.exec();
				}

			}
			else if (var_conf.variant_type==VariantType::RES)
			{
				//check RE index exists in RE list
				if (var_conf.variant_index<0 || var_conf.variant_index>=res.count())
				{
					THROW(ProgrammingException, "RE list does not contain RE with index '" + QString::number(var_conf.variant_index) + "' in NGSD::setReportConfig!");
				}

				//get RE id
				const RepeatLocus& re = res[var_conf.variant_index];
				int re_base_id = repeatExpansionId(re.region(), re.unit());
				int re_id = repeatExpansionGenotypeId(re_base_id, processed_sample_id.toInt());

				//check if update or new import
				if (var_conf.id < 0)
				{
					//check if RE-report config combination is already imported
					QString var_conf_id_str = getValue("SELECT id FROM `report_configuration_re` WHERE `report_configuration_id`=" + QString::number(report_config_id) + " AND `repeat_expansion_genotype_id`=:0", true, QString::number(re_id)).toString();
					if (!var_conf_id_str.isEmpty())
					{
						// report-RE combination is already imported -> update
						var_conf.id = var_conf_id_str.toInt();
						rvc_to_update.append(var_conf);
						query = query_update_re;
					}
					else
					{
						//actual new variant
						query = query_new_re;
					}
				}
				else
				{
					//update
					query = query_update_re;
				}

				query.bindValue(0, report_config_id);
				query.bindValue(1, re_id);
				query.bindValue(2, var_conf.report_type);
				query.bindValue(3, var_conf.causal);
				query.bindValue(4, var_conf.inheritance);
				query.bindValue(5, var_conf.de_novo);
				query.bindValue(6, var_conf.mosaic);
				query.bindValue(7, var_conf.comp_het);
				query.bindValue(8, var_conf.exclude_artefact);
				query.bindValue(9, var_conf.exclude_phenotype);
				query.bindValue(10, var_conf.exclude_other);
				query.bindValue(11, var_conf.comments.isEmpty() ? "" : var_conf.comments);
				query.bindValue(12, var_conf.comments2.isEmpty() ? "" : var_conf.comments2);
				query.bindValue(13, var_conf.manualReAllele1IsValid() ? var_conf.manual_re_allele1.toInt() : QVariant(QVariant::Int));
				query.bindValue(14, var_conf.manualReAllele1IsValid() ? var_conf.manual_re_allele2.toInt() : QVariant(QVariant::Int));

				if (var_conf.id < 0)
				{
					//new variant
					query.exec();
					var_conf.id = query.lastInsertId().toInt();
					rvc_to_update.append(var_conf);
				}
				else
				{
					//update
					query.bindValue(15, var_conf.id);
					query.exec();
				}
			}
			else
			{
				THROW(NotImplementedException, "Storing of report config variants with type '" + QString::number((int)var_conf.variant_type) + "' not implemented!");
			}
		}

		//update given report config
		foreach(const ReportVariantConfiguration& var_conf, rvc_to_update)
		{
			config->set(var_conf);
		}

		//delete variants from the database
		foreach(const ReportVariantConfiguration& var_conf, config->variantsToDelete())
		{
			// delete variant
			if (var_conf.id >= 0)
			{
				//delete from NGSD
				SqlQuery query = getQuery();
				if (var_conf.variant_type == VariantType::SNVS_INDELS) query.exec("DELETE FROM `report_configuration_variant` WHERE `id`=" + QString::number(var_conf.id));
				else if (var_conf.variant_type == VariantType::CNVS) query.exec("DELETE FROM `report_configuration_cnv` WHERE `id`=" + QString::number(var_conf.id));
				else if (var_conf.variant_type == VariantType::SVS) query.exec("DELETE FROM `report_configuration_sv` WHERE `id`=" + QString::number(var_conf.id));
				else if (var_conf.variant_type == VariantType::RES) query.exec("DELETE FROM `report_configuration_re` WHERE `id`=" + QString::number(var_conf.id));
				else THROW(NotImplementedException, "Removing of report config variants with type '" + QString::number((int)var_conf.variant_type) + "' not implemented!");
			}
			// else -> variant wasn't pushed into the NGSD -> skip deletion
		}
		// clear to-delete list
		config->clearDeletionList();

		//store other causal variant
		if(config->other_causal_variant_.isValid())
		{
			SqlQuery query = getQuery();
			query.prepare(QString("INSERT INTO `report_configuration_other_causal_variant` (`report_configuration_id`, `coordinates`, `gene`, `type`, `inheritance`, `comment`, `comment_reviewer1`, ")
						  + "`comment_reviewer2`) VALUES (:0, :1, :2, :3, :4, :5, :6, :7) ON DUPLICATE KEY UPDATE id=id");
			query.bindValue(0, report_config_id);
			query.bindValue(1, config->other_causal_variant_.coordinates);
			query.bindValue(2, config->other_causal_variant_.gene);
			query.bindValue(3, config->other_causal_variant_.type);
			query.bindValue(4, config->other_causal_variant_.inheritance);
			query.bindValue(5, config->other_causal_variant_.comment);
			query.bindValue(6, config->other_causal_variant_.comment_reviewer1);
			query.bindValue(7, config->other_causal_variant_.comment_reviewer2);
			query.exec();
		}

		commit();
	}
	catch(...)
	{
		rollback();
		throw;
	}

	return report_config_id;
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
	query.exec("DELETE FROM `report_configuration_re` WHERE `report_configuration_id`=" + rc_id);
	query.exec("DELETE FROM `report_configuration_other_causal_variant` WHERE report_configuration_id=" + rc_id);
	query.exec("DELETE FROM `report_configuration` WHERE `id`=" + rc_id);
}

ReportVariantConfiguration NGSD::reportVariantConfiguration(int id, VariantType type, QStringList& messages, const VariantList& variants, const CnvList& cnvs, const BedpeFile& svs, const RepeatLocusList& res)
{
	ReportVariantConfiguration var_conf;
	SqlQuery query = getQuery();

	if (type == VariantType::SNVS_INDELS)
	{
		query.exec("SELECT * FROM report_configuration_variant WHERE id=" + QString::number(id));
		if (query.size() != 1) THROW(DatabaseException, "Invalid report variant configuration id!");
		query.next();

		var_conf.variant_index=-1;
		if (variants.count() > 0)
		{
			//get variant id
			Variant var = variant(query.value("variant_id").toString());
			for (int i=0; i<variants.count(); ++i)
			{
				if (var==variants[i])
				{
					var_conf.variant_index = i;
				}
			}
			if (var_conf.variant_index==-1) messages << "Could not find variant '" + var.toString() + "' in given variant list.";
		}
		var_conf.variant_type = VariantType::SNVS_INDELS;
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
		var_conf.rna_info = query.value("rna_info").toString();
		var_conf.manual_var = query.value("manual_var").toString();
		var_conf.manual_genotype = query.value("manual_genotype").toString();
	}
	else if (type == VariantType::CNVS)
	{
		query.exec("SELECT *, (manual_cn IS NULL) as manual_cn_is_null FROM report_configuration_cnv WHERE id=" + QString::number(id));
		if (query.size() != 1) THROW(DatabaseException, "Invalid report variant configuration id!");
		query.next();

		var_conf.variant_index=-1;
		if (cnvs.count() > 0)
		{
			//get CNV id
			CopyNumberVariant var = cnv(query.value("cnv_id").toInt());
			for (int i=0; i<cnvs.count(); ++i)
			{
				if (cnvs[i].hasSamePosition(var))
				{
					var_conf.variant_index = i;
				}
			}
			if (var_conf.variant_index==-1) messages << "Could not find CNV '" + var.toString() + "' in given variant list.";
		}
		var_conf.variant_type = VariantType::CNVS;
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
		var_conf.rna_info = query.value("rna_info").toString();
		if (query.value("manual_start").toInt()>0)
		{
			var_conf.manual_cnv_start = query.value("manual_start").toString();
		}
		if (query.value("manual_end").toInt()>0)
		{
			var_conf.manual_cnv_end = query.value("manual_end").toString();
		}
		if (!query.value("manual_cn_is_null").toBool() && query.value("manual_cn").toInt()>=0) //special handling for int that is NULL (not correctly handled by Qt)
		{
			var_conf.manual_cnv_cn = query.value("manual_cn").toString();
		}
	}
	else if (type == VariantType::SVS)
	{
		query.exec("SELECT * FROM report_configuration_sv WHERE id=" + QString::number(id));
		if (query.size() != 1) THROW(DatabaseException, "Invalid report variant configuration id!");
		query.next();

		var_conf.variant_index=-1;
		if (svs.count() > 0)
		{
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
			if (var_conf.variant_index==-1) messages << "Could not find SV '" + BedpeFile::typeToString(sv.type()) + " " + sv.positionRange() + "' in given variant list.";
		}

		var_conf.variant_type = VariantType::SVS;
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
		var_conf.rna_info = query.value("rna_info").toString();
		if (query.value("manual_start").toInt()>0)
		{
			var_conf.manual_sv_start = query.value("manual_start").toString();
		}
		if (query.value("manual_end").toInt()>0)
		{
			var_conf.manual_sv_end = query.value("manual_end").toString();
		}
		var_conf.manual_sv_genotype = query.value("manual_genotype").toString();
		if (query.value("manual_start_bnd").toInt()>0)
		{
			var_conf.manual_sv_start_bnd = query.value("manual_start_bnd").toString();
		}
		if (query.value("manual_end_bnd").toInt()>0)
		{
			var_conf.manual_sv_end_bnd = query.value("manual_end_bnd").toString();
		}

	}
	else if (type == VariantType::RES)
	{
		query.exec("SELECT * FROM report_configuration_re WHERE id=" + QString::number(id));
		if (query.size() != 1) THROW(DatabaseException, "Invalid report variant configuration id!");
		query.next();

		var_conf.variant_index=-1;
		if (res.count() > 0)
		{
			//get variant id
			RepeatLocus re = repeatExpansionGenotype(query.value("repeat_expansion_genotype_id").toInt());
			for (int i=0; i<res.count(); ++i)
			{
				if (re.sameRegionAndLocus(res[i]))
				{
					var_conf.variant_index = i;
				}
			}
			if (var_conf.variant_index==-1) messages << "Could not find repeat locus '" + re.toString(true, false) + "' in given locus list.";
		}
		var_conf.variant_type = VariantType::RES;
		var_conf.report_type = query.value("type").toString();
		var_conf.causal = query.value("causal").toBool();
		var_conf.inheritance = query.value("inheritance").toString();
		var_conf.de_novo = query.value("de_novo").toBool();
		var_conf.mosaic = query.value("mosaic").toBool();
		var_conf.comp_het = query.value("compound_heterozygous").toBool();
		var_conf.exclude_artefact = query.value("exclude_artefact").toBool();
		var_conf.exclude_phenotype = query.value("exclude_phenotype").toBool();
		var_conf.exclude_other = query.value("exclude_other").toBool();
		var_conf.comments = query.value("comments").toString();
		var_conf.comments2 = query.value("comments2").toString();
		var_conf.manual_var = query.value("manual_var").toString();
		var_conf.manual_genotype = query.value("manual_genotype").toString();
	}
	else
	{
		THROW(ArgumentException, "Invalid variant type!")
	}

	return var_conf;
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

QSet<int> NGSD::relatedSamples(int sample_id, const QString& relation, QString sample_type)
{
	// check relation
	if (!getEnum("sample_relations", "relation").contains(relation))
	{
		THROW(ArgumentException, "Invalid relation type '" + relation + "' given!");
	}

	QSet<int> output;

	if (sample_type.isEmpty())
	{
		SqlQuery query = getQuery();
		query.exec("SELECT sample1_id, sample2_id FROM sample_relations WHERE (sample1_id=" + QString::number(sample_id) + " OR sample2_id=" + QString::number(sample_id) + ") AND relation='" + relation + "'");
		while(query.next())
		{
			int id1 = query.value("sample1_id").toInt();
			int id2 = query.value("sample2_id").toInt();

			if (id1==sample_id)
			{
				output << id2;
			}
			else if (id1!=sample_id)
			{
				output << id1;
			}
		}
	}
	else
	{
		//check sample type
		if (!getEnum("sample", "sample_type").contains(sample_type))
		{
			THROW(ArgumentException, "Invalid sample type '" + sample_type + "'!");
		}

		foreach(int id, getValuesInt("SELECT sr.sample2_id FROM sample_relations sr, sample s WHERE sr.sample2_id=s.id AND sr.relation='" + relation + "' AND s.sample_type='" + sample_type + "' AND sr.sample1_id='" + QString::number(sample_id) + "'"))
		{
			output << id;
		}
		foreach(int id, getValuesInt("SELECT sr.sample1_id FROM sample_relations sr, sample s WHERE sr.sample1_id=s.id AND sr.relation='" + relation + "' AND s.sample_type='" + sample_type + "' AND sr.sample2_id='" + QString::number(sample_id) + "'"))
		{
			output << id;
		}
	}

	return output;
}

void NGSD::addSampleRelation(const SampleRelation& rel, int user_id)
{
	SqlQuery query = getQuery();
	query.prepare("INSERT INTO `sample_relations`(`sample1_id`, `relation`, `sample2_id`, `user_id`) VALUES (:0, :1, :2, :3)");
	query.bindValue(0, sampleId(rel.sample1));
	query.bindValue(1, rel.relation);
	query.bindValue(2, sampleId(rel.sample2));
	if (user_id==-1) user_id = LoginManager::userId();
	query.bindValue(3, user_id);
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

int NGSD::setSomaticReportConfig(QString t_ps_id, QString n_ps_id, const SomaticReportConfiguration& config, const VariantList& snvs, const CnvList& cnvs, const BedpeFile& svs, const VariantList& germl_snvs, QString user_name)
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
		query.exec("DELETE FROM `somatic_report_configuration_sv` WHERE somatic_report_configuration_id=" + QByteArray::number(id));

		//Update somatic report configuration: last_edit_by, last_edit_user and target_file
		query.prepare("UPDATE `somatic_report_configuration` SET `last_edit_by`= :0, `last_edit_date` = CURRENT_TIMESTAMP, `target_file`= :1, `tum_content_max_af` =:2, `tum_content_max_clonality` =:3, `tum_content_hist` =:4, `msi_status` =:5, `cnv_burden` =:6, `hrd_statement` =:7, `cnv_loh_count` =:8, `cnv_tai_count` =:9, `cnv_lst_count` =:10, `tmb_ref_text` =:11, `quality` =:12, `fusions_detected`=:13, `cin_chr`=:14, `limitations` = :15, `filter_base_name` =:16, `tum_content_estimated` =:17, `tum_content_estimated_value` =:18, `include_mutation_burden` =:19, `filters` =:20 WHERE id=:21");
		query.bindValue(0, userId(user_name));
		if(target_file != "") query.bindValue(1, target_file);
		else query.bindValue(1, QVariant(QVariant::String));
		query.bindValue(2, config.includeTumContentByMaxSNV());
		query.bindValue(3, config.includeTumContentByClonality());
		query.bindValue(4, config.includeTumContentByHistological());
		query.bindValue(5, config.msiStatus());
		query.bindValue(6, config.cnvBurden());

		if(getEnum("somatic_report_configuration", "hrd_statement").contains(config.hrdStatement())) query.bindValue(7, config.hrdStatement());
		else query.bindValue(7, QVariant(QVariant::String));

		query.bindValue(8, config.cnvLohCount());
		query.bindValue(9, config.cnvTaiCount());
		query.bindValue(10, config.cnvLstCount());

		query.bindValue(11, config.tmbReferenceText());

		if(config.quality().count() > 0) query.bindValue(12, config.quality().join(","));
		else query.bindValue(12, QVariant(QVariant::String));

		query.bindValue(13, config.fusionsDetected());

		if(config.cinChromosomes().count() > 0)	query.bindValue( 14, config.cinChromosomes().join(',') );
		else query.bindValue( 14, QVariant(QVariant::String) );

		if( !config.limitations().isEmpty()) query.bindValue(15, config.limitations() );
		else query.bindValue( 15, QVariant(QVariant::String) );

		if( !config.filterName().isEmpty() ) query.bindValue(16, config.filterName());
		else query.bindValue( 16, QVariant(QVariant::String) );

		query.bindValue(17, config.includeTumContentByEstimated());
		query.bindValue(18, config.tumContentByEstimated());
		query.bindValue(19, config.includeMutationBurden());

		if (config.filters().count() > 0) query.bindValue(20, config.filters().toText().join("\n"));
		else query.bindValue(20, QVariant(QVariant::String));

		query.bindValue(21, id);
		query.exec();
	}
	else
	{
		SqlQuery query = getQuery();
		query.prepare("INSERT INTO `somatic_report_configuration` (`ps_tumor_id`, `ps_normal_id`, `created_by`, `created_date`, `last_edit_by`, `last_edit_date`, `target_file`, `tum_content_max_af`, `tum_content_max_clonality`, `tum_content_hist`, `msi_status`, `cnv_burden`, `hrd_statement`, `cnv_loh_count`, `cnv_tai_count`, `cnv_lst_count`, `tmb_ref_text`, `quality`, `fusions_detected`, `cin_chr`, `limitations`, `filter_base_name`,  `tum_content_estimated`, `tum_content_estimated_value`, `include_mutation_burden`, `filters`) VALUES (:0, :1, :2, :3, :4, CURRENT_TIMESTAMP, :5, :6, :7, :8, :9, :10, :11, :12, :13, :14, :15, :16, :17, :18, :19, :20, :21, :22, :23, :24)");

		query.bindValue(0, t_ps_id);
		query.bindValue(1, n_ps_id);
		query.bindValue(2, userId(config.createdBy()));
		query.bindValue(3, config.createdAt());
		query.bindValue(4, userId(user_name));
		if(target_file != "") query.bindValue(5, target_file);
		else query.bindValue(5, QVariant(QVariant::String));

		query.bindValue(6, config.includeTumContentByMaxSNV());
		query.bindValue(7, config.includeTumContentByClonality());
		query.bindValue(8, config.includeTumContentByHistological());

		query.bindValue(9, config.msiStatus());
		query.bindValue(10, config.cnvBurden());

		if( getEnum("somatic_report_configuration", "hrd_statement").contains(config.hrdStatement()) ) query.bindValue(11, config.hrdStatement());
		else query.bindValue(11, QVariant(QVariant::String));

		query.bindValue(12, config.cnvLohCount());
		query.bindValue(13, config.cnvTaiCount());
		query.bindValue(14, config.cnvLstCount());


		query.bindValue(15, config.tmbReferenceText());

		if(config.quality().count() > 0) query.bindValue(16, config.quality().join(","));
		else query.bindValue(16, QVariant(QVariant::String));

		query.bindValue(17, config.fusionsDetected());

		if(config.cinChromosomes().count() != 0) query.bindValue(18, config.cinChromosomes().join(','));
		else query.bindValue(18, QVariant(QVariant::String));

		if(!config.limitations().isEmpty()) query.bindValue(19, config.limitations());
		else query.bindValue(19, QVariant(QVariant::String));

		if( !config.filterName().isEmpty() ) query.bindValue( 20, config.filterName() );
		else query.bindValue( 20, QVariant(QVariant::String) );

		query.bindValue(21, config.includeTumContentByEstimated());
		query.bindValue(22, config.tumContentByEstimated());
		query.bindValue(23, config.includeMutationBurden());

		if (config.filters().count() > 0) query.bindValue(24, config.filters().toText().join("\n"));
		else query.bindValue(24, QVariant(QVariant::String));

		query.exec();
		id = query.lastInsertId().toInt();
	}

	//Store variants in NGSD
	SqlQuery query_var = getQuery();

	query_var.prepare("INSERT INTO `somatic_report_configuration_variant` (`somatic_report_configuration_id`, `variant_id`, `exclude_artefact`, `exclude_low_tumor_content`, `exclude_low_copy_number`, `exclude_high_baf_deviation`, `exclude_other_reason`, `include_variant_alteration`, `include_variant_description`, `comment`) VALUES (:0, :1, :2, :3, :4, :5, :6, :7, :8, :9)");

	SqlQuery query_cnv = getQuery();
	query_cnv.prepare("INSERT INTO `somatic_report_configuration_cnv` (`somatic_report_configuration_id`, `somatic_cnv_id`, `exclude_artefact`, `exclude_low_tumor_content`, `exclude_low_copy_number`, `exclude_high_baf_deviation`, `exclude_other_reason`, `comment`) VALUES (:0, :1, :2, :3, :4, :5, :6, :7)");

	SqlQuery query_sv = getQuery();
	query_sv.prepare("INSERT INTO `somatic_report_configuration_sv` (`somatic_report_configuration_id`, `somatic_sv_deletion_id`, `somatic_sv_duplication_id`, `somatic_sv_insertion_id`, `somatic_sv_inversion_id`, `somatic_sv_translocation_id`, `exclude_artefact`, `exclude_unclear_effect`, `exclude_other`, `description`, `comment`, `rna_info`, `manual_start`, `manual_end`, `manual_hgvs_type`, `manual_hgvs_suffix`, `manual_start_bnd`, `manual_end_bnd`, `manual_hgvs_type_bnd`, `manual_hgvs_suffix_bnd`) VALUES (:0, :1, :2, :3, :4, :5, :6, :7, :8, :9, :10, :11, :12, :13, :14, :15, :16, :17, :18, :19)");



	foreach(const auto& var_conf, config.variantConfig())
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
		else if (var_conf.variant_type == VariantType::SVS)
		{
			//check SV index exists in SV list
			if (var_conf.variant_index<0 || var_conf.variant_index >= svs.count())
			{
				THROW(ProgrammingException, "SV list does not contain SV with index '" + QString::number(var_conf.variant_index) + "' in NGSD::setSomaticReportConfig!");
			}

			//check that report SV callset exists
			QVariant callset_id = getValue("SELECT id FROM somatic_sv_callset WHERE ps_tumor_id='" + t_ps_id + "' AND ps_normal_id='" + n_ps_id + "'", true);
			if (!callset_id.isValid())
			{
				THROW(ProgrammingException, "No SV callset defined for tumor-normal processed sample ids  " + t_ps_id + "-" + n_ps_id + "in NGSD::setSomaticReportConfig!");
			}

			//get SV id and table (add SV if not in DB)
			const BedpeLine& sv = svs[var_conf.variant_index];
			QString sv_id = somaticSvId(sv, callset_id.toInt(), svs, false);
			if (sv_id == "")
			{
				sv_id = addSomaticSv(callset_id.toInt(), sv, svs);
			}

			//define SQL query
			query_sv.bindValue(0, id);
			query_sv.bindValue(1, QVariant(QVariant::String));
			query_sv.bindValue(2, QVariant(QVariant::String));
			query_sv.bindValue(3, QVariant(QVariant::String));
			query_sv.bindValue(4, QVariant(QVariant::String));
			query_sv.bindValue(5, QVariant(QVariant::String));
			query_sv.bindValue(6, var_conf.exclude_artefact);
			query_sv.bindValue(7, var_conf.exclude_unclear_effect);
			query_sv.bindValue(8, var_conf.exclude_other_reason);
			query_sv.bindValue(9, var_conf.description);
			query_sv.bindValue(10, var_conf.comment);
			query_sv.bindValue(11, var_conf.rna_info);
			query_sv.bindValue(12, var_conf.manualSvStartValid() ? var_conf.manual_sv_start : QVariant());
			query_sv.bindValue(13, var_conf.manualSvEndValid() ? var_conf.manual_sv_end : QVariant());
			query_sv.bindValue(14, var_conf.manual_sv_hgvs_type);
			query_sv.bindValue(15, var_conf.manual_sv_hgvs_suffix);
			query_sv.bindValue(16, var_conf.manualSvStartBndValid() ? var_conf.manual_sv_start_bnd : QVariant());
			query_sv.bindValue(17, var_conf.manualSvEndBndValid() ? var_conf.manual_sv_end_bnd: QVariant());
			query_sv.bindValue(18, var_conf.manual_sv_hgvs_type_bnd);
			query_sv.bindValue(19, var_conf.manual_sv_hgvs_suffix_bnd);

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
			THROW(NotImplementedException, "Storing of somatic report configuration variant with type '" + QByteArray::number((int)var_conf.variant_type) + "' not implemented!");
		}
	}

	if(germl_snvs.count() > 0)
	{
		SqlQuery query_germl_var = getQuery();

		query_germl_var.prepare("INSERT INTO `somatic_report_configuration_germl_var` (`somatic_report_configuration_id`, `variant_id`, `tum_freq`, `tum_depth`) VALUES (:0, :1, :2, :3)");

		foreach(const auto& var_conf, config.variantConfigGermline())
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
	query.exec("DELETE FROM `somatic_report_configuration_sv` WHERE somatic_report_configuration_id=" + report_conf_id);
	query.exec("DELETE FROM `somatic_report_configuration` WHERE `id`=" + report_conf_id);
}

SomaticReportConfiguration NGSD::somaticReportConfig(QString t_ps_id, QString n_ps_id, const VariantList& snvs, const CnvList& cnvs, const BedpeFile& svs, const VariantList& germline_snvs, QStringList& messages)
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

	output.setIncludeTumContentByMaxSNV(query.value("tum_content_max_af").toBool());
	output.setIncludeTumContentByClonality(query.value("tum_content_max_clonality").toBool());
	output.setIncludeTumContentByHistological(query.value("tum_content_hist").toBool());
	output.setIncludeTumContentByEstimated(query.value("tum_content_estimated").toBool());

	if(!query.value("tum_content_estimated_value").isNull()) output.setTumContentByEstimated(query.value("tum_content_estimated_value").toDouble() );
	else output.setTumContentByEstimated(0);

	output.setMsiStatus(query.value("msi_status").toBool());
	output.setCnvBurden(query.value("cnv_burden").toBool());
	output.setIncludeMutationBurden(query.value("include_mutation_burden").toBool());

	output.setHrdStatement( query.value("hrd_statement").toString() );
	output.setCnvLohCount( query.value("cnv_loh_count").toInt() );
	output.setCnvLstCount( query.value("cnv_lst_count").toInt() );
	output.setCnvTaiCount( query.value("cnv_tai_count").toInt() );


	if(query.value("tmb_ref_text").isNull()) output.setTmbReferenceText("");
	else output.setTmbReferenceText(query.value("tmb_ref_text").toString());

	if(query.value("quality").isNull()) output.setQuality(QStringList());
	else output.setQuality(query.value("quality").toString().split(","));

	output.setFusionsDetected(query.value("fusions_detected").toBool());

	if(!query.value("cin_chr").isNull()) output.setCinChromosomes( query.value("cin_chr").toString().split(',') );

	if(!query.value("limitations").isNull()) output.setLimitations( query.value("limitations").toString() );

	if(!query.value("filter_base_name").isNull()) output.setFilterName( query.value("filter_base_name").toString() );

	if(!query.value("filters").isNull())
	{
		output.setFilters(FilterCascade::fromText(query.value("filters").toString().split("\n")));
	} else if (!query.value("filter_base_name").isNull()) { // TODO temp loading help while converting to having the filters completely in the DB
		QString filterFileName = QCoreApplication::applicationDirPath() + QDir::separator() + "GSvar_filters.ini";
		output.setFilters(FilterCascadeFile::load(filterFileName, query.value("filter_base_name").toString()));
	}
	else
	{
		output.setFilters(FilterCascade());
	}

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
			messages << "Could not find somatic variant '" + var.toString() + "' in given variant list. The report configuration of this variant will be lost if you change anything in the report configuration!";
		}

		var_conf.exclude_artefact = query.value("exclude_artefact").toBool();
		var_conf.exclude_low_tumor_content = query.value("exclude_low_tumor_content").toBool();
		var_conf.exclude_low_copy_number = query.value("exclude_low_copy_number").toBool();
		var_conf.exclude_high_baf_deviation = query.value("exclude_high_baf_deviation").toBool();
		var_conf.exclude_other_reason = query.value("exclude_other_reason").toBool();

		var_conf.include_variant_alteration = query.value("include_variant_alteration").toString();
		var_conf.include_variant_description = query.value("include_variant_description").toString();

		var_conf.comment = query.value("comment").toString();

		output.addSomaticVariantConfiguration(var_conf);
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
			messages << "Could not find somatic CNV '" + cnv.toString() + "' in given variant list. The report configuration of this variant will be lost if you change anything in the report configuration!";
			continue;
		}

		var_conf.exclude_artefact = query.value("exclude_artefact").toBool();
		var_conf.exclude_low_tumor_content = query.value("exclude_low_tumor_content").toBool();
		var_conf.exclude_low_copy_number = query.value("exclude_low_copy_number").toBool();
		var_conf.exclude_high_baf_deviation = query.value("exclude_high_baf_deviation").toBool();
		var_conf.exclude_other_reason = query.value("exclude_other_reason").toBool();
		var_conf.comment = query.value("comment").toString();

		output.addSomaticVariantConfiguration(var_conf);
	}

	//Load SVs
	query.exec("SELECT * FROM somatic_report_configuration_sv WHERE somatic_report_configuration_id=" + QString::number(config_id));
	while(query.next())
	{
		SomaticReportVariantConfiguration var_conf;
		var_conf.variant_type = VariantType::SVS;

		//get SV id
		QString sv_id;
		StructuralVariantType type;

		//determine SV type and id
		if(!query.value("somatic_sv_deletion_id").isNull())
		{
			type = StructuralVariantType::DEL;
			sv_id = query.value("somatic_sv_deletion_id").toString();
		}
		else if(!query.value("somatic_sv_duplication_id").isNull())
		{
			type = StructuralVariantType::DUP;
			sv_id = query.value("somatic_sv_duplication_id").toString();
		}
		else if(!query.value("somatic_sv_insertion_id").isNull())
		{
			type = StructuralVariantType::INS;
			sv_id = query.value("somatic_sv_insertion_id").toString();
		}
		else if(!query.value("somatic_sv_inversion_id").isNull())
		{
			type = StructuralVariantType::INV;
			sv_id = query.value("somatic_sv_inversion_id").toString();
		}
		else if(!query.value("somatic_sv_translocation_id").isNull())
		{
			type = StructuralVariantType::BND;
			sv_id = query.value("somatic_sv_translocation_id").toString();
		}
		else
		{
			THROW(DatabaseException, "Somatic report config entry does not contain a SV id!");
		}

		BedpeLine sv = somaticSv(sv_id, type, svs);

		//skip variants that are not found, e.g. when trios and single sample analysis are used alternatingly
		var_conf.variant_index = svs.findMatch(sv, true, false);
		if (var_conf.variant_index==-1) continue;

		var_conf.exclude_artefact = query.value("exclude_artefact").toBool();
		var_conf.exclude_unclear_effect = query.value("exclude_unclear_effect").toBool();
		var_conf.exclude_other_reason = query.value("exclude_other").toBool();
		var_conf.description = query.value("description").toString();
		var_conf.comment = query.value("comment").toString();
		var_conf.rna_info = query.value("rna_info").toString();

		if (query.value("manual_start").toInt()>0)
		{
			var_conf.manual_sv_start = query.value("manual_start").toString();
		}
		if (query.value("manual_end").toInt()>0)
		{
			var_conf.manual_sv_end = query.value("manual_end").toString();
		}
		var_conf.manual_sv_hgvs_type = query.value("manual_hgvs_type").toString();
		var_conf.manual_sv_hgvs_suffix = query.value("manual_hgvs_suffix").toString();
		if (query.value("manual_start_bnd").toInt()>0)
		{
			var_conf.manual_sv_start_bnd = query.value("manual_start_bnd").toString();
		}
		if (query.value("manual_end_bnd").toInt()>0)
		{
			var_conf.manual_sv_end_bnd = query.value("manual_end_bnd").toString();
		}
		var_conf.manual_sv_hgvs_type_bnd = query.value("manual_hgvs_type_bnd").toString();
		var_conf.manual_sv_hgvs_suffix_bnd = query.value("manual_hgvs_suffix_bnd").toString();

		output.addSomaticVariantConfiguration(var_conf);
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
			messages << "Could not find germline variant '" + var.toString() + "' in given variant list. The report configuration of this variant will be lost if you change anything in the report configuration!";
		}

		if(!query.value("tum_freq").isNull()) var_conf.tum_freq = query.value("tum_freq").toDouble();
		else var_conf.tum_freq = std::numeric_limits<double>::quiet_NaN();

		if(!query.value("tum_depth").isNull() ) var_conf.tum_depth = query.value("tum_depth").toDouble();
		else var_conf.tum_depth = std::numeric_limits<double>::quiet_NaN();

		output.addGermlineVariantConfiguration(var_conf);
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
		output.imprinting_source_allele = imprinting[symbol].expressed_allele;
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

void AnalysisJob::checkValid(int ngsd_id)
{
	//general checks
	if (samples.isEmpty()) THROW(ArgumentException, "Job with NGSD id  '" + QString::number(ngsd_id) + " contains no sample!");

	//get infos
	QSet<QString> infos;
	foreach(const AnalysisJobSample& s, samples)
	{
		infos << s.info;
	}

	//trio checks
	if (type=="trio")
	{
		if (samples.count()!=3) THROW(ArgumentException, "Trio job with NGSD id '" + QString::number(ngsd_id) + " does not contains 3 sample!");

		if (!infos.contains("child")) THROW(ArgumentException, "Trio job with NGSD id '" + QString::number(ngsd_id) + " does not contain 'child' sample!");
		if (!infos.contains("father")) THROW(ArgumentException, "Trio job with NGSD id '" + QString::number(ngsd_id) + " does not contain 'father' sample!");
		if (!infos.contains("mother")) THROW(ArgumentException, "Trio job with NGSD id '" + QString::number(ngsd_id) + " does not contain 'mother' sample!");
	}

	//multi-sample checks
	if (type=="multi sample")
	{
		if (samples.count()<2) THROW(ArgumentException, "Trio job with NGSD id '" + QString::number(ngsd_id) + " does not contains 3 sample!");
	}

	//somatic checks
	if (type=="somatic")
	{
		if (samples.count()>3) THROW(ArgumentException, "Somatic job with NGSD id '" + QString::number(ngsd_id) + " contains more than 3 samples!");
		if (!infos.contains("tumor")) THROW(ArgumentException, "Somatic job with NGSD id '" + QString::number(ngsd_id) + " does not contain 'tumor' sample!");
		if (samples.count()>=2)
		{
			if (!infos.contains("normal")) THROW(ArgumentException, "Somatic job with NGSD id '" + QString::number(ngsd_id) + " does not contain 'tumor' sample!");
		}
		else if (samples.count()>=3)
		{
			if (!infos.contains("tumor_rna")) THROW(ArgumentException, "Somatic job with NGSD id '" + QString::number(ngsd_id) + " does not contain 'tumor_rna' sample!");
		}
	}
}

QString SomaticReportConfigurationData::history() const
{
	QStringList output;
	output << "The report configuration was created by " + created_by + " on " + created_date + ".";
	if (last_edit_by!="") output << "The report configuration was last updated by " + last_edit_by + " on " + last_edit_date + ".";
	if (mtb_xml_upload_date != "") output << "The XML file was last uploaded to MTB on " + mtb_xml_upload_date + ".";
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
		case SET:
			return"SET";
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

bool TableFieldInfo::isValid(QString text) const
{
	if (is_nullable && text.isEmpty()) return true;

	switch(type)
	{
		case BOOL:
			return text=="0" || text=="1";
			break;
		case INT:
		case TIMESTAMP:
		case FK: //hande FKs as ints - to check if it's a valid column id, we would need a NGSD instance...
			{
				bool ok = false;
				int tmp = text.toInt(&ok);
				return ok && !(is_unsigned && tmp<0);
			}
			break;
		case FLOAT:
			{
				bool ok = false;
				double tmp = text.toDouble(&ok);
				return ok && !(is_unsigned && tmp<0);
			}
			break;
		case TEXT:
			return true;
			break;
		case VARCHAR:
			return text.length()<=type_constraints.max_length;
			break;
		case VARCHAR_PASSWORD:
			return text.length()<=type_constraints.max_length;
			break;
		case ENUM:
			return type_constraints.valid_strings.contains(text);
			break;
		case DATE:
			return QDate::fromString(text, Qt::ISODate).isValid();
			break;
		case DATETIME:
			return QDateTime::fromString(text, Qt::ISODate).isValid();
			break;
		default:
			THROW(NotImplementedException, "Unhandled type '" + QString::number(type) + "' in TableFieldInfo::isValid!");
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

void NGSD::exportTable(const QString& table, QTextStream& out, QString where_clause, QMap<QString, QSet<int>> *sql_history)
{
	if (!table.isEmpty())
	{
		TableInfo table_info = NGSD().tableInfo(table);
		int field_count = table_info.fieldCount();
		QStringList field_names = table_info.fieldNames();

		SqlQuery query = getQuery();
		QString where;
		if (!where_clause.isEmpty()) where = " WHERE (" + where_clause + ")";
		QString sql_query = "SELECT * FROM " + table + where;

		query.exec(sql_query);


		out << "--\n-- TABLE `" + table + "`\n--\n";
		if (query.size() <= 0) out << "-- No records found --\n";

		QString query_prefix = "INSERT INTO `" + table + "` (`" + field_names.join("`, `") + "`) VALUES ";
		int row_count = 0;
		while(query.next())
		{
			if ((sql_history != nullptr) && (field_names.contains("id")))
			{
				if (sql_history->value(table).contains(query.value("id").toInt())) continue;
				(*sql_history)[table].insert(query.value("id").toInt());
			}

			if ((row_count>0) && (row_count<1000)) out << ", ";
			row_count++;
			if (row_count == 1)
			{
				out << query_prefix;
			}
			QStringList values;
			for (int i=0; i<field_count; i++)
			{
				QString field_value = query.value(field_names[i]).toString();

				//handle nullable fields
				if ((field_value.isEmpty() || field_value=="0") && table_info.fieldInfo()[i].is_nullable)
				{
					field_value = "NULL";
				}

				//prevent ';\n' because that is interpreted as end of query in import
				if (table_info.fieldInfo()[i].type==TableFieldInfo::TEXT || table_info.fieldInfo()[i].type==TableFieldInfo::VARCHAR)
				{
					field_value.replace(";\n",",\n");
					field_value.replace("; \n",",\n");
					field_value.replace(";  \n",",\n");
				}
                values.append(escapeText(field_value));
			}

            QString insert_query =  "(" + values.join(", ") + ")";
            insert_query = insert_query.replace("'NULL'", "NULL");
			out << insert_query;

			if (row_count>=1000)
			{
				row_count = 0;
				out << ";\n";
			}
		}
		if ((row_count>0) && (row_count<=1000)) out << ";\n";
		out << "\n";
	}
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
	cache_instance.same_patients.clear();
	cache_instance.related_samples.clear();
	cache_instance.approved_gene_names.clear();
	cache_instance.gene2id.clear();
	cache_instance.enum_values.clear();
	cache_instance.non_approved_to_approved_gene_names.clear();
	cache_instance.phenotypes_by_id.clear();
	cache_instance.phenotypes_accession_to_id.clear();

	cache_instance.gene_transcripts.clear();
	cache_instance.gene_transcripts_index.createIndex();
	cache_instance.gene_transcripts_id2index.clear();
	cache_instance.gene_transcripts_symbol2indices.clear();

	cache_instance.gene_expression_id2gene.clear();
	cache_instance.gene_expression_gene2id.clear();
}


NGSD::Cache::Cache()
	: gene_transcripts()
	, gene_transcripts_index(gene_transcripts)
{
}

void NGSD::initTranscriptCache()
{
	//make sure initialization is done only once
	static bool initializing = false;
	if (initializing)
	{
		while(initializing)
		{
			QThread::msleep(1);
		}
		return;
	}
	initializing = true;

	//get preferred transcript list
	QSet<QByteArray> pts;
	foreach(QString trans, getValues("SELECT DISTINCT name FROM preferred_transcripts"))
	{
		pts.insert(trans.toUtf8());
	}

	TranscriptList& cache = getCache().gene_transcripts;
	ChromosomalIndex<TranscriptList>& index = getCache().gene_transcripts_index;
	QHash<int, int>& id2index = getCache().gene_transcripts_id2index;
	QHash<QByteArray, QSet<int>>& symbol2indices = getCache().gene_transcripts_symbol2indices;


	//get exon coordinates for each transcript from NGSD
	QHash<int, QList<QPair<int, int>>> tmp_coords;
	SqlQuery query = getQuery();
	query.exec("SELECT transcript_id, start, end FROM gene_exon ORDER BY start, end");
	while(query.next())
	{
		int trans_id = query.value(0).toInt();
		int start = query.value(1).toInt();
		int end = query.value(2).toInt();
		tmp_coords[trans_id] << qMakePair(start, end);
	}

	//create all transcripts
	QHash<QByteArray, int> tmp_name2id;
	query.exec("SELECT t.id, g.symbol, t.name, t.source, t.strand, t.chromosome, t.start_coding, t.end_coding, t.biotype, t.is_gencode_basic, t.is_ensembl_canonical, t.is_mane_select, t.is_mane_plus_clinical, t.version, g.ensembl_id FROM gene_transcript t, gene g WHERE t.gene_id=g.id");
	while(query.next())
	{
		int trans_id = query.value(0).toInt();

		//get base information
		Transcript transcript;
		transcript.setGene(query.value(1).toByteArray());
		transcript.setGeneId(query.value(14).toByteArray());
		transcript.setName(query.value(2).toByteArray());
		transcript.setSource(Transcript::stringToSource(query.value(3).toString()));
		transcript.setStrand(Transcript::stringToStrand(query.value(4).toByteArray()));
		transcript.setBiotype(Transcript::stringToBiotype(query.value(8).toByteArray()));
		transcript.setPreferredTranscript(pts.contains(transcript.name()));
		transcript.setGencodeBasicTranscript(query.value(9).toInt()!=0);
		transcript.setEnsemblCanonicalTranscript(query.value(10).toInt()!=0);
		transcript.setManeSelectTranscript(query.value(11).toInt()!=0);
		transcript.setManePlusClinicalTranscript(query.value(12).toInt()!=0);
		transcript.setVersion(query.value(13).toInt());

		//get exons
		BedFile regions;
		Chromosome chr = "chr"+query.value(5).toByteArray();
		foreach(const auto& coord, tmp_coords[trans_id])
		{
			regions.append(BedLine(chr, coord.first, coord.second));
		}
		int start_coding = query.value(6).isNull() ? 0 : query.value(6).toInt();
		int end_coding = query.value(7).isNull() ? 0 : query.value(7).toInt();
		if (transcript.strand()==Transcript::MINUS)
		{
			int tmp = start_coding;
			start_coding = end_coding;
			end_coding = tmp;
		}
		transcript.setRegions(regions, start_coding, end_coding);

		cache << transcript;
		tmp_name2id[transcript.name()] = trans_id;
	}

	//sort and build indices
	cache.sortByPosition();
	for (int i=0; i<cache.count(); ++i)
	{
		const Transcript& trans = cache[i];

		int trans_id = tmp_name2id[trans.name()];
		id2index[trans_id] = i;

		symbol2indices[trans.gene()] << i;
	}

	//build index
	index.createIndex();

	initializing = false;
}

void NGSD::initGeneExpressionCache()
{
	//make sure initialization is done only once
	static bool initializing = false;
	if (initializing)
	{
		while(initializing)
		{
			QThread::msleep(1);
		}
		return;
	}
	initializing = true;

	QMap<int, QByteArray>& id2gene = getCache().gene_expression_id2gene;
	QMap<QByteArray, int>& gene2id = getCache().gene_expression_gene2id;

	//reset cache
	id2gene.clear();
	gene2id.clear();

	SqlQuery query = getQuery();
	query.exec("SELECT id, symbol FROM expression_gene");
	while(query.next())
	{
		id2gene.insert(query.value(0).toInt(), query.value(1).toByteArray());
		gene2id.insert(query.value(1).toByteArray(), query.value(0).toInt());
	}

	initializing = false;
}
