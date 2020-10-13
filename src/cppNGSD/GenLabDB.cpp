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

QList<Phenotype> GenLabDB::phenotypes(QString ps_name)
{
	QList<Phenotype> output;

	NGSD ngsd;

	QString s_name = (ps_name + "_").split('_')[0];
	foreach(QString name, QStringList() << ps_name << s_name)
	{
		SqlQuery query = getQuery();
		query.exec("SELECT code FROM v_ngs_hpo WHERE labornummer='" + name + "'");
		while(query.next())
		{
			QByteArray hpo_id = query.value(0).toByteArray().trimmed();
			if (hpo_id.isEmpty()) continue;

			Phenotype pheno = ngsd.phenotypeByAccession(hpo_id, false);
			if (pheno.name().isEmpty()) continue;
			if (output.contains(pheno)) continue;

			output << pheno;
		}
	}

	return output;
}

QStringList GenLabDB::orphanet(QString ps_name)
{
	QStringList output;

	QString s_name = (ps_name + "_").split('_')[0];
	foreach(QString name, QStringList() << ps_name << s_name)
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

	QString s_name = (ps_name + "_").split('_')[0];
	foreach(QString name, QStringList() << ps_name << s_name)
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

	QString s_name = (ps_name + "_").split('_')[0];
	foreach(QString name, QStringList() << ps_name << s_name)
	{
		SqlQuery query = getQuery();
		query.exec("SELECT ANAMNESE FROM v_ngs_anamnese WHERE LABORNUMMER='" + name + "' AND ANAMNESE != 'leer'");
		if(query.next())
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

	QString s_name = (ps_name + "_").split('_')[0];
	foreach(QString name, QStringList() << ps_name << s_name)
	{
		SqlQuery query = getQuery();
		query.exec("SELECT tumoranteil FROM v_ngs_tumoranteil WHERE labornummer='" + name + "' AND tumoranteil IS NOT NULL");
		while(query.next())
		{
			QString fraction = query.value(0).toString().trimmed();
			if (fraction.isEmpty()) continue;

			if (output.contains(fraction)) continue;

			output << fraction;
		}
	}

	return output;
}

QPair<QString, QString> GenLabDB::diseaseInfo(QString ps_name)
{
	QString group = "n/a";
	QString status = "n/a";

	QString s_name = (ps_name + "_").split('_')[0];
	foreach(QString name, QStringList() << ps_name << s_name)
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
				if (tmp=="Angehöriger gesund")
				{
					status = "Unaffected";
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

	QString s_name = (ps_name + "_").split('_')[0];
	foreach(QString name, QStringList() << ps_name << s_name)
	{
		SqlQuery query = getQuery();
		query.exec("SELECT identnr FROM v_ngs_sap WHERE labornummer='" + name + "'");
		if(query.next())
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

void GenLabDB::addMissingMetaDataToNGSD(QString ps_name, bool log, bool add_disease_group_status, bool add_disease_details)
{
	//init
	NGSD db;
	QString sample_id = db.sampleId(ps_name);
	SampleData sample_data = db.getSampleData(sample_id);

	//sample disease group/status
	if (add_disease_group_status)
	{
		bool modified_group = false;
		bool modified_status = false;
		QPair<QString, QString> disease_info = diseaseInfo(ps_name);
		if (disease_info.first!="n/a" && sample_data.disease_group=="n/a" && db.getEnum("sample", "disease_group").contains(disease_info.first))
		{
			sample_data.disease_group = disease_info.first;
			modified_group = true;
		}
		if (disease_info.second!="n/a" && sample_data.disease_status=="n/a")
		{
			sample_data.disease_status = disease_info.second;
			modified_status = true;
		}
		if (modified_group || modified_status)
		{
			db.setSampleDiseaseData(sample_id, sample_data.disease_group, sample_data.disease_status);
			if (log)
			{
				if (modified_group) Log::info(ps_name + ": Imported disease group from GenLab: " + sample_data.disease_group);
				if (modified_status) Log::info(ps_name + ": Imported disease status from GenLab: " + sample_data.disease_status);
			}
		}
	}

	//sample disease details
	if (add_disease_details)
	{
		QList<SampleDiseaseInfo> disease_details = db.getSampleDiseaseInfo(sample_id);
		QDateTime date = QDateTime::currentDateTime();
		QString user = "genlab_import";
		bool modified_details = false;
		foreach(QString text, anamnesis(ps_name))
		{
			if(addDiseaseInfoIfMissing("clinical phenotype (free text)", text, date, user, disease_details))
			{
				modified_details = true;
				if (log) Log::info(ps_name + ": Imported anamnesis from GenLab: " + text);
			}
		}
		foreach(Phenotype pheno, phenotypes(ps_name))
		{
			if(addDiseaseInfoIfMissing("HPO term id", pheno.accession(), date, user, disease_details))
			{
				modified_details = true;
				if (log) Log::info(ps_name + ": Imported HPO id from GenLab: " + pheno.accession());
			}
		}
		foreach(QString orpha, orphanet(ps_name))
		{
			if(addDiseaseInfoIfMissing("Orpha number", orpha, date, user, disease_details))
			{
				modified_details = true;
				if (log) Log::info(ps_name + ": Imported Orpha code from GenLab: " + orpha);
			}
		}
		foreach(QString icd10, diagnosis(ps_name))
		{
			if(addDiseaseInfoIfMissing("ICD10 code", icd10, date, user, disease_details))
			{
				modified_details = true;
				if (log) Log::info(ps_name + ": Imported ICD10 from GenLab: " + icd10);
			}
		}
		if (sample_data.is_tumor)
		{
			foreach(QString fraction, tumorFraction(ps_name))
			{
				if(addDiseaseInfoIfMissing("tumor fraction", fraction, date, user, disease_details))
				{
					modified_details = true;
					if (log) Log::info(ps_name + ": Imported tumor fraction from GenLab: " + fraction);
				}
			}
		}
		if (modified_details)
		{
			db.setSampleDiseaseInfo(sample_id, disease_details);
		}
	}
}

bool GenLabDB::addDiseaseInfoIfMissing(QString type, QString value, QDateTime date, QString user, QList<SampleDiseaseInfo>& disease_details)
{
	foreach(const SampleDiseaseInfo& entry, disease_details)
	{
		if (entry.type==type && entry.disease_info==value) return false;
	}

	SampleDiseaseInfo new_entry;
	new_entry.disease_info = value;
	new_entry.type = type;
	new_entry.user = user;
	new_entry.date = date;
	disease_details << new_entry;

	return true;
}

