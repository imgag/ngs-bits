#include "NGSD.h"
#include "Exceptions.h"
#include "Helper.h"
#include "Log.h"
#include <QFileInfo>
#include <QPair>
#include "Settings.h"

NGSD::NGSD(bool test_db)
	: db_used_externally_as_static_(false)
	, test_db_(test_db)
{
	db_.reset(new QSqlDatabase(QSqlDatabase::addDatabase("QMYSQL", "NGSD_" + Helper::randomString(20))));

	//connect to DB
	QString prefix = "ngsd";
	if (test_db_) prefix += "_test";
	db_->setHostName(Settings::string(prefix + "_host"));
	db_->setPort(Settings::integer(prefix + "_port"));
	db_->setDatabaseName(Settings::string(prefix + "_name"));
	db_->setUserName(Settings::string(prefix + "_user"));
	db_->setPassword(Settings::string(prefix + "_pass"));
	if (!db_->open())
	{
		THROW(DatabaseException, "Could not connect to the NGSD database!");
	}

	//prepare queries for speed-up
	q_approved_.reset(new SqlQuery(getQuery()));
	q_approved_->prepare("SELECT DISTINCT g.symbol FROM gene g, gene_transcript gt WHERE g.id=gt.gene_id AND g.chromosome=:1 AND gt.start_coding IS NOT NULL AND ((gt.start_coding>=:2 AND gt.start_coding<=:3) OR (:4>=gt.start_coding AND :5<=gt.end_coding)) ORDER BY g.symbol");
}

QString NGSD::userId()
{
	QString user_name = Helper::userName();
	QString user_id = getValue("SELECT id FROM user WHERE user_id='" + user_name + "'", true).toString();
	if (user_id=="")
	{
		THROW(DatabaseException, "Could not determine NGSD user ID for user name '" + user_name + "! Do you have an NGSD user account?");
	}

	return user_id;
}

QString NGSD::sampleName(const QString& filename, bool throw_if_fails)
{
	QString basename = QFileInfo(filename).baseName();
	QStringList parts = basename.append('_').split('_');

	if (parts[0]=="")
	{
		if (throw_if_fails)
		{
			THROW(ArgumentException, "File name '" + basename + "' does not start with a valid NGSD sample name!");
		}
		else
		{
			return "";
		}
	}

	return parts[0];
}

QString NGSD::sampleIsTumor(const QString& filename)
{
	QVariant value = getValue("SELECT tumor FROM sample WHERE id='" + sampleId(filename, false) + "'");
	if (value.isNull()) return "n/a";
	return value.toInt() ? "yes" : "no";
}

QString NGSD::sampleIsFFPE(const QString& filename)
{
	QVariant value = getValue("SELECT ffpe FROM sample WHERE id='" + sampleId(filename, false) + "'");
	if (value.isNull()) return "n/a";
	return value.toInt() ? "yes" : "no";
}

QString NGSD::processedSampleName(const QString& filename, bool throw_if_fails)
{
	QString basename = QFileInfo(filename).baseName();
	QStringList parts = basename.append('_').split('_');

	bool ok = false;
	parts[1].toInt(&ok);
	if (parts[0]=="" || !ok)
	{
		if (throw_if_fails)
		{
			THROW(ArgumentException, "File name '" + basename + "' does not start with a valid NGSD processed sample name!");
		}
		else
		{
			return "";
		}
	}

	return parts[0] + "_" + parts[1];
}

QString NGSD::sampleId(const QString& filename, bool throw_if_fails)
{
	QStringList parts = QFileInfo(filename).baseName().append('_').split('_');

	//get sample ID
	SqlQuery query = getQuery(); //use binding (user input)
	query.prepare("SELECT id FROM sample WHERE name=:sample");
	query.bindValue(":sample", parts[0]);
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
	QStringList parts = QFileInfo(filename).baseName().append('_').split('_');

	//get sample ID
	SqlQuery query = getQuery(); //use binding (user input)
	query.prepare("SELECT ps.id FROM processed_sample ps, sample s WHERE s.name=:sample AND ps.sample_id=s.id AND ps.process_id=:psnum");
	query.bindValue(":sample", parts[0]);
	query.bindValue(":psnum", QString::number(parts[1].toInt()));
	query.exec();
	if (query.size()==0)
	{
		if(throw_if_fails)
		{
			THROW(DatabaseException, "Processed sample name '" + parts[0] + "_" + parts[1] + "' not found in NGSD!");
		}
		else
		{
			return "";
		}
	}
	query.next();
	return query.value(0).toString();
}

QString NGSD::variantId(const Variant& variant, bool throw_if_fails)
{
	SqlQuery query = getQuery(); //use binding user input (safety)
	query.prepare("SELECT id FROM variant WHERE chr=:chr AND start='"+QString::number(variant.start())+"' AND end='"+QString::number(variant.end())+"' AND ref=:ref AND obs=:obs");
	query.bindValue(":chr", variant.chr().str());
	query.bindValue(":ref", variant.ref());
	query.bindValue(":obs", variant.obs());
	query.exec();
	if (query.size()==0)
	{
		if (throw_if_fails)
		{
			THROW(DatabaseException, "Variant " + variant.toString() + " not found in NGSD!");
		}
		else
		{
			return "-1";
		}
	}
	query.next();
	return query.value(0).toString();
}

