#include "GPD.h"
#include "Helper.h"
#include "Exceptions.h"
#include <QSqlError>
#include <QSqlQuery>
#include <QVariant>
#include "Settings.h"
#include "Log.h"

GPD::GPD()
{
	//connect to DB
	db_ = QSqlDatabase::addDatabase("QMYSQL", "GPD_" + Helper::randomString(20));
	db_.setHostName(Settings::string("gpd_host"));
	db_.setDatabaseName(Settings::string("gpd_name"));
	db_.setUserName(Settings::string("gpd_user"));
	db_.setPassword(Settings::string("gpd_pass"));

	if (!db_.open())
	{
		THROW(DatabaseException, "Could not connect to the GPD database!");
	}

	Log::info("MYSQL openend  - name: " + db_.connectionName() + " valid: " + (db_.isValid() ? "yes" : "no"));
}

QVariant GPD::getValue(const QString& query, bool no_value_is_ok)
{
	QSqlQuery q = execute(query);

	q.next();
	if (q.size()==0)
	{
		if (no_value_is_ok)
		{
			return QVariant();
		}
		else
		{
			THROW(DatabaseException, "GPD single value query returned no value: " + query);
		}
	}
	if (q.size()>1)
	{
		THROW(DatabaseException, "GPD single value query returned several values: " + query);
	}

	return q.value(0);
}

QSqlQuery GPD::execute(const QString& query)
{
	QSqlQuery q(db_);
	if (!q.exec(query))
	{
		Log::info("MYSQL error  - name: " + db_.connectionName() + " error: " + q.lastError().text() + " query: " + query);

		THROW(DatabaseException, "GPD query error: " + q.lastError().text() + "\nname: " + db_.connectionName());
	}

	return q;
}

GPD::~GPD()
{
	Log::info("MYSQL closing  - name: " + db_.connectionName());
	db_.close();
}

int GPD::addColumn(VariantList& variants, QString name, QString description)
{
	variants.annotations().append(VariantAnnotationDescription(name, description));
	for (int i=0; i<variants.count(); ++i)
	{
		variants[i].annotations().append("");
	}

	return variants.annotations().count() - 1;
}

void GPD::annotate(VariantList& variants)
{	
	emit initProgress("GPD annotation", true);

	//remove all GPD-specific columns
	removeColumnIfPresent(variants, "GPD_gene", true);
	removeColumnIfPresent(variants, "GPD_var", true);

	//get required column indices
	int gene_idx = variants.annotationIndexByName("gene", true, true);
	int g_gen_idx = addColumn(variants, "GPD_gene", "GPD annotation of genes.");
	int g_var_idx = addColumn(variants, "GPD_var", "GPD annotation of variants.");

	//(re-)annotate the variants
	for (int i=0; i<variants.count(); ++i)
	{
		Variant& v = variants[i];

		QVariant id = getValue("SELECT id FROM gene WHERE hgnc_apprsymbol='" + v.annotations()[gene_idx] + "'");
		v.annotations()[g_gen_idx] = id.toByteArray();

		id = getValue("SELECT id FROM variant WHERE chr='" + v.chr().strNormalized(true) + "' AND start='"+ QString::number(v.start()) +"' AND end='"+ QString::number(v.end()) +"' AND ref='"+ v.ref() +"' AND obs='"+ v.obs() +"' AND build_id='1'");
		v.annotations()[g_var_idx] = id.toByteArray();

		emit updateProgress(100*i/variants.count());
	}
}

bool GPD::removeColumnIfPresent(VariantList& variants, QString name, bool exact_name_match)
{
	int index = variants.annotationIndexByName(name, exact_name_match, false);
	if (index==-1) return false;

	variants.removeAnnotation(index);
	return true;
}
