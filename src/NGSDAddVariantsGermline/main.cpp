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
		addFlag("cnv_force", "Force import of CNVs, even if already imported.");
		addInfile("sv", "SV list in BEDPE format (as produced by megSAP).", true, true);
		addFlag("sv_force", "Force import of SVs, even if already imported.");
		addOutfile("out", "Output file. If unset, writes to STDOUT.", true);
		addFloat("max_af", "Maximum allele frequency of small variants to import (1000g and gnomAD).", true, 0.05);
		addFlag("test", "Uses the test database instead of on the production database.");
		addFlag("debug", "Enable verbose debug output.");
		addFlag("no_time", "Disable timing output.");

		changeLog(2021,  7, 19, "Added support for 'CADD' and 'SpliceAI' columns in 'variant' table.");
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

		out << "\n";
		out << "### importing small variants for " << ps_name << " ###\n";
		out << "filename: " << filename << "\n";

		QTime timer;
		timer.start();
		QTime sub_timer;
		QStringList sub_times;

		//check if variants were already imported for this PID
		QString ps_id = db.processedSampleId(ps_name);
		int count_old = db.importStatus(ps_id).small_variants;
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

		out << "\n";
		out << "### importing CNVs for " << ps_name << " ###\n";
		out << "filename: " << filename << "\n";

		//prevent import if report config contains CNVs
		QString ps_id = db.processedSampleId(ps_name);
		int report_conf_id = db.reportConfigId(ps_id);
		if (report_conf_id!=-1)
		{
			SqlQuery query = db.getQuery();
			query.exec("SELECT * FROM report_configuration_cnv WHERE report_configuration_id=" + QString::number(report_conf_id));
			if (query.size()>0)
			{
				out << "Skipped import of CNVs for sample " + ps_name + ": a report configuration with CNVs exists for this sample!\n";
				return;
			}
		}

		QTime timer;
		timer.start();


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

	void importSVs(NGSD& db, QTextStream& out, QString ps_name, bool debug, bool no_time, bool sv_force)
	{
		QString filename = getInfile("sv");
		if (filename=="") return;

		out << "\n";
		out << "### importing SVs for " << ps_name << " ###\n";
		out << "filename: " << filename << "\n";

		QTime timer;
		timer.start();

		// get processed sample id
		int ps_id = Helper::toInt(db.processedSampleId(ps_name));
		if(debug) out << "Processed sample id: " << ps_id << endl;

		//prevent import if report config contains SVs
		int report_conf_id = db.reportConfigId(QString::number(ps_id));
		if (report_conf_id!=-1)
		{
			SqlQuery query = db.getQuery();
			query.exec("SELECT * FROM report_configuration_sv WHERE report_configuration_id=" + QString::number(report_conf_id));
			if (query.size()>0)
			{
				out << "Skipped import of SVs for sample " + ps_name + ": a report configuration with SVs exists for this sample!\n";
				return;
			}
		}

		// check if processed sample has already been imported
		QString previous_callset_id = db.getValue("SELECT id FROM sv_callset WHERE processed_sample_id=:0", true, QString::number(ps_id)).toString();
		if(previous_callset_id!="" && !sv_force)
		{
			out << "NOTE: SVs were already imported for '" << ps_name << "' - skipping import\n";
			return;
		}
		//remove old imports of this processed sample
		if (previous_callset_id!="" && sv_force)
		{
			db.getQuery().exec("DELETE FROM sv_deletion WHERE sv_callset_id='" + previous_callset_id + "'");
			db.getQuery().exec("DELETE FROM sv_duplication WHERE sv_callset_id='" + previous_callset_id + "'");
			db.getQuery().exec("DELETE FROM sv_inversion WHERE sv_callset_id='" + previous_callset_id + "'");
			db.getQuery().exec("DELETE FROM sv_insertion WHERE sv_callset_id='" + previous_callset_id + "'");
			db.getQuery().exec("DELETE FROM sv_translocation WHERE sv_callset_id='" + previous_callset_id + "'");
			db.getQuery().exec("DELETE FROM sv_callset WHERE id='" + previous_callset_id + "'");

			out << "Deleted previous SV callset\n";
		}


		// open BEDPE file
		BedpeFile svs;
		svs.load(filename);

		// create SV callset for given processed sample
		QByteArray caller;
		QByteArray caller_version;
		QDate date;
		foreach (const QByteArray &header, svs.headers())
		{
			// parse date
			if (header.startsWith("##fileDate="))
			{
				date = QDate::fromString(header.split('=')[1].trimmed(), "yyyyMMdd");
			}

			// parse manta version
			if (header.startsWith("##source="))
			{
				QByteArrayList application_string = header.split('=')[1].trimmed().split(' ');
				if (application_string[0].startsWith("GenerateSVCandidates")) caller = "Manta";
				caller_version = application_string[1].trimmed();
			}

		}

		// check if all required data is available
		if (caller == "") THROW(FileParseException, "Caller is missing");
		if(caller_version == "") THROW(FileParseException, "Version is missing");
		if(date.isNull()) THROW(FileParseException, "Date is missing");

		// create callset entry
		SqlQuery insert_callset = db.getQuery();
		insert_callset.prepare("INSERT INTO `sv_callset` (`processed_sample_id`, `caller`, `caller_version`, `call_date`) VALUES (:0,:1,:2,:3)");
		insert_callset.bindValue(0, ps_id);
		insert_callset.bindValue(1, caller);
		insert_callset.bindValue(2, caller_version);
		insert_callset.bindValue(3, date);
		insert_callset.exec();
		int callset_id = insert_callset.lastInsertId().toInt();

		if(debug) out << "Callset id: " << callset_id << endl;

		// import structural variants
		int sv_imported = 0;
		for (int i = 0; i < svs.count(); i++)
		{
			// ignore SVs on special chromosomes
			if (!svs[i].chr1().isNonSpecial() || !svs[i].chr2().isNonSpecial()) continue;

			int sv_id = db.addSv(callset_id, svs[i], svs);
			sv_imported++;
			if (debug)
			{
				QByteArray db_table_name;
				switch (svs[i].type())
				{
					case StructuralVariantType::DEL:
						db_table_name = "sv_deletion";
						break;
					case StructuralVariantType::DUP:
						db_table_name = "sv_duplication";
						break;
					case StructuralVariantType::INS:
						db_table_name = "sv_insertion";
						break;
					case StructuralVariantType::INV:
						db_table_name = "sv_inversion";
						break;
					case StructuralVariantType::BND:
						db_table_name = "sv_translocation";
						break;
					default:
						THROW(FileParseException, "Invalid structural variant type!");
						break;
				}
				out << "DEBUG: " << svs[i].positionRange() << " sv: " << BedpeFile::typeToString(svs[i].type()) << " quality: "
					<< db.getValue("SELECT quality_metrics FROM " + db_table_name + " WHERE id=" + QByteArray::number(sv_id)).toString() << "\n";
			}

		}

		out << "Imported SVs: " << sv_imported << "\n";
		out << "Skipped SVs: " << svs.count() - sv_imported << "\n";

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
		bool sv_force = getFlag("sv_force");

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
		importSVs(db, stream, ps_name, debug, no_time, sv_force);
	}
};

#include "main.moc"

int main(int argc, char *argv[])
{
	ConcreteTool tool(argc, argv);
	return tool.execute();
}
