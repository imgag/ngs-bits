#include <QSqlQuery>
#include <QSqlError>
#include "GenLabDB.h"
#include "LoginManager.h"
#include "Helper.h"
#include "Settings.h"
#include "Exceptions.h"
#include "Log.h"

QMap<QString, TableInfo> GenLabDB::infos_;

GenLabDB::GenLabDB()
{
	bool genlab_mssql;
	QString host;
	int port;
	QString name;
	QString user;
	QString pass;

	//get settings
	if (NGSHelper::isClientServerMode() && !NGSHelper::isRunningOnServer())
	{
		genlab_mssql = LoginManager::genlab_mssql();
		host = LoginManager::genlabHost();
		port = LoginManager::genlabPort();
		name = LoginManager::genlabName();
		user = LoginManager::genlabUser();
		pass = LoginManager::genlabPassword();
	}
	else
	{
		genlab_mssql = Settings::boolean("genlab_mssql", true);
		host = Settings::string("genlab_host", true);
		port = Settings::contains("genlab_port") ? Settings::integer("genlab_port") : -1;
		name = Settings::string("genlab_name", true);
		user = Settings::string("genlab_user", true);
		pass = Settings::string("genlab_pass", true);
	}


	if (!genlab_mssql) //MySQL server
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

bool GenLabDB::isAvailable()
{
	if (NGSHelper::isClientServerMode() && !NGSHelper::isRunningOnServer())
	{
		return true;
	}

	return Settings::contains("genlab_host") && Settings::contains("genlab_name") && Settings::contains("genlab_user") && Settings::contains("genlab_pass"); //port is not required for MsSQL
}

PhenotypeList GenLabDB::phenotypes(QString ps_name)
{
	PhenotypeList output;

	NGSD ngsd;

	//ignore HPO terms of certain subbranches
	static QSet<int> ignored_terms_ids;
	if (ignored_terms_ids.isEmpty())
	{
		ignored_terms_ids << ngsd.phenotypeIdByAccession("HP:0000001"); //"All"
		ignored_terms_ids << ngsd.phenotypeIdByAccession("HP:0000118"); //"Phenotypic abnormality"

		//"Mode of inheritance"
		int parent_term = ngsd.phenotypeIdByAccession("HP:0000005");
		ignored_terms_ids << parent_term;
		foreach(const Phenotype& pheno, ngsd.phenotypeChildTerms(parent_term, true))
		{
			ignored_terms_ids << ngsd.phenotypeIdByAccession(pheno.accession());
		}
		//"Frequency"
		parent_term = ngsd.phenotypeIdByAccession("HP:0040279");
		ignored_terms_ids << parent_term;
		foreach(const Phenotype& pheno, ngsd.phenotypeChildTerms(parent_term, true))
		{
			ignored_terms_ids << ngsd.phenotypeIdByAccession(pheno.accession());
		}
		//"Blood group"
		parent_term = ngsd.phenotypeIdByAccession("HP:0032223");
		ignored_terms_ids << parent_term;
		foreach(const Phenotype& pheno, ngsd.phenotypeChildTerms(parent_term, true))
		{
			ignored_terms_ids << ngsd.phenotypeIdByAccession(pheno.accession());
		}
	}


	foreach(QString name, names(ps_name))
	{
		SqlQuery query = getQuery();
		query.exec("SELECT code FROM v_ngs_hpo WHERE labornummer='" + name + "'");
		while(query.next())
		{
			QByteArray hpo_id = query.value(0).toByteArray().trimmed();
			if (hpo_id.isEmpty()) continue;

			int id = ngsd.phenotypeIdByAccession(hpo_id, false);
			if (id==-1) continue;
			if (ignored_terms_ids.contains(id)) continue;

			Phenotype pheno = ngsd.phenotype(id);
			if (output.containsAccession(pheno.accession())) continue;

			output << pheno;
		}
	}

	return output;
}

QStringList GenLabDB::orphanet(QString ps_name)
{
	QStringList output;

	foreach(QString name, names(ps_name))
	{
		SqlQuery query = getQuery();
		query.exec("SELECT code FROM v_ngs_orpha WHERE labornummer='" + name + "'");
		while(query.next())
		{
			QString orpha_num = query.value(0).toString().toUpper().trimmed();
			if (orpha_num.isEmpty()) continue;

			if (!orpha_num.startsWith("ORPHA:"))
			{
				orpha_num.prepend("ORPHA:");
			}

			if (output.contains(orpha_num)) continue;

			output << orpha_num;
		}
	}

	return output;
}

QStringList GenLabDB::diagnosis(QString ps_name)
{
	QStringList output;

	foreach(QString name, names(ps_name))
	{
		SqlQuery query = getQuery();
		query.exec("SELECT code FROM v_ngs_icd10 WHERE labornummer='" + name + "'");
		while(query.next())
		{
			QString diagnosis = query.value(0).toString().trimmed();
			if (diagnosis.isEmpty()) continue;

			if (output.contains(diagnosis)) continue;

			output << diagnosis;
		}
	}

	return output;
}

QStringList GenLabDB::anamnesis(QString ps_name)
{
	QStringList output;

	foreach(QString name, names(ps_name))
	{
		SqlQuery query = getQuery();
		query.exec("SELECT ANAMNESE FROM v_ngs_anamnese WHERE LABORNUMMER='" + name + "' AND ANAMNESE != 'leer'");
		while(query.next())
		{
			QString anamnesis = query.value(0).toString();
			anamnesis = anamnesis.replace(QChar::Null, ' ').trimmed(); //somehow GenLab contains Null characters
			if (anamnesis.isEmpty()) continue;

			if (output.contains(anamnesis)) continue;

			output << anamnesis;
		}
	}

	return output;
}

QStringList GenLabDB::tumorFraction(QString ps_name)
{
	QStringList output;

	foreach(QString name, names(ps_name))
	{
		SqlQuery query = getQuery();
		query.exec("SELECT tumoranteil FROM v_ngs_tumoranteil WHERE labornummer='" + name + "' AND tumoranteil IS NOT NULL");
		while(query.next())
		{
			QVariant fraction = query.value(0);
			if (fraction.isNull() || fraction.toDouble()==0.0) continue; //0% tumor is the default and makes no sense > skip it

			if (output.contains(fraction.toString())) continue;

			output << fraction.toString();
		}
	}

	return output;
}

QString GenLabDB::yearOfBirth(QString ps_name)
{
	foreach(QString name, names(ps_name))
	{
		SqlQuery query = getQuery();
		query.exec("SELECT Geburtsjahr FROM v_ngs_dates WHERE LABORNUMMER='" + name + "' AND Geburtsjahr IS NOT NULL");
		while(query.next())
		{
			return query.value(0).toString();
		}
	}

	return "";
}

QString GenLabDB::yearOfOrderEntry(QString ps_name)
{
	foreach(QString name, names(ps_name))
	{
		SqlQuery query = getQuery();
		query.exec("SELECT Datum_Auftragseingang FROM v_ngs_dates WHERE LABORNUMMER='" + name + "' AND Datum_Auftragseingang IS NOT NULL");
		while(query.next())
		{
			return query.value(0).toDateTime().toString("yyyy");
		}
	}

	return "";
}

QPair<QString, QString> GenLabDB::diseaseInfo(QString ps_name)
{
	QString group = "n/a";
	QString status = "n/a";

	foreach(QString name, names(ps_name))
	{
		SqlQuery query = getQuery();
		query.exec("SELECT krankheitsgruppe, patienttyp FROM v_krankheitsgruppe_pattyp WHERE labornummer='" + name + "'");
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
				else if (tmp=="Angehöriger gesund")
				{
					status = "Unaffected";
				}
				else if (tmp=="Angehöriger unklar")
				{
					status = "Unclear";
				}
			}

			if (group!="n/a" || status!="n/a") break;
		}
	}

	return qMakePair(group, status);
}


