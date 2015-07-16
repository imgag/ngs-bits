#include "NGSD.h"
#include "Exceptions.h"
#include "Helper.h"
#include "Log.h"
#include <QSqlError>
#include <QSqlQuery>
#include <QFileInfo>
#include "Settings.h"


NGSD::NGSD()
{
	//connect to DB
	db_ = QSqlDatabase::addDatabase("QMYSQL", Helper::randomString(20));
	db_.setHostName(Settings::string("ngsd_host"));
	db_.setDatabaseName(Settings::string("ngsd_name"));
	db_.setUserName(Settings::string("ngsd_user"));
	db_.setPassword(Settings::string("ngsd_pass"));

	if (!db_.open())
	{
		THROW(DatabaseException, "Could not connect to the NGSD database!");
	}
}

QVariant NGSD::getValue(const QString& query, bool no_value_is_ok)
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
			THROW(DatabaseException, "NGSD single value query returned no value: " + query);
		}
	}
	if (q.size()>1)
	{
		THROW(DatabaseException, "NGSD single value query returned several values: " + query);
	}

	return q.value(0);
}

QVariantList NGSD::getValues(const QString& query)
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

QSqlQuery NGSD::execute(const QString& query)
{
	QSqlQuery q(db_);
	if (!q.exec(query))
	{
		THROW(DatabaseException, "NGSD query error: " + q.lastError().text());
	}

	return q;
}

int NGSD::addColumn(VariantList& variants, QString name, QString description)
{
	variants.annotations().append(VariantAnnotationDescription(name, description));
	for (int i=0; i<variants.count(); ++i)
	{
		variants[i].annotations().append("");
	}

	return variants.annotations().count() - 1;
}

bool NGSD::removeColumnIfPresent(VariantList& variants, QString name, bool exact_name_match)
{
	int index = variants.annotationIndexByName(name, exact_name_match, false);
	if (index==-1) return false;

	variants.removeAnnotation(index);
	return true;
}

NGSD::~NGSD()
{
	db_.close();
}

QString NGSD::getExternalSampleName(const QString& filename)
{
	//query
	QVariant value = getValue("SELECT name_external FROM sample WHERE name='" + sampleName(filename) + "'");

	//output
	if (value.isNull() || !value.isValid() || value.toString().trimmed().isEmpty())
	{
		return "n/a";
	}
	return value.toString();
}

QString NGSD::getProcessingSystem(const QString& filename, SystemType type)
{
	//get sample ID
	QString s_id = getValue("SELECT id FROM sample WHERE name='" + sampleName(filename) + "'").toString();

	//get processing system ID
	QString sys_id = getValue("SELECT processing_system_id FROM processed_sample WHERE sample_id='" + s_id + "' AND process_id='" + processedSampleNumber(filename) + "'").toString();

	QString what;
	if (type==SHORT)
	{
		what = "name_short";
	}
	else if (type==LONG)
	{
		what = "name";
	}
	else if (type==BOTH)
	{
		what = "CONCAT(name, ' (', name_short, ')')";
	}
	else
	{
		THROW(ProgrammingException, "Unknown SystemType '" + QString::number(type) + "'!");
	}
	return getValue("SELECT " + what + " FROM processing_system WHERE id='" + sys_id + "'").toString();
}

QPair<QString, QString> NGSD::getValidationStatus(const QString& filename, const Variant& variant)
{
	//get sample ID
	QString s_id = getValue("SELECT id FROM sample WHERE name='" + sampleName(filename) + "'").toString();

	//get processed sample ID
	QString ps_id = getValue("SELECT id FROM processed_sample WHERE sample_id='" + s_id + "' AND process_id='" + processedSampleNumber(filename) + "'").toString();

	//get variant ID
	QString v_id = getValue("SELECT id FROM variant WHERE chr='"+variant.chr().str()+"' AND start='"+QString::number(variant.start())+"' AND end='"+QString::number(variant.end())+"' AND ref='"+variant.ref()+"' AND obs='"+variant.obs()+"'", false).toString();

	//get validation info
	QSqlQuery result = execute("SELECT validated, validation_comment FROM detected_variant WHERE processed_sample_id='" + ps_id + "' AND variant_id='" + v_id + "'");
	result.next();
	return qMakePair(result.value(0).toString().trimmed(), result.value(1).toString().trimmed());
}

