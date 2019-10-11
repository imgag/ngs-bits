#include "ToolBase.h"
#include "NGSD.h"
#include "TSVFileStream.h"
#include "KeyValuePair.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QFileInfo>


class ConcreteTool
		: public ToolBase
{
	Q_OBJECT

public:
	ConcreteTool(int& argc, char *argv[])
		: ToolBase(argc, argv)
	{
	}

	virtual void setup()
	{
		setDescription("Imports variants of a processed sample into the NGSD.");
		addString("ps", "Processed sample name", false);
		//optional
		addInfile("var", "Small variant list in GSvar format (as produced by megSAP).", true, true);
		addFlag("var_force", "Force import of small variants, even if already imported.");
		addInfile("cnv", "CNV list in TSV format (as produced by megSAP).", true, true);
		addFlag("cnv_force", "Force import of CNVs, even if already imported. Attention: This will also delete classifications and report configuration!");
		addOutfile("out", "Output file. If unset, writes to STDOUT.", true);
		addFloat("max_af", "Maximum allele frequency of small variants to import (1000g and gnomAD)", true, 0.05);
		addFlag("test", "Uses the test database instead of on the production database.");
		addFlag("debug", "Enable verbose debug output.");
	}

	///split key-value pair based on separator
	static KeyValuePair split(const QByteArray& string, char sep)
	{
		QByteArrayList parts = string.split(sep);

		QString key = parts.takeFirst().trimmed().mid(2); //remove '##'
		QString value = parts.join(sep).trimmed();

		return KeyValuePair(key, value);
	}

	///returns the most frequent string
	static int mostFrequent(const QStringList& parts)
	{
		int max = 0;
		QString max_cn;
		QHash<QString, int> cn_counts;
		foreach(const QString& cn, parts)
		{
			int count_new = cn_counts[cn] + 1;
			if (count_new>max)
			{
				max = count_new;
				max_cn = cn;
			}
			cn_counts[cn] = count_new;
		}

		return Helper::toInt(max_cn, "copy-number");
	}


	void importSmallVariants(NGSD& db, QTextStream& out, QString ps_name, bool debug, bool var_force)
	{
		QString filename = getInfile("var");
		if (filename=="") return;

		out << "\n";
		out << "### importing small variants for " << ps_name << " ###\n";
		out << "filename: " << filename << "\n";

		//check if variants were already imported for this PID
		QString ps_id = db.processedSampleId(ps_name);
		int count_old = db.getValue("SELECT count(*) FROM detected_variant WHERE processed_sample_id=:0", true, ps_id).toInt();
		out << "Found " << count_old  << " variants already imported into NGSD!\n";
		if(count_old>0 && !var_force)
		{
			THROW(ArgumentException, "Variants were already imported for '" + ps_name + "'. Use the flag '-var_force' to overwrite them.");
		}

		//remove old variants (and store class4/5 variants for check)
		QSet<int> var_ids_class_4_or_5;
		if (count_old>0 && var_force)
		{
			//get class4/5 variant ids
			QStringList tmp = db.getValues("SELECT vc.variant_id FROM detected_variant dv, variant_classification vc WHERE dv.processed_sample_id=:0 AND dv.variant_id=vc.variant_id AND (vc.class='4' OR vc.class='5')", ps_id);
			foreach(const QString& id, tmp)
			{
				var_ids_class_4_or_5 << id.toInt();
			}
			out << "Found " << var_ids_class_4_or_5.size()  << " class 4/5 variants for the sample!\n";

			//remove old variants
			SqlQuery query = db.getQuery();
			query.exec("DELETE FROM detected_variant WHERE processed_sample_id='" + ps_id + "'");
			out << "Deleted previous variants\n";
		}
		if (debug)
		{
			out << "DEBUG: Found " << var_ids_class_4_or_5.count() << " class 4/5 variants\n";
		}

		//abort if there are no variants in the input file
		VariantList variants;
		variants.load(filename);
		if (variants.count()==0)
		{
			out << "No variants imported (empty GSvar file).\n";
			return;
		}

		//add missing variants
		double max_af = getFloat("max_af");
		QList<int> variant_ids = db.addVariants(variants, max_af);

		//add detected variants
		int i_geno = variants.getSampleHeader().infoByID(ps_name).column_index;
		SqlQuery q_insert = db.getQuery();
		q_insert.prepare("INSERT INTO detected_variant (processed_sample_id, variant_id, genotype) VALUES (" + ps_id + ", :0, :1)");
		db.transaction();
		for (int i=0; i<variants.count(); ++i)
		{
			//skip high-AF variants
			int variant_id = variant_ids[i];
			if (variant_id==-1) continue;

			//remove class 4/5 variant from list (see check below)
			var_ids_class_4_or_5.remove(variant_id);

			//bind
			q_insert.bindValue(0, variant_id);
			q_insert.bindValue(1, variants[i].annotations()[i_geno]);
			q_insert.exec();
		}
		db.commit();

		//check that all important variant are still there (we unset all re-imported variants above)
		foreach(int id, var_ids_class_4_or_5)
		{
			THROW(ArgumentException, "Variant (" + db.variant(QString::number(id)).toString(false, 20) + ") with classification 4/5 is no longer in variant list!");
		}


		//output
		int c_skipped = variant_ids.count(-1);
		out << "Imported " << (variant_ids.count()-c_skipped) << " variants\n";
		if (debug)
		{
			out << "DEBUG: Skipped " << variant_ids.count(-1) << " high-AF variants!\n";
		}
	}

	void importCNVs(NGSD& db, QTextStream& out, QString ps_name, bool debug, bool cnv_force)
	{
		QString filename = getInfile("cnv");
		if (filename=="") return;

		out << "\n";
		out << "### importing CNVs for " << ps_name << " ###\n";
		out << "filename: " << filename << "\n";

		//get processed sample id
		QString ps_id = db.processedSampleId(ps_name);

		//check if CNV callset already exists > delete old callset
		QString last_callset_id = db.getValue("SELECT id FROM cnv_callset WHERE processed_sample_id=:0", true, ps_id).toString();
		if(last_callset_id!="" && !cnv_force)
		{
			out << "NOTE: CNVs were already imported for '" << ps_name << "' - skipping import";
			return;
		}

		//check if variants were already imported for this PID
		if (last_callset_id!="" && cnv_force)
		{
			db.getQuery().exec("DELETE FROM cnv WHERE cnv_callset_id='" + last_callset_id + "'");
			db.getQuery().exec("DELETE FROM cnv_callset WHERE id='" + last_callset_id + "'");

			out << "Deleted previous CNV callset\n";
		}

		//parse file header
		QString analysis_type;
		QString caller_version;
		QDateTime call_date;
		QJsonObject quality_metrics;
		TSVFileStream in(filename);
		foreach(QByteArray line, in.comments())
		{
			if (line.startsWith("##ANALYSISTYPE="))
			{
				KeyValuePair pair = split(line, '=');
				analysis_type = pair.value;
			}
			else if (line.contains(":"))
			{
				KeyValuePair pair = split(line, ':');

				if (pair.key.endsWith(" version"))
				{
					caller_version = pair.value;
				}
				else if (pair.key.endsWith(" finished on"))
				{
					call_date = QDateTime::fromString(pair.value, "yyyy-MM-dd hh:mm:ss");
				}
				else //quality metrics
				{
					if (pair.key.startsWith(ps_name + " ")) //remove sample name prefix (CnvHunter only)
					{
						pair.key = pair.key.mid(ps_name.length()+1).trimmed();
					}

					quality_metrics.insert(pair.key, pair.value);
				}
			}
			else
			{
				THROW(FileParseException, "Invalid header line '" + line + "' in file '" + filename + "'!");
			}
		}
		QByteArray caller;
		if (analysis_type=="CNVHUNTER_GERMLINE_SINGLE")
		{
			caller = "CnvHunter";

			if (call_date.isNull()) //fallback for CnvHunter file which does not contain the date!
			{
				call_date = QFileInfo(filename).created();
			}
		}
		else if (analysis_type=="CLINCNV_GERMLINE_SINGLE")
		{
			caller = "ClinCNV";
		}
		else if (analysis_type=="")
		{
			THROW(FileParseException, "CNV file '" + filename + "' does not contain '##ANALYSISTYPE=' line!\nIt is outdated - please re-run the CNV calling!");
		}
		else
		{
			THROW(FileParseException, "CNV file '" + filename + "' contains invalid analysis type '" + analysis_type + "'!\n");
		}

		QJsonDocument json_doc;
		json_doc.setObject(quality_metrics);

		//output
		out << "caller: " << caller << "\n";
		out << "caller version: " << caller_version << "\n";
		if (debug)
		{
			out << "DEBUG: callset quality: " << json_doc.toJson(QJsonDocument::Compact) << "\n";
		}

		//import CNV call set
		SqlQuery q_set = db.getQuery();
		q_set.prepare("INSERT INTO `cnv_callset` (`processed_sample_id`, `caller`, `caller_version`, `call_date`, `quality_metrics`, `quality`) VALUES (:0,:1,:2,:3,:4,:5)");
		q_set.bindValue(0, ps_id);
		q_set.bindValue(1, caller);
		q_set.bindValue(2, caller_version);
		q_set.bindValue(3, call_date);
		q_set.bindValue(4, json_doc.toJson(QJsonDocument::Compact));
		q_set.bindValue(5, "n/a");
		q_set.exec();
		QString callset_id = q_set.lastInsertId().toString();

		//prepare CNV query
		SqlQuery q_cnv = db.getQuery();
		q_cnv.prepare("INSERT INTO `cnv` (`cnv_callset_id`, `chr`, `start`, `end`, `cn`, `quality_metrics`) VALUES ('" + callset_id + "',:0,:1,:2,:3,:4)");

		//import CNVs
		int c_imported = 0;
		int c_skipped_low_quality = 0;
		while(!in.atEnd())
		{
			QByteArrayList parts = in.readLine();

			//parse line
			int cn = -1;
			QJsonObject quality_metrics;
			bool skip = false;
			for(int i=0; i<in.columns(); ++i)
			{
				const QByteArray& col_name = in.header()[i];
				QString entry = parts[i];

				if (caller == "CnvHunter")
				{
					if (col_name=="region_copy_numbers")
					{
						cn = mostFrequent(entry.split(','));
					}
					if (col_name=="region_count" || col_name=="region_zscores" || col_name=="region_copy_numbers")
					{
						quality_metrics.insert(col_name, entry);
					}
				}
				else if (caller == "ClinCNV")
				{
					if (col_name=="CN_change")
					{
						cn = Helper::toInt(entry, "copy-number");
					}
					if (col_name=="loglikelihood" || col_name=="no_of_regions" || col_name=="qvalue")
					{
						quality_metrics.insert(col_name, entry);
					}
					if (col_name=="loglikelihood")
					{
						if (Helper::toDouble(entry, "log-likelihood")<15)
						{
							++c_skipped_low_quality;
							skip = true;
						}

					}
				}
			}
			QJsonDocument json_doc;
			json_doc.setObject(quality_metrics);

			if (skip) continue;

			//debug output
			if (debug)
			{
				out << "DEBUG: " << parts[0] << ":" << parts[1] << "-" << parts[2] << " cn:" << cn << " quality: " << json_doc.toJson(QJsonDocument::Compact) << "\n";
			}

			//insert into database
			q_cnv.bindValue(0, parts[0]);
			q_cnv.bindValue(1, parts[1]);
			q_cnv.bindValue(2, parts[2]);
			q_cnv.bindValue(3, cn);
			q_cnv.bindValue(4, json_doc.toJson(QJsonDocument::Compact));
			q_cnv.exec();

			++c_imported;
		}

		out << "imported cnvs: " << c_imported << "\n";
		out << "skipped low-quality cnvs: " << c_skipped_low_quality << "\n";
	}

	virtual void main()
	{
		//init
		NGSD db(getFlag("test"));
		QSharedPointer<QFile> out = Helper::openFileForWriting(getOutfile("out"), true);
		QTextStream stream(out.data());
		QString ps_name = getString("ps");
		bool debug = getFlag("debug");
		bool var_force = getFlag("var_force");
		bool cnv_force = getFlag("cnv_force");

		//prevent tumor samples from being imported into the germline variant tables
		QString s_id = db.sampleId(ps_name);
		SampleData sample_data = db.getSampleData(s_id);
		if (sample_data.is_tumor)
		{
			THROW(ArgumentException, "Cannot import tumor data from sample " + ps_name + " into germline tables!");
		}

		//import
		importSmallVariants(db, stream, ps_name, debug, var_force);
		importCNVs(db, stream, ps_name, debug, cnv_force);
	}
};

#include "main.moc"

int main(int argc, char *argv[])
{
	ConcreteTool tool(argc, argv);
	return tool.execute();
}
