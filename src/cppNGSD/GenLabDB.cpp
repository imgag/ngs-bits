#include <QSqlQuery>
#include <QSqlError>
#include "GenLabDB.h"
#include "Helper.h"
#include "Settings.h"
#include "Exceptions.h"
#include "Log.h"

QMap<QString, TableInfo> GenLabDB::infos_;

GenLabDB::GenLabDB()
{
	//get settings
	QString host = Settings::string("genlab_host");
	QString name = Settings::string("genlab_name");
	QString user = Settings::string("genlab_user");
	QString pass = Settings::string("genlab_pass");

	if (Settings::string("genlab_mssql")!="true") //MySQL server
	{
		db_.reset(new QSqlDatabase(QSqlDatabase::addDatabase("QMYSQL", "GENLAB_" + Helper::randomString(20))));

		db_->setHostName(host);
		int port = Settings::integer("genlab_port");
		db_->setPort(port);
		db_->setDatabaseName(name);
		db_->setUserName(user);
		db_->setPassword(pass);
	}
	else //Microsoft SQL server
	{
		db_.reset(new QSqlDatabase(QSqlDatabase::addDatabase("QODBC3", "GENLAB_" + Helper::randomString(20))));
		db_->setDatabaseName("DRIVER={SQL Server};SERVER="+host+"\\"+name+";UID="+user+";PWD="+pass+";");
	}

	if (!db_->open())
	{
		THROW(DatabaseException, "Could not connect to the GenLab database: " + db_->lastError().text());
	}
}

GenLabDB::~GenLabDB()
{
	//close database and remove it
	QString connection_name = db_->connectionName();
	db_.clear();
	QSqlDatabase::removeDatabase(connection_name);
}

bool GenLabDB::isOpen() const
{
	return QSqlQuery(*db_).exec("SELECT 1");
}

QStringList GenLabDB::tables() const
{
	QStringList output;

	QSqlQuery query(*db_);
	query.exec("SELECT DISTINCT TABLE_NAME FROM information_schema.TABLES");
	while(query.next())
	{
		output << query.value(0).toString();
	}

	return output;
}

const TableInfo& GenLabDB::tableInfo(const QString& table) const
{
	//create if necessary
	if (!infos_.contains(table))
	{
		//check table exists
		if (!tables().contains(table))
		{
			THROW(DatabaseException, "Table '" + table + "' not found in GenLab database!");
		}

		TableInfo output;
		output.setTable(table);

		QList<TableFieldInfo> infos;
		SqlQuery query = getQuery();
		query.exec("SELECT column_name, data_type, is_nullable, column_default, character_maximum_length FROM information_schema.columns WHERE table_name='" + table + "' ORDER BY ordinal_position");
		while(query.next())
		{
			//qDebug() << query.value(0) << query.value(1) << query.value(2) << query.value(3) << query.value(4);
			TableFieldInfo info;

			//name
			info.name = query.value("column_name").toString();

			//index
			info.index = output.fieldCount();

			//type
			QString type = query.value("data_type").toString().toLower();
			info.is_unsigned = type.contains(" unsigned");
			if (info.is_unsigned)
			{
				type = type.replace(" unsigned", "");
			}
			if(type=="int" || type=="smallint") info.type = TableFieldInfo::INT;
			else if(type=="decimal") info.type = TableFieldInfo::FLOAT;
			else if(type=="datetime") info.type = TableFieldInfo::DATETIME;
			else if(type=="nvarchar")
			{
				info.type = TableFieldInfo::VARCHAR;
				if (!query.value("character_maximum_length").isNull())
				{
					info.type_constraints.max_length = query.value("character_maximum_length").toInt();
				}
			}
			else
			{
				THROW(ProgrammingException, "Unhandled SQL field type '" + type + "' in field '" + info.name + "' of table '" + table + "'!");
			}

			//nullable
			info.is_nullable = query.value("is_nullable").toString().toLower()=="YES";

			//default value
			info.default_value =  query.value("column_default").isNull() ? QString() : query.value(4).toString();

			//labels
			info.label = info.name;
			info.label.replace('_', ' ');

			infos.append(info);
		}

		output.setFieldInfo(infos);
		infos_.insert(table, output);
	}

	return infos_[table];
}

bool GenLabDB::entriesExistForSample(QString sample_name)
{
	QStringList tables;
	tables << "v_ngs_anamnese" << "v_krankheitsgruppe_pattyp" << "v_ngs_einsender" << "v_ngs_geschlecht" << "v_ngs_hpo" << "v_ngs_icd10" << "v_ngs_orpha" << "v_ngs_sap" << "v_ngs_tumoranteil";
	foreach(QString table, tables)
	{
		SqlQuery query = getQuery();
		query.exec("SELECT COUNT(*) FROM " + table + " WHERE labornummer='" + sample_name + "'");
		query.next();
		int count = query.value(0).toInt();
		if (count>0) return true;
	}

	return false;
}

