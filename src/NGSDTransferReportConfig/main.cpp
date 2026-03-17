#include "SampleSimilarity.h"
#include "ToolBase.h"
#include "NGSD.h"
#include "qforeach.h"
#include "qsqlrecord.h"

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
		setDescription("Transfers (germline) Report Configuration from one sample to another.");
		addString("source_ps", "Processed sample name from which the ReportConfig is taken.", false);
		addString("target_ps", "Processed sample name to which the ReportConfig is transferred to.", false);

		//optional
		addFlag("force", "Transfer report even if some variants aren't present in the target sample (Missing variants will be written into the `report_configuration_failed_transfer` table.)");
		addFlag("test", "Uses the test database instead of on the production database.");
	}

	QString reportVariant2Text(const NGSD& db, int rvc_id, VariantType variant_type, const QString& variant_text, const QString& source_ps_name)
	{
		QStringList report_variant_info;

		report_variant_info.append("SourceSample:" + source_ps_name);
		report_variant_info.append("Variant:" + variant_text);
		report_variant_info.append("VariantType:" +  variantTypeToString(variant_type));

		SqlQuery query = db.getQuery();
		if (variant_type == VariantType::SNVS_INDELS) query.prepare("SELECT * FROM report_configuration_variant WHERE id= :id");
		else if (variant_type == VariantType::CNVS) query.prepare("SELECT * FROM report_configuration_cnv WHERE id= :id");
		else if (variant_type == VariantType::SVS) query.prepare("SELECT * FROM report_configuration_sv WHERE id= :id");
		else if (variant_type == VariantType::RES) query.prepare("SELECT * FROM report_configuration_re WHERE id= :id");
		else THROW(ArgumentException, "Invalid VariantType!");
		query.bindValue(":id", rvc_id);
		query.exec();

		if (query.next())
		{
			QSqlRecord record = query.record();

			for (int i = 0; i < record.count(); i++)
			{
				QString column = record.fieldName(i);
				QString value = query.value(i).toString().replace("\t", " ").replace("\n", "<BR>").replace("\r", "").replace("\v", "").replace("\f", "");
				// filter output
				if (column.startsWith("exclude_")) continue;
				if (value.isEmpty()) continue;

				report_variant_info.append(column + ":" + value);
			}
		}
		else
		{
			THROW(ArgumentException, "No ReportConfigurationVariant with id=" + QString::number(rvc_id) + " found!");
		}

		//return a single line
		return report_variant_info.join('\t');
	}

	virtual void main()
	{
		//init
		NGSD db(getFlag("test"));
		QString source_ps_name = getString("source_ps");
		QString source_ps_id = db.processedSampleId(source_ps_name);
		QString target_ps_name = getString("target_ps");
		QString target_ps_id = db.processedSampleId(target_ps_name);
		bool force = getFlag("force");

		//output
		QTextStream std_out(stdout);
		std_out << source_ps_name + " (ps_id: " + source_ps_id + ") > " + target_ps_name + " (ps_id: " + target_ps_id + ")\n";

		//checks
		if (source_ps_id == target_ps_id) THROW(ArgumentException, "Source and target sample cannot be the same!")
		// check if source has ReportConfig
		int rc_id = db.reportConfigId(source_ps_id);
		if (rc_id == -1) THROW(ArgumentException, "Source sample doesn't have a ReportConfig!");
		// check if target has ReportConfig
		if (db.reportConfigId(target_ps_id) != -1) THROW(ArgumentException, "Target sample already has a ReportConfig!");

		//check sample similariaty
		BedFile roi;
		roi.load(":/Resources/hg38_coding_highconf_all_kits.bed");
		SampleSimilarity sc;
		sc.calculateSimilarity(SampleSimilarity::genotypesFromGSvar(db.processedSamplePath(source_ps_id, PathType::GSVAR), false, roi),
							   SampleSimilarity::genotypesFromGSvar(db.processedSamplePath(target_ps_id, PathType::GSVAR), false, roi));
		std_out << "Sample correlation:\t" + QString::number(sc.sampleCorrelation(), 'f', 4) + " (overlapping variants: " + QString::number(sc.olCount()) + ")\n\n";
		if (sc.sampleCorrelation() < 0.9)
		{
			THROW(ArgumentException, "Sample correlation between " + source_ps_name + " and " + target_ps_name + " to low! (" + QString::number(sc.sampleCorrelation(), 'f', 4)
										 + ", should be above 0.9)");
		}



		//load source variant files
		VariantList src_variants;
		src_variants.load(db.processedSamplePath(source_ps_id, PathType::GSVAR));
		CnvList src_cnvs;
		src_cnvs.load(db.processedSamplePath(source_ps_id, PathType::COPY_NUMBER_CALLS));
		BedpeFile src_svs;
		src_svs.load(db.processedSamplePath(source_ps_id, PathType::STRUCTURAL_VARIANTS));
		RepeatLocusList src_res;
		src_res.load(db.processedSamplePath(source_ps_id, PathType::REPEAT_EXPANSIONS));

		// get source report		
		QSharedPointer<ReportConfiguration> source_report_config = db.reportConfig(rc_id, src_variants, src_cnvs, src_svs, src_res);
		QList<ReportVariantConfiguration> src_report_variants = source_report_config->variantConfig();


		//load target variant files
		VariantList target_variants;
		target_variants.load(db.processedSamplePath(target_ps_id, PathType::GSVAR));
		CnvList target_cnvs;
		target_cnvs.load(db.processedSamplePath(target_ps_id, PathType::COPY_NUMBER_CALLS));
		BedpeFile target_svs;
		target_svs.load(db.processedSamplePath(target_ps_id, PathType::STRUCTURAL_VARIANTS));
		RepeatLocusList target_res;
		target_res.load(db.processedSamplePath(target_ps_id, PathType::REPEAT_EXPANSIONS));

		//load callset IDs
		int target_cnv_callset_id = db.getValue("SELECT id FROM cnv_callset WHERE processed_sample_id=:0", false, target_ps_id).toInt();
		int target_sv_callset_id = db.getValue("SELECT id FROM sv_callset WHERE processed_sample_id=:0", false, target_ps_id).toInt();

		//create target report and transfer meta data
		QSharedPointer<ReportConfiguration> target_report_config = QSharedPointer<ReportConfiguration>(new ReportConfiguration());
		target_report_config->setOtherCausalVariant(source_report_config->otherCausalVariant());

		target_report_config->setCreatedBy(source_report_config->createdBy());
		target_report_config->setCreatedAt(source_report_config->createdAt());

		// parse source report varaints:
		QList<QPair<ReportVariantConfiguration,Variant>> variants_to_transfer;
		QList<QPair<ReportVariantConfiguration,CopyNumberVariant>> cnvs_to_transfer;
		QList<QPair<ReportVariantConfiguration,BedpeLine>> svs_to_transfer;
		QList<QPair<ReportVariantConfiguration,RepeatLocus>> res_to_transfer;




		foreach (const ReportVariantConfiguration& rvc, src_report_variants)
		{
			if (rvc.variant_type == VariantType::SNVS_INDELS)
			{
				int var_id = db.getValue("SELECT variant_id FROM report_configuration_variant WHERE id=:0", false, QString::number(rvc.id)).toInt();
				Variant var = db.variant(QString::number(var_id));
				// qDebug() << var.toString(' ');
				variants_to_transfer.append(QPair<ReportVariantConfiguration,Variant>(rvc, var));
			}
			else if(rvc.variant_type == VariantType::CNVS)
			{
				int cnv_id = db.getValue("SELECT cnv_id FROM report_configuration_cnv WHERE id=:0", false, QString::number(rvc.id)).toInt();
				CopyNumberVariant db_cnv = db.cnv(cnv_id);
				// qDebug() << db_cnv.toString();
				CopyNumberVariant cnv = src_cnvs[rvc.variant_index];
				// qDebug() << cnv.toString();
				//sanity check
				if (!db_cnv.hasSamePosition(cnv)) THROW(ArgumentException, "CNVs of ReportConfig differ between NGSD and CNV file! (File:" + cnv.toString() + " <> NGSD:" + db_cnv.toString() + ")");

				cnvs_to_transfer.append(QPair<ReportVariantConfiguration,CopyNumberVariant>(rvc, cnv));
			}
			else if(rvc.variant_type == VariantType::SVS)
			{
				QVariant sv_id = db.getValue("SELECT sv_deletion_id FROM report_configuration_sv WHERE id=:0", false, QString::number(rvc.id));
				if (!sv_id.isNull())
				{
					BedpeLine sv = db.structuralVariant(sv_id.toInt(), StructuralVariantType::DEL, src_svs);
					// qDebug() << sv.toString();
					svs_to_transfer.append(QPair<ReportVariantConfiguration,BedpeLine>(rvc, sv));
					continue;
				}
				sv_id = db.getValue("SELECT sv_duplication_id FROM report_configuration_sv WHERE id=:0", false, QString::number(rvc.id));
				if (!sv_id.isNull())
				{
					BedpeLine sv = db.structuralVariant(sv_id.toInt(), StructuralVariantType::DUP, src_svs);
					// qDebug() << sv.toString();
					svs_to_transfer.append(QPair<ReportVariantConfiguration,BedpeLine>(rvc, sv));
					continue;
				}
				sv_id = db.getValue("SELECT sv_insertion_id FROM report_configuration_sv WHERE id=:0", false, QString::number(rvc.id));
				if (!sv_id.isNull())
				{
					BedpeLine sv = db.structuralVariant(sv_id.toInt(), StructuralVariantType::INS, src_svs);
					// qDebug() << sv.toString();
					svs_to_transfer.append(QPair<ReportVariantConfiguration,BedpeLine>(rvc, sv));
					continue;
				}
				sv_id = db.getValue("SELECT sv_inversion_id FROM report_configuration_sv WHERE id=:0", false, QString::number(rvc.id));
				if (!sv_id.isNull())
				{
					BedpeLine sv = db.structuralVariant(sv_id.toInt(), StructuralVariantType::INV, src_svs);
					// qDebug() << sv.toString();
					svs_to_transfer.append(QPair<ReportVariantConfiguration,BedpeLine>(rvc, sv));
					continue;
				}
				sv_id = db.getValue("SELECT sv_translocation_id FROM report_configuration_sv WHERE id=:0", false, QString::number(rvc.id));
				if (!sv_id.isNull())
				{
					BedpeLine sv = db.structuralVariant(sv_id.toInt(), StructuralVariantType::BND, src_svs);
					// qDebug() << sv.toString();
					svs_to_transfer.append(QPair<ReportVariantConfiguration,BedpeLine>(rvc, sv));
					continue;
				}
				THROW(ArgumentException, "Invalid SV type!");
			}
			else if(rvc.variant_type == VariantType::RES)
			{
				int re_id = db.getValue("SELECT repeat_expansion_genotype_id FROM report_configuration_re WHERE id=:0", false, QString::number(rvc.id)).toInt();
				RepeatLocus re = db.repeatExpansionGenotype(re_id);
				// qDebug() << re.toString(true, true);
				res_to_transfer.append(QPair<ReportVariantConfiguration,RepeatLocus>(rvc, re));
			}
			else
			{
				THROW(ArgumentException, "Invalid variant type!");
			}
		}


		//report for all variants to transfer
		QStringList report;
		report << "\t#type\tsource\t\t\t\ttarget";

		//report for all missed variants
		QStringList report_missed;
		report_missed << "\t#type\tsource";

		//db entries for missed variants
		QStringList missed_variants;

		// check if report variants are in target sample
		bool complete_transfer_possible = true;
		//SNVs/InDels
		int n_match = 0;
		int n_missed = 0;
		int n_missed_excluded = 0;
		foreach (const auto pair, variants_to_transfer)
		{
			int idx = target_variants.indexOf(pair.second);
			if (idx > -1)
			{
				n_match++;
				//transfer to new report
				int ngsd_id = db.variantId(pair.second).toInt();
				ReportVariantConfiguration rvc = pair.first;
				rvc.id = -1; //delete previous id to generate a new variant config in NGSD
				rvc.variant_index = idx;
				target_report_config->set(rvc);

				report << "\tSNV/InDel\t" + pair.second.toString() + "\t" + db.variant(QString::number(ngsd_id)).toString();
			}
			else
			{
				//only warn if variant was excluded
				if (!pair.first.showInReport())
				{
					qDebug() << "Warning: excuded report variant " + pair.second.toString() + " not found in target sample!";
					report_missed << "\tSNV/InDel\t" + pair.second.toString() + "(excluded)";
					n_missed_excluded++;
				}
				else
				{
					qDebug() << "Error: report variant " + pair.second.toString() + " not found in target sample!";
					report_missed << "\tSNV/InDel\t" + pair.second.toString();
					missed_variants.append(reportVariant2Text(db, pair.first.id, pair.first.variant_type, pair.second.toString(), source_ps_name));
					n_missed++;
					complete_transfer_possible = false;
				}


			}
		}

		std_out << "	SNVs	all/match/missed/missed_excluded	" + QString::number(variants_to_transfer.size()) + "/" + QString::number(n_match) + "/" + QString::number(n_missed) + "/" + QString::number(n_missed_excluded) + "\n";

		//CNVs
		n_match = 0;
		n_missed = 0;
		n_missed_excluded = 0;
		foreach (const auto pair, cnvs_to_transfer)
		{

			int idx = target_cnvs.findMatch(pair.second, pair.second.copyNumber(src_cnvs.annotationHeaders()), true);
			if (idx > -1)
			{
				n_match++;
				//transfer to new report
				ReportVariantConfiguration rvc = pair.first;
				rvc.id = -1; //delete previous id to generate a new variant config in NGSD
				rvc.variant_index = idx;
				QString ngsd_id = db.cnvId(target_cnvs[idx], target_cnv_callset_id, false);
				if (ngsd_id.isEmpty())
				{
					// rvc.id = -1;
					report << "\tCNV\t" + pair.second.toString() + "\t" + target_cnvs[idx].toString() + " (not in NGSD yet)";
				}
				else
				{
					// rvc.id = ngsd_id.toInt();
					report << "\tCNV\t" + pair.second.toString() + "\t" + db.cnv(ngsd_id.toInt()).toString() + " (already in NGSD)";
				}

				target_report_config->set(rvc);

			}
			else
			{
				//only warn if variant was excluded
				if (!pair.first.showInReport())
				{
					qDebug() << "Warning: excuded report cnv " + pair.second.toString() + " not found in target sample!";
					report_missed << "\tCNV\t" + pair.second.toString() + "(excluded)";
					n_missed_excluded++;
				}
				else
				{
					qDebug() << "Error: report cnv " + pair.second.toString() + " not found in target sample!";
					report_missed << "\tCNV\t" + pair.second.toString();
					missed_variants.append(reportVariant2Text(db, pair.first.id, pair.first.variant_type, pair.second.toString(), source_ps_name));
					n_missed++;
					complete_transfer_possible = false;
				}
			}
		}
		std_out << "	CNVs	all/match/missed/missed_excluded	" + QString::number(cnvs_to_transfer.size()) + "/" + QString::number(n_match) + "/" + QString::number(n_missed) + "/" + QString::number(n_missed_excluded) + "\n";

		//SVs
		n_match = 0;
		n_missed = 0;
		n_missed_excluded = 0;
		foreach (const auto pair, svs_to_transfer)
		{
			int idx = target_svs.findMatch(pair.second, false, false, true);
			if ( idx > -1)
			{
				n_match++;
				//transfer to new report
				int ngsd_id = db.svId(target_svs[idx], target_sv_callset_id, target_svs).toInt();
				ReportVariantConfiguration rvc = pair.first;
				rvc.id = -1; //delete previous id to generate a new variant config in NGSD
				rvc.variant_index = idx;
				target_report_config->set(rvc);

				report << "\tSV\t" + pair.second.toString(true) + "\t" + db.structuralVariant(ngsd_id, target_svs[idx].type(), target_svs, true).toString(true);
			}
			else
			{
				//only warn if variant was excluded
				if (!pair.first.showInReport())
				{
					qDebug() << "Warning: excuded report sv " + pair.second.toString() + " not found in target sample!";
					report_missed << "\tSV\t" + pair.second.toString(true) + "(excluded)";
					n_missed_excluded++;
				}
				else
				{
					qDebug() << "Error: report sv " + pair.second.toString() + " not found in target sample!";
					report_missed << "\tSV\t" + pair.second.toString(true);
					missed_variants.append(reportVariant2Text(db, pair.first.id, pair.first.variant_type, pair.second.toString(true), source_ps_name));
					n_missed++;
					complete_transfer_possible = false;
				}


			}
		}
		//Debug
		std_out << "	SVs	all/match/missed/missed_excluded	" + QString::number(svs_to_transfer.size()) + "/" + QString::number(n_match) + "/" + QString::number(n_missed) + "/" + QString::number(n_missed_excluded) + "\n";


		//REs
		n_match = 0;
		n_missed = 0;
		n_missed_excluded = 0;
		foreach (const auto pair, res_to_transfer)
		{
			// int idx = getREIndex(target_res, pair.second);
			int idx = target_res.findMatch(pair.second, true);
			if ( idx > -1)
			{
				n_match++;
				//transfer to new report
				int re_id = db.repeatExpansionId(pair.second.region(), pair.second.unit());
				int ngsd_id = db.repeatExpansionGenotypeId(re_id, target_ps_id.toInt());
				ReportVariantConfiguration rvc = pair.first;
				rvc.id = -1; //delete previous id to generate a new variant config in NGSD
				rvc.variant_index = idx;
				target_report_config->set(rvc);

				report << "\tRE\t" + pair.second.toString(true, true) + "\t" + db.repeatExpansionGenotype(ngsd_id).toString(true, true);
			}
			else
			{
				//only warn if variant was excluded
				if (!pair.first.showInReport())
				{
					qDebug() << "Warning: excuded report re " + pair.second.toString(true, true) + " not found in target sample!";
					report_missed << "\tRE\t" + pair.second.toString(true, true) + "(excluded)";
					n_missed_excluded++;
				}
				else
				{
					qDebug() << "Error: report re " + pair.second.toString(true, true) + " not found in target sample!";
					report_missed << "\tRE\t" + pair.second.toString(true, true);
					missed_variants.append(reportVariant2Text(db, pair.first.id, pair.first.variant_type, pair.second.toString(true, true), source_ps_name));
					n_missed++;
					complete_transfer_possible = false;
				}


			}
		}

		std_out << "	REs	all/match/missed/missed_excluded	" + QString::number(res_to_transfer.size()) + "/" + QString::number(n_match) + "/" + QString::number(n_missed) + "/" + QString::number(n_missed_excluded) + "\n";

		//Report variants to transfer
		if (report.size() > 1)
		{
			std_out << "\n";
			std_out << "The following variants can be transferred:\n";
			std_out << report.join("\n");
			std_out << "\n";
		}


		//Report variants which cannot be transferred
		if (report_missed.size() > 1)
		{
			std_out << "\n";
			std_out << "The following variants cannot be transferred:\n";
			std_out << report_missed.join("\n");
			std_out << "\n";
		}

		std_out << "\n";

		//Abort if transfer is not possibe
		if (!complete_transfer_possible)
		{
			if (force)
			{
				std_out << "WARNING: Only partial ReportConfig transfer " + source_ps_name + ">" + target_ps_name + " possible!\n\n";
			}
			else
			{
				std_out << "ERROR: ReportConfig transfer " + source_ps_name + ">" + target_ps_name + " is not possible!\n\n";
				THROW(ArgumentException, "ReportConfig transfer " + source_ps_name + ">" + target_ps_name + " is not possible!");
			}

		}
		else
		{
			std_out << "ReportConfig transfer " + source_ps_name + ">" + target_ps_name + " is possible.\n\n";
		}



		//perform transfer
		try
		{
			db.transaction();
			int target_report_config_id = db.setReportConfig(target_ps_id, target_report_config, target_variants, target_cnvs, target_svs, target_res);
			//update lastEditBy/lastEditAt
			SqlQuery query = db.getQuery();
			query.exec("UPDATE `report_configuration` SET `last_edit_by`='" + QString::number(db.userId(source_report_config->lastUpdatedBy())) + "', `last_edit_date`='"
					   + source_report_config->lastUpdatedAt().toString("yyyy-MM-dd HH:mm:ss") + "' WHERE id=" +  QString::number(target_report_config_id));
			if (source_report_config->isFinalized())
			{
				query.exec("UPDATE `report_configuration` SET `finalized_by`='" + QString::number(db.userId(source_report_config->finalizedBy())) + "', `finalized_date`='"
						   + source_report_config->finalizedAt().toString("yyyy-MM-dd HH:mm:ss") + "' WHERE id=" +  QString::number(target_report_config_id));
			}
			//save untransferable variants to NGSD
			if (missed_variants.size() > 0)
			{
				query.prepare("INSERT INTO report_configuration_failed_transfer (processed_sample_id, status, variant_description) VALUES (:0, :1, :2)");
				foreach (const QString& variant_description, missed_variants)
				{
					query.bindValue(0, target_ps_id.toInt());
					query.bindValue(1, "open");
					query.bindValue(2, variant_description);
					query.exec();
				}
			}

			db.commit();
		}
		catch(...)
		{
			db.rollback();
			throw;
		}
	}
};

#include "main.moc"

int main(int argc, char *argv[])
{
	ConcreteTool tool(argc, argv);
	return tool.execute();
}
