#include <QSqlQuery>
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
		THROW(DatabaseException, "Could not connect to the GenLab database:!");
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

int GenLabDB::entriesForSample(QString sample_name)
{
	QSqlQuery query = db_->exec("SELECT * FROM v_ngs_sap WHERE labornummer='" + sample_name + "'");
	return query.size();
}

QList<Phenotype> GenLabDB::phenotypes(QString sample_name)
{
	NGSD ngsd;
	QList<Phenotype> output;

	QSqlQuery query = db_->exec("SELECT HPOTERM1,HPOTERM2,HPOTERM3,HPOTERM4 FROM v_ngs_sap WHERE labornummer='" + sample_name + "'");
	while(query.next())
	{
		for (int i=0; i<4; ++i)
		{
			if (query.value(i).toString().trimmed().isEmpty()) continue;

			QByteArray pheno_id = query.value(i).toByteArray();
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
	}

	return output;
}

QStringList GenLabDB::diagnosis(QString sample_name)
{
	QSqlQuery query = db_->exec("SELECT ICD10DIAGNOSE FROM v_ngs_sap WHERE labornummer='" + sample_name + "'");

	QStringList output;
	while(query.next())
	{
		output << query.value(0).toString();
	}
	output.removeDuplicates();

	return output;
}

QStringList GenLabDB::tumorFraction(QString sample_name)
{
	QSqlQuery query = db_->exec("SELECT TUMORANTEIL FROM v_ngs_sap WHERE labornummer='" + sample_name + "'");


	QStringList output;
	while(query.next())
	{
		output << query.value(0).toString();
	}
	output.removeDuplicates();

	return output;
}

