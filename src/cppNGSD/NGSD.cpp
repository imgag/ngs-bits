#include "NGSD.h"
#include "Exceptions.h"
#include "Helper.h"
#include "Log.h"
#include "Settings.h"
#include "ChromosomalIndex.h"
#include "NGSHelper.h"
#include "VariantFilter.h"
#include <QFileInfo>
#include <QPair>

NGSD::NGSD(bool test_db)
	: test_db_(test_db)
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
		THROW(DatabaseException, "Could not connect to the NGSD database: '" + prefix + "'");
	}
}

QString NGSD::userId(QString user_name)
{
	QString user_id = getValue("SELECT id FROM user WHERE user_id='" + user_name + "'", true).toString();
	if (user_id=="")
	{
		THROW(DatabaseException, "Could not determine NGSD user ID for user name '" + user_name + "! Do you have an NGSD user account?");
	}

	return user_id;
}

SampleData NGSD::getSampleData(const QString& sample_id)
{
	//execute query
	SqlQuery query = getQuery();
	query.exec("SELECT s.name, s.name_external, s.gender, s.quality, s.comment, s.disease_group, s.disease_status, s.tumor, s.ffpe FROM sample s WHERE id=" + sample_id);
	if (query.size()==0)
	{
		THROW(ProgrammingException, "Invalid 'id' for table 'sample' given: '" + sample_id + "'");
	}
	query.next();

	//create output
	SampleData output;
	output.name = query.value(0).toByteArray().trimmed();
	output.name_external = query.value(1).toByteArray().trimmed();
	output.gender = query.value(2).toByteArray();
	output.quality = query.value(3).toByteArray();
	output.comments = query.value(4).toByteArray().trimmed();
	output.disease_group = query.value(5).toByteArray();
	output.disease_status = query.value(6).toByteArray();
	output.is_tumor = query.value(7).toByteArray()=="1";
	output.is_ffpe = query.value(8).toByteArray()=="1";
	return output;
}