QString GenLabDB::sapID(QString ps_name)
{
	QString output;

	foreach(QString name, names(ps_name))
	{
		SqlQuery query = getQuery();
		query.exec("SELECT SAPID FROM v_ngs_patient_ids WHERE labornummer='" + name + "'");
		while (query.next())
		{
			QString id = query.value(0).toString().trimmed();
			if (!id.isEmpty())
			{
				output = id;
				break;
			}
		}
	}

	return output;
}

QList<SampleRelation> GenLabDB::relatives(QString ps_name)
{
	NGSD db;
	QList<SampleRelation> output;

	foreach(QString name, names(ps_name))
	{
		SqlQuery query = getQuery();
		query.exec("SELECT BEZIEHUNGSTEXT, Labornummer_Verwandter FROM v_ngs_duo WHERE Labornummer_Index='" + name + "'");
		while(query.next())
		{
			QByteArray relation = query.value(0).toByteArray().toUpper();
			if (relation=="VATER") relation = "parent-child";
			else if (relation=="MUTTER") relation = "parent-child";
			else if (relation=="SCHWESTER") relation = "siblings";
			else if (relation=="BRUDER") relation = "siblings";
			else if (relation=="ZWILLINGSSCHWESTER") relation = "twins";
			else if (relation=="ZWILLINGSBRUDER") relation = "twins";
			else if (relation=="COUSIN") relation = "cousins";
			else if (relation=="COUSINE") relation = "cousins";
			else THROW(ProgrammingException, "Unhandled sample relation '" + relation + "'!");

			QByteArray sample2 = query.value(1).toByteArray();
			if (sample2.contains('_'))
			{
				sample2 = sample2.split('_')[0];
			}
			//skip if sample is not (yet) contained in NGSD, e.g. a RNA that still has to be sequenced
			if (db.sampleId(sample2, false).isEmpty()) continue;

			QByteArray sample = ps_name.toLatin1();
			if (sample.contains('_'))
			{
				sample = sample.split('_')[0];
			}

			output << SampleRelation{sample2, relation, sample};
		}
	}

	return output;
}

