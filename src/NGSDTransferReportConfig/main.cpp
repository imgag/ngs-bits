#include "ToolBase.h"
#include "NGSD.h"
#include "qforeach.h"
#include "BasicStatistics.h"

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
		addString("target_ps", "Processed sample name to which the ReportConfig is transfered to.", false);

		//optional
		addFlag("test", "Uses the test database instead of on the production database.");
	}

	int getCNVIndex(const CnvList& cnvs, const CopyNumberVariant& cnv, int copy_number)
	{
		int idx = -1;
		//debug
		qDebug() << cnvs.count();
		for (int i = 0; i < cnvs.count(); ++i)
		{
			if (cnvs[i].chr() != cnv.chr()) continue;
			if (cnvs[i].copyNumber(cnvs.annotationHeaders()) != copy_number) continue;
			if (!cnvs[i].overlapsWith(cnv.chr(), cnv.start(), cnv.end())) continue;
			// compute overlap
			int overlap_length = std::min(cnvs[i].end(), cnv.end()) - std::max(cnvs[i].start(), cnv.start());
			double overlap_frac1 = (double) overlap_length / cnv.size();
			double overlap_frac2 = (double) overlap_length / cnvs[i].size();
			//overlap has to be at least 90% in both directions
			if (overlap_frac1 < 0.9 || overlap_frac2 < 0.9) continue;

			//else: match found
			idx = i;
			break;
		}
		return idx;
	}

	int getREIndex(const RepeatLocusList& res, const RepeatLocus& re)
	{
		int idx = -1;

		for (int i = 0; i < res.count(); ++i)
		{
			if (!res[i].sameRegionAndLocus(re)) continue;
			//fuzzy repeat match: at least 95% identity of max allele
			int re1_max_allele = std::max(res[i].allele1asInt(), res[i].allele2asInt());
			int re2_max_allele = std::max(re.allele1asInt(), re.allele2asInt());

			double allele_frac = std::min((double) re1_max_allele/re2_max_allele, (double) re2_max_allele/re1_max_allele);


			if (allele_frac >= 0.95)
			{
				idx = i;
				break;
			}
		}
		return idx;
	}

	virtual void main()
	{
		//init
		NGSD db(getFlag("test"));
		QString source_ps_name = getString("source_ps");
		QString source_ps_id = db.processedSampleId(source_ps_name);
		QString target_ps_name = getString("target_ps");
		QString target_ps_id = db.processedSampleId(target_ps_name);

		//checks
		if (source_ps_id == target_ps_id) THROW(ArgumentException, "Source and target sample cannot be the same!")
		// check if target has ReportConfig
		if (db.reportConfigId(target_ps_id) != -1) THROW(ArgumentException, "Target sample already has a ReportConfig!");
		//check sample id
		if (db.sampleId(source_ps_name) != db.sampleId(target_ps_name)) THROW(ArgumentException, "Source and target sample have to be from the same DNA!");
		// check sample similarity?



		//output
		QTextStream std_out(stdout);
		std_out << source_ps_name + " (ps_id: " + source_ps_id + ") > " + target_ps_name + " (ps_id: " + target_ps_id + ")\n";

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
		int rc_id = db.reportConfigId(source_ps_id);
		if (rc_id == -1) THROW(ArgumentException, "Source sample doesn't have a ReportConfig!");
		QSharedPointer<ReportConfiguration> source_report_config = db.reportConfig(rc_id, src_variants, src_cnvs, src_svs, src_res);
		QList<ReportVariantConfiguration> src_report_variants = source_report_config->variantConfig();
		//qDebug() << src_report_variants.size();


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

		//Debug
		qDebug() << "Files loaded!";

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

		//Debug
		qDebug() << "Report meta data copied!";



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

		//Debug
		qDebug() << "ReportConfig loaded!";

		//report for all variants to transfer
		QStringList report;
		report << "\t#type\tsource\t\t\t\ttarget";

		// check if report variants are in target sample
		bool transfer_possible = true;
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
				// qDebug() << "Report variant " + pair.second.toString(' ') + " found in target sample.";
				//transfer to new report
				int ngsd_id = db.variantId(pair.second).toInt();
				ReportVariantConfiguration rvc = pair.first;
				rvc.id = -1; //delete previous id to generate a new variant config in NGSD
				rvc.variant_index = idx;
				target_report_config->set(rvc);

				report << "\tSNV/InDel\t" + pair.second.toString('-') + "\t" + db.variant(QString::number(ngsd_id)).toString('-');
			}
			else
			{
				//only warn if variant was excluded
				if (pair.first.exclude_artefact
					|| pair.first.exclude_frequency
					|| pair.first.exclude_gus
					|| pair.first.exclude_hit2_missing
					|| pair.first.exclude_mechanism
					|| pair.first.exclude_other
					|| pair.first.exclude_phenotype
					|| pair.first.exclude_used_other_var_type)
				{
					qDebug() << "Warning: excuded report variant " + pair.second.toString(' ') + " not found in target sample!";
					n_missed_excluded++;
				}
				else
				{
					qDebug() << "Error: report variant " + pair.second.toString(' ') + " not found in target sample!";
					n_missed++;
					transfer_possible = false;
				}


			}
		}

		//Debug
		qDebug() << "Target SNVs checked!";

		std_out << "	SNVs	all/match/missed/missed_excluded	" + QString::number(variants_to_transfer.size()) + "/" + QString::number(n_match) + "/" + QString::number(n_missed) + "/" + QString::number(n_missed_excluded) + "\n";

		//CNVs
		n_match = 0;
		n_missed = 0;
		n_missed_excluded = 0;
		foreach (const auto pair, cnvs_to_transfer)
		{

			int idx = getCNVIndex(target_cnvs, pair.second, pair.second.copyNumber(src_cnvs.annotationHeaders()));
			//Debug
			// qDebug() << "CNV index extracted!";
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
					report << "\tCNV\t" + pair.second.toString() + "\t" + db.cnv(rvc.id).toString() + " (already in NGSD)";
				}

				target_report_config->set(rvc);


			}
			else
			{
				//only warn if variant was excluded
				if (pair.first.exclude_artefact
					|| pair.first.exclude_frequency
					|| pair.first.exclude_gus
					|| pair.first.exclude_hit2_missing
					|| pair.first.exclude_mechanism
					|| pair.first.exclude_other
					|| pair.first.exclude_phenotype
					|| pair.first.exclude_used_other_var_type)
				{
					// qDebug() << "Warning: excuded report cnv " + pair.second.toString() + " not found in target sample!";
					n_missed_excluded++;
				}
				else
				{
					qDebug() << "Error: report cnv " + pair.second.toString() + " not found in target sample!";
					n_missed++;
					transfer_possible = false;
				}
			}
		}
		//Debug
		qDebug() << "Target CNVs checked!";
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
				if (pair.first.exclude_artefact
					|| pair.first.exclude_frequency
					|| pair.first.exclude_gus
					|| pair.first.exclude_hit2_missing
					|| pair.first.exclude_mechanism
					|| pair.first.exclude_other
					|| pair.first.exclude_phenotype
					|| pair.first.exclude_used_other_var_type)
				{
					qDebug() << "Warning: excuded report sv " + pair.second.toString() + " not found in target sample!";
					n_missed_excluded++;
				}
				else
				{
					qDebug() << "Error: report sv " + pair.second.toString() + " not found in target sample!";
					n_missed++;
					transfer_possible = false;
				}


			}
		}
		//Debug
		qDebug() << "Target SVs checked!";
		std_out << "	SVs	all/match/missed/missed_excluded	" + QString::number(svs_to_transfer.size()) + "/" + QString::number(n_match) + "/" + QString::number(n_missed) + "/" + QString::number(n_missed_excluded) + "\n";


		//REs
		n_match = 0;
		n_missed = 0;
		n_missed_excluded = 0;
		foreach (const auto pair, res_to_transfer)
		{
			int idx = getREIndex(target_res, pair.second);
			if ( idx > -1)
			{
				n_match++;
				// qDebug() << "Report re " + pair.second.toString(true, true) + " found in target sample.";
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
				if (pair.first.exclude_artefact
					|| pair.first.exclude_frequency
					|| pair.first.exclude_gus
					|| pair.first.exclude_hit2_missing
					|| pair.first.exclude_mechanism
					|| pair.first.exclude_other
					|| pair.first.exclude_phenotype
					|| pair.first.exclude_used_other_var_type)
				{
					qDebug() << "Warning: excuded report re " + pair.second.toString(true, true) + " not found in target sample!";
					n_missed_excluded++;
				}
				else
				{
					qDebug() << "Error: report re " + pair.second.toString(true, true) + " not found in target sample!";
					n_missed++;
					transfer_possible = false;
				}


			}
		}
		//Debug
		qDebug() << "Target REs checked!";
		std_out << "	REs	all/match/missed/missed_excluded	" + QString::number(res_to_transfer.size()) + "/" + QString::number(n_match) + "/" + QString::number(n_missed) + "/" + QString::number(n_missed_excluded) + "\n";

		//Report variants to transfer
		std_out << "\n";
		std_out << report.join("\n");
		std_out << "\n";

		//Abort if transfer is not possibe
		if (!transfer_possible)
		{
			std_out << "ERROR: ReportConfig transfer " + source_ps_name + ">" + target_ps_name + " is not possible!\n\n";
			THROW(ArgumentException, "ReportConfig transfer " + source_ps_name + ">" + target_ps_name + " is not possible!");
		}

		std_out << "ReportConfig transfer " + source_ps_name + ">" + target_ps_name + " is possible.\n\n";


		//TODO: activate for production database
		//perform transfer
		if (getFlag("test"))
		{
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
				db.commit();
			}
			catch(...)
			{
				db.rollback();
				throw;
			}
		}



	}
};

#include "main.moc"

int main(int argc, char *argv[])
{
	ConcreteTool tool(argc, argv);
	return tool.execute();
}