QVariant NGSD::getValue(const QString& query, bool no_value_is_ok)
{
	SqlQuery q = getQuery();
	q.exec(query);

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

	q.next();
	return q.value(0);
}

QStringList NGSD::getValues(const QString& query)
{
	SqlQuery q = getQuery();
	q.exec(query);

	QStringList output;
	output.reserve(q.size());
	while(q.next())
	{
		output.append(q.value(0).toString());
	}
	return output;
}

void NGSD::executeQueriesFromFile(QString filename)
{
	QStringList lines = Helper::loadTextFile(filename, true);
	QString query = "";
	for(const QString& line : lines)
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
	if (query.endsWith(';'))
	{
		//qDebug() << query;
		getQuery().exec(query);
		query.clear();
	}
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
	//Log::info("MYSQL closing  - name: " + db_->connectionName());

	//close prepared queries
	q_approved_.clear();

	//close database and remove it
	QString connection_name = db_->connectionName();
	db_.clear();
	if (!db_used_externally_as_static_)
	{
		QSqlDatabase::removeDatabase(connection_name);
	}
}

bool NGSD::isOpen() const
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

void NGSD::init(QString password)
{
	//remove existing tables
	SqlQuery query = getQuery();
	query.exec("SHOW TABLES");
	if (query.size()>0)
	{
		//check password for re-init of production DB
		if (!test_db_ && password!=Settings::string("ngsd_pass"))
		{
			THROW(DatabaseException, "Password provided for re-initialization of procution database is incorrect!");
		}

		//get table list
		QStringList tables;
		while(query.next())
		{
			tables << query.value(0).toString();
		}

		//remove old tables
		if (!tables.isEmpty())
		{
			query.exec("SET FOREIGN_KEY_CHECKS = 0;");
			query.exec("DROP TABLE " + tables.join(","));
			query.exec("SET FOREIGN_KEY_CHECKS = 1;");
		}
	}

	//initilize
	executeQueriesFromFile(":/resources/NGSD_schema.sql");
}

QString NGSD::getExternalSampleName(const QString& filename)
{
	QString value = getValue("SELECT name_external FROM sample WHERE id='" + sampleId(filename) + "'").toString().trimmed();

	return value=="" ? "n/a" : value;
}

QString NGSD::getProcessingSystem(const QString& filename, SystemType type)
{
	//get processing system ID
	QString sys_id = getValue("SELECT processing_system_id FROM processed_sample WHERE id='" + processedSampleId(filename) + "'").toString();

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
	return getValue("SELECT " + what + " FROM processing_system WHERE id='" + sys_id + "'").toString().trimmed();
}


QString NGSD::getGenomeBuild(const QString& filename)
{
	return getValue("SELECT g.build FROM processed_sample ps, processing_system sys, genome g WHERE ps.id='" + processedSampleId(filename) + "' AND ps.processing_system_id=sys.id AND sys.genome_id=g.id").toString();
}

QPair<QString, QString> NGSD::getValidationStatus(const QString& filename, const Variant& variant)
{
	SqlQuery query = getQuery();
	query.exec("SELECT status, comment FROM variant_validation WHERE sample_id='" + sampleId(filename) + "' AND variant_id='" + variantId(variant) + "'");
	if (query.size()==0)
	{
		return QPair<QString, QString>("n/a", "");
	}
	else
	{
		query.next();
		return QPair<QString, QString>(query.value(0).toString().trimmed(), query.value(1).toString().trimmed());
	}
}