QList<Phenotype> GenLabDB::phenotypes(QString sample_name)
{
	NGSD ngsd;
	QList<Phenotype> output;

	SqlQuery query = getQuery();
	query.exec("SELECT code FROM v_ngs_hpo WHERE labornummer='" + sample_name + "' AND code IS NOT NULL");
	while(query.next())
	{
		if (query.value(0).toString().trimmed().isEmpty()) continue;

		QByteArray pheno_id = query.value(0).toByteArray();
		try
		{
			Phenotype pheno = ngsd.phenotypeByAccession(pheno_id, false);
			if (!pheno.name().isEmpty() && !output.contains(pheno))
			{
				output << pheno;
			}
		}
		catch(DatabaseException e)
		{
			Log::error("Invalid HPO term ID '" + pheno_id + "' found in GenLab: " + e.message());
		}
	}

	return output;
}

QStringList GenLabDB::orphanet(QString sample_name)
{
	SqlQuery query = getQuery();
	query.exec("SELECT code FROM v_ngs_orpha WHERE labornummer='" + sample_name + "' AND code IS NOT NULL");

	QStringList output;
	while(query.next())
	{
		QString orpha_num = query.value(0).toString().trimmed().toUpper();
		if (orpha_num.isEmpty()) continue;

		if (!orpha_num.startsWith("ORPHA:"))
		{
			orpha_num.prepend("ORPHA:");
		}

		output << orpha_num;
	}
	output.removeDuplicates();

	return output;
}

QStringList GenLabDB::diagnosis(QString sample_name)
{
	SqlQuery query = getQuery();
	query.exec("SELECT code FROM v_ngs_icd10 WHERE labornummer='" + sample_name + "' AND code IS NOT NULL");

	QStringList output;
	while(query.next())
	{
		QString diagnosis = query.value(0).toString().trimmed();
		if (diagnosis.isEmpty()) continue;

		output << diagnosis;
	}
	output.removeDuplicates();

	return output;
}

QString GenLabDB::anamnesis(QString sample_name)
{
	QString output;

	SqlQuery query = getQuery();
	query.exec("SELECT ANAMNESE FROM v_ngs_anamnese WHERE LABORNUMMER='" + sample_name + "' AND ANAMNESE IS NOT NULL AND ANAMNESE != 'leer'");

	if(query.next())
	{
		return query.value(0).toString().trimmed();
	}

	return output;
}

QStringList GenLabDB::tumorFraction(QString sample_name)
{
	SqlQuery query = getQuery();
	query.exec("SELECT TUMORANTEIL FROM v_ngs_tumoranteil WHERE labornummer='" + sample_name + "' AND TUMORANTEIL IS NOT NULL");

	QStringList output;
	while(query.next())
	{
		QString fraction = query.value(0).toString().trimmed();
		if (fraction.isEmpty()) continue;

		output << fraction;
	}
	output.removeDuplicates();

	return output;
}

QPair<QString, QString> GenLabDB::diseaseInfo(QString ps_name)
{
	QString group = "n/a";
	QString status = "n/a";

	SqlQuery query = getQuery();
	query.exec("SELECT krankheitsgruppe,patienttyp FROM v_krankheitsgruppe_pattyp WHERE labornummer='" + ps_name + "'");
	while (query.next())
	{
		//group
		if (!query.value(0).isNull())
		{
			QString tmp = query.value(0).toString().trimmed();
			if (!tmp.isEmpty())
			{
				group = tmp;
			}
		}
		//status
		if (!query.value(1).isNull())
		{
			QString tmp = query.value(1).toString().trimmed();
			if (tmp=="Index" || tmp=="Angehöriger betroffen")
			{
				status = "Affected";
			}
			if (tmp=="Angehöriger gesund")
			{
				status = "Unaffected";
			}
		}
	}

	//fallback to sample (not consistent in GenLab)
	if (group=="n/a" && status=="n/a" && ps_name.contains("_"))
	{
		QStringList parts = ps_name.split("_");
		QString s_name = parts.mid(0, parts.count()-1).join("_");
		return diseaseInfo(s_name);
	}

	return qMakePair(group, status);
}


QString GenLabDB::sapID(QString imgag_lab_id)
{
	SqlQuery query = getQuery();

	QString sample_name = imgag_lab_id.append('_').split('_')[0];


	query.exec("SELECT identnr, labornummer FROM v_ngs_sap WHERE labornummer='" + sample_name + "'");

	if(query.next()) return query.value(0).toString();

	query.exec("SELECT identnr, labornummer FROM v_ngs_sap WHERE labornummer LIKE '"+ sample_name +"_[0-9][0-9]'" );

	if(query.next()) return query.value(0).toString();

	return "";
}

