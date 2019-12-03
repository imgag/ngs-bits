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
		addFloat("max_af", "Maximum allele frequency of small variants to import (1000g and gnomAD).", true, 0.05);
		addFlag("test", "Uses the test database instead of on the production database.");
		addFlag("debug", "Enable verbose debug output.");
		addFlag("no_time", "Disable timing output.");
	}

	///split key-value pair based on separator
	static KeyValuePair split(const QByteArray& string, char sep)
	{
		QByteArrayList parts = string.split(sep);

		QString key = parts.takeFirst().trimmed().mid(2); //remove '##'
		QString value = parts.join(sep).trimmed();

		return KeyValuePair(key, value);
	}

	void importSmallVariants(NGSD& db, QTextStream& out, QString ps_name, bool debug, bool no_time, bool var_force)
	{
		QString filename = getInfile("var");
		if (filename=="") return;

		QTime timer;
		timer.start();
		QTime sub_timer;
		QStringList sub_times;

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
			sub_timer.start();

			//get class4/5 variant ids
			QStringList tmp = db.getValues("SELECT vc.variant_id FROM detected_variant dv, variant_classification vc WHERE dv.processed_sample_id=:0 AND dv.variant_id=vc.variant_id AND (vc.class='4' OR vc.class='5')", ps_id);
			foreach(const QString& id, tmp)
			{
				var_ids_class_4_or_5 << id.toInt();
			}
			out << "Found " << var_ids_class_4_or_5.size()  << " class 4/5 variants for the sample!\n";
			sub_times << ("getting class 4/5 variants took: " + Helper::elapsedTime(sub_timer));

			//remove old variants
			sub_timer.start();
			SqlQuery query = db.getQuery();
			query.exec("DELETE FROM detected_variant WHERE processed_sample_id='" + ps_id + "'");
			out << "Deleted previous variants\n";
			sub_times << ("deleted previous detected variants took: " + Helper::elapsedTime(sub_timer));
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
		sub_timer.start();
		int c_add, c_update;
		double max_af = getFloat("max_af");
		QList<int> variant_ids = db.addVariants(variants, max_af, c_add, c_update);
		out << "Imported variants (added:" << c_add << " updated:" << c_update << ")\n";
		sub_times << ("adding variants took: " + Helper::elapsedTime(sub_timer));

		//add detected variants
		sub_timer.start();
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
		sub_times << ("adding detected variants took: " + Helper::elapsedTime(sub_timer));

		//check that all important variant are still there (we unset all re-imported variants above)
		foreach(int id, var_ids_class_4_or_5)
		{
			THROW(ArgumentException, "Variant (" + db.variant(QString::number(id)).toString(false, 20) + ") with classification 4/5 is no longer in variant list!");
		}


		//output
		int c_skipped = variant_ids.count(-1);
		out << "Imported " << (variant_ids.count()-c_skipped) << " detected variants\n";
		if (debug)
		{
			out << "DEBUG: Skipped " << variant_ids.count(-1) << " high-AF variants!\n";
		}

		//output timing
		if (!no_time)
		{
			out << "Import took: " << Helper::elapsedTime(timer) << "\n";
			foreach(QString line, sub_times)
			{
				out << "  " << line.trimmed() << "\n";
			}
		}
	}

	void importCNVs(NGSD& db, QTextStream& out, QString ps_name, bool debug, bool no_time, bool cnv_force)
	{
		QString filename = getInfile("cnv");
		if (filename=="") return;

		QTime timer;
		timer.start();

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

		//load CNVs
		CnvList cnvs;
		cnvs.load(filename);

		//parse file header
		QString caller_version;
		QDateTime call_date;
		QJsonObject quality_metrics;
		foreach(const QByteArray& line, cnvs.comments())
		{
			if (line.contains(":"))
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
		if (cnvs.type()==CnvListType::CNVHUNTER_GERMLINE_SINGLE)
		{
			if (call_date.isNull()) //fallback for CnvHunter file which does not contain the date!
			{
				call_date = QFileInfo(filename).created();
			}
		}

		QJsonDocument json_doc;
		json_doc.setObject(quality_metrics);

		//output
		QString caller = cnvs.callerAsString();
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
		int callset_id = q_set.lastInsertId().toInt();

		//import CNVs
		int c_imported = 0;
		int c_skipped_low_quality = 0;
		for (int i=0; i<cnvs.count(); ++i)
		{
			QString cnv_id = db.addCnv(callset_id, cnvs[i], cnvs, 15.0);
			if (cnv_id.isEmpty())
			{
				++c_skipped_low_quality;
			}
			else
			{
				++c_imported;
				if (debug)
				{
					out << "DEBUG: " << cnvs[i].toString() << " cn:" << db.getValue("SELECT cn FROM cnv WHERE id=" + cnv_id).toString() << " quality: " << db.getValue("SELECT quality_metrics FROM cnv WHERE id=" + cnv_id).toString() << "\n";
				}
			}
		}

		out << "Imported cnvs: " << c_imported << "\n";
		out << "Skipped low-quality cnvs: " << c_skipped_low_quality << "\n";

		//output timing
		if (!no_time)
		{
			out << "Import took: " << Helper::elapsedTime(timer) << "\n";
		}
	}

	virtual void main()
	{
		//init
		NGSD db(getFlag("test"));
		QSharedPointer<QFile> out = Helper::openFileForWriting(getOutfile("out"), true);
		QTextStream stream(out.data());
		QString ps_name = getString("ps");
		bool debug = getFlag("debug");
		bool no_time = getFlag("no_time");
		bool var_force = getFlag("var_force");
		bool cnv_force = getFlag("cnv_force");

		//prevent import if report config exists
		int report_conf_id = db.reportConfigId(ps_name);
		if (report_conf_id!=-1)
		{
			THROW(ArgumentException, "Cannot import variant data for sample " + ps_name + ": a report configuration exists for this sample!");
		}

		//prevent tumor samples from being imported into the germline variant tables
		QString s_id = db.sampleId(ps_name);
		SampleData sample_data = db.getSampleData(s_id);
		if (sample_data.is_tumor)
		{
			THROW(ArgumentException, "Cannot import variant data for sample " + ps_name + ": the sample is a tumor sample according to NGSD!");
		}

		//import
		importSmallVariants(db, stream, ps_name, debug, no_time, var_force);
		importCNVs(db, stream, ps_name, debug, no_time, cnv_force);
	}
};

#include "main.moc"

int main(int argc, char *argv[])
{
	ConcreteTool tool(argc, argv);
	return tool.execute();
}