ProcessedSampleData NGSD::getProcessedSampleData(const QString& processed_sample_id)
{
	//execute query
	SqlQuery query = getQuery();
	query.exec("SELECT CONCAT(s.name,'_',LPAD(ps.process_id,2,'0')) as ps_name, sys.name_manufacturer as sys_name, ps.quality, ps.comment, p.name as p_name, r.name as r_name, ps.normal_id FROM sample s, project p, processing_system sys, processed_sample ps LEFT JOIN sequencing_run r ON ps.sequencing_run_id=r.id WHERE ps.sample_id=s.id AND ps.project_id=p.id AND ps.processing_system_id=sys.id AND ps.id=" + processed_sample_id);
	if (query.size()==0)
	{
		THROW(ProgrammingException, "Invalid 'id' for table 'processed_sample' given: '" + processed_sample_id + "'");
	}
	query.next();

	//create output
	ProcessedSampleData output;
	output.name = query.value(0).toByteArray().trimmed();
	output.processing_system = query.value(1).toByteArray().trimmed();
	output.quality = query.value(2).toByteArray();
	output.comments = query.value(3).toByteArray().trimmed();
	output.project_name = query.value(4).toByteArray();
	output.run_name = query.value(5).toByteArray();
	output.normal_sample_name = query.value(6).toByteArray().trimmed();
	if (output.normal_sample_name!="")
	{
		output.normal_sample_name = normalSample(processed_sample_id);
	}
	return output;

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

QString NGSD::normalSample(const QString& processed_sample_id)
{
	QVariant value = getValue("SELECT normal_id FROM processed_sample WHERE id=" + processed_sample_id, true);
	if (value.isNull()) return "";

	return getValue("SELECT CONCAT(s.name,'_',LPAD(ps.process_id,2,'0')) FROM processed_sample ps, sample s WHERE ps.sample_id=s.id AND ps.id=" + value.toString()).toString();
}

void NGSD::setSampleDiseaseData(const QString& sample_id, const QString& disease_group, const QString& disease_status)
{
	getQuery().exec("UPDATE sample SET disease_group='" + disease_group + "', disease_status='" + disease_status + "' WHERE id='" + sample_id + "'");
}

ProcessingSystemData NGSD::getProcessingSystemData(const QString& processed_sample_id, bool windows_path)
{
	ProcessingSystemData output;

	SqlQuery query = getQuery();
	query.exec("SELECT sys.name_manufacturer, sys.name_short, sys.type, sys.target_file, sys.adapter1_p5, sys.adapter2_p7, sys.shotgun, g.build FROM processing_system sys, genome g, processed_sample ps WHERE sys.genome_id=g.id AND sys.id=ps.processing_system_id AND ps.id=" + processed_sample_id);
	query.next();

	output.name = query.value(0).toString();
	output.name_short = query.value(1).toString();
	output.type = query.value(2).toString();
	output.target_file = query.value(3).toString();
	if (windows_path)
	{
		QString p_linux = getTargetFilePath(false, false);
		QString p_win = getTargetFilePath(false, true);
		output.target_file.replace(p_linux, p_win);
	}
	output.adapter1_p5 = query.value(4).toString();
	output.adapter2_p7 = query.value(5).toString();
	output.shotgun = query.value(6).toString()=="1";
	output.genome = query.value(7).toString();

	return output;
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
	query.prepare("SELECT id FROM sample WHERE name=:0");
	query.bindValue(0, parts[0]);
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
	QString sample = parts[0];
	QString ps_num = parts[1];
	if (ps_num.size()>2) ps_num = ps_num.left(2);

	//get sample ID
	SqlQuery query = getQuery(); //use binding (user input)
	query.prepare("SELECT ps.id FROM processed_sample ps, sample s WHERE s.name=:0 AND ps.sample_id=s.id AND ps.process_id=:1");
	query.bindValue(0, sample);
	query.bindValue(1, QString::number(ps_num.toInt()));
	query.exec();
	if (query.size()==0)
	{
		if(throw_if_fails)
		{
			THROW(DatabaseException, "Processed sample name '" + sample + "_" + ps_num + "' not found in NGSD!");
		}
		else
		{
			return "";
		}
	}
	query.next();
	return query.value(0).toString();
}

QString NGSD::processedSamplePath(const QString& processed_sample_id, PathType type)
{
	SqlQuery query = getQuery();
	query.prepare("SELECT CONCAT(s.name,'_',LPAD(ps.process_id,2,'0')), p.type, p.name FROM processed_sample ps, sample s, project p, processing_system sys WHERE ps.processing_system_id=sys.id AND ps.sample_id=s.id AND ps.project_id=p.id AND ps.id=:0");
	query.bindValue(0, processed_sample_id);
	query.exec();
	if (query.size()==0) THROW(DatabaseException, "Processed sample with id '" + processed_sample_id + "' not found in NGSD!");
	query.next();

	//create sample folder
	QString output = Settings::string("projects_folder") + "/";
	QString p_type = query.value(1).toString();
	output += p_type;
	QString p_name = query.value(2).toString();
	output += "/" + p_name + "/";
	QString ps_name = query.value(0).toString();
	output += "Sample_" + ps_name + "/";

	//append file name if requested
	if (type==BAM) output += ps_name + ".bam";
	else if (type==GSVAR) output += ps_name + ".GSvar";
	else if (type==VCF) output += ps_name + "_var_annotated.vcf.gz";
	else if (type!=FOLDER) THROW(ProgrammingException, "Unknown PathType '" + QString::number(type) + "'!");

	//convert to canonical path
	output = QFileInfo(output).absoluteFilePath();

	return output;
}

QString NGSD::variantId(const Variant& variant, bool throw_if_fails)
{
	SqlQuery query = getQuery(); //use binding user input (safety)
	query.prepare("SELECT id FROM variant WHERE chr=:0 AND start='"+QString::number(variant.start())+"' AND end='"+QString::number(variant.end())+"' AND ref=:1 AND obs=:2");
	query.bindValue(0, variant.chr().strNormalized(true));
	query.bindValue(1, variant.ref());
	query.bindValue(2, variant.obs());
	query.exec();
	if (query.size()==0)
	{
		if (throw_if_fails)
		{
			THROW(DatabaseException, "Variant " + variant.toString() + " not found in NGSD!");
		}
		else
		{
			return "";
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

NGSD::~NGSD()
{
	//Log::info("MYSQL closing  - name: " + db_->connectionName());

	//close database and remove it
	QString connection_name = db_->connectionName();
	db_.clear();
	QSqlDatabase::removeDatabase(connection_name);
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

QMap<QString, QString> NGSD::getProcessingSystems(bool skip_systems_without_roi, bool windows_paths)
{
	QMap<QString, QString> out;

	//load paths
	QString p_win;
	QString p_linux;
	if (windows_paths)
	{
		p_linux = getTargetFilePath(false, false);
		p_win = getTargetFilePath(false, true);
	}

	//load processing systems
	SqlQuery query = getQuery();
	query.exec("SELECT name_manufacturer, target_file FROM processing_system");
	while(query.next())
	{
		QString name = query.value(0).toString();
		QString roi = query.value(1).toString().replace(p_linux, p_win);
		if (roi=="" && skip_systems_without_roi) continue;
		out.insert(name, roi);
	}

	return out;
}

ValidationInfo NGSD::getValidationStatus(const QString& filename, const Variant& variant)
{
	SqlQuery query = getQuery();
	query.exec("SELECT status, type, comment FROM variant_validation WHERE sample_id='" + sampleId(filename) + "' AND variant_id='" + variantId(variant) + "'");
	if (query.size()==0)
	{
		return ValidationInfo();
	}
	else
	{
		query.next();
		return ValidationInfo{ query.value(0).toString().trimmed(), query.value(1).toString().trimmed(), query.value(2).toString().trimmed() };
	}
}

QCCollection NGSD::getQCData(const QString& processed_sample_id)
{
	//get QC data
	SqlQuery q = getQuery();
	q.exec("SELECT n.name, nm.value, n.description, n.qcml_id FROM processed_sample_qc as nm, qc_terms as n WHERE nm.processed_sample_id='" + processed_sample_id + "' AND nm.qc_terms_id=n.id");
	QCCollection output;
	while(q.next())
	{
		output.insert(QCValue(q.value(0).toString(), q.value(1).toString(), q.value(2).toString(), q.value(3).toString()));
	}

	//get KASP data
	SqlQuery q2 = getQuery();
	q2.exec("SELECT random_error_prob FROM kasp_status WHERE processed_sample_id='" + processed_sample_id + "'");
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

QVector<double> NGSD::getQCValues(const QString& accession, const QString& processed_sample_id)
{
	//get processing system ID
	QString sys_id = getValue("SELECT processing_system_id FROM processed_sample WHERE id='" + processed_sample_id + "'").toString();

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

void NGSD::annotate(VariantList& variants, QString filename, BedFile roi, double max_af)
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

	//load target region (if given)
	QScopedPointer<ChromosomalIndex<BedFile>> roi_index;
	if (roi.count()!=0)
	{
		if (!roi.isSorted())
		{
			THROW(ArgumentException, "Target region unsorted, but needs to be sorted for indexing!");
		}

		roi_index.reset(new ChromosomalIndex<BedFile>(roi));
	}

	//apply AF filter (if given)
	VariantFilter filter(variants);
	if (max_af>0)
	{
		filter.flagByAlleleFrequency(max_af);
	}

	//get required column indices
	int ihdb_all_hom_idx = variants.addAnnotationIfMissing("ihdb_allsys_hom", "Homozygous variant counts in NGSD independent of the processing system.");
	int ihdb_all_het_idx =  variants.addAnnotationIfMissing("ihdb_allsys_het", "Heterozygous variant counts in NGSD independent of the processing system.");
	int class_idx = variants.addAnnotationIfMissing("classification", "Classification from the NGSD.");
	int clacom_idx = variants.addAnnotationIfMissing("classification_comment", "Classification comment from the NGSD.");
	int valid_idx = variants.addAnnotationIfMissing("validated", "Validation information from the NGSD. Validation results of other samples are listed in brackets!");
	int comment_idx = variants.addAnnotationIfMissing("comment", "Variant comments from the NGSD.");
	int geneinfo_idx = variants.addAnnotationIfMissing("gene_info", "Gene information from NGSD (inheritance mode, ExAC pLI score).");
	int gene_idx = variants.annotationIndexByName("gene", true, false);

	/*
	//Timing benchmarks
	//Outcome for Qt 5.5.0:
	// - Prepared queries take about twice as long
	// - Setting the query to forward-only has no effect
	*/
	bool benchmark = false;
	QTime timer;
	long long time_id_co = 0;
	long long time_cl = 0;
	long long time_vv = 0;
	long long time_vvo = 0;
	long long time_gt = 0;

	//(re-)annotate the variants
	SqlQuery query = getQuery();
	QByteArray v_id;
	QByteArray comment;
	for (int i=0; i<variants.count(); ++i)
	{
		Variant& v = variants[i];

		//skip variants with too high allele frequency
		if(max_af>0.0)
		{
			if (!filter.flags()[i]) continue;
		}

		//skip variant outside the target region
		if (!roi_index.isNull())
		{
			if (roi_index->matchingIndex(v.chr(), v.start(), v.end())==-1)
			{
				continue;
			}
		}

		//variant id and comment
		if (benchmark) timer.start();
		query.exec("SELECT id, comment FROM variant WHERE chr='"+v.chr().strNormalized(true)+"' AND start='"+QString::number(v.start())+"' AND end='"+QString::number(v.end())+"' AND ref='"+v.ref()+"' AND obs='"+v.obs()+"'");
		if (query.size()==1)
		{
			query.next();
			v_id = query.value(0).toByteArray();
			comment = query.value(1).toByteArray();
		}
		else
		{
			v_id = "-1";
			comment = "";
		}
		if (benchmark) time_id_co += timer.elapsed();

		//variant classification
		if (benchmark) timer.restart();
		QVariant classification = getValue("SELECT class FROM variant_classification WHERE variant_id='" + v_id + "'", true);
		if (!classification.isNull())
		{
			v.annotations()[class_idx] = classification.toByteArray().replace("n/a", "");
			v.annotations()[clacom_idx] = getValue("SELECT comment FROM variant_classification WHERE variant_id='" + v_id + "'", true).toByteArray().replace("\n", " ").replace("\t", " ");
		}
		if (benchmark) time_cl += timer.elapsed();

		//validation info
		if (benchmark) timer.restart();
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
		if (benchmark) time_vv += timer.elapsed();

		//validation info other samples
		if (benchmark) timer.restart();
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
		if (benchmark) time_vvo += timer.elapsed();

		//genotype counts
		if (benchmark) timer.restart();
		int allsys_hom_count = 0;
		int allsys_het_count = 0;
		query.exec("SELECT count_hom, count_het FROM detected_variant_counts WHERE variant_id='"+v_id+"'");
		if (query.size()==1) // use counts from cache
		{
			query.next();
			allsys_hom_count = query.value(0).toInt();
			allsys_het_count = query.value(1).toInt();
		}
		else // no cache value => count
		{
			query.exec("SELECT COUNT(DISTINCT ps.sample_id), dv.genotype FROM detected_variant dv, processed_sample ps WHERE dv.variant_id='"+v_id+"' AND dv.processed_sample_id=ps.id GROUP BY dv.genotype");
			while(query.next())
			{
				if (query.value(1).toByteArray()=="hom")
				{
					 allsys_hom_count = query.value(0).toInt();
				}
				else
				{
					allsys_het_count = query.value(0).toInt();
				}
			}
		}
		//qDebug() << (v.isSNV() ? "S" : "I") << allsys_hom_count << allsys_het_count << timer.elapsed();

		v.annotations()[ihdb_all_hom_idx] = QByteArray::number(allsys_hom_count);
		v.annotations()[ihdb_all_het_idx] = QByteArray::number(allsys_het_count);
		v.annotations()[comment_idx] = comment.replace("\n", " ").replace("\t", " ");
		v.annotations()[valid_idx] = val_status;
		if (benchmark) time_gt += timer.elapsed();

		//gene info
		if (gene_idx!=-1)
		{
			QByteArrayList genes = v.annotations()[gene_idx].split(',');
			std::transform(genes.begin(), genes.end(), genes.begin(), [this](const QByteArray& g) { return geneInfo(g).toString().toLatin1(); });
			v.annotations()[geneinfo_idx] = genes.join(", ");
		}

		emit updateProgress(100*i/variants.count());
	}

	if (benchmark)
	{
		qDebug() << "id+com  : " << time_id_co;
		qDebug() << "class   : " << time_cl;
		qDebug() << "val     : " << time_vv;
		qDebug() << "val oth : " << time_vvo;
		qDebug() << "counts  : " << time_gt;
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

	//get required column indices
	int som_ihdb_c_idx = variants.addAnnotationIfMissing("som_ihdb_c", "Somatic variant count within NGSD.");
	int som_ihdb_p_idx = variants.addAnnotationIfMissing("som_ihdb_p", "Projects with somatic variant in NGSD.");

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


void NGSD::setValidationStatus(const QString& filename, const Variant& variant, const ValidationInfo& info, QString user_name)
{
	QString s_id = sampleId(filename);
	QString v_id = variantId(variant);
	QVariant vv_id = getValue("SELECT id FROM variant_validation WHERE sample_id='" + s_id + "' AND variant_id='" + v_id + "'");

	SqlQuery query = getQuery(); //use binding (user input)
	if (vv_id.isNull()) //insert
	{
		QString user_id = userId(user_name);
		QString geno = getValue("SELECT genotype FROM detected_variant WHERE variant_id='" + v_id + "' AND processed_sample_id='" + processedSampleId(filename) + "'", false).toString();
		query.prepare("INSERT INTO variant_validation (user_id, sample_id, variant_id, genotype, status, type, comment) VALUES ('" + user_id + "','" + s_id + "','" + v_id + "','" + geno + "',:0,:1,:2)");
	}
	else //update
	{
		query.prepare("UPDATE variant_validation SET status=:0, type=:1, comment=:2 WHERE id='" + vv_id.toString() + "'");
	}
	query.bindValue(0, info.status);
	query.bindValue(1, info.type);
	query.bindValue(2, info.comments);
	query.exec();
}

ClassificationInfo NGSD::getClassification(const Variant& variant)
{
	SqlQuery query = getQuery();
	query.exec("SELECT class, comment FROM variant_classification WHERE variant_id='" + variantId(variant) + "'");
	if (query.size()==0)
	{
		return ClassificationInfo();
	}
	else
	{
		query.next();
		return ClassificationInfo {query.value(0).toString().trimmed(), query.value(1).toString().trimmed() };
	}
}

void NGSD::setClassification(const Variant& variant, ClassificationInfo info)
{
	SqlQuery query = getQuery(); //use binding (user input)
	query.prepare("INSERT INTO variant_classification (variant_id, class, comment) VALUES (" + variantId(variant) + ",:0,:1) ON DUPLICATE KEY UPDATE class=:2, comment=:3");
	query.bindValue(0, info.classification);
	query.bindValue(1, info.comments);
	query.bindValue(2, info.classification);
	query.bindValue(3, info.comments);
	query.exec();
}

void NGSD::addVariantPublication(QString filename, const Variant& variant, QString database, QString classification, QString details)
{
	QString s_id = sampleId(filename);
	QString v_id = variantId(variant);
	QString user_id = userId();

	//insert
	getQuery().exec("INSERT INTO variant_publication (sample_id, variant_id, db, class, details, user_id) VALUES ("+s_id+","+v_id+", '"+database+"', '"+classification+"', '"+details+"', "+user_id+")");
}

QString NGSD::getVariantPublication(QString filename, const Variant& variant)
{
	QString s_id = sampleId(filename, false);
	QString v_id = variantId(variant, false);

	if (s_id=="" || v_id=="") return "";

	//select
	SqlQuery query = getQuery();
	query.exec("SELECT vp.db, vp.class, vp.details, vp.date, u.name FROM variant_publication vp LEFT JOIN user u on vp.user_id=u.id WHERE sample_id="+s_id+" AND variant_id="+v_id);

	//create output
	QStringList output;
	while (query.next())
	{
		output << "db: " + query.value("db").toString() + " class: " + query.value("class").toString() + " user: " + query.value("name").toString() + " date: " + query.value("date").toString().replace("T", " ") + "\n  " + query.value("details").toString().replace(";", "\n  ").replace("=", ": ");
	}

	return output.join("\n");
}

QString NGSD::comment(const Variant& variant)
{
	return getValue("SELECT comment FROM variant WHERE id='" + variantId(variant) + "'").toString();
}

QString NGSD::url(const QString& filename, const Variant& variant)
{
	return Settings::string("NGSD")+"/variants/view/" + processedSampleId(filename) + "," + variantId(variant);
}

QString NGSD::url(const QString& filename)
{
	return Settings::string("NGSD")+"/processedsamples/view/" + processedSampleId(filename);
}

QString NGSD::urlSearch(const QString& search_term)
{
	return Settings::string("NGSD")+"/search/processSearch/search_term=" + search_term;
}

int NGSD::lastAnalysisOf(QString processed_sample_id)
{
	SqlQuery query = getQuery();
	query.exec("SELECT j.id FROM analysis_job j, analysis_job_sample js WHERE js.analysis_job_id=j.id AND js.processed_sample_id=" + processed_sample_id + " AND j.type='single sample' ORDER BY j.id DESC LIMIT 1");
	if (query.next())
	{
		return query.value(0).toInt();
	}

	return -1;
}

AnalysisJob NGSD::analysisInfo(int job_id)
{
	AnalysisJob output;

	SqlQuery query = getQuery();
	query.exec("SELECT * FROM analysis_job WHERE id=" + QString::number(job_id));
	if (query.next())
	{
		output.type = query.value("type").toString();
		output.args = query.value("args").toString();
		output.sge_id = query.value("sge_id").toString();
		output.sge_queue = query.value("sge_queue").toString();

		//extract samples
		SqlQuery query2 = getQuery();
		query2.exec("SELECT CONCAT(s.name,'_',LPAD(ps.process_id,2,'0')), js.info FROM analysis_job_sample js, processed_sample ps, sample s WHERE js.analysis_job_id=" + QString::number(job_id) + " AND js.processed_sample_id=ps.id AND ps.sample_id=s.id ORDER by js.id ASC");
		while(query2.next())
		{
			output.samples << AnalysisJobSample { query2.value(0).toString(), query2.value(1).toString() };
		}

		//extract status
		query2.exec("SELECT js.time, u.user_id, js.status, js.output FROM analysis_job_history js LEFT JOIN user u ON js.user_id=u.id  WHERE js.analysis_job_id=" + QString::number(job_id) + " ORDER BY js.id ASC");
		while(query2.next())
		{
			output.history << AnalysisJobHistoryEntry { query2.value(0).toDateTime(), query2.value(1).toString(), query2.value(2).toString(), query2.value(3).toString().split('\n') };
		}
	}

	return output;
}

void NGSD::queueAnalysis(QString type, QStringList args, QList<AnalysisJobSample> samples, QString user_name)
{
	SqlQuery query = getQuery();

	//insert job
	query.exec("INSERT INTO `analysis_job`(`type`, `args`) VALUES ('" + type + "','" + args.join(" ") +  "')");
	QString job_id = query.lastInsertId().toString();

	//insert samples
	foreach(const AnalysisJobSample& sample, samples)
	{
		query.exec("INSERT INTO `analysis_job_sample`(`analysis_job_id`, `processed_sample_id`, `info`) VALUES (" + job_id + ",'" + processedSampleId(sample.name) + "','" + sample.info + "')");
	}

	//insert status
	query.exec("INSERT INTO `analysis_job_history`(`analysis_job_id`, `time`, `user_id`, `status`, `output`) VALUES (" + job_id + ",'" + Helper::dateTime("") + "'," + userId(user_name) + ",'queued', '')");
}

void NGSD::cancelAnalysis(int job_id, QString user_name)
{
	SqlQuery query = getQuery();
	query.exec("INSERT INTO `analysis_job_history`(`analysis_job_id`, `time`, `user_id`, `status`, `output`) VALUES (" + QString::number(job_id) + ",'" + Helper::dateTime("") + "'," + userId(user_name) + ",'canceled', '')");
}

QString NGSD::getTargetFilePath(bool subpanels, bool windows)
{
	QString key = windows ? "target_file_folder_windows" : "target_file_folder_linux";
	QString output = Settings::string(key);
	if (output=="")
	{
		THROW(ProgrammingException, "'" + key + "' entry is missing in settings!");
	}

	if (subpanels)
	{
		output += "/subpanels/";
	}

	return output;
}

void NGSD::fixGeneNames(QTextStream* messages, bool fix_errors, QString table, QString column)
{
	SqlQuery query = getQuery();
	query.exec("SELECT DISTINCT " + column + " FROM " + table + " tmp WHERE NOT EXISTS(SELECT * FROM gene WHERE symbol=tmp." + column + ")");
	while(query.next())
	{
		*messages << "Outdated gene name in '" << table << "': " << query.value(0).toString() << endl;
		if (fix_errors)
		{
			QString gene = query.value(0).toString();
			auto approved_data = geneToApprovedWithMessage(gene);
			if (approved_data.second.startsWith("ERROR"))
			{
				*messages << "  FAIL: Cannot fix error in '" << gene << "' because: " << approved_data.second << endl;
			}
			else
			{
				getQuery().exec("UPDATE " + table + " SET " + column + "='" + approved_data.first + "' WHERE " + column + "='" + gene +"'");
			}
		}
	}
}

void NGSD::maintain(QTextStream* messages, bool fix_errors)
{
	SqlQuery query = getQuery();

	// (1) tumor samples variants that have been imported into 'detected_variant' table
	query.exec("SELECT CONCAT(s.name,'_',LPAD(ps.process_id,2,'0')), ps.id FROM sample s, processed_sample ps WHERE ps.sample_id=s.id AND s.tumor='1' AND EXISTS(SELECT * FROM detected_variant WHERE processed_sample_id=ps.id)");
	while(query.next())
	{
		*messages << "Tumor sample imported into germline variant table: " << query.value(0).toString() << endl;

		if (fix_errors)
		{
			getQuery().exec("DELETE FROM detected_variant WHERE processed_sample_id=" + query.value(1).toString());
		}
	}

	// (2) outdated gene names
	fixGeneNames(messages, fix_errors, "geneinfo_germline", "symbol");
	fixGeneNames(messages, fix_errors,"hpo_genes", "gene");

	// (3) variants/qc-data/KASP present for merged processed samples
	query.exec("SELECT CONCAT(s.name,'_',LPAD(ps.process_id,2,'0')), p.type, p.name, s.id, ps.id FROM sample s, processed_sample ps, project p WHERE ps.sample_id=s.id AND ps.project_id=p.id");
	while(query.next())
	{
		QString ps_name = query.value(0).toString();
		QString p_type = query.value(1).toString();

		QString folder = Settings::string("projects_folder") + "/" + p_type + "/" + query.value(2).toString() + "/Sample_" + ps_name + "/";
		if (!QFile::exists(folder))
		{
			QString ps_id = query.value(4).toString();

			//check if merged
			bool merged = false;
			SqlQuery query2 = getQuery();
			query2.exec("SELECT CONCAT(s.name,'_',LPAD(ps.process_id,2,'0')), p.type, p.name FROM sample s, processed_sample ps, project p WHERE ps.sample_id=s.id AND ps.project_id=p.id AND s.id='" + query.value(3).toString()+"' AND ps.id!='" + ps_id + "'");
			while(query2.next())
			{
				QString folder2 = Settings::string("projects_folder") + "/" + query2.value(1).toString() + "/" + query2.value(2).toString() + "/Sample_" + query2.value(0).toString() + "/";
				if (QFile::exists(folder2))
				{
					QStringList files = Helper::findFiles(folder2, ps_name + "*.fastq.gz", false);
					if (files.count()>0)
					{
						//qDebug() << "Sample " << ps_name << " merged into sample folder " << folder2 << "!" << endl;
						merged = true;
					}
				}
			}

			//check status
			if (merged)
			{
				//check if variants are present
				int c_var = getValue("SELECT COUNT(*) FROM detected_variant WHERE processed_sample_id='" + ps_id + "'").toInt();
				if (c_var>0)
				{
					*messages << "Merged sample " << ps_name << " has variant data!" << endl;

					if (fix_errors)
					{
						getQuery().exec("DELETE FROM detected_variant WHERE processed_sample_id='" + ps_id + "'");
					}
				}
				int c_qc = getValue("SELECT COUNT(*) FROM processed_sample_qc WHERE processed_sample_id='" + ps_id + "'").toInt();
				if (c_qc>0)
				{
					*messages << "Merged sample " << ps_name << " has QC data!" << endl;

					if (fix_errors)
					{
						getQuery().exec("DELETE FROM processed_sample_qc WHERE processed_sample_id='" + ps_id + "'");
					}
				}
				if (p_type=="diagnostic")
				{
					QVariant kasp = getValue("SELECT random_error_prob FROM kasp_status WHERE processed_sample_id='" + ps_id + "'");
					if (kasp.isNull())
					{
						*messages << "Merged sample " << ps_name << " KASP result missing!" << endl;

						if (fix_errors)
						{
							getQuery().exec("INSERT INTO `kasp_status`(`processed_sample_id`, `random_error_prob`, `snps_evaluated`, `snps_match`) VALUES ('" + ps_id + "',999,0,0)");
						}
					}
				}
			}
		}
	}

	//(4) variants for bad processed samples
	query.exec("SELECT CONCAT(s.name,'_',LPAD(ps.process_id,2,'0')), ps.id FROM sample s, processed_sample ps WHERE ps.sample_id=s.id AND ps.quality='bad'");
	while(query.next())
	{
		QString ps_id = query.value(1).toString();

		//check if variants are present
		int c_var = getValue("SELECT COUNT(*) FROM detected_variant WHERE processed_sample_id='" + ps_id + "'").toInt();
		if (c_var>0)
		{
			*messages << "Bad sample " << query.value(0).toString() << " has variant data!" << endl;

			if (fix_errors)
			{
				getQuery().exec("DELETE FROM detected_variant WHERE processed_sample_id='" + ps_id + "'");
			}
		}
	}
}

void NGSD::setComment(const Variant& variant, const QString& text)
{
	SqlQuery query = getQuery();
	query.prepare("UPDATE variant SET comment=:1 WHERE id='" + variantId(variant) + "'");
	query.bindValue(0, text);
	query.exec();
}

QString NGSD::nextProcessingId(const QString& sample_id)
{
	QString max_num = getValue("SELECT MAX(process_id) FROM processed_sample WHERE sample_id=" + sample_id).toString();

	return max_num.isEmpty() ? "1" : QString::number(max_num.toInt()+1);
}

void NGSD::precalculateGenotypeCounts(QTextStream* messages, int progress_interval)
{
	//init
	QTime timer_overall;
	timer_overall.start();
	int deleted = 0;

	//get variant IDs
	SqlQuery query = getQuery();
	query.exec("SELECT id FROM variant");
	int variant_count = query.size();
	if (messages)
	{
		(*messages) << Helper::dateTime() << "\tstarting processing of " << variant_count << " variants" << endl;
	}

	QTime timer;
	timer.start();
	int i = 0;
	while(query.next())
	{
		//update counts
		QByteArray var_id = query.value(0).toByteArray();
		QByteArray count_het = getValue("SELECT COUNT(DISTINCT ps.sample_id) FROM detected_variant dv, processed_sample ps WHERE dv.variant_id='"+var_id+"' AND dv.processed_sample_id=ps.id AND dv.genotype='het'").toByteArray();
		QByteArray count_hom = getValue("SELECT COUNT(DISTINCT ps.sample_id) FROM detected_variant dv, processed_sample ps WHERE dv.variant_id='"+var_id+"' AND dv.processed_sample_id=ps.id AND dv.genotype='hom'").toByteArray();
		getQuery().exec("INSERT INTO detected_variant_counts (variant_id, count_het, count_hom) VALUES ("+var_id+",'"+count_het+"', "+count_hom+") ON DUPLICATE KEY UPDATE count_het='"+count_het+"',count_hom='"+count_hom+"'");

		//delete variants that are not used
		if (count_het=="0" && count_hom=="0")
		{
			int used = getValue("SELECT COUNT(*) FROM detected_somatic_variant WHERE variant_id='" + var_id + "'").toInt();
			if (used==0) used += getValue("SELECT COUNT(*) FROM variant_validation WHERE variant_id='" + var_id + "'").toInt();
			if (used==0) used += getValue("SELECT COUNT(*) FROM variant_classification WHERE variant_id='" + var_id + "'").toInt();
			if (used==0)
			{
				getQuery().exec("DELETE FROM `detected_variant_counts` WHERE variant_id='" + var_id + "'");
				getQuery().exec("DELETE FROM `variant` WHERE id='" + var_id + "'");
				++deleted;
			}
		}

		//print progress
		++i;
		if (progress_interval>0 && (i%progress_interval)==0)
		{
			if (messages)
			{
				(*messages) << Helper::dateTime() << "\tprogress: variant " << i << " / " << variant_count << " - deleted " << deleted << " - took " << Helper::elapsedTime(timer) << endl;
				timer.restart();
			}
		}
	}
	if (messages)
	{
		(*messages) << Helper::dateTime() << "\tfinished processing " << variant_count << " variants" << endl;
		(*messages) << Helper::dateTime() << "\tdeleted " << deleted << " variants" << endl;
		(*messages) << Helper::dateTime() << "\ttook " << Helper::elapsedTime(timer_overall) << endl;
	}
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
		type = type.mid(6,type.length()-8);
		cache[hash] = type.split("','");
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

void NGSD::clearTable(QString table)
{
	SqlQuery query = getQuery();
	query.exec("DELETE FROM " + table);
}

int NGSD::geneToApprovedID(const QByteArray& gene)
{
	//approved
	SqlQuery q_gene = getQuery();
	q_gene.prepare("SELECT id FROM gene WHERE symbol=:0");
	q_gene.bindValue(0, gene);
	q_gene.exec();
	if (q_gene.size()==1)
	{
		q_gene.next();
		return q_gene.value(0).toInt();
	}

	//previous
	SqlQuery q_prev = getQuery();
	q_prev.prepare("SELECT g.id FROM gene g, gene_alias ga WHERE g.id=ga.gene_id AND ga.symbol=:0 AND ga.type='previous'");
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
	SqlQuery q_syn = getQuery();
	q_syn.prepare("SELECT g.id FROM gene g, gene_alias ga WHERE g.id=ga.gene_id AND ga.symbol=:0 AND ga.type='synonym'");
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

QByteArray NGSD::geneToApproved(QByteArray gene, bool return_input_when_unconvertable)
{
	gene = gene.trimmed().toUpper();

	//already approved gene
	if (approvedGeneNames().contains(gene))
	{
		return gene;
	}

	//check if already cached
	static QMap<QByteArray, QByteArray> mapping;
	if (mapping.contains(gene))
	{
		if (return_input_when_unconvertable && mapping[gene].isEmpty())
		{
			return gene;
		}

		return mapping[gene];
	}

	//not cached => try to convert
	int gene_id = geneToApprovedID(gene);
	mapping[gene] = (gene_id!=-1) ? geneSymbol(gene_id) : "";

	if (return_input_when_unconvertable && mapping[gene].isEmpty())
	{
		return gene;
	}

	return mapping[gene];
}

GeneSet NGSD::genesToApproved(GeneSet genes, bool return_input_when_unconvertable)
{
	GeneSet output;

	foreach(const QByteArray& gene, genes)
	{
		QByteArray gene_new = geneToApproved(gene, return_input_when_unconvertable);
		if (!gene_new.isEmpty())
		{
			output.insert(gene_new);
		}
	}

	return output;
}

QPair<QString, QString> NGSD::geneToApprovedWithMessage(const QString& gene)
{
	//approved
	SqlQuery q_gene = getQuery();
	q_gene.prepare("SELECT id FROM gene WHERE symbol=:0");
	q_gene.bindValue(0, gene);
	q_gene.exec();
	if (q_gene.size()==1)
	{
		q_gene.next();
		return qMakePair(gene, QString("KEPT: " + gene + " is an approved symbol"));
	}

	//previous
	SqlQuery q_prev = getQuery();
	q_prev.prepare("SELECT g.symbol FROM gene g, gene_alias ga WHERE g.id=ga.gene_id AND ga.symbol=:0 AND ga.type='previous'");
	q_prev.bindValue(0, gene);
	q_prev.exec();
	if (q_prev.size()==1)
	{
		q_prev.next();
		return qMakePair(q_prev.value(0).toString(), "REPLACED: " + gene + " is a previous symbol");
	}
	else if(q_prev.size()>1)
	{
		QString genes;
		while(q_prev.next())
		{
			if (!genes.isEmpty()) genes.append(", ");
			genes.append(q_prev.value(0).toString());
		}
		return qMakePair(gene, "ERROR: " + gene + " is a previous symbol of the genes " + genes);
	}

	//synonymous
	SqlQuery q_syn = getQuery();
	q_syn.prepare("SELECT g.symbol FROM gene g, gene_alias ga WHERE g.id=ga.gene_id AND ga.symbol=:0 AND ga.type='synonym'");
	q_syn.bindValue(0, gene);
	q_syn.exec();
	if (q_syn.size()==1)
	{
		q_syn.next();
		return qMakePair(q_syn.value(0).toString(), "REPLACED: " + gene + " is a synonymous symbol");
	}
	else if(q_syn.size()>1)
	{
		QByteArray genes;
		while(q_syn.next())
		{
			if (!genes.isEmpty()) genes.append(", ");
			genes.append(q_syn.value(0).toString());
		}
		return qMakePair(gene, "ERROR: " + gene + " is a synonymous symbol of the genes " + genes);
	}

	return qMakePair(gene, QString("ERROR: " + gene + " is unknown symbol"));
}

GeneSet NGSD::previousSymbols(int id)
{
	GeneSet output;

	SqlQuery q = getQuery();
	q.exec("SELECT symbol FROM gene_alias WHERE gene_id='" + QByteArray::number(id) + "' AND type='previous'");
	while(q.next())
	{
		output.insert(q.value(0).toByteArray());
	}

	return output;
}

GeneSet NGSD::synonymousSymbols(int id)
{
	GeneSet output;

	SqlQuery q = getQuery();
	q.exec("SELECT symbol FROM gene_alias WHERE gene_id='" + QByteArray::number(id) + "' AND type='synonymous'");
	while(q.next())
	{
		output.insert(q.value(0).toByteArray());
	}

	return output;
}

QList<Phenotype> NGSD::phenotypes(const QByteArray& symbol)
{
	QList<Phenotype> output;

	SqlQuery query = getQuery();
	query.prepare("SELECT t.hpo_id, t.name FROM hpo_term t, hpo_genes g WHERE g.gene=:0 AND t.id=g.hpo_term_id ORDER BY t.name ASC");
	query.bindValue(0, symbol);
	query.exec();
	while(query.next())
	{
		output << Phenotype(query.value(0).toByteArray(), query.value(1).toByteArray());
	}

	return output;
}

QList<Phenotype> NGSD::phenotypes(QStringList search_terms)
{
	//trim terms and remove empty terms
	std::for_each(search_terms.begin(), search_terms.end(), [](QString& term){ term = term.trimmed(); });
	search_terms.removeAll("");

	QList<Phenotype> list;

	if (search_terms.isEmpty()) //no terms => all phenotypes
	{
		SqlQuery query = getQuery();
		query.exec("SELECT hpo_id, name FROM hpo_term ORDER BY name ASC");
		while(query.next())
		{
			list << Phenotype(query.value(0).toByteArray(), query.value(1).toByteArray());
		}
	}
	else //search for terms (intersect results of all terms)
	{
		bool first = true;
		QSet<Phenotype> set;
		SqlQuery query = getQuery();
		query.prepare("SELECT hpo_id, name FROM hpo_term WHERE name LIKE :0 OR hpo_id LIKE :1");
		foreach(const QString& term, search_terms)
		{
			query.bindValue(0, "%" + term + "%");
			query.bindValue(1, "%" + term + "%");
			query.exec();
			QSet<Phenotype> tmp;
			while(query.next())
			{
				tmp << Phenotype(query.value(0).toByteArray(), query.value(1).toByteArray());
			}

			if (first)
			{
				set = tmp;
				first = false;
			}
			else
			{
				set = set.intersect(tmp);
			}
		}

		list = set.toList();
		std::sort(list.begin(), list.end(), [](const Phenotype& a, const Phenotype& b){ return a.name()<b.name(); });
	}

	return list;
}

GeneSet NGSD::phenotypeToGenes(const Phenotype& phenotype, bool recursive)
{
	//prepare queries
	SqlQuery pid2genes = getQuery();
	pid2genes.prepare("SELECT gene FROM hpo_genes WHERE hpo_term_id=:0");
	SqlQuery pid2children = getQuery();
	pid2children.prepare("SELECT child FROM hpo_parent WHERE parent=:0");

	//convert phenotype to id
	SqlQuery tmp = getQuery();
	tmp.prepare("SELECT id FROM hpo_term WHERE name=:0");
	tmp.bindValue(0, phenotype.name());
	tmp.exec();
	if (!tmp.next()) THROW(ProgrammingException, "Unknown phenotype '" + phenotype.toString() + "'!");
	QList<int> pheno_ids;
	pheno_ids << tmp.value(0).toInt();

	GeneSet genes;
	while (!pheno_ids.isEmpty())
	{
		int id = pheno_ids.last();
		pheno_ids.removeLast();

		//add genes of current phenotype
		pid2genes.bindValue(0, id);
		pid2genes.exec();
		while(pid2genes.next())
		{
			QString gene = pid2genes.value(0).toString();
			QPair<QString, QString> geneinfo = geneToApprovedWithMessage(gene);
			genes.insert(geneinfo.first.toLatin1());
		}

		//add sub-phenotypes
		if (recursive)
		{
			pid2children.bindValue(0, id);
			pid2children.exec();
			while(pid2children.next())
			{
				pheno_ids << pid2children.value(0).toInt();
			}
		}
	}

	return genes;
}

QList<Phenotype> NGSD::phenotypeChildTems(const Phenotype& phenotype, bool recursive)
{
	//prepare queries
	SqlQuery pid2children = getQuery();
	pid2children.prepare("SELECT t.id, t.hpo_id, t.name  FROM hpo_parent p, hpo_term t WHERE p.parent=:0 AND p.child=t.id");

	//convert phenotype to id
	QList<int> pheno_ids;
	bool ok;
	pheno_ids << getValue("SELECT id FROM hpo_term WHERE name='" + phenotype.name() + "'").toInt(&ok);
	if (!ok) THROW(ProgrammingException, "Unknown phenotype '" + phenotype.toString() + "'!");

	QList<Phenotype> terms;
	while (!pheno_ids.isEmpty())
	{
		int id = pheno_ids.takeLast();

		pid2children.bindValue(0, id);
		pid2children.exec();
		while(pid2children.next())
		{
			terms.append(Phenotype(pid2children.value(1).toByteArray(), pid2children.value(2).toByteArray()));
			if (recursive)
			{
				pheno_ids << pid2children.value(0).toInt();
			}
		}
	}

	return terms;
}

QByteArray NGSD::phenotypeAccessionToName(const QByteArray& accession)
{
	return getValue("SELECT name FROM hpo_term WHERE hpo_id='" + accession + "'", false).toByteArray();
}

const GeneSet& NGSD::approvedGeneNames()
{
	static GeneSet output;

	if (output.count()==0)
	{
		SqlQuery query = getQuery();
		query.exec("SELECT symbol from gene");

		while(query.next())
		{
			output.insert(query.value(0).toByteArray());
		}
	}

	return output;
}


GeneSet NGSD::genesOverlapping(const Chromosome& chr, int start, int end, int extend)
{
	//init static data (load gene regions file from NGSD to memory)
	static BedFile bed;
	static ChromosomalIndex<BedFile> index(bed);
	if (bed.count()==0)
	{
		//add transcripts
		SqlQuery query = getQuery();
		query.exec("SELECT g.symbol, gt.chromosome, MIN(ge.start), MAX(ge.end) FROM gene g, gene_transcript gt, gene_exon ge WHERE ge.transcript_id=gt.id AND gt.gene_id=g.id GROUP BY gt.id");
		while(query.next())
		{
			bed.append(BedLine(query.value(1).toString(), query.value(2).toInt(), query.value(3).toInt(), QList<QByteArray>() << query.value(0).toByteArray()));
		}

		//sort and index
		bed.sort();
		index.createIndex();
	}

	//create gene list
	GeneSet genes;
	QVector<int> matches = index.matchingIndices(chr, start-extend, end+extend);
	foreach(int i, matches)
	{
		genes << bed[i].annotations()[0];
	}
	return genes;
}

GeneSet NGSD::genesOverlappingByExon(const Chromosome& chr, int start, int end, int extend)
{
	//init static data (load gene regions file from NGSD to memory)
	static BedFile bed;
	static ChromosomalIndex<BedFile> index(bed);
	if (bed.count()==0)
	{
		SqlQuery query = getQuery();
		query.exec("SELECT DISTINCT g.symbol, gt.chromosome, ge.start, ge.end FROM gene g, gene_exon ge, gene_transcript gt WHERE ge.transcript_id=gt.id AND gt.gene_id=g.id");
		while(query.next())
		{
			bed.append(BedLine(query.value(1).toString(), query.value(2).toInt(), query.value(3).toInt(), QList<QByteArray>() << query.value(0).toByteArray()));
		}
		bed.sort();
		index.createIndex();
	}

	//create gene list
	GeneSet genes;
	QVector<int> matches = index.matchingIndices(chr, start-extend, end+extend);
	foreach(int i, matches)
	{
		genes << bed[i].annotations()[0];
	}

	return genes;
}

BedFile NGSD::genesToRegions(const GeneSet& genes, Transcript::SOURCE source, QString mode, bool fallback, bool annotate_transcript_names, QTextStream* messages)
{
	QString source_str = Transcript::sourceToString(source);

	//check mode
	QStringList valid_modes;
	valid_modes << "gene" << "exon";
	if (!valid_modes.contains(mode))
	{
		THROW(ArgumentException, "Invalid mode '" + mode + "'. Valid modes are: " + valid_modes.join(", ") + ".");
	}

	//prepare queries
	SqlQuery q_transcript = getQuery();
	q_transcript.prepare("SELECT id, chromosome, start_coding, end_coding, name FROM gene_transcript WHERE source='" + source_str + "' AND gene_id=:1");
	SqlQuery q_transcript_fallback = getQuery();
	q_transcript_fallback.prepare("SELECT id, chromosome, start_coding, end_coding, name FROM gene_transcript WHERE gene_id=:1");
	SqlQuery q_range = getQuery();
	q_range.prepare("SELECT MIN(start), MAX(end) FROM gene_exon WHERE transcript_id=:1");
	SqlQuery q_exon = getQuery();
	q_exon.prepare("SELECT start, end FROM gene_exon WHERE transcript_id=:1");

	//process input data
	BedFile output;
	foreach(QByteArray gene, genes)
	{
		//get approved gene id
		int id = geneToApprovedID(gene);
		if (id==-1)
		{
			if (messages) *messages << "Gene name '" << gene << "' is no HGNC-approved symbol. Skipping it!" << endl;
			continue;
		}
		gene = geneSymbol(id);

		//prepare annotations
		QList<QByteArray> annos;
		annos << gene;

		//GENE mode
		if (mode=="gene")
		{
			bool hits = false;

			//add source transcripts
			q_transcript.bindValue(0, id);
			q_transcript.exec();
			while(q_transcript.next())
			{
				if (annotate_transcript_names)
				{
					annos.clear();
					annos << gene + " " + q_transcript.value(4).toByteArray();
				}

				q_range.bindValue(0, q_transcript.value(0).toInt());
				q_range.exec();
				q_range.next();

				output.append(BedLine("chr"+q_transcript.value(1).toByteArray(), q_range.value(0).toInt(), q_range.value(1).toInt(), annos));
				hits = true;
			}

			//add fallback transcripts
			if (!hits && fallback)
			{
				q_transcript_fallback.bindValue(0, id);
				q_transcript_fallback.exec();
				while(q_transcript_fallback.next())
				{
					if (annotate_transcript_names)
					{
						annos.clear();
						annos << gene + " " + q_transcript_fallback.value(4).toByteArray();
					}


					q_range.bindValue(0, q_transcript_fallback.value(0).toInt());
					q_range.exec();
					q_range.next();

					output.append(BedLine("chr"+q_transcript_fallback.value(1).toByteArray(), q_range.value(0).toInt(), q_range.value(1).toInt(), annos));

					hits = true;
				}
			}

			if (!hits && messages!=nullptr)
			{
				*messages << "No transcripts found for gene '" + gene + "'. Skipping it!" << endl;
			}
		}

		//EXON mode
		else if (mode=="exon")
		{
			bool hits = false;

			q_transcript.bindValue(0, id);
			q_transcript.exec();
			while(q_transcript.next())
			{
				if (annotate_transcript_names)
				{
					annos.clear();
					annos << gene + " " + q_transcript.value(4).toByteArray();
				}

				int trans_id = q_transcript.value(0).toInt();
				bool is_coding = !q_transcript.value(2).isNull() && !q_transcript.value(3).isNull();
				int start_coding = q_transcript.value(2).toInt();
				int end_coding = q_transcript.value(3).toInt();

				q_exon.bindValue(0, trans_id);
				q_exon.exec();
				while(q_exon.next())
				{
					int start = q_exon.value(0).toInt();
					int end = q_exon.value(1).toInt();
					if (is_coding)
					{
						start = std::max(start_coding, start);
						end = std::min(end_coding, end);

						//skip non-coding exons of coding transcripts
						if (end<start_coding || start>end_coding) continue;
					}

					output.append(BedLine("chr"+q_transcript.value(1).toByteArray(), start, end, annos));
					hits = true;
				}
			}

			//fallback
			if (!hits && fallback)
			{
				q_transcript_fallback.bindValue(0, id);
				q_transcript_fallback.exec();
				while(q_transcript_fallback.next())
				{
					if (annotate_transcript_names)
					{
						annos.clear();
						annos << gene + " " + q_transcript_fallback.value(4).toByteArray();
					}

					int trans_id = q_transcript_fallback.value(0).toInt();
					bool is_coding = !q_transcript_fallback.value(2).isNull() && !q_transcript_fallback.value(3).isNull();
					int start_coding = q_transcript_fallback.value(2).toInt();
					int end_coding = q_transcript_fallback.value(3).toInt();
					q_exon.bindValue(0, trans_id);
					q_exon.exec();
					while(q_exon.next())
					{
						int start = q_exon.value(0).toInt();
						int end = q_exon.value(1).toInt();
						if (is_coding)
						{
							start = std::max(start_coding, start);
							end = std::min(end_coding, end);

							//skip non-coding exons of coding transcripts
							if (end<start_coding || start>end_coding) continue;
						}

						output.append(BedLine("chr"+q_transcript_fallback.value(1).toByteArray(), start, end, annos));
						hits = true;
					}
				}
			}

			if (!hits && messages!=nullptr)
			{
				*messages << "No transcripts found for gene '" << gene << "'. Skipping it!" << endl;
			}
		}
	}

	output.sort(!annotate_transcript_names);
	return output;
}

QList<Transcript> NGSD::transcripts(int gene_id, Transcript::SOURCE source, bool coding_only)
{
	QList<Transcript> output;

	//get chromosome
	QString gene_id_str = QString::number(gene_id);

	//get transcripts
	SqlQuery query = getQuery();
	query.exec("SELECT id, name, chromosome, start_coding, end_coding, strand FROM gene_transcript WHERE gene_id=" + gene_id_str + " AND source='" + Transcript::sourceToString(source) + "' " + (coding_only ? "AND start_coding IS NOT NULL AND end_coding IS NOT NULL" : "") + " ORDER BY name");
	while(query.next())
	{
		//get base information
		Transcript transcript;
		transcript.setName(query.value(1).toString());
		transcript.setSource(source);
		transcript.setStrand(Transcript::stringToStrand(query.value(5).toString()));

		//get exons
		BedFile regions;
		QByteArray chr = query.value(2).toByteArray();
		int start_coding = query.value(3).toUInt();
		int end_coding = query.value(4).toUInt();
		SqlQuery query2 = getQuery();
		int id = query.value(0).toUInt();
		query2.exec("SELECT start, end FROM gene_exon WHERE transcript_id=" + QString::number(id) + " ORDER BY start");
		while(query2.next())
		{
			int start = query2.value(0).toUInt();
			int end = query2.value(1).toUInt();
			if (coding_only)
			{
				start = std::max(start, start_coding);
				end = std::min(end, end_coding);
				if (end<start_coding || start>end_coding) continue;
			}
			regions.append(BedLine(chr, start, end));
		}
		regions.merge();
		transcript.setRegions(regions);

		output.push_back(transcript);
	}

	return output;
}

Transcript NGSD::longestCodingTranscript(int gene_id, Transcript::SOURCE source, bool fallback_ensembl, bool fallback_ensembl_nocoding)
{
	QList<Transcript> list = transcripts(gene_id, source, true);
	if (list.isEmpty() && fallback_ensembl)
	{
		list = transcripts(gene_id, Transcript::ENSEMBL, true);
	}
	if (list.isEmpty() && fallback_ensembl_nocoding)
	{
		list = transcripts(gene_id, Transcript::ENSEMBL, false);
	}

	if (list.isEmpty()) return Transcript();

	//get longest transcript (transcripts regions are merged!)
	auto max_it = std::max_element(list.begin(), list.end(), [](const Transcript& a, const Transcript& b){ return a.regions().baseCount() < b.regions().baseCount(); });
	return *max_it;
}

DiagnosticStatusData NGSD::getDiagnosticStatus(const QString& processed_sample_id)
{
	//get processed sample ID
	if (processed_sample_id=="") return DiagnosticStatusData();

	//get status data
	SqlQuery q = getQuery();
	q.exec("SELECT s.status, u.name, s.date, s.outcome, s.genes_causal, s.inheritance_mode, s.genes_incidental, s.comment FROM diag_status as s, user as u WHERE s.processed_sample_id='" + processed_sample_id +  "' AND s.user_id=u.id");
	if (q.size()==0) return DiagnosticStatusData();

	//process
	q.next();
	DiagnosticStatusData output;
	output.dagnostic_status = q.value(0).toString();
	output.user = q.value(1).toString();
	output.date = q.value(2).toDateTime();
	output.outcome = q.value(3).toString();
	output.genes_causal = q.value(4).toString();
	output.inheritance_mode = q.value(5).toString();
	output.genes_incidental = q.value(6).toString();
	output.comments = q.value(7).toString();

	return output;
}

void NGSD::setDiagnosticStatus(const QString& processed_sample_id, DiagnosticStatusData status, QString user_name)
{
	//get current user ID
	QString user_id = userId(user_name);

	//get previous status
	DiagnosticStatusData status_before = getDiagnosticStatus(processed_sample_id);

	//update status

	SqlQuery query = getQuery();
	query.prepare("INSERT INTO diag_status (processed_sample_id, status, user_id, outcome, genes_causal, inheritance_mode, genes_incidental, comment) " \
					"VALUES ("+processed_sample_id+",'"+status.dagnostic_status+"', "+user_id+", '"+status.outcome+"', :0, '"+status.inheritance_mode+"', :1, :2) " \
					"ON DUPLICATE KEY UPDATE status='"+status.dagnostic_status+"',user_id="+user_id+", outcome='"+status.outcome+"', genes_causal=:3, inheritance_mode='"+status.inheritance_mode+"', genes_incidental=:4, comment=:5"
					);
	query.bindValue(0, status.genes_causal);
	query.bindValue(1, status.genes_incidental);
	query.bindValue(2, status.comments);
	query.bindValue(3, status.genes_causal);
	query.bindValue(4, status.genes_incidental);
	query.bindValue(5, status.comments);
	query.exec();
}

void NGSD::setProcessedSampleQuality(const QString& processed_sample_id, const QString& quality)
{
	getQuery().exec("UPDATE processed_sample SET quality='" + quality + "' WHERE id='" + processed_sample_id + "'");
}

GeneInfo NGSD::geneInfo(QByteArray symbol)
{
	GeneInfo output;

	//get approved symbol
	symbol = symbol.trimmed();
	auto approved = geneToApprovedWithMessage(symbol);
	output.symbol = approved.first;
	output.notice = approved.second;
	SqlQuery query = getQuery();
	query.prepare("SELECT name FROM gene WHERE symbol=:0");
	query.bindValue(0, output.symbol);
	query.exec();
	if (query.size()==0)
	{
		output.name = "";
	}
	else
	{
		query.next();
		output.name = query.value(0).toString();
	}

	query.prepare("SELECT inheritance, exac_pli, comments FROM geneinfo_germline WHERE symbol=:0");
	query.bindValue(0, output.symbol);
	query.exec();
	if (query.size()==0)
	{
		output.inheritance = "n/a";
		output.exac_pli = "n/a";
		output.comments = "";
	}
	else
	{
		query.next();
		output.inheritance = query.value(0).toString();
		output.exac_pli = query.value(1).isNull() ? "n/a" : QString::number(query.value(1).toDouble(), 'f', 2);
		output.comments = query.value(2).toString();
	}

	return output;
}

void NGSD::setGeneInfo(GeneInfo info)
{
	SqlQuery query = getQuery();
	query.prepare("INSERT INTO geneinfo_germline (symbol, inheritance, exac_pli, comments) VALUES (:0, :1, NULL, :2) ON DUPLICATE KEY UPDATE inheritance=:3, comments=:4");
	query.bindValue(0, info.symbol);
	query.bindValue(1, info.inheritance);
	query.bindValue(2, info.comments);
	query.bindValue(3, info.inheritance);
	query.bindValue(4, info.comments);
	query.exec();
}


QString AnalysisJob::runTimeAsString() const
{
	//determine start time
	QDateTime start;
	QDateTime end = QDateTime::currentDateTime();
	foreach(const AnalysisJobHistoryEntry& entry, history)
	{
		if (entry.status=="queued")
		{
			start = entry.time;
		}
		if (entry.status=="error" || entry.status=="finished" || entry.status=="canceled")
		{
			end = entry.time;
		}
	}

	//calculate sec, min, hour
	double s = start.secsTo(end);
	double m = floor(s/60.0);
	s -= 60.0 * m;
	double h = floor(m/60.0);
	m -= 60.0 * h;

	QStringList parts;
	if (h>0) parts << QString::number(h, 'f', 0) + "h";
	if (h>0 || m>0) parts << QString::number(m, 'f', 0) + "m";
	parts << QString::number(s, 'f', 0) + "s";

	return parts.join(" ");
}