QCCollection NGSD::getQCData(const QString& filename)
{
	//get sample ids
	QString s_id = getValue("SELECT id FROM sample WHERE name='" + sampleName(filename) + "'").toString();
	QString ps_number = processedSampleNumber(filename);
	QString ps_id = getValue("SELECT id FROM processed_sample WHERE sample_id='" + s_id + "' AND process_id='" + ps_number + "'").toString();

	//get NGSO data
	QSqlQuery q = execute("SELECT n.name, nm.value, n.description, n.ngso_id FROM nm_processed_sample_ngso as nm, ngso as n WHERE nm.processed_sample_id='" + ps_id + "' AND nm.ngso_id=n.id");
	QCCollection output;
	while(q.next())
	{
		output.insert(QCValue(q.value(0).toString(), q.value(1).toString(), q.value(2).toString(), q.value(3).toString()));
	}

	//get KASP data
	QSqlQuery q2 = execute("SELECT random_error_prob FROM kasp_status WHERE processed_sample_id='" + ps_id + "'");
	QString value = "n/a";
	if (q2.size()>0)
	{
		q2.next();
		float numeric_value = 100.0 * q2.value(0).toFloat();
		if (numeric_value>1.0)
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

QVector<double> NGSD::getQCValues(const QString& accession, const QString& filename)
{
	//get sample ID
	QString s_id = getValue("SELECT id FROM sample WHERE name='" + sampleName(filename) + "'").toString();

	//get processing system ID
	QString sys_id = getValue("SELECT processing_system_id FROM processed_sample WHERE sample_id='" + s_id + "' AND process_id='" + processedSampleNumber(filename) + "'").toString();

	//get NGSO id
	QString ngso_id = getValue("SELECT id FROM ngso WHERE ngso_id='" + accession + "'").toString();

	//get QC data
	QSqlQuery q = execute("SELECT nm.value FROM nm_processed_sample_ngso as nm, processed_sample as ps WHERE ps.processing_system_id='" + sys_id + "' AND nm.ngso_id='" + ngso_id + "' AND nm.processed_sample_id=ps.id ");

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

void NGSD::annotate(VariantList& variants, QString filename, QString ref_file, bool add_comment)
{	
	//open refererence genome file
	FastaFileIndex reference(ref_file);

	//get sample ids
	QString s_id = getValue("SELECT id FROM sample WHERE name='" + sampleName(filename) + "'").toString();
	QString ps_number = processedSampleNumber(filename);
	QString ps_id = getValue("SELECT id FROM processed_sample WHERE sample_id='" + s_id + "' AND process_id='" + ps_number + "'").toString();
	QString sys_id = getValue("SELECT processing_system_id FROM processed_sample WHERE sample_id='" + s_id + "' AND process_id='" + ps_number + "'").toString();

	//check if we could determine the sample
	bool found_in_db = true;
	if (s_id=="" || ps_number=="" || ps_id=="" || sys_id=="")
	{
		Log::warn("Could not find processed sample in NGSD by name '" + filename + "'. Annotation will be incomplete because processing system could not be determined!");
		found_in_db = false;
	}

	//get sample ids that have processed samples with the same processing system (not same sample, variants imported, same processing system, good quality of sample, not tumor)
	QSet<int> sys_sample_ids;
	QVariantList ps_ids_var = getValues("SELECT DISTINCT s.id FROM processed_sample as ps, sample s WHERE ps.processing_system_id='" + sys_id + "' AND ps.sample_id=s.id AND s.tumor='0' AND s.quality='good' AND s.id!='" + s_id + "' AND (SELECT count(id) FROM detected_variant as dv WHERE dv.processed_sample_id = ps.id)>0");
	foreach(const QVariant& var, ps_ids_var)
	{
		sys_sample_ids.insert(var.toInt());
	}

	//remove all NGSD-specific columns
	QList<VariantAnnotationDescription> descs = variants.annotations();
	foreach(const VariantAnnotationDescription& desc, descs)
	{
		if (desc.name().startsWith("ihdb_"))
		{
			removeColumnIfPresent(variants, desc.name(), true);
		}
	}
	removeColumnIfPresent(variants, "classification", true);
	removeColumnIfPresent(variants, "validated", true);
	removeColumnIfPresent(variants, "comment", true);

	//get required column indices
	QString num_samples = QString::number(sys_sample_ids.count());
	int ihdb_hom_idx = addColumn(variants, "ihdb_hom", "Homozygous variant counts in NGSD for the same processing system (" + num_samples + " samples).");
	int ihdb_het_idx = addColumn(variants, "ihdb_het", "Heterozyous variant counts in NGSD for the same processing system (" + num_samples + " samples).");
	int ihdb_wt_idx  = addColumn(variants, "ihdb_wt", "Wildtype variant counts in NGSD for the same processing system (" + num_samples + " samples).");
	int ihdb_all_hom_idx = addColumn(variants, "ihdb_allsys_hom", "Homozygous variant counts in NGSD independent of the processing system.");
	int ihdb_all_het_idx =  addColumn(variants, "ihdb_allsys_het", "Heterozygous variant counts in NGSD independent of the processing system.");
	int class_idx = addColumn(variants, "classification", "VUS classification from the NGSD.");
	int valid_idx = addColumn(variants, "validated", "Validation information from the NGSD.");
	if (add_comment) addColumn(variants, "comment", "Comments from the NGSD.");
	int comment_idx = variants.annotationIndexByName("comment", true, false);

	//(re-)annotate the variants
	for (int i=0; i<variants.count(); ++i)
	{
		Variant& v = variants[i];

		QSqlQuery query;
		if (v.isSNV())
		{
			query = execute("SELECT dv.processed_sample_id, dv.genotype, dv.validated, s.id, dv.comment FROM detected_variant as dv, variant as v, processed_sample ps, sample s WHERE dv.processed_sample_id=ps.id and ps.sample_id=s.id and dv.variant_id=v.id AND s.tumor='0' AND v.chr='"+v.chr().str()+"' AND v.start='"+QString::number(v.start())+"' AND v.end='"+QString::number(v.end())+"' AND v.ref='"+v.ref()+"' AND v.obs='"+v.obs()+"'");
		}
		else
		{
			QPair<int, int> indel_region = Variant::indelRegion(v.chr(), v.start(), v.end(), v.ref(), v.obs(), reference);
			query = execute("SELECT dv.processed_sample_id, dv.genotype, dv.validated, s.id, dv.comment FROM detected_variant as dv, variant as v, processed_sample ps, sample s WHERE dv.processed_sample_id=ps.id and ps.sample_id=s.id and dv.variant_id=v.id AND s.tumor='0' AND v.chr='"+v.chr().str()+"' AND v.start>='"+QString::number(indel_region.first-1)+"' AND v.end<='"+ QString::number(indel_region.second+1)+"' AND v.ref='"+v.ref()+"' AND v.obs='"+v.obs()+"'");
		}

		//process variants
		int allsys_hom_count = 0;
		int allsys_het_count = 0;
		int sys_hom_count = 0;
		int sys_het_count = 0;
		int tp_count = 0;
		int fp_count = 0;
		QByteArray validated = "n/a";
		QByteArray comment = "";
		QStringList comment_others;
		QSet<QString> processed_ps_ids;
		QSet<QString> processed_s_ids;
		while(query.next())
		{
			QByteArray current_ps_id = query.value(0).toByteArray();
			QByteArray current_geno = query.value(1).toByteArray();
			QByteArray current_validation = query.value(2).toByteArray().replace("true positive", "TP").replace("false positive", "FP");
			QByteArray current_sample = query.value(3).toByteArray();
			QByteArray current_comment = query.value(4).toByteArray();

			//skip already seen processed samples (there could be several variants because of indel window, but we want to process only one)
			if (processed_ps_ids.contains(current_ps_id)) continue;
			processed_ps_ids.insert(current_ps_id);

			//set infos of the current processed sample
			if(current_ps_id==ps_id)
			{
				validated = current_validation;
				comment = current_comment;
			}
			else
			{
				if (current_comment!="")
				{
					comment_others.append(current_comment);
				}
			}

			//skip the current sample for general statistics
			if (current_sample==s_id) continue;

			//skip already seen samples for general statistics (there could be several processings of the same sample because of different processing systems or because of experment repeats due to quality issues)
			if (processed_s_ids.contains(current_sample)) continue;
			processed_s_ids.insert(current_sample);

			//genotype all processing systems
			if (current_geno=="hom")
			{
				++allsys_hom_count;
			}
			else if (current_geno=="het")
			{
				++allsys_het_count;
			}

			//validation
			if (current_validation=="TP")
			{
				++tp_count;
			}
			else if (current_validation=="FP")
			{
				++fp_count;
			}

			//genotype specific processing system
			if (sys_sample_ids.contains(current_sample.toInt()))
			{
				if (current_geno=="hom")
				{
					++sys_hom_count;
				}
				else if (current_geno=="het")
				{
					++sys_het_count;
				}
			}
		}

		//update variant data
		v.annotations()[ihdb_all_hom_idx] = QByteArray::number(allsys_hom_count);
		v.annotations()[ihdb_all_het_idx] = QByteArray::number(allsys_het_count);
		QByteArray classification = getValue("SELECT vus FROM variant WHERE chr='"+v.chr().str()+"' AND start='"+QString::number(v.start())+"' AND end='"+QString::number(v.end())+"' AND ref='"+v.ref()+"' AND obs='"+v.obs()+"'").toByteArray();
		if (classification=="") classification="n/a";
		v.annotations()[class_idx] = classification;
		if (found_in_db)
		{
			v.annotations()[ihdb_hom_idx] = QByteArray::number((double)sys_hom_count / sys_sample_ids.count(), 'f', 4);
			v.annotations()[ihdb_het_idx] =  QByteArray::number((double)sys_het_count / sys_sample_ids.count(), 'f', 4);
			v.annotations()[ihdb_wt_idx] =  QByteArray::number((double)(sys_sample_ids.count() - sys_hom_count - sys_het_count) / sys_sample_ids.count(), 'f', 4);

			if (comment_others.count()>0)
			{
				if (comment=="") comment = "n/a";
				comment += " (" + comment_others.join(";") + ")";
			}
			if (tp_count>0 || fp_count>0)
			{
				validated += " (";
				if(tp_count>0) validated += QString::number(tp_count) + "xTP ";
				if(fp_count>0) validated += QString::number(fp_count) + "xFP ";
				validated += ")";
				validated.replace(" )", ")");
			}
			v.annotations()[valid_idx] = validated;
			if (add_comment)
			{
				v.annotations()[comment_idx] = comment;
			}
		}
		else
		{
			v.annotations()[ihdb_hom_idx] = "n/a";
			v.annotations()[ihdb_het_idx] = "n/a";
			v.annotations()[ihdb_wt_idx] = "n/a";
			v.annotations()[valid_idx] = "n/a";
			if (add_comment)
			{
				v.annotations()[comment_idx] = "n/a";
			}
		}
	}
}

void NGSD::annotateSomatic(VariantList& variants, QString filename, QString ref_file)
{
	//open refererence genome file
	FastaFileIndex reference(ref_file);

	//get sample ids
	QStringList samples = filename.split('-');
	QString ts_id = getValue("SELECT id FROM sample WHERE name='" + sampleName(samples[0]) + "'").toString();

	//check if we could determine the sample
	if (ts_id=="")
	{
		Log::warn("Could not find processed sample in NGSD by name '" + sampleName(samples[0]) + "'. Annotation will be incomplete because processing system could not be determined!");
	}

	//remove all NGSD-specific columns
	QList<VariantAnnotationDescription> descs = variants.annotations();
	foreach(const VariantAnnotationDescription& desc, descs)
	{
		if (desc.name().startsWith("som_ihdb"))
		{
			removeColumnIfPresent(variants, desc.name(), true);
		}
	}

	//get required column indices
	int som_ihdb_c_idx = addColumn(variants, "som_ihdb_c", "Somatic variant count within NGSD.");
	int som_ihdb_p_idx = addColumn(variants, "som_ihdb_p", "Projects with somatic variant in NGSD.");
	//(re-)annotate the variants
	for (int i=0; i<variants.count(); ++i)
	{
		Variant& v = variants[i];

		QSqlQuery query;
		if (v.isSNV())
		{
			query = execute("SELECT s.id, dsv.processed_sample_id_tumor, p.name FROM detected_somatic_variant as dsv, variant as v, processed_sample ps, sample as s, project as p WHERE ps.project_id=p.id AND dsv.processed_sample_id_tumor=ps.id and dsv.variant_id=v.id AND  ps.sample_id=s.id  AND s.tumor='1' AND v.chr='"+v.chr().str()+"' AND v.start='"+QString::number(v.start())+"' AND v.end='"+QString::number(v.end())+"' AND v.ref='"+v.ref()+"' AND v.obs='"+v.obs()+"'");
		}
		else
		{
			QPair<int, int> indel_region = Variant::indelRegion(v.chr(), v.start(), v.end(), v.ref(), v.obs(), reference);
			query = execute("SELECT s.id, dsv.processed_sample_id_tumor, p.name FROM project as p, detected_somatic_variant as dsv, variant as v, processed_sample ps, sample as s WHERE ps.project_id=p.id AND dsv.processed_sample_id_tumor=ps.id and dsv.variant_id=v.id AND  ps.sample_id=s.id  AND s.tumor='1' AND v.chr='"+v.chr().str()+"' AND v.start>='"+QString::number(indel_region.first)+"' AND v.end<='"+ QString::number(indel_region.second)+"' AND v.ref='"+v.ref()+"' AND v.obs='"+v.obs()+"'");
		}

		//process variants
		QMap<QByteArray, int> project_map;
		QSet<QByteArray> processed_ps_ids;
		QSet<QByteArray> processed_s_ids;
		while(query.next())
		{
			QByteArray current_sample = query.value(0).toByteArray();
			QByteArray current_ps_id = query.value(1).toByteArray();
			QByteArray current_project = query.value(2).toByteArray();

			//skip already seen processed samples (there could be several variants because of indel window, but we want to process only one)
			if (processed_ps_ids.contains(current_ps_id)) continue;
			processed_ps_ids.insert(current_ps_id);

			//skip the current sample for general statistics
			if (current_sample==ts_id) continue;

			//skip already seen samples for general statistics (there could be several processings of the same sample because of different processing systems or because of experment repeats due to quality issues)
			if (processed_s_ids.contains(current_sample)) continue;
			processed_s_ids.insert(current_sample);

			// count
			if(!project_map.contains(current_project))	project_map.insert(current_project,0);
			++project_map[current_project];
		}

		QByteArray somatic_projects;
		int somatic_count = 0;
		QMap<QByteArray, int>::const_iterator j = project_map.constBegin();
		while(j!=project_map.constEnd())
		{
			somatic_count += j.value();
			somatic_projects += j.key() + ",";
			++j;
		}
		v.annotations()[som_ihdb_c_idx] = QByteArray::number(somatic_count);
		v.annotations()[som_ihdb_p_idx] = somatic_projects;
	}
}


void NGSD::setValidationStatus(const QString& filename, const Variant& variant, const QString& status, const QString& comment)
{
	//get sample ID
	QString s_id = getValue("SELECT id FROM sample WHERE name='" + sampleName(filename) + "'").toString();

	//get processed sample ID
	QString ps_id = getValue("SELECT id FROM processed_sample WHERE sample_id='" + s_id + "' AND process_id='" + processedSampleNumber(filename) + "'").toString();

	//get variant ID
	QString v_id = getValue("SELECT id FROM variant WHERE chr='"+variant.chr().str()+"' AND start='"+QString::number(variant.start())+"' AND end='"+QString::number(variant.end())+"' AND ref='"+variant.ref()+"' AND obs='"+variant.obs()+"'", false).toString();

	//set
	execute("UPDATE detected_variant SET validated='" + status + "',validation_comment='" + comment + "' WHERE processed_sample_id='" + ps_id + "' AND variant_id='" + v_id + "'");
}

void NGSD::setClassification(const Variant& variant, const QString& classification)
{
	//get user ID
	QString user_id = getValue("SELECT id FROM user WHERE user_id='" + Helper::userName() + "'", false).toString();

	//get variant ID
	QString v_id = getValue("SELECT id FROM variant WHERE chr='"+variant.chr().str()+"' AND start='"+QString::number(variant.start())+"' AND end='"+QString::number(variant.end())+"' AND ref='"+variant.ref()+"' AND obs='"+variant.obs()+"'", false).toString();

	//get variant ID
	execute("UPDATE variant SET vus='" + classification + "', vus_user='" + user_id + "', vus_date=CURRENT_TIMESTAMP WHERE id='"+v_id+"'");
}

QString NGSD::comment(const QString& filename, const Variant& variant)
{
	//get sample ID
	QString s_id = getValue("SELECT id FROM sample WHERE name='" + sampleName(filename) + "'").toString();

	//get processed sample ID
	QString ps_id = getValue("SELECT id FROM processed_sample WHERE sample_id='" + s_id + "' AND process_id='" + processedSampleNumber(filename) + "'").toString();

	//get variant ID
	QString v_id = getValue("SELECT id FROM variant WHERE chr='"+variant.chr().str()+"' AND start='"+QString::number(variant.start())+"' AND end='"+QString::number(variant.end())+"' AND ref='"+variant.ref()+"' AND obs='"+variant.obs()+"'", false).toString();

	//get comment
	return getValue("SELECT comment FROM detected_variant WHERE processed_sample_id='" + ps_id + "' AND variant_id='" + v_id + "'").toString();
}

QString NGSD::url(const QString& filename, const Variant& variant)
{
	//get sample ID
	QString s_id = getValue("SELECT id FROM sample WHERE name='" + sampleName(filename) + "'").toString();

	//get processed sample ID
	QString ps_id = getValue("SELECT id FROM processed_sample WHERE sample_id='" + s_id + "' AND process_id='" + processedSampleNumber(filename) + "'").toString();

	//get variant ID
	QString v_id = getValue("SELECT id FROM variant WHERE chr='"+variant.chr().str()+"' AND start='"+QString::number(variant.start())+"' AND end='"+QString::number(variant.end())+"' AND ref='"+variant.ref()+"' AND obs='"+variant.obs()+"'").toString();

	//get detected variant ID
	QString dv_id = getValue("SELECT id FROM detected_variant WHERE processed_sample_id='" + ps_id + "' AND variant_id='" + v_id + "'", false).toString();

	return Settings::string("NGSD")+"/variants/view/" + dv_id;
}

QString NGSD::url(const QString& filename)
{
	//get sample ID
	QString s_id = getValue("SELECT id FROM sample WHERE name='" + sampleName(filename) + "'").toString();

	//get processed sample ID
	QString ps_id = getValue("SELECT id FROM processed_sample WHERE sample_id='" + s_id + "' AND process_id='" + processedSampleNumber(filename) + "'", false).toString();

	//get url
	return Settings::string("NGSD")+"/processedsamples/view/" + ps_id;
}

QString NGSD::urlSearch(const QString& search_term)
{
	return Settings::string("NGSD")+"/search/processSearch/search_term="+search_term;
}

void NGSD::setComment(const QString& filename, const Variant& variant, const QString& text)
{
	//get sample ID
	QString s_id = getValue("SELECT id FROM sample WHERE name='" + sampleName(filename) + "'").toString();

	//get processed sample ID
	QString ps_id = getValue("SELECT id FROM processed_sample WHERE sample_id='" + s_id + "' AND process_id='" + processedSampleNumber(filename) + "'").toString();

	//get variant ID
	QString v_id = getValue("SELECT id FROM variant WHERE chr='"+variant.chr().str()+"' AND start='"+QString::number(variant.start())+"' AND end='"+QString::number(variant.end())+"' AND ref='"+variant.ref()+"' AND obs='"+variant.obs()+"'", false).toString();

	//set
	execute("UPDATE detected_variant SET comment='" + text + "' WHERE processed_sample_id='" + ps_id + "' AND variant_id='" + v_id + "'");
}

void NGSD::setReport(const QString& filename, const Variant& variant, bool in_report)
{
	//get sample ID
	QString s_id = getValue("SELECT id FROM sample WHERE name='" + sampleName(filename) + "'").toString();

	//get processed sample ID
	QString ps_id = getValue("SELECT id FROM processed_sample WHERE sample_id='" + s_id + "' AND process_id='" + processedSampleNumber(filename) + "'").toString();

	//get variant ID
	QString v_id = getValue("SELECT id FROM variant WHERE chr='"+variant.chr().str()+"' AND start='"+QString::number(variant.start())+"' AND end='"+QString::number(variant.end())+"' AND ref='"+variant.ref()+"' AND obs='"+variant.obs()+"'", false).toString();

	//set
	execute("UPDATE detected_variant SET report=" + QString(in_report ? "1" : "0" ) + " WHERE processed_sample_id='" + ps_id + "' AND variant_id='" + v_id + "'");
}

QStringList NGSD::getEnum(QString table, QString column)
{
	//check cache
	static QMap<QString, QStringList> cache;
	QString hash = table+"."+column;
	if (cache.contains(hash))
	{
		return cache.value(hash);
	}

	//DB query
	QSqlQuery q = execute("DESCRIBE "+table+" "+column);
	while (q.next())
	{
		QString type = q.value(1).toString();
		type.replace("'", "");
		type.replace("enum(", "");
		type.replace(")", "");
		cache[hash] = type.split(",");
		return cache[hash];
	}

	THROW(ProgrammingException, "Could not determine enum values of column '"+column+"' in table '"+table+"'!");
}

QStringList NGSD::getDiagnosticStatus(const QString& filename)
{
	//get sample ID
	QString s_id = getValue("SELECT id FROM sample WHERE name='" + sampleName(filename) + "'").toString();

	//get processed sample ID
	QString ps_id = getValue("SELECT id FROM processed_sample WHERE sample_id='" + s_id + "' AND process_id='" + processedSampleNumber(filename) + "'").toString();
	if (ps_id=="") return QStringList();

	//get status
	QSqlQuery q = execute("SELECT s.status, u.name, s.date, s.outcome FROM diag_status as s, user as u WHERE s.processed_sample_id='" + ps_id +  "' AND s.user_id=u.id");
	if (q.size()==0) return (QStringList() << "n/a" << "n/a" << "n/a" << "n/a");
	q.next();
	return (QStringList() << q.value(0).toString() << q.value(1).toString() << q.value(2).toString().replace('T', ' ') << q.value(3).toString());
}

void NGSD::setDiagnosticStatus(const QString& filename, QString status)
{
	//get sample ID
	QString sample_name = sampleName(filename);
	QString s_id = getValue("SELECT id FROM sample WHERE name='" + sample_name + "'").toString();

	//get processed sample ID
	QString ps_num = processedSampleNumber(filename);
	QString ps_id = getValue("SELECT id FROM processed_sample WHERE sample_id='" + s_id + "' AND process_id='" + ps_num + "'").toString();

	//get user ID
	QString user_id = getValue("SELECT id FROM user WHERE user_id='" + Helper::userName() + "'", false).toString();

	//update status
	execute("INSERT INTO diag_status (processed_sample_id, status, user_id) VALUES ("+ps_id+",'"+status+"', "+user_id+") ON DUPLICATE KEY UPDATE status='"+status+"',user_id="+user_id+"");

	//add new processed sample if scheduled for repetition
	if (status.startsWith("repeat"))
	{
		QString next_ps_num = getValue("SELECT MAX(process_id)+1 FROM processed_sample WHERE sample_id=" + s_id).toString();
		QSqlQuery query = execute("SELECT mid1_i7, mid2_i5, operator_id, processing_system_id, project_id, molarity FROM processed_sample WHERE id=" + ps_id);
		query.next();
		QString mid1 = query.value(0).toString();
		if (mid1=="0") mid1="NULL";
		QString mid2 = query.value(1).toString();
		if (mid2=="0") mid2="NULL";
		QString op_id = query.value(2).toString();
		if (op_id=="0") op_id="NULL";
		QString sys_id = query.value(3).toString();
		QString proj_id = query.value(4).toString();
		QString molarity = query.value(5).toString();
		if (molarity=="0") molarity="NULL";
		QString user_name = getValue("SELECT name FROM user WHERE id='" + user_id + "'").toString();
		QString comment = user_name + " requested re-sequencing (" + status + ") of sample " + sample_name + "_" + ps_num.rightJustified(2, '0') + " on " + Helper::dateTime("dd.MM.yyyy hh:mm:ss");
		if (status=="repeat sequencing only")
		{
			execute("INSERT INTO processed_sample (sample_id, process_id, mid1_i7, mid2_i5, operator_id, processing_system_id, comment, project_id, molarity, status) VALUES ("+ s_id +","+ next_ps_num +","+ mid1 +","+ mid2 +","+ op_id +","+ sys_id +",'"+ comment +"',"+ proj_id +","+ molarity +",'todo')");
		}
		else if (status=="repeat library and sequencing")
		{
			execute("INSERT INTO processed_sample (sample_id, process_id, operator_id, processing_system_id, comment, project_id, status) VALUES ("+ s_id +","+ next_ps_num +","+ op_id +","+ sys_id +",'"+ comment +"',"+ proj_id +",'todo')");
		}
		else
		{
			THROW(ProgrammingException, "Unknown diagnostic status '" + status +"!'");
		}
	}
}

void NGSD::setReportOutcome(const QString& filename, QString outcome)
{
	//get sample ID
	QString sample_name = sampleName(filename);
	QString s_id = getValue("SELECT id FROM sample WHERE name='" + sample_name + "'").toString();

	//get processed sample ID
	QString ps_num = processedSampleNumber(filename);
	QString ps_id = getValue("SELECT id FROM processed_sample WHERE sample_id='" + s_id + "' AND process_id='" + ps_num + "'").toString();

	//get user ID
	QString user_id = getValue("SELECT id FROM user WHERE user_id='" + Helper::userName() + "'", false).toString();

	//update status
	execute("INSERT INTO diag_status (processed_sample_id, status, user_id, outcome) VALUES ("+ps_id+",'pending',"+user_id+",'"+outcome+"') ON DUPLICATE KEY UPDATE user_id="+user_id+",outcome='"+outcome+"'");
}

QString NGSD::sampleName(const QString& filename)
{
	QStringList parts = QFileInfo(filename).baseName().append('_').split('_');

	return parts[0];
}

QString NGSD::processedSampleNumber(const QString& filename)
{
	QStringList parts = QFileInfo(filename).baseName().append('_').split('_');

	bool ok = false;
	int ps_num = parts[1].toInt(&ok);
	if (!ok) return "";

	return QString::number(ps_num);
}