QCCollection NGSD::getQCData(const QString& filename)
{
	QString ps_id = processedSampleId(filename, false);

	//get QC data
	SqlQuery q = getQuery();
	q.exec("SELECT n.name, nm.value, n.description, n.qcml_id FROM processed_sample_qc as nm, qc_terms as n WHERE nm.processed_sample_id='" + ps_id + "' AND nm.qc_terms_id=n.id");
	QCCollection output;
	while(q.next())
	{
		output.insert(QCValue(q.value(0).toString(), q.value(1).toString(), q.value(2).toString(), q.value(3).toString()));
	}

	//get KASP data
	SqlQuery q2 = getQuery();
	q2.exec("SELECT random_error_prob FROM kasp_status WHERE processed_sample_id='" + ps_id + "'");
	QString value = "n/a";
	if (q2.size()>0)
	{
		q2.next();
		float numeric_value = 100.0 * q2.value(0).toFloat();
		if (numeric_value>100.0) //special case: random_error_prob>100%
		{
			value = "<font color=orange>KASP not performed (see NGSD)</font>";
		}
		else if (numeric_value>1.0) //random_error_prob>1% => warn
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
	//get processing system ID
	QString sys_id = getValue("SELECT processing_system_id FROM processed_sample WHERE id='" + processedSampleId(filename) + "'").toString();

	//get QC id
	QString qc_id = getValue("SELECT id FROM qc_terms WHERE qcml_id='" + accession + "'").toString();

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

void NGSD::annotate(VariantList& variants, QString filename)
{
	initProgress("NGSD annotation", true);

	//get sample ids
	QString s_id = sampleId(filename, false);
	QString ps_id = processedSampleId(filename, false);
	QString sys_id = getValue("SELECT processing_system_id FROM processed_sample WHERE id='" + processedSampleId(filename, false) + "'").toString();

	//check if we could determine the sample
	bool found_in_db = true;
	if (s_id=="" || ps_id=="" || sys_id=="")
	{
		Log::warn("Could not find processed sample in NGSD by name '" + filename + "'. Annotation will be incomplete because processing system could not be determined!");
		found_in_db = false;
	}

	//get sample ids that have processed samples with the same processing system (not same sample, variants imported, same processing system, good quality of sample, not tumor)
	QSet<int> sys_sample_ids;
	SqlQuery tmp = getQuery();
	tmp.exec("SELECT DISTINCT s.id FROM processed_sample as ps, sample s WHERE ps.processing_system_id='" + sys_id + "' AND ps.sample_id=s.id AND s.tumor='0' AND s.quality='good' AND s.id!='" + s_id + "' AND (SELECT count(id) FROM detected_variant as dv WHERE dv.processed_sample_id = ps.id)>0");
	while(tmp.next())
	{
		sys_sample_ids.insert(tmp.value(0).toInt());
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
	removeColumnIfPresent(variants, "classification_comment", true);
	removeColumnIfPresent(variants, "validated", true);
	removeColumnIfPresent(variants, "comment", true);

	//get required column indices
	QString num_samples = QString::number(sys_sample_ids.count());
	int ihdb_hom_idx = addColumn(variants, "ihdb_hom", "Homozygous variant counts in NGSD for the same processing system (" + num_samples + " samples).");
	int ihdb_het_idx = addColumn(variants, "ihdb_het", "Heterozyous variant counts in NGSD for the same processing system (" + num_samples + " samples).");
	int ihdb_wt_idx  = addColumn(variants, "ihdb_wt", "Wildtype variant counts in NGSD for the same processing system (" + num_samples + " samples).");
	int ihdb_all_hom_idx = addColumn(variants, "ihdb_allsys_hom", "Homozygous variant counts in NGSD independent of the processing system.");
	int ihdb_all_het_idx =  addColumn(variants, "ihdb_allsys_het", "Heterozygous variant counts in NGSD independent of the processing system.");
	int class_idx = addColumn(variants, "classification", "Classification from the NGSD.");
	int clacom_idx = addColumn(variants, "classification_comment", "Classification comment from the NGSD.");
	int valid_idx = addColumn(variants, "validated", "Validation information from the NGSD.");
	if (variants.annotationIndexByName("comment", true, false)==-1) addColumn(variants, "comment", "Comments from the NGSD. Comments of other samples are listed in brackets!");
	int comment_idx = variants.annotationIndexByName("comment", true, false);

	//(re-)annotate the variants
	SqlQuery query = getQuery();
	for (int i=0; i<variants.count(); ++i)
	{
		//QTime timer;
		//timer.start();

		//variant id
		Variant& v = variants[i];
		QByteArray v_id = variantId(v, false).toLatin1();

		//variant classification
		QVariant classification = getValue("SELECT class FROM variant_classification WHERE variant_id='" + v_id + "'", true);
		if (!classification.isNull())
		{
			v.annotations()[class_idx] = classification.toByteArray().replace("n/a", "");
			v.annotations()[clacom_idx] = getValue("SELECT comment FROM variant_classification WHERE variant_id='" + v_id + "'", true).toByteArray().replace("\n", " ").replace("\t", " ");
		}
		//int t_v = timer.elapsed();
		//timer.restart();

		//detected variant infos
		int dv_id = -1;
		QByteArray comment = "";
		if (found_in_db)
		{
			query.exec("SELECT id, comment FROM detected_variant WHERE processed_sample_id='" + ps_id + "' AND variant_id='" + v_id + "'");
			if (query.size()==1)
			{
				query.next();
				dv_id = query.value(0).toInt();
				comment = query.value(1).toByteArray();
			}
		}

		//validation info
		int vv_id = -1;
		QByteArray val_status = "";
		if (found_in_db)
		{
			query.exec("SELECT id, status FROM variant_validation WHERE sample_id='" + s_id + "' AND variant_id='" + v_id + "'");
			if (query.size()==1)
			{
				query.next();
				vv_id = query.value(0).toInt();
				val_status = query.value(1).toByteArray().replace("n/a", "");
			}
		}

		//int t_dv = timer.elapsed();
		//timer.restart();

		//validation info other samples
		int tps = 0;
		int fps = 0;
		query.exec("SELECT id, status FROM variant_validation WHERE variant_id='"+v_id+"' AND status!='n/a'");
		while(query.next())
		{
			if (query.value(0).toInt()==vv_id) continue;
			if (query.value(1).toByteArray()=="true positive") ++tps;
			else if (query.value(1).toByteArray()=="false positive") ++fps;
		}
		if (tps>0 || fps>0)
		{
			if (val_status=="") val_status = "n/a";
			val_status += " (" + QByteArray::number(tps) + "xTP, " + QByteArray::number(fps) + "xFP)";
		}
		//int t_val = timer.elapsed();
		//timer.restart();

		//comments other samples
		QList<QByteArray> comments;
		query.exec("SELECT id, comment FROM detected_variant WHERE variant_id='"+v_id+"' AND comment IS NOT NULL");
		while(query.next())
		{
			if (query.value(0).toInt()==dv_id) continue;
			QByteArray tmp = query.value(1).toByteArray().trimmed();
			if (tmp!="") comments.append(tmp);
		}
		if (comments.size()>0)
		{
			if (comment=="") comment = "n/a";
			comment += " (";
			for (int i=0; i<comments.count(); ++i)
			{
				if (i>0)
				{
					comment += ", ";
				}
				comment += comments[i];
			}
			comment += ")";
		}
		//int t_com = timer.elapsed();
		//timer.restart();

		//genotype counts
		int allsys_hom_count = 0;
		int allsys_het_count = 0;
		int sys_hom_count = 0;
		int sys_het_count = 0;
		QSet<int> s_ids_done;
		int s_id_int = s_id.toInt();
		query.exec("SELECT dv.genotype, ps.sample_id FROM detected_variant as dv, processed_sample ps WHERE dv.processed_sample_id=ps.id AND dv.variant_id='" + v_id + "'");
		while(query.next())
		{
			//skip this sample id
			int current_sample = query.value(1).toInt();
			if (current_sample==s_id_int) continue;

			//skip already seen samples (there could be several processings of the same sample because of different processing systems or because of experment repeats due to quality issues)
			if (s_ids_done.contains(current_sample)) continue;
			s_ids_done.insert(current_sample);

			QByteArray current_geno = query.value(0).toByteArray();
			if (current_geno=="hom")
			{
				++allsys_hom_count;
				if (sys_sample_ids.contains(current_sample))
				{
					++sys_hom_count;
				}
			}
			else if (current_geno=="het")
			{
				++allsys_het_count;
				if (sys_sample_ids.contains(current_sample))
				{
					++sys_het_count;
				}
			}
		}
		//qDebug() << (v.isSNV() ? "S" : "I") << query.size() << t_v << t_dv << t_val << t_com << timer.elapsed();

		v.annotations()[ihdb_all_hom_idx] = QByteArray::number(allsys_hom_count);
		v.annotations()[ihdb_all_het_idx] = QByteArray::number(allsys_het_count);
		if (found_in_db)
		{
			v.annotations()[ihdb_hom_idx] = QByteArray::number((double)sys_hom_count / sys_sample_ids.count(), 'f', 4);
			v.annotations()[ihdb_het_idx] =  QByteArray::number((double)sys_het_count / sys_sample_ids.count(), 'f', 4);
			v.annotations()[ihdb_wt_idx] =  QByteArray::number((double)(sys_sample_ids.count() - sys_hom_count - sys_het_count) / sys_sample_ids.count(), 'f', 4);
			v.annotations()[valid_idx] = val_status;
			v.annotations()[comment_idx] = comment.replace("\n", " ").replace("\t", " ");
		}
		else
		{
			v.annotations()[ihdb_hom_idx] = "n/a";
			v.annotations()[ihdb_het_idx] = "n/a";
			v.annotations()[ihdb_wt_idx] = "n/a";
			v.annotations()[valid_idx] = "n/a";
			v.annotations()[comment_idx] = "n/a";
		}

		emit updateProgress(100*i/variants.count());
	}
}

void NGSD::annotateSomatic(VariantList& variants, QString filename)
{
	//get sample ids
	QStringList samples = filename.split('-');
	QString ts_id = sampleId(samples[0], false);

	//check if we could determine the sample
	if (ts_id=="")
	{
		Log::warn("Could not find processed sample in NGSD from name '" + QFileInfo(filename).baseName() + "'. Annotation will be incomplete because processing system could not be determined!");
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

		SqlQuery query = getQuery();
		query.exec("SELECT s.id, dsv.processed_sample_id_tumor, p.name FROM detected_somatic_variant as dsv, variant as v, processed_sample ps, sample as s, project as p WHERE ps.project_id=p.id AND dsv.processed_sample_id_tumor=ps.id and dsv.variant_id=v.id AND  ps.sample_id=s.id  AND s.tumor='1' AND v.chr='"+v.chr().str()+"' AND v.start='"+QString::number(v.start())+"' AND v.end='"+QString::number(v.end())+"' AND v.ref='"+v.ref()+"' AND v.obs='"+v.obs()+"'");

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
	QString s_id = sampleId(filename);
	QString v_id = variantId(variant);
	QVariant vv_id = getValue("SELECT id FROM variant_validation WHERE sample_id='" + s_id + "' AND variant_id='" + v_id + "'");


	SqlQuery query = getQuery(); //use binding (user input)
	if (vv_id.isNull()) //insert
	{
		QString geno = getValue("SELECT genotype FROM detected_variant WHERE variant_id='" + v_id + "' AND processed_sample_id='" + processedSampleId(filename) + "'", false).toString();
		query.prepare("INSERT INTO variant_validation (sample_id, variant_id, genotype, status, comment) VALUES ('" + s_id + "','" + v_id + "','" + geno + "',:status,:comment)");
	}
	else //update
	{
		query.prepare("UPDATE variant_validation SET status=:status, comment=:comment WHERE id='" + vv_id.toString() + "'");
	}
	query.bindValue(":status", status);
	query.bindValue(":comment", comment);
	query.exec();
}

QPair<QString, QString> NGSD::getClassification(const Variant& variant)
{
	SqlQuery query = getQuery();
	query.exec("SELECT class, comment FROM variant_classification WHERE variant_id='" + variantId(variant) + "'");
	if (query.size()==0)
	{
		return QPair<QString, QString>("n/a", "");
	}
	else
	{
		query.next();
		return QPair<QString, QString>(query.value(0).toString().trimmed(), query.value(1).toString().trimmed());
	}
}

void NGSD::setClassification(const Variant& variant, const QString& classification, const QString& comment)
{
	QString v_id = variantId(variant);
	QVariant vc_id = getValue("SELECT id FROM variant_classification WHERE variant_id='" + v_id + "'");

	SqlQuery query = getQuery(); //use binding (user input)
	if (vc_id.isNull()) //insert
	{
		query.prepare("INSERT INTO variant_classification (variant_id, class, comment) VALUES ('" + v_id + "',:class,:comment)");
	}
	else //update
	{
		query.prepare("UPDATE variant_classification SET class=:class, comment=:comment WHERE id='" + vc_id.toString() + "'");
	}
	query.bindValue(":class", classification);
	query.bindValue(":comment", comment);
	query.exec();
}

QString NGSD::comment(const QString& filename, const Variant& variant)
{
	return getValue("SELECT comment FROM detected_variant WHERE processed_sample_id='" + processedSampleId(filename) + "' AND variant_id='" + variantId(variant) + "'").toString();
}

QString NGSD::url(const QString& filename, const Variant& variant)
{
	QString dv_id = getValue("SELECT id FROM detected_variant WHERE processed_sample_id='" + processedSampleId(filename) + "' AND variant_id='" + variantId(variant) + "'", false).toString();

	return Settings::string("NGSD")+"/variants/view/" + dv_id;
}

QString NGSD::url(const QString& filename)
{
	return Settings::string("NGSD")+"/processedsamples/view/" + processedSampleId(filename);
}

QString NGSD::urlSearch(const QString& search_term)
{
	return Settings::string("NGSD")+"/search/processSearch/search_term=" + search_term;
}

void NGSD::setComment(const QString& filename, const Variant& variant, const QString& text)
{
	getQuery().exec("UPDATE detected_variant SET comment='" + text + "' WHERE processed_sample_id='" + processedSampleId(filename) + "' AND variant_id='" + variantId(variant) + "'");
}

void NGSD::setReportVariants(const QString& filename, const VariantList& variants, QSet<int> selected_indices)
{
	QString ps_id = processedSampleId(filename);

	//get variant ID
	for(int i=0; i<variants.count(); ++i)
	{
		getQuery().exec("UPDATE detected_variant SET report=" + QString(selected_indices.contains(i) ? "1" : "0" ) + " WHERE processed_sample_id='" + ps_id + "' AND variant_id='" + variantId(variants[i]) + "'");
	}
}

QString NGSD::nextProcessingId(const QString& sample_id)
{
	QString max_num = getValue("SELECT MAX(process_id) FROM processed_sample WHERE sample_id=" + sample_id).toString();

	return max_num.isEmpty() ? "1" : QString::number(max_num.toInt()+1);
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
	SqlQuery q = getQuery();
	q.exec("DESCRIBE "+table+" "+column);
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


void NGSD::tableExists(QString table)
{
	SqlQuery query = getQuery();
	query.exec("SHOW TABLES LIKE '" + table + "'");
	if (query.size()==0)
	{
		THROW(DatabaseException, "Table '" + table + "' does not exist!")
	}
}

bool NGSD::tableEmpty(QString table)
{
	SqlQuery query = getQuery();
	query.exec("SELECT COUNT(*) FROM " + table);
	query.next();
	return query.value(0).toInt()==0;
}

int NGSD::geneToApprovedID(const QByteArray& gene)
{
	//init
	static SqlQuery q_gene = getQuery(true);
	static SqlQuery q_prev = getQuery(true);
	static SqlQuery q_syn = getQuery(true);
	static bool init = false;
	if (!init)
	{
		q_gene.prepare("SELECT id FROM gene WHERE symbol=:1");
		q_prev.prepare("SELECT g.id FROM gene g, gene_alias ga WHERE g.id=ga.gene_id AND ga.symbol=:1 AND ga.type='previous'");
		q_syn.prepare("SELECT g.id FROM gene g, gene_alias ga WHERE g.id=ga.gene_id AND ga.symbol=:1 AND ga.type='synonym'");
		init = true;
	}

	//approved
	q_gene.bindValue(0, gene);
	q_gene.exec();
	if (q_gene.size()==1)
	{
		q_gene.next();
		return q_gene.value(0).toInt();
	}

	//previous
	q_prev.bindValue(0, gene);
	q_prev.exec();
	if (q_prev.size()==1)
	{
		q_prev.next();
		return q_prev.value(0).toInt();
	}
	else if(q_prev.size()>1)
	{
		return -1;
	}

	//synonymous
	q_syn.bindValue(0, gene);
	q_syn.exec();
	if (q_syn.size()==1)
	{
		q_syn.next();
		return q_syn.value(0).toInt();
	}

	return -1;
}

QByteArray NGSD::geneSymbol(int id)
{
	return getValue("SELECT symbol FROM gene WHERE id='" + QString::number(id) + "'").toByteArray();
}

QPair<QByteArray, QByteArray> NGSD::geneToApproved(const QByteArray& gene)
{
	//init
	static SqlQuery q_gene = getQuery(true);
	static SqlQuery q_prev = getQuery(true);
	static SqlQuery q_syn = getQuery(true);
	static bool init = false;
	if (!init)
	{
		q_gene.prepare("SELECT id FROM gene WHERE symbol=:1");
		q_prev.prepare("SELECT g.symbol FROM gene g, gene_alias ga WHERE g.id=ga.gene_id AND ga.symbol=:1 AND ga.type='previous'");
		q_syn.prepare("SELECT g.symbol FROM gene g, gene_alias ga WHERE g.id=ga.gene_id AND ga.symbol=:1 AND ga.type='synonym'");
		init = true;
	}

	//approved
	q_gene.bindValue(0, gene);
	q_gene.exec();
	if (q_gene.size()==1)
	{
		q_gene.next();
		return qMakePair(gene, QByteArray("KEPT: is approved symbol"));
	}

	//previous
	q_prev.bindValue(0, gene);
	q_prev.exec();
	if (q_prev.size()==1)
	{
		q_prev.next();
		return qMakePair(q_prev.value(0).toByteArray(), "REPLACED: " + gene + " is a previous symbol");
	}
	else if(q_prev.size()>1)
	{
		QByteArray genes;
		while(q_prev.next())
		{
			if (!genes.isEmpty()) genes.append(", ");
			genes.append(q_prev.value(0).toByteArray());
		}
		return qMakePair(gene, "ERROR: is a previous symbol of the genes " + genes);
	}

	//synonymous
	q_syn.bindValue(0, gene);
	q_syn.exec();
	if (q_syn.size()==1)
	{
		q_syn.next();
		return qMakePair(q_syn.value(0).toByteArray(), "REPLACED: " + gene + " is a synonymous symbol");
	}
	else if(q_syn.size()>1)
	{
		QByteArray genes;
		while(q_syn.next())
		{
			if (!genes.isEmpty()) genes.append(", ");
			genes.append(q_syn.value(0).toByteArray());
		}
		return qMakePair(gene, "ERROR: is a synonymous symbol of the genes " + genes);
	}

	return qMakePair(gene, QByteArray("ERROR: is unknown symbol"));
}

QStringList NGSD::previousSymbols(QString symbol)
{
	return getValues("SELECT ga.symbol FROM gene g, gene_alias ga WHERE g.id=ga.gene_id AND g.symbol='" + symbol + "' AND ga.type='previous'");
}

QStringList NGSD::synonymousSymbols(QString symbol)
{
	return getValues("SELECT ga.symbol FROM gene g, gene_alias ga WHERE g.id=ga.gene_id AND g.symbol='" + symbol + "' AND ga.type='synonymous'");
}

QStringList NGSD::phenotypes(QString symbol)
{
	return getValues("SELECT t.name FROM hpo_term t, hpo_genes g WHERE g.gene='" + symbol + "' AND t.id=g.hpo_term_id");
}

QStringList NGSD::phenotypes(QStringList terms)
{
	qDebug() << endl << "DB: " << db_->hostName() << db_->databaseName() << getValue("SELECT COUNT(id) FROM hpo_term");

	//trim terms and remove empty terms
	QStringList tmp;
	foreach (QString t, terms)
	{
		t = t.trimmed();
		if (!t.isEmpty()) tmp.append(t);
	}
	qDebug() << "TERMS" << tmp;

	QStringList output;
	if (terms.isEmpty()) //no terms => report all
	{
		output = getValues("SELECT name FROM hpo_term ORDER BY name ASC");
		qDebug() << "out" << output;
	}
	else
	{
		SqlQuery query = getQuery();
		query.prepare("SELECT name FROM hpo_term WHERE name LIKE '%:0%'");
		foreach(QString t, tmp)
		{
			query.bindValue(0, t);
			query.exec();
			while(query.next())
			{
				qDebug() << "out" << query.value(0).toString();
				output.append(query.value(0).toString());
			}
		}

		output.sort();
		output.removeDuplicates();
	}

	return output;
}

QStringList NGSD::genesOverlapping(QByteArray chr, int start, int end, int extend)
{
	//annotate
	q_approved_->bindValue(0, chr.replace("chr", ""));
	q_approved_->bindValue(1, start - extend);
	q_approved_->bindValue(2, end + extend);
	q_approved_->bindValue(3, start - extend);
	q_approved_->bindValue(4, start - extend);
	q_approved_->exec();

	QStringList genes;
	while(q_approved_->next())
	{
		genes.append(q_approved_->value(0).toString());
	}

	return genes;
}

BedFile NGSD::genesToRegions(QStringList genes, QString source, QString mode, bool messages)
{
	//check mode
	QStringList valid_modes;
	valid_modes << "gene" << "exon";
	if (!valid_modes.contains(mode)) THROW(ArgumentException, "Invalid mode '" + mode + "'. Valid modes are: " + valid_modes.join(", ") + ".");

	//check source
	QStringList valid_sources = getEnum("gene_transcript", "source");
	if (!valid_sources.contains(source)) THROW(ArgumentException, "Invalid source '" + source + "'. Valid modes are: " + valid_sources.join(", ") + ".");

	//init
	BedFile output;
	QTextStream stream(stderr);

	//prepare queries
	SqlQuery q_transcript = getQuery();
	q_transcript.prepare("SELECT id, start_coding, end_coding FROM gene_transcript WHERE source='" + source + "' AND gene_id=:1 AND start_coding IS NOT NULL");
	SqlQuery q_exon = getQuery();
	q_exon.prepare("SELECT start, end FROM gene_exon WHERE transcript_id=:1");

	//process input data
	foreach(QString gene, genes)
	{
		//get approved gene id
		gene = gene.toUpper();
		int id = geneToApprovedID(gene.toUtf8());
		if (id==-1)
		{
			if (messages) stream << "Gene name '" << gene << "' is no HGNC-approved symbol. Skipping it!" << endl;
			continue;
		}

		//get chromosome
		QString chr = "chr" + getValue("SELECT chromosome FROM gene WHERE id='" + QString::number(id) + "'").toString();

		//preprare annotations
		QStringList annos;
		annos << gene;

		if (mode=="gene")
		{
			int start_coding = std::numeric_limits<int>::max();
			int end_coding = -std::numeric_limits<int>::max();

			q_transcript.bindValue(0, id);
			q_transcript.exec();
			while(q_transcript.next())
			{
				start_coding = std::min(start_coding, q_transcript.value(1).toInt());
				end_coding = std::max(end_coding, q_transcript.value(2).toInt());
			}

			if (start_coding>end_coding)
			{
				if (messages) stream << "No coding transcripts found for gene name '" << gene << "'. Skipping it!" << endl;
			}
			else
			{
				output.append(BedLine(chr, start_coding, end_coding, annos));
			}
		}
		else if (mode=="exon")
		{
			int line_count = 0;
			q_transcript.bindValue(0, id);
			q_transcript.exec();
			while(q_transcript.next())
			{
				int trans_id = q_transcript.value(0).toInt();
				int start_coding = q_transcript.value(1).toInt();
				int end_coding = q_transcript.value(2).toInt();
				q_exon.bindValue(0, trans_id);
				q_exon.exec();
				while(q_exon.next())
				{
					int start = std::max(start_coding, q_exon.value(0).toInt());
					int end = std::min(end_coding, q_exon.value(1).toInt());
					if (end<start_coding || start>end_coding) continue;

					output.append(BedLine(chr, start, end, annos));
					++line_count;
				}
			}
			if (line_count==0)
			{
				if (messages) stream << "No coding exons found for gene name '" << gene << "'. Skipping it!" << endl;
				continue;
			}
		}
	}

	output.sort(true);
	return output;
}

QStringList NGSD::getDiagnosticStatus(const QString& filename)
{
	QString ps_id = processedSampleId(filename, false);
	if (ps_id=="")
	{
		return QStringList();
	}

	//get status
	SqlQuery q = getQuery();
	q.exec("SELECT s.status, u.name, s.date, s.outcome FROM diag_status as s, user as u WHERE s.processed_sample_id='" + ps_id +  "' AND s.user_id=u.id");
	if (q.size()==0)
	{
		return (QStringList() << "n/a" << "n/a" << "n/a" << "n/a");
	}
	q.next();
	return (QStringList() << q.value(0).toString() << q.value(1).toString() << q.value(2).toString().replace('T', ' ') << q.value(3).toString());
}

void NGSD::setDiagnosticStatus(const QString& filename, QString status)
{
	//get sample ID
	QString s_id = sampleId(filename);

	//get processed sample ID
	QString ps_id = processedSampleId(filename);

	//get user ID
	QString user_id = userId();

	//update status
	getQuery().exec("INSERT INTO diag_status (processed_sample_id, status, user_id) VALUES ("+ps_id+",'"+status+"', "+user_id+") ON DUPLICATE KEY UPDATE status='"+status+"',user_id="+user_id+"");

	//add new processed sample if scheduled for repetition
	if (status.startsWith("repeat"))
	{
		QString next_ps_num = nextProcessingId(s_id);
		SqlQuery query = getQuery();
		query.exec("SELECT mid1_i7, mid2_i5, operator_id, processing_system_id, project_id, molarity FROM processed_sample WHERE id=" + ps_id);
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
		QString comment = user_name + " requested re-sequencing (" + status + ") of sample " + processedSampleName(filename) + " on " + Helper::dateTime("dd.MM.yyyy hh:mm:ss");
		if (status=="repeat sequencing only")
		{
			getQuery().exec("INSERT INTO processed_sample (sample_id, process_id, mid1_i7, mid2_i5, operator_id, processing_system_id, comment, project_id, molarity) VALUES ("+ s_id +","+ next_ps_num +","+ mid1 +","+ mid2 +","+ op_id +","+ sys_id +",'"+ comment +"',"+ proj_id +","+ molarity +")");
		}
		else if (status=="repeat library and sequencing")
		{
			getQuery().exec("INSERT INTO processed_sample (sample_id, process_id, operator_id, processing_system_id, comment, project_id) VALUES ("+ s_id +","+ next_ps_num +","+ op_id +","+ sys_id +",'"+ comment +"',"+ proj_id +")");
		}
		else
		{
			THROW(ProgrammingException, "Unknown diagnostic status '" + status +"!'");
		}
	}
}

void NGSD::setReportOutcome(const QString& filename, QString outcome)
{
	QString user_id = userId();
	getQuery().exec("INSERT INTO diag_status (processed_sample_id, status, user_id, outcome) VALUES (" + processedSampleId(filename) + ",'pending'," + user_id + ",'" + outcome + "') ON DUPLICATE KEY UPDATE user_id="+user_id+",outcome='"+outcome+"'");
}

QString NGSD::getProcessedSampleQuality(const QString& filename, bool colored)
{
	QString quality = getValue("SELECT quality FROM processed_sample WHERE id='" + processedSampleId(filename) + "'", false).toString();
	if (colored)
	{
		if (quality=="good") quality = "<font color=green>"+quality+"</font>";
		if (quality=="medium") quality = "<font color=orange>"+quality+"</font>";
		if (quality=="bad") quality = "<font color=red>"+quality+"</font>";
	}
	return quality;
}

void NGSD::setProcessedSampleQuality(const QString& filename, QString quality)
{
	getQuery().exec("UPDATE processed_sample SET quality='" + quality + "' WHERE id='" + processedSampleId(filename) + "'");
}

GeneInfo NGSD::geneInfo(QString symbol)
{
	GeneInfo output;

	//get approved symbol
	symbol = symbol.trimmed();
	auto approved = geneToApproved(symbol.toLatin1());
	output.symbol = approved.first;
	output.notice = approved.second;

	//update geneinfo_germline entry if necessary
	if (output.notice.startsWith("REPLACED:"))
	{
		SqlQuery query = getQuery();
		query.prepare("UPDATE geneinfo_germline SET symbol=:0 WHERE symbol=:1");
		query.bindValue(0, output.symbol);
		query.bindValue(1, symbol);
		query.exec();
	}

	SqlQuery query = getQuery();
	query.exec("SELECT inheritance, comments FROM geneinfo_germline WHERE symbol='" + output.symbol + "'");
	if (query.size()==0)
	{
		output.inheritance = "n/a";
		output.comments = "";
	}
	else
	{
		query.next();
		output.inheritance = query.value(0).toString();
		output.comments = query.value(1).toString();
	}

	return output;
}

void NGSD::setGeneInfo(GeneInfo info)
{
	SqlQuery query = getQuery();
	query.prepare("INSERT INTO geneinfo_germline (symbol, inheritance, comments) VALUES (:0, :1, :2) ON DUPLICATE KEY UPDATE inheritance=:3, comments=:4");
	query.bindValue(0, info.symbol);
	query.bindValue(1, info.inheritance);
	query.bindValue(2, info.comments);
	query.bindValue(3, info.inheritance);
	query.bindValue(4, info.comments);
	query.exec();
}
