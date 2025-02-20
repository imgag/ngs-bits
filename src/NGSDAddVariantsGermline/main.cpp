#include "ToolBase.h"
#include "NGSD.h"
#include "TSVFileStream.h"
#include "KeyValuePair.h"
#include "RepeatLocusList.h"
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
		addFlag("var_update", "Import missing small variants - doesn't change others.");
		addInfile("cnv", "CNV list in TSV format (as produced by megSAP).", true, true);
		addInfile("sv", "SV list in BEDPE format (as produced by megSAP).", true, true);
		addInfile("re", "RE list in VCF format (as produced by megSAP).", true, true);
		addFlag("force", "Force import of variants, even if they are already imported.");
		addOutfile("out", "Output file. If unset, writes to STDOUT.", true);
		addFloat("max_af", "Maximum allele frequency of small variants to import (gnomAD).", true, 0.05);
		addFlag("test", "Uses the test database instead of on the production database.");
		addFlag("debug", "Enable verbose debug output.");
		addFlag("no_time", "Disable timing output.");

		changeLog(2024,  8, 28, "Merged all force parameters into one. Implmented skipping of small variants import if the same callset was already imported.");
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

	void importSmallVariants(NGSD& db, QTextStream& out, QString ps_name, bool debug, bool no_time, bool force, bool var_update)
	{
		QString filename = getInfile("var");
		if (filename=="") return;

        out << QT_ENDL;
        out << "### importing small variants for " << ps_name << " ###" << QT_ENDL;
        out << "filename: " << filename << QT_ENDL;

		//check arguments
		if (force && var_update) THROW(ArgumentException, "Flags -force and -var_update cannot be used at the same time! Use -force to delete old variants and reimport, and -var_update to only import missing variants.")

        QElapsedTimer timer;
		timer.start();
        QElapsedTimer sub_timer;
		QStringList sub_times;

		//check if variants were already imported for this PID
		QString ps_id = db.processedSampleId(ps_name);
		int count_old = db.importStatus(ps_id).small_variants;
        out << "Found " << count_old  << " variants already imported into NGSD!" << QT_ENDL;
		if(count_old>0 && !force && !var_update)
		{
            out << "Skipped import because variants were already imported for '" + ps_name + "'. Use the flag '-force' to overwrite them." << QT_ENDL;
			return;
		}

		//load variant list
		VariantList variants;
		variants.load(filename);

		//add missing variants and update variant meta data
		sub_timer.start();
		int c_add, c_update;
		double max_af = getFloat("max_af");
		QList<int> variant_ids = db.addVariants(variants, max_af, c_add, c_update);
        out << "Imported variants (added:" << c_add << " updated:" << c_update << ")" << QT_ENDL;
		sub_times << ("adding variants took: " + Helper::elapsedTime(sub_timer));

		//skip import of detected variants if same callset was already imported
		VariantCaller caller = variants.getCaller();
		QDate calling_date = variants.getCallingDate();
		if (!caller.name.isEmpty() && calling_date.isValid())
		{
			VariantCallingInfo calling_info_ngsd = db.variantCallingInfo(ps_id);
			if (calling_info_ngsd.small_caller==caller.name && calling_info_ngsd.small_caller_version==caller.version && calling_info_ngsd.small_call_date==calling_date.toString(Qt::ISODate))
			{
                out << "Skipped import because variants were already imported with the same caller, caller version and calling date!" << QT_ENDL;
				if (!no_time)
				{
                    out << "Import took: " << Helper::elapsedTime(timer) << QT_ENDL;
				}

				return;
			}
		}

		//remove old variants
		if (count_old>0 && force)
		{
			sub_timer.start();
			db.deleteVariants(ps_id, VariantType::SNVS_INDELS);
            out << "Deleted previous variants" << QT_ENDL;
			sub_times << ("deleted previous detected variants took: " + Helper::elapsedTime(sub_timer));
		}

		//add callset (if caller info in header)
		if (caller.name!="" && caller.version!="")
		{
			SqlQuery q_set = db.getQuery();
			q_set.exec("DELETE FROM small_variants_callset WHERE processed_sample_id='" + ps_id + "'");
			q_set.prepare("INSERT INTO small_variants_callset (`processed_sample_id`, `caller`, `caller_version`, `call_date`) VALUES (:0,:1,:2,:3)");
			q_set.bindValue(0, ps_id);
			q_set.bindValue(1, caller.name);
			q_set.bindValue(2, caller.version);
			q_set.bindValue(3, calling_date);
			q_set.exec();
		}

		//abort if there are no variants in the input file
		if (variants.count()==0)
		{
            out << "No variants imported (empty GSvar file)." << QT_ENDL;
			return;
		}

		//update: add the missing variants to the processed sample while keeping the others the same.
		if (var_update)
		{
			sub_timer.start();

			//remove all variants that are already linked to the processed sample
			QList<int> variant_ids_new;
			VariantList variants_new;
			variants_new.copyMetaData(variants);
            QSet<int> existing_var_ids = LIST_TO_SET(db.getValuesInt("SELECT variant_id FROM detected_variant WHERE processed_sample_id='" + ps_id + "'"));
			for (int i=0; i<variant_ids.count(); ++i)
			{
				int variant_id = variant_ids[i];
				if (existing_var_ids.contains(variant_id)) continue;

				variant_ids_new << variant_id;
				variants_new.append(variants[i]);
			}
            out << "Ignored " << (variants.count()-variants_new.count()) << " already imported variants" << QT_ENDL;
			variant_ids = variant_ids_new;
			variants = variants_new;
			sub_times << ("Determining already imported variants took: " + Helper::elapsedTime(sub_timer));
		}

		//add detected variants
		sub_timer.start();
		int i_geno = variants.getSampleHeader().infoByID(ps_name).column_index;
		SqlQuery q_insert = db.getQuery();
		q_insert.prepare("INSERT INTO detected_variant (processed_sample_id, variant_id, genotype, mosaic) VALUES (" + ps_id + ", :0, :1, :2)");
		db.transaction();
		for (int i=0; i<variants.count(); ++i)
		{
			//skip high-AF variants or too long variants
			int variant_id = variant_ids[i];
			if (variant_id==-1) continue;

			//bind
			q_insert.bindValue(0, variant_id);
			q_insert.bindValue(1, variants[i].annotations()[i_geno]);
			q_insert.bindValue(2, variants[i].filters().contains("mosaic"));
			q_insert.exec();
		}
		db.commit();
		sub_times << ("adding detected variants took: " + Helper::elapsedTime(sub_timer));

		//output
		int c_skipped = variant_ids.count(-1);
        out << "Imported " << (variant_ids.count()-c_skipped) << " detected variants" << QT_ENDL;
		if (debug)
		{
            out << "DEBUG: Skipped " << variant_ids.count(-1) << " high-AF variants!" << QT_ENDL;
		}

		//output timing
		if (!no_time)
		{
            out << "Import took: " << Helper::elapsedTime(timer) << QT_ENDL;
			foreach(QString line, sub_times)
			{
                out << "  " << line.trimmed() << QT_ENDL;
			}
		}
	}

	void importCNVs(NGSD& db, QTextStream& out, QString ps_name, bool debug, bool no_time, bool force)
	{
		QString filename = getInfile("cnv");
		if (filename=="") return;

        out << QT_ENDL;
        out << "### importing CNVs for " << ps_name << " ###" << QT_ENDL;
        out << "filename: " << filename << QT_ENDL;

		//prevent import if report config contains CNVs
		QString ps_id = db.processedSampleId(ps_name);
		int report_conf_id = db.reportConfigId(ps_id);
		if (report_conf_id!=-1)
		{
			SqlQuery query = db.getQuery();
			query.exec("SELECT * FROM report_configuration_cnv WHERE report_configuration_id=" + QString::number(report_conf_id));
			if (query.size()>0)
			{
                out << "Skipped import of CNVs for sample " + ps_name + ": a report configuration with CNVs exists for this sample!" << QT_ENDL;
				return;
			}
		}

        QElapsedTimer timer;
		timer.start();


		//check if CNV callset already exists > delete old callset
		QString last_callset_id = db.getValue("SELECT id FROM cnv_callset WHERE processed_sample_id=:0", true, ps_id).toString();
		if(last_callset_id!="" && !force)
		{
			out << "NOTE: CNVs were already imported for '" << ps_name << "' - skipping import";
			return;
		}

		//check if variants were already imported for this PID
		if (last_callset_id!="" && force)
		{
			db.getQuery().exec("DELETE FROM cnv WHERE cnv_callset_id='" + last_callset_id + "'");
			db.getQuery().exec("DELETE FROM cnv_callset WHERE id='" + last_callset_id + "'");

            out << "Deleted previous CNV callset" << QT_ENDL;
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
					quality_metrics.insert(pair.key, pair.value);
				}
			}
		}

		QJsonDocument json_doc;
		json_doc.setObject(quality_metrics);

		//output
		QString caller = cnvs.callerAsString();
        out << "caller: " << caller << QT_ENDL;
        out << "caller version: " << caller_version << QT_ENDL;
		if (debug)
		{
            out << "DEBUG: callset quality: " << json_doc.toJson(QJsonDocument::Compact) << QT_ENDL;
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
                    out << "DEBUG: " << cnvs[i].toString() << " cn:" << db.getValue("SELECT cn FROM cnv WHERE id=" + cnv_id).toString() << " quality: " << db.getValue("SELECT quality_metrics FROM cnv WHERE id=" + cnv_id).toString() << QT_ENDL;
				}
			}
		}

        out << "Imported cnvs: " << c_imported << QT_ENDL;
        out << "Skipped low-quality cnvs: " << c_skipped_low_quality << QT_ENDL;

		//output timing
		if (!no_time)
		{
            out << "Import took: " << Helper::elapsedTime(timer) << QT_ENDL;
		}
	}

	void importSVs(NGSD& db, QTextStream& out, QString ps_name, bool debug, bool no_time, bool force)
	{
		QString filename = getInfile("sv");
		if (filename=="") return;

        out << QT_ENDL;
        out << "### importing SVs for " << ps_name << " ###" << QT_ENDL;
        out << "filename: " << filename << QT_ENDL;

        QElapsedTimer timer;
		timer.start();

		// get processed sample id
		int ps_id = Helper::toInt(db.processedSampleId(ps_name));
        if(debug) out << "Processed sample id: " << ps_id << QT_ENDL;

		//prevent import if report config contains SVs
		int report_conf_id = db.reportConfigId(QString::number(ps_id));
		if (report_conf_id!=-1)
		{
			SqlQuery query = db.getQuery();
			query.exec("SELECT * FROM report_configuration_sv WHERE report_configuration_id=" + QString::number(report_conf_id));
			if (query.size()>0)
			{
                out << "Skipped import of SVs for sample " + ps_name + ": a report configuration with SVs exists for this sample!" << QT_ENDL;
				return;
			}
		}

		// check if processed sample has already been imported
		QString previous_callset_id = db.getValue("SELECT id FROM sv_callset WHERE processed_sample_id=:0", true, QString::number(ps_id)).toString();
		if(previous_callset_id!="" && !force)
		{
            out << "NOTE: SVs were already imported for '" << ps_name << "' - skipping import" << QT_ENDL;
			return;
		}
		//remove old imports of this processed sample
		if (previous_callset_id!="" && force)
		{
			db.getQuery().exec("DELETE FROM sv_deletion WHERE sv_callset_id='" + previous_callset_id + "'");
			db.getQuery().exec("DELETE FROM sv_duplication WHERE sv_callset_id='" + previous_callset_id + "'");
			db.getQuery().exec("DELETE FROM sv_inversion WHERE sv_callset_id='" + previous_callset_id + "'");
			db.getQuery().exec("DELETE FROM sv_insertion WHERE sv_callset_id='" + previous_callset_id + "'");
			db.getQuery().exec("DELETE FROM sv_translocation WHERE sv_callset_id='" + previous_callset_id + "'");
			db.getQuery().exec("DELETE FROM sv_callset WHERE id='" + previous_callset_id + "'");

            out << "Deleted previous SV callset" << QT_ENDL;
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

			// parse caller and caller version
			// Manta:    ##source=GenerateSVCandidates 1.6.0
			// DRAGEN:   ##source=DRAGEN 01.011.608.3.9.3
			// Sniffles: ##source=Sniffles2_2.0.7
			if (header.startsWith("##source="))
			{
				QByteArray application_string = header.split('=')[1].trimmed();

				int sep_idx = application_string.indexOf(" ");
				if(sep_idx==-1) sep_idx = application_string.indexOf("_"); //fallback: use '_' as version seperator
				if(sep_idx==-1)  THROW(FileParseException, "Source line does not contain version after first space/underscore: " + header);

				QByteArray tmp = application_string.left(sep_idx).trimmed();
				if (tmp=="GenerateSVCandidates") caller = "Manta";
				else if (tmp=="DRAGEN") caller = "DRAGEN";
				else if (tmp=="Sniffles2") caller = "Sniffles";

				caller_version = application_string.mid(sep_idx+1).trimmed();
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

        if(debug) out << "Callset id: " << callset_id << QT_ENDL;

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
                    << db.getValue("SELECT quality_metrics FROM " + db_table_name + " WHERE id=" + QByteArray::number(sv_id)).toString() << QT_ENDL;
			}

		}

        out << "Imported SVs: " << sv_imported << QT_ENDL;
        out << "Skipped SVs: " << svs.count() - sv_imported << QT_ENDL;

		//output timing
		if (!no_time)
		{
            out << "Import took: " << Helper::elapsedTime(timer) << QT_ENDL;
		}
	}

	void importREs(NGSD& db, QTextStream& out, QString ps_name, bool debug, bool no_time, bool force)
	{
		QString filename = getInfile("re");
		if (filename=="") return;

        out << QT_ENDL;
        out << "### importing REs for " << ps_name << " ###" << QT_ENDL;
        out << "filename: " << filename << QT_ENDL;

        QElapsedTimer timer;
		timer.start();

		// get processed sample id
		QString ps_id = db.processedSampleId(ps_name);
		if(debug)
		{
            out << "Processed sample id: " << ps_id << QT_ENDL;
            out << "REs in NGSD: " << db.getValue("SELECT count(*) FROM repeat_expansion").toString() << QT_ENDL;
		}

		int report_conf_id = db.reportConfigId(ps_id);
		if (report_conf_id!=-1)
		{
			SqlQuery query = db.getQuery();
			query.exec("SELECT * FROM report_configuration_re WHERE report_configuration_id=" + QString::number(report_conf_id));
			if (query.size()>0)
			{
                out << "Skipped import of REs for sample " + ps_name + ": a report configuration with REs exists for this sample!" << QT_ENDL;
				return;
			}
		}

		//check if CNV callset already exists > delete old callset
		QString last_callset_id = db.getValue("SELECT id FROM re_callset WHERE processed_sample_id=:0", true, ps_id).toString();
		if(last_callset_id!="" && !force)
		{
			out << "NOTE: REs were already imported for '" << ps_name << "' - skipping import";
			return;
		}

		// check if processed sample has already been imported
		int imported_re_genotypes = db.getValue("SELECT count(*) FROM repeat_expansion_genotype WHERE processed_sample_id=:0", true, ps_id).toInt();
		if(imported_re_genotypes>0 && !force)
		{
            out << "NOTE: REs were already imported for '" << ps_name << "' - skipping import" << QT_ENDL;
			return;
		}

		//remove old imports of this processed sample
		if ((last_callset_id!="" || imported_re_genotypes>0) && force)
		{
			db.getQuery().exec("DELETE FROM re_callset WHERE processed_sample_id='" + ps_id + "'");

			QSqlQuery query = db.getQuery();
			query.exec("DELETE FROM repeat_expansion_genotype WHERE processed_sample_id='" + ps_id + "'");
            out << "Deleted " << query.numRowsAffected() << " previous repeat expansion calls" << QT_ENDL;
		}

		//load VCF file
		RepeatLocusList res;
		res.load(filename);

		SqlQuery insert_callset = db.getQuery();
		insert_callset.prepare("INSERT INTO `re_callset` (`processed_sample_id`, `caller`, `caller_version`, `call_date`) VALUES (:0,:1,:2,:3)");
		insert_callset.bindValue(0, ps_id);
		insert_callset.bindValue(1, RepeatLocusList::typeToString(res.caller()));
		insert_callset.bindValue(2, res.callerVersion());
		insert_callset.bindValue(3, res.callDate());
		insert_callset.exec();

		//prepare queries
		SqlQuery q_insert = db.getQuery();
		q_insert.prepare("INSERT INTO repeat_expansion_genotype (`processed_sample_id`, `repeat_expansion_id`, `allele1`, `allele2`, `filter`) VALUES (:0,:1,:2,:3,:4)");

		//add repeat expansions
		int re_imported = 0;
		int skipped_not_ngsd = 0;
		int skipped_no_gt = 0;
		int skipped_invalid = 0;
		for(int r=0; r<res.count(); ++r)
		{
			const RepeatLocus& re = res[r];

			//get repeat ID
			int repeat_id = db.repeatExpansionId(re.region(), re.unit(), false);
			if (repeat_id==-1)
			{
                if (debug) out << "Skipped repeat '" << re.toString(true, false) << "' because it is not in NGSD!" << QT_ENDL;
				++skipped_not_ngsd;
				continue;
			}

			//check genotypes data is available
			if (re.allele1().isEmpty())
			{
                if (debug) out << "Skipped repeat '" << re.toString(true, true) << "' because genotype could not be determined." << QT_ENDL;
				++skipped_no_gt;
				continue;
			}

			if (!re.isValid())
			{
                if (debug) out << "Skipped repeat '" << re.toString(true, true) << "' because it is not valid!" << QT_ENDL;
				++skipped_invalid;
				continue;
			}

			q_insert.bindValue(0, ps_id);
			q_insert.bindValue(1, repeat_id);
			q_insert.bindValue(2, re.allele1());
			q_insert.bindValue(3, re.allele2().isEmpty() ? QVariant() : re.allele2());
			q_insert.bindValue(4, re.filters().isEmpty() ? QVariant() : re.filters().join(","));
			q_insert.exec();

			++re_imported;
		}

        out << "Imported REs: " << re_imported << QT_ENDL;
        out << "Skipped REs not found in NGSD: " << skipped_not_ngsd << QT_ENDL;
        out << "Skipped REs without genotype: " << skipped_no_gt << QT_ENDL;
        out << "Skipped REs not valid: " << skipped_invalid  << " (should not happen)" << QT_ENDL;

		//output timing
		if (!no_time)
		{
            out << "Import took: " << Helper::elapsedTime(timer) << QT_ENDL;
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
		bool force = getFlag("force");
		bool var_update = getFlag("var_update");

		//prevent tumor samples from being imported into the germline variant tables
		QString s_id = db.sampleId(ps_name);
		SampleData sample_data = db.getSampleData(s_id);
		if (sample_data.is_tumor)
		{
			THROW(ArgumentException, "Cannot import variant data for sample " + ps_name + ": the sample is a tumor sample according to NGSD!");
		}

		//import
		importSmallVariants(db, stream, ps_name, debug, no_time, force, var_update);
		importCNVs(db, stream, ps_name, debug, no_time, force);
		importSVs(db, stream, ps_name, debug, no_time, force);
		importREs(db, stream, ps_name, debug, no_time, force);
	}
};

#include "main.moc"

int main(int argc, char *argv[])
{
	ConcreteTool tool(argc, argv);
	return tool.execute();
}
