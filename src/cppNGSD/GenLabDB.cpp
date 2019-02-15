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
	db_.reset(new QSqlDatabase(QSqlDatabase::addDatabase("QMYSQL", "GENLAB_" + Helper::randomString(20))));

	db_->setHostName(Settings::string("genlab_host"));
	db_->setPort(Settings::integer("genlab_port"));
	db_->setDatabaseName(Settings::string("genlab_name"));
	db_->setUserName(Settings::string("genlab_user"));
	db_->setPassword(Settings::string("genlab_pass"));
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
	static bool is_initialized = false;
	static bool is_open = false;
	if (!is_initialized)
	{
		is_open = QSqlQuery(*db_).exec("SELECT 1");
		is_initialized = true;
	}

	return is_open;
}

bool GenLabDB::entriesExistForSample(QString sample_name)
{
	QStringList tables;
	tables << "v_ngs_einsender" << "v_ngs_geschlecht" << "v_ngs_icd10" << "v_ngs_hpo"  << "v_ngs_tumoranteil";
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

QStringList GenLabDB::diagnosis(QString sample_name)
{
	QSqlQuery query = db_->exec("SELECT code FROM v_ngs_icd10 WHERE labornummer='" + sample_name + "' AND code IS NOT NULL");

	QStringList output;
	while(query.next())
	{
		if (query.value(0).toString().trimmed().isEmpty()) continue;

		output << query.value(0).toString();
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
		if (query.value(0).toString().trimmed().isEmpty()) continue;

		output << query.value(0).toString();
	}
	output.removeDuplicates();

	return output;
}

