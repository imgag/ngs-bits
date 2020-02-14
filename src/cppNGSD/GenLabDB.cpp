#include <QSqlQuery>
#include <QSqlError>
#include "GenLabDB.h"
#include "Helper.h"
#include "Settings.h"
#include "Exceptions.h"
#include "Log.h"
#include "NGSD.h"

GenLabDB::GenLabDB()
{
	//get settings
	QString host = Settings::string("genlab_host");
	int port = Settings::integer("genlab_port");
	QString name = Settings::string("genlab_name");
	QString user = Settings::string("genlab_user");
	QString pass = Settings::string("genlab_pass");

	if (Settings::string("genlab_mssql")!="true") //MySQL server
	{
		db_.reset(new QSqlDatabase(QSqlDatabase::addDatabase("QMYSQL", "GENLAB_" + Helper::randomString(20))));

		db_->setHostName(host);
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

bool GenLabDB::entriesExistForSample(QString sample_name)
{
	QStringList tables;
	tables << "v_ngs_einsender" << "v_ngs_geschlecht" << "v_ngs_icd10" << "v_ngs_hpo"  << "v_ngs_tumoranteil" << "v_ngs_orpha";
	foreach(QString table, tables)
	{
		QSqlQuery query = db_->exec("SELECT COUNT(*) FROM " + table + " WHERE labornummer='" + sample_name + "'");
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

	QSqlQuery query = db_->exec("SELECT code FROM v_ngs_hpo WHERE labornummer='" + sample_name + "' AND code IS NOT NULL");
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
	QSqlQuery query = db_->exec("SELECT code FROM v_ngs_orpha WHERE labornummer='" + sample_name + "' AND code IS NOT NULL");

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
	QSqlQuery query = db_->exec("SELECT code FROM v_ngs_icd10 WHERE labornummer='" + sample_name + "' AND code IS NOT NULL");

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

QStringList GenLabDB::tumorFraction(QString sample_name)
{
	QSqlQuery query = db_->exec("SELECT TUMORANTEIL FROM v_ngs_tumoranteil WHERE labornummer='" + sample_name + "' AND TUMORANTEIL IS NOT NULL");

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

	QSqlQuery query = db_->exec("SELECT krankheitsgruppe,patienttyp FROM v_krankheitsgruppe_pattyp WHERE labornummer='" + ps_name + "'");
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