QString GenLabDB::gender(QString ps_name)
{
	foreach(QString name, names(ps_name))
	{
		SqlQuery query = getQuery();
		query.exec("SELECT geschlecht FROM v_ngs_geschlecht WHERE labornummer='" + name + "'");
		if(query.next())
		{
			QString gender = query.value(0).toString().trimmed();
			if (gender=="1") return "female";
			if (gender=="2") return "male";
		}
	}

	return "";
}

QString GenLabDB::patientIdentifier(QString ps_name)
{
	QString output;

	foreach(QString name, names(ps_name))
	{
		SqlQuery query = getQuery();
		query.exec("SELECT GenlabID FROM v_ngs_patient_ids WHERE labornummer='" + name + "'");
		if(query.next())
		{
			QString id = query.value(0).toString().trimmed();
			if (id!="") output = id;
		}
	}

	return output;
}

QStringList GenLabDB::studies(QString ps_name)
{
	QStringList output;

	foreach(QString name, names(ps_name))
	{
		SqlQuery query = getQuery();
		query.exec("SELECT STUDIE FROM v_ngs_studie WHERE LABORNUMMER='" + name + "'");
		while (query.next())
		{
			QString study = query.value(0).toString().trimmed();
			if (study.isEmpty()) continue;
			if (!output.contains(study))
			{
				output << study;
			}
		}
	}

	output.sort();

	return output;
}

QList<int> GenLabDB::studySamples(QString study, QStringList& errors)
{
	QList<int> output;
	NGSD db;
	errors.clear();

	SqlQuery query = getQuery();
	query.exec("SELECT [LABORNUMMER],[STUDIE] FROM [genlab].[dbo].[v_ngs_studie] WHERE STUDIE='"+study+"'");
	while(query.next())
	{
		QString sample = query.value("LABORNUMMER").toString().replace("-", "").trimmed();
		if (sample.isEmpty()) continue;

		QString sample_id = db.sampleId(sample, false);
		if (sample_id.isEmpty())
		{
			errors << "Sample '" + sample + "' not found in NGSD!";
			continue;
		}

		QStringList ps_ids = db.getValues("SELECT ps.id FROM sample s, processed_sample ps WHERE s.id=" + sample_id + " AND ps.sample_id=s.id");
		if (ps_ids.isEmpty())
		{
			errors << "Sample '" + sample + "' has no processed samples in NGSD!";
			continue;
		}

		foreach(QString ps_id, ps_ids)
		{
			output << Helper::toInt(ps_id, "processed sample ID");
		}
	}

	return output;
}

QStringList GenLabDB::names(QString ps_name)
{
	QStringList output;
	output << ps_name;
	if (ps_name.contains("_"))
	{
		QString s_name = ps_name.split('_')[0];
		output << s_name;
	}
	return output;
}

