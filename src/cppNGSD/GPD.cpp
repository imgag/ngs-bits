#include "GPD.h"
#include "Helper.h"
#include "Exceptions.h"
#include <QSqlError>
#include <QSqlQuery>
#include <QVariant>
#include "Settings.h"

GPD::GPD()
{
	//connect to DB
	db_ = QSqlDatabase::addDatabase("QMYSQL", Helper::randomString(20));
	db_.setHostName(Settings::string("gpd_host"));
	db_.setDatabaseName(Settings::string("gpd_name"));
	db_.setUserName(Settings::string("gpd_user"));
	db_.setPassword(Settings::string("gpd_pass"));

	if (!db_.open())
	{
		THROW(DatabaseException, "Could not connect to the GPD database!");
	}
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


QVariantList GPD::getValues(const QString& query)
{
	QSqlQuery q = execute(query);

	QVariantList output;
	output.reserve(q.size());
	while(q.next())
	{
		output.append(q.value(0));
	}
	return output;
}

QSqlQuery GPD::execute(const QString& query)
{
	QSqlQuery q(db_);
	if (!q.exec(query))
	{
		THROW(DatabaseException, "GPD query error: " + q.lastError().text());
	}

	return q;
}

GPD::~GPD()
{
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

void GPD::annotateSomatic(VariantList& variants)
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

		//gene
		//get annotation categories and count for each gene
		QString gene_annotation(v.annotations()[gene_idx]);
		QStringList genes = gene_annotation.split(',');
		QMap<QString, int> at_map;
		QVariantList annotation_types;
		for (int j=0; j<genes.count();++j)
		{
			QVariant id = getValue("SELECT id FROM gene WHERE hgnc_apprsymbol='" + genes.at(j) + "'");

			if(!id.isNull())
			{
				//e.g. KRAS: SELECT at.name FROM annotation_table_fields as atf,annotation as a,annotation_type as at WHERE atf.`table` = 'gene' && atf.row = 12742 && atf.annotation_id = a.id && a.annotation_type_id = `at`.id;
				QString sql = "SELECT at.name ";
				sql += "FROM annotation_table_fields as atf,annotation as a,annotation_type as at ";
				sql += "WHERE atf.`table` = 'gene' AND atf.row = " + id.toString() + " AND atf.annotation_id = a.id AND a.annotation_type_id = `at`.id;";
				annotation_types = getValues(sql);
			}

			foreach(const QVariant& at, annotation_types)
			{
				if(!at_map.contains(at.toString()))	at_map[at.toString()] = 0;
				++at_map[at.toString()];
			}
		}

		QStringList annotations;
		foreach(QString key, at_map.keys())
		{
			annotations.append("[" + key + " ] x" + QString::number(at_map[key]) + ",");
		}
		v.annotations()[g_gen_idx] = annotations.join("").toStdString().c_str();

		//variant
		//get annotation categories and count for specific variant
		QVariant id = getValue("SELECT id FROM variant WHERE chr='" + v.chr().strNormalized(true) + "' AND start='"+ QString::number(v.start()) +"' AND end='"+ QString::number(v.end()) +"' AND ref='"+ v.ref() +"' AND obs='"+ v.obs() +"' AND build_id='1'");
		at_map.clear();
		annotation_types.clear();
		if(!id.isNull())
		{
			//SELECT at.name FROM annotation_table_fields as atf,annotation as a,annotation_type as at WHERE atf.`table` = 'variant' && atf.row = 210286 && atf.annotation_id = a.id && a.annotation_type_id = `at`.id;
			QString sql = "SELECT at.name ";
			sql += "FROM annotation_table_fields as atf,annotation as a,annotation_type as at ";
			sql += "WHERE atf.`table` = 'variant' && atf.row = " + id.toString() + " && atf.annotation_id = a.id && a.annotation_type_id = `at`.id";
			annotation_types = getValues(sql);

			foreach(const QVariant& at, annotation_types)
			{
				if(!at_map.contains(at.toString()))	at_map[at.toString()] = 0;
				++at_map[at.toString()];
			}
		}

		annotations.clear();
		foreach(QString key, at_map.keys())
		{
			annotations.append("[" + key + " ] x" + QString::number(at_map[key]) + ",");
		}
		v.annotations()[g_var_idx] = annotations.join("").toStdString().c_str();

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
