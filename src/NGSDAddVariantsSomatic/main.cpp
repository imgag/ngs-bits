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
		setDescription("Imports variants of a tumor-normal processed sample into the NGSD.");
		addString("t_ps", "Tumor processed sample name", false);
		addString("n_ps", "Normal processed sample name", false);
		//optional
		addInfile("var", "Small variant list (i.e. SNVs and small INDELs) in GSvar format (as produced by megSAP).", true, true);
		addFlag("var_force", "Force import of detected small variants, even if already imported.");
		addInfile("cnv", "CNV list in TSV format (as produced by megSAP).", true, true);
		addFlag("cnv_force", "Force import of CNVs, even if already imported.");
		addInfile("sv", "SV list in TSV format (as produced by megSAP).", true, true);
		addFlag("sv_force", "Force import of SVs, even if already imported.");
		addOutfile("out", "Output file. If unset, writes to STDOUT.", true);
		addFlag("test", "Uses the test database instead of on the production database.");
		addFlag("debug", "Enable verbose debug output.");
		addFlag("no_time", "Disable timing output.");
	}

	//import SNVs/INDELs from tumor-normal GSVar file
	void importSmallVariants(NGSD& db, QTextStream& out, QString t_ps_name, QString n_ps_name, bool no_time, bool var_force)
	{
		QString filename = getInfile("var");
		if(filename=="") return;

		QString ps_full_name = t_ps_name + "-" + n_ps_name;

		out << endl;
		out << "### importing small variants for " << ps_full_name << " ###" << endl;
		out << "filename: " << filename << endl;

		QString t_ps_id = db.processedSampleId(t_ps_name);
		QString n_ps_id = db.processedSampleId(n_ps_name);

		int report_conf_id = db.somaticReportConfigId(t_ps_id, n_ps_id);

		//DO NOT IMPORT Anything if a report config exists and contains small variants
		if(report_conf_id != -1)
		{
			SqlQuery query = db.getQuery();
			query.exec("SELECT * FROM somatic_report_configuration_variant WHERE somatic_report_configuration_id=" + QString::number(report_conf_id));
			if(query.size()>0)
			{
				out << "Skipped import of small variants for sample " << ps_full_name << ": a somatic report configuration with small variants exists for this sample!" << endl;
				return;
			}
		}

		QTime timer;
		timer.start();
		QTime sub_timer;
		QStringList sub_times;

		int count_old = db.getValue("SELECT count(*) FROM detected_somatic_variant WHERE processed_sample_id_tumor=" + t_ps_id + " AND processed_sample_id_normal=" + n_ps_id).toInt();
		out << "Found " << count_old << " variants already imported into NGSD!" << endl;
		if(count_old>0 && !var_force)
		{
			THROW(ArgumentException, "Variants were already imported for '" + ps_full_name + "'. Use the flag '-var_force' to overwrite them.");
		}

		//Remove old variants
		sub_timer.start();
		if(count_old>0 && var_force)
		{
			sub_timer.start();

			SqlQuery query = db.getQuery();
			query.exec("DELETE FROM detected_somatic_variant WHERE processed_sample_id_tumor=" + t_ps_id +" AND processed_sample_id_normal=" + n_ps_id);
			out << "Deleted previous somatic variants." << endl;
			sub_times << ("Deleted previous detected somatic variants took: " + Helper::elapsedTime(sub_timer));
		}

		VariantList variants;
		variants.load(filename);
		if(variants.count() == 0)
		{
			out << "No somatic variants imported (empty GSvar file)." << endl;
			return;
		}

		//add missing variants
		sub_timer.start();


		int c_add, c_update;
		QList<int> variant_ids = db.addVariants(variants, 1.0, c_add, c_update);
		out << "Imported variants (added:" << c_add << " updated:" << c_update << ")" << endl;
		sub_times << ("adding variants took: " + Helper::elapsedTime(sub_timer));

		//add detected somatic variants
		sub_timer.start();

		int i_depth = variants.annotationIndexByName("tumor_dp");
		int i_frq = variants.annotationIndexByName("tumor_af");
		int i_qual = variants.annotationIndexByName("quality");

		SqlQuery q_insert = db.getQuery();
		q_insert.prepare("INSERT INTO detected_somatic_variant (processed_sample_id_tumor, processed_sample_id_normal, variant_id, variant_frequency, depth, quality_snp) VALUES (" + t_ps_id +", "+ n_ps_id +", :0, :1, :2, :3)");

		db.transaction();
		for(int i=0; i<variants.count(); ++i)
		{
			q_insert.bindValue(0, variant_ids[i]);
			q_insert.bindValue(1, variants[i].annotations()[i_frq]);
			q_insert.bindValue(2, variants[i].annotations()[i_depth]);
			q_insert.bindValue(3, variantQuality(variants[i], i_qual));
			q_insert.exec();
		}
		db.commit();
		sub_times << ("Adding detected somatic variants took: " + Helper::elapsedTime(sub_timer));

		if(!no_time)
		{
			out << "Import took: " << Helper::elapsedTime(timer) << endl;
			foreach(const QString& line, sub_times)
			{
				out << " " << line.trimmed() << endl;
			}
		}
	}

	void importCNVs(NGSD& db, QTextStream& out, QString t_ps_name, QString n_ps_name, bool debug, bool no_time, bool cnv_force, double min_ll)
	{
		QString filename = getInfile("cnv");
		if(filename == "") return;

		QTime timer;
		timer.start();

		QString ps_full_name = t_ps_name + "-" + n_ps_name;
		out << endl;
		out << "### importing somatic CNVs for " << ps_full_name << " ###" << endl;
		out << "filename: " << filename << endl;

		//Prevent import if somatic report config contains CNVS
		QString t_ps_id = db.processedSampleId(t_ps_name);
		QString n_ps_id = db.processedSampleId(n_ps_name);
		int report_conf_id = db.somaticReportConfigId(t_ps_id, n_ps_id);

		if(report_conf_id != -1)
		{
			SqlQuery query = db.getQuery();
			query.exec("SELECT * FROM somatic_report_configuration_cnv WHERE somatic_report_configuration_id=" + QString::number(report_conf_id));

			if(query.size() > 0)
			{
				out << "Skipped import of somatic SNVs for sample " << ps_full_name << ": a somatic report configuration with CNVs exists for this sample" << endl;
				return;
			}
		}

		QString last_callset_id = db.getValue("SELECT id FROM somatic_cnv_callset WHERE ps_tumor_id=" + t_ps_id + " AND ps_normal_id=" + n_ps_id).toString();

		if(last_callset_id!="" && !cnv_force)
		{
			out << "Skipped import of CNVs for sample " << ps_full_name << ": a report configuration with somatic CNVs exists for this sample!" << endl;
			return;
		}

		//Delete old CNVs if forced
		if(last_callset_id!="" && cnv_force)
		{
			db.getQuery().exec("DELETE FROM somatic_cnv WHERE somatic_cnv_callset_id ='" + last_callset_id + "'");
			db.getQuery().exec("DELETE FROM somatic_cnv_callset WHERE id='" + last_callset_id + "'");

			out << "Deleted previous somatic CNV callset" << endl;
		}

		//Load CNVs
		CnvList cnvs;
		cnvs.load(filename);

		if(cnvs.type() != CnvListType::CLINCNV_TUMOR_NORMAL_PAIR)
		{
			THROW(ArgumentException, "CNV file is not a tumor normal sample. Use NGSDAddVariantsGermline for germline CNVs.");
		}

		CnvListCallData call_data = CnvList::getCallData(cnvs, filename, "", true);

		QJsonDocument json_doc;
		json_doc.setObject(call_data.quality_metrics);

		out << "caller: " << call_data.caller << endl;
		out << "caller version: " << call_data.caller_version << endl;

		if(debug)
		{
			out << "DEBUG: callset quality: " << json_doc.toJson(QJsonDocument::Compact) << endl;
		}

		//Import somatic cnv callset
		SqlQuery q_set = db.getQuery();
		q_set.prepare("INSERT INTO `somatic_cnv_callset` (`ps_tumor_id`, `ps_normal_id`, `caller`, `caller_version`, `call_date`, `quality_metrics`, `quality`) VALUES (:0, :1, :2, :3, :4, :5, :6)");
		q_set.bindValue(0, t_ps_id);
		q_set.bindValue(1, n_ps_id);
		q_set.bindValue(2, call_data.caller);
		q_set.bindValue(3, call_data.caller_version);
		q_set.bindValue(4, call_data.call_date);
		q_set.bindValue(5, json_doc.toJson(QJsonDocument::Compact));
		q_set.bindValue(6, "n/a");
		q_set.exec();

		int callset_id = q_set.lastInsertId().toInt();


		//Import CNVs
		int c_imported = 0;
		int c_skipped_low_quality = 0;

		for(int i=0; i<cnvs.count(); ++i)
		{
			QString cnv_id = db.addSomaticCnv(callset_id, cnvs[i], cnvs, min_ll);
			if(cnv_id.isEmpty())
			{
				++c_skipped_low_quality;
			}
			else
			{
				++c_imported;
				if(debug)
				{
					out << "DEBUG: " << cnvs[i].toString() << " tumor_cn:" <<db.getValue("SELECT tumor_cn FROM somatic_cnv WHERE id=" + cnv_id).toString();
					out << " quality" << db.getValue("SELECT quality_metrics FROM somatic_cnv WHERE id=" + cnv_id).toString();
				}
			}
		}

		out << "Imported somatic cnvs: " << c_imported << endl;
		out << "Skipped low-quality cnvs: " << c_skipped_low_quality << endl;

		if(!no_time)
		{
			out << "Import took: " << Helper::elapsedTime(timer) << endl;
		}

	}

	void importSVs(NGSD& db, QTextStream& out, QString t_ps_name, QString n_ps_name, bool debug, bool no_time, bool sv_force)
	{
		QString filename = getInfile("sv");
		if (filename=="") return;

		out << endl;
		out << "### importing SVs for tumor-normal pair " << t_ps_name << "-" << n_ps_name << " ###" << endl;
		out << "filename: " << filename << endl;

		QTime timer;
		timer.start();

		// get processed sample id
		QString ps_full_name = t_ps_name + "-" + n_ps_name;
		QString t_ps_id = db.processedSampleId(t_ps_name);
		QString n_ps_id = db.processedSampleId(n_ps_name);
		if(debug) out << "Processed sample ids. Tumor: " << t_ps_id << " Normal: " << n_ps_id << endl;

		//prevent import if report config contains SVs
		int report_conf_id = db.somaticReportConfigId(t_ps_id, n_ps_id);
		if (report_conf_id!=-1)
		{
			SqlQuery query = db.getQuery();
			query.exec("SELECT * FROM somatic_report_configuration_sv WHERE somatic_report_configuration_id=" + QString::number(report_conf_id));

			if(query.size() > 0)
			{
				out << "Skipped import of somatic SNVs for sample " << ps_full_name << ": a somatic report configuration with SVs exists for this sample" << endl;
				return;
			}
		}

		// check if processed sample has already been imported
		QString previous_callset_id = db.getValue("SELECT id FROM somatic_sv_callset WHERE ps_tumor_id=" + t_ps_id + " AND ps_normal_id=" + n_ps_id, true ).toString();
		if(previous_callset_id!="" && !sv_force)
		{
			out << "NOTE: SVs were already imported for '" << ps_full_name << "' - skipping import" << endl;
			return;
		}

		//Delete old SVs if forced
		if(previous_callset_id!="" && sv_force)
		{
			db.getQuery().exec("DELETE FROM somatic_sv_deletion 	 WHERE somatic_sv_callset_id='" + previous_callset_id + "'");
			db.getQuery().exec("DELETE FROM somatic_sv_duplication 	 WHERE somatic_sv_callset_id='" + previous_callset_id + "'");
			db.getQuery().exec("DELETE FROM somatic_sv_inversion 	 WHERE somatic_sv_callset_id='" + previous_callset_id + "'");
			db.getQuery().exec("DELETE FROM somatic_sv_insertion 	 WHERE somatic_sv_callset_id='" + previous_callset_id + "'");
			db.getQuery().exec("DELETE FROM somatic_sv_translocation WHERE somatic_sv_callset_id='" + previous_callset_id + "'");
			db.getQuery().exec("DELETE FROM somatic_sv_callset 		 WHERE id='" + previous_callset_id + "'");

			out << "Deleted previous SV callset" << endl;
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
		if (caller_version == "") THROW(FileParseException, "Version is missing");
		if (date.isNull()) THROW(FileParseException, "Date is missing");

		// create callset entry
		SqlQuery insert_callset = db.getQuery();
		insert_callset.prepare("INSERT INTO `somatic_sv_callset` (`ps_tumor_id`, `ps_normal_id`, `caller`, `caller_version`, `call_date`) VALUES (:0,:1,:2,:3, :4)");
		insert_callset.bindValue(0, t_ps_id);
		insert_callset.bindValue(1, n_ps_id);
		insert_callset.bindValue(2, caller);
		insert_callset.bindValue(3, caller_version);
		insert_callset.bindValue(4, date);
		insert_callset.exec();
		int callset_id = insert_callset.lastInsertId().toInt();

		if(debug) out << "Callset id: " << callset_id << endl;

		// import structural variants
		int sv_imported = 0;
		for (int i = 0; i < svs.count(); i++)
		{
			// ignore SVs on special chromosomes
			if (!svs[i].chr1().isNonSpecial() || !svs[i].chr2().isNonSpecial()) continue;

			QString sv_id = db.addSomaticSv(callset_id, svs[i], svs);
			sv_imported++;
			if (debug)
			{
				QString db_table_name = db.somaticSvTableName(svs[i].type());

				out << "DEBUG: " << svs[i].positionRange() << " sv: " << BedpeFile::typeToString(svs[i].type()) << " quality: "
					<< db.getValue("SELECT quality_metrics FROM " + db_table_name + " WHERE id=" + sv_id).toString() << endl;
			}
		}

		out << "Imported SVs: " << sv_imported << endl;
		out << "Skipped SVs: " << svs.count() - sv_imported << endl;


		if(!no_time)
		{
			out << "Import took: " << Helper::elapsedTime(timer) << endl;
		}

	}

	virtual void main()
	{
		//init
		NGSD db(getFlag("test"));
		QSharedPointer<QFile> out = Helper::openFileForWriting(getOutfile("out"), true);
		QTextStream stream(out.data());

		QString t_ps = getString("t_ps");
		QString n_ps = getString("n_ps");
		bool debug = getFlag("debug");
		bool no_time = getFlag("no_time");
		bool var_force = getFlag("var_force");
		bool cnv_force = getFlag("cnv_force");
		bool sv_force = getFlag("sv_force");

		//prevent tumor samples from being imported into the germline variant tables
		SampleData sample_data = db.getSampleData(db.sampleId(t_ps));
		if (!sample_data.is_tumor)
		{
			THROW(ArgumentException, "Cannot import variant data for sample " + t_ps +"-" + n_ps + ": the sample is not a somatic sample according to NGSD!");
		}

		importSmallVariants(db, stream, t_ps, n_ps, no_time, var_force);

		importCNVs(db, stream, t_ps, n_ps, debug, no_time, cnv_force, 15.0);

		importSVs(db , stream , t_ps, n_ps, debug, no_time, sv_force);
	}
private:
	int variantQuality(const Variant& variant, int i_qual) const
	{
		QByteArrayList parts = variant.annotations()[i_qual].split(';');
		foreach(const QByteArray& part, parts)
		{
			if(part.startsWith("QUAL="))
			{
				return part.mid(5).toInt();
			}
		}
		THROW(FileParseException, "Could not parse quality for variant " + variant.toString());
	}
};

#include "main.moc"

int main(int argc, char *argv[])
{
	ConcreteTool tool(argc, argv);
	return tool.execute();
}
