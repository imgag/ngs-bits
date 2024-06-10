#include "ReportConfiguration.h"
#include "Exceptions.h"
#include "NGSD.h"
#include "Helper.h"
#include "LoginManager.h"

/*************************************** ReportVariantConfiguration ***************************************/

ReportVariantConfiguration::ReportVariantConfiguration()
	: variant_type(VariantType::SNVS_INDELS)
	, variant_index(-1)
	, report_type()
	, causal(false)
	, classification("n/a")
	, inheritance("n/a")
	, de_novo(false)
	, mosaic(false)
	, comp_het(false)
	, exclude_artefact(false)
	, exclude_frequency(false)
	, exclude_phenotype(false)
	, exclude_mechanism(false)
	, exclude_other(false)
	, comments()
	, comments2()
	, rna_info("n/a")
	, manual_var()
	, manual_genotype()
	, manual_cnv_start()
	, manual_cnv_end()
	, manual_cnv_cn()
	, manual_cnv_hgvs_type()
	, manual_cnv_hgvs_suffix()
	, manual_sv_start()
	, manual_sv_end()
	, manual_sv_genotype()
	, manual_sv_hgvs_type()
	, manual_sv_hgvs_suffix()
	, manual_sv_start_bnd()
	, manual_sv_end_bnd()
	, manual_sv_hgvs_type_bnd()
	, manual_sv_hgvs_suffix_bnd()
{
}

bool ReportVariantConfiguration::showInReport() const
{
	return !(exclude_artefact || exclude_frequency || exclude_phenotype || exclude_mechanism || exclude_other);
}

bool ReportVariantConfiguration::isValid(QStringList& errors, FastaFileIndex& ref_index)
{
	errors.clear();

	//check variant type
	if (variant_type==VariantType::INVALID)
	{
		errors << "Variant type is invalid!";
	}

	//check variant index is set
	if (variant_index<0)
	{
		errors << "Variant index not set!";
	}

	//check report type is set
	if (NGSD::isAvailable()) //This check needs the NGSD production database
	{
		if (!getTypeOptions().contains(report_type))
		{
			errors << "Report type '" + report_type + "' invalid! Valid are: '" + getTypeOptions().join("', '") + "'";
		}
	}

	//check causal variant is not excluced
	bool exclude = exclude_artefact || exclude_frequency || exclude_phenotype || exclude_mechanism || exclude_other;
	if (causal && exclude)
	{
		errors << "Variant cannot be causal and excluded at the same time!";
	}

	//manual small variant curation
	if (!manual_var.isEmpty())
	{
		if (variant_type==VariantType::SNVS_INDELS)
		{
			QString error;
			if(!manualVarIsValid(ref_index, &error)) errors << "manually curated variant is invalid: " + error;
		}
		else errors << "small variant sequence is manually set for variant which is not a small variant!";
	}
	if (!manual_genotype.isEmpty())
	{
		if (variant_type==VariantType::SNVS_INDELS)
		{
			if (!manualVarGenoIsValid()) errors << "manually curated genotype '"+manual_genotype+"' is invalid. Valid are 'hom' or 'het'!";
		}
		else errors << "small variant genotype is manually set for variant which is not a small variant!";
	}

	//manual CNV curation
	if (!manual_cnv_start.isEmpty())
	{
		if (variant_type==VariantType::CNVS)
		{
			if(!manualCnvStartIsValid()) errors << "manual start position is set, but not a valid integer. Value is '" + manual_cnv_start + "'";
		}
		else errors << "CNV start position is manually set for variant which is not a CNV!";
	}
	if (!manual_cnv_end.isEmpty())
	{
		if (variant_type==VariantType::CNVS)
		{
			if(!manualCnvEndIsValid()) errors << "manual end position is set, but not a valid integer. Value is '" + manual_cnv_end + "'";
		}
		else errors << "CNV end position is manually set for variant which is not a CNV!";
	}
	if (!manual_cnv_cn.isEmpty())
	{
		if (variant_type==VariantType::CNVS)
		{
			if (!manualCnvCnIsValid()) errors << "manual copy-number is set, but not a valid integer. Value is '" + manual_cnv_cn + "'";
		}
		else errors << "CNV copy-number is manually set for variant which is not a CNV!";
	}

	//manual SV curation
	if (!manual_sv_start.isEmpty())
	{
		if (variant_type==VariantType::SVS)
		{
			if(!manualSvStartIsValid()) errors << "manual start position is set, but not a valid integer. Value is '" + manual_sv_start + "'";
		}
		else errors << "SV start position is manually set for variant which is not a SV!";
	}
	if (!manual_sv_end.isEmpty())
	{
		if (variant_type==VariantType::SVS)
		{
			if(!manualSvEndIsValid()) errors << "manual end position is set, but not a valid integer. Value is '" + manual_sv_end + "'";
		}
		else errors << "SV end position is manually set for variant which is not a SV!";
	}
	if (!manual_sv_genotype.isEmpty())
	{
		if (variant_type==VariantType::SVS)
		{
			if (!manualSvGenoIsValid()) errors << "manually curated genotype '"+manual_sv_genotype+"' is invalid. Valid are 'hom' or 'het'!";
		}
		else errors << "SV genotype is manually set for variant which is not a SV!";
	}
	if (!manual_sv_start_bnd.isEmpty())
	{
		if (variant_type==VariantType::SVS)
		{
			if(!manualSvStartBndIsValid()) errors << "manual start position is set, but not a valid integer. Value is '" + manual_sv_start_bnd + "'";
		}
		else errors << "SV start position of 2nd breakpoint is manually set for variant which is not a SV!";
	}
	if (!manual_sv_end_bnd.isEmpty())
	{
		if (variant_type==VariantType::SVS)
		{
			if(!manualSvEndBndIsValid()) errors << "manual end position is set, but not a valid integer. Value is '" + manual_sv_end_bnd + "'";
		}
		else errors << "SV end position of 2nd breakpoint is manually set for variant which is not a SV!";
	}
	if (!manual_re_allele1.isEmpty())
	{
		if (variant_type==VariantType::RES)
		{
			if(!manualReAllele1IsValid()) errors << "manual allele 1 is set, but not a valid integer. Value is '" + manual_re_allele1 + "'";
		}
		else errors << "RE allele 1 is manually set for variant which is not a RE!";
	}
	if (!manual_re_allele2.isEmpty())
	{
		if (variant_type==VariantType::RES)
		{
			if(!manualReAllele2IsValid()) errors << "manual allele 2 is set, but not a valid integer. Value is '" + manual_re_allele2 + "'";
		}
		else errors << "RE allele 2 is manually set for variant which is not a RE!";
	}

	return errors.isEmpty();
}

bool ReportVariantConfiguration::operator==(const ReportVariantConfiguration& rhs)
{
	return variant_type == rhs.variant_type &&
			variant_index == rhs.variant_index &&
			report_type == rhs.report_type &&
			causal == rhs.causal &&
			classification == rhs.classification &&
			inheritance == rhs.inheritance &&
			de_novo == rhs.de_novo &&
			mosaic == rhs.mosaic &&
			comp_het == rhs.comp_het &&
			exclude_artefact == rhs.exclude_artefact &&
			exclude_frequency == rhs.exclude_frequency &&
			exclude_phenotype == rhs.exclude_phenotype &&
			exclude_mechanism == rhs.exclude_mechanism &&
			exclude_other == rhs.exclude_other &&
			comments == rhs.comments &&
			comments2 == rhs.comments2 &&
			rna_info == rhs.rna_info &&
			manual_cnv_start == rhs.manual_cnv_start &&
			manual_cnv_end == rhs.manual_cnv_end &&
			manual_cnv_cn == rhs.manual_cnv_cn &&
			manual_cnv_hgvs_type == rhs.manual_cnv_hgvs_type &&
			manual_cnv_hgvs_suffix == rhs.manual_cnv_hgvs_suffix &&
			manual_var == rhs.manual_var &&
			manual_genotype == rhs.manual_genotype &&
			manual_sv_start == rhs.manual_sv_start &&
			manual_sv_end == rhs.manual_sv_end &&
			manual_sv_genotype == rhs.manual_sv_genotype &&
			manual_sv_hgvs_type == rhs.manual_sv_hgvs_type &&
			manual_sv_hgvs_suffix == rhs.manual_sv_hgvs_suffix &&
			manual_sv_start_bnd == rhs.manual_sv_start_bnd &&
			manual_sv_end_bnd == rhs.manual_sv_end_bnd &&
			manual_sv_hgvs_type_bnd == rhs.manual_sv_hgvs_type_bnd &&
			manual_sv_hgvs_suffix_bnd == rhs.manual_sv_hgvs_suffix_bnd;
}

bool ReportVariantConfiguration::isManuallyCurated() const
{
	if (variant_type==VariantType::SNVS_INDELS)
	{
		return !manual_var.isEmpty() || manualVarGenoIsValid();
	}
	else if(variant_type==VariantType::CNVS)
	{
		return manualCnvStartIsValid() || manualCnvEndIsValid() || !manual_cnv_cn.isEmpty() || !manual_cnv_hgvs_type.isEmpty() || !manual_cnv_hgvs_suffix.isEmpty();
	}
	else if (variant_type==VariantType::SVS)
	{
		return manualSvStartIsValid() || manualSvEndIsValid() || manualSvGenoIsValid() || manualSvStartBndIsValid() || manualSvEndBndIsValid() || !manual_sv_hgvs_type.isEmpty() || !manual_sv_hgvs_suffix.isEmpty() || !manual_sv_hgvs_type_bnd.isEmpty() || !manual_sv_hgvs_suffix_bnd.isEmpty();
	}
	else if (variant_type==VariantType::RES)
	{
		return manualReAllele1IsValid() || manualReAllele2IsValid();
	}

	THROW(ArgumentException, "ReportVariantConfiguration::isManuallyCurated() called on invalid variant type!");
}

bool ReportVariantConfiguration::manualVarIsValid(FastaFileIndex& ref_index, QString* error) const
{
	if (manual_var.isEmpty()) return false;

	try
	{
		Variant v = Variant::fromString(manual_var);
		v.checkValid(ref_index);
	}
	catch(Exception& e)
	{
		if (error!=nullptr) (*error) = e.message();
		return false;
	}

	return true;
}

bool ReportVariantConfiguration::manualVarGenoIsValid() const
{
	return manual_genotype=="hom" || manual_genotype=="het";
}

void ReportVariantConfiguration::updateVariant(Variant& v, FastaFileIndex& ref_index, int genotype_col_idx) const
{
	if (manualVarIsValid(ref_index))
	{
		Variant v2 = Variant::fromString(manual_var);
		v.setChr(v2.chr());
		v.setStart(v2.start());
		v.setEnd(v2.end());
		v.setRef(v2.ref());
		v.setObs(v2.obs());
	}
	if (manualVarGenoIsValid())
	{
		v.annotations()[genotype_col_idx] = manual_genotype.toUtf8();
	}
}

bool ReportVariantConfiguration::manualCnvStartIsValid() const
{
	if (manual_cnv_start.isEmpty()) return false;

	bool ok = false;
	int value = manual_cnv_start.toInt(&ok);
	if (!ok) return false;

	return value>0;
}

bool ReportVariantConfiguration::manualCnvEndIsValid() const
{
	if (manual_cnv_end.isEmpty()) return false;

	bool ok = false;
	int value = manual_cnv_end.toInt(&ok);
	if (!ok) return false;

	return value>0;
}

bool ReportVariantConfiguration::manualCnvCnIsValid() const
{
	if (manual_cnv_cn.isEmpty()) return false;

	bool ok = false;
	int value = manual_cnv_cn.toInt(&ok);
	if (!ok) return false;

	return value>=0;
}

void ReportVariantConfiguration::updateCnv(CopyNumberVariant& cnv, const QByteArrayList& annotation_headers, NGSD& db) const
{
	//update start, end and copy-number
	if (manualCnvStartIsValid()) cnv.setStart(manual_cnv_start.toInt());
	if (manualCnvEndIsValid()) cnv.setEnd(manual_cnv_end.toInt());
	if (manualCnvCnIsValid()) cnv.setCopyNumber(manual_cnv_cn.toInt(), annotation_headers);

	//update gene list if coordinates changed
	if (manualCnvStartIsValid() || manualCnvEndIsValid())
	{
		cnv.setGenes(db.genesOverlapping(cnv.chr(), cnv.start(), cnv.end(), 5000));
	}
}

bool ReportVariantConfiguration::manualSvStartIsValid() const
{
	if (manual_sv_start.isEmpty()) return false;

	bool ok = false;
	int value = manual_sv_start.toInt(&ok);
	if (!ok) return false;

	return value>0;
}

bool ReportVariantConfiguration::manualSvEndIsValid() const
{
	if (manual_sv_end.isEmpty()) return false;

	bool ok = false;
	int value = manual_sv_end.toInt(&ok);
	if (!ok) return false;

	return value>0;
}

bool ReportVariantConfiguration::manualSvGenoIsValid() const
{
	return manual_sv_genotype=="hom" || manual_sv_genotype=="het";
}

bool ReportVariantConfiguration::manualSvStartBndIsValid() const
{
	if (manual_sv_start_bnd.isEmpty()) return false;

	bool ok = false;
	int value = manual_sv_start_bnd.toInt(&ok);
	if (!ok) return false;

	return value>0;
}

bool ReportVariantConfiguration::manualSvEndBndIsValid() const
{
	if (manual_sv_end_bnd.isEmpty()) return false;

	bool ok = false;
	int value = manual_sv_end_bnd.toInt(&ok);
	if (!ok) return false;

	return value>0;
}

void ReportVariantConfiguration::updateSv(BedpeLine& sv, const QByteArrayList& annotation_headers, NGSD& db) const
{
	if (manualSvStartIsValid())
	{
		int coord = manual_sv_start.toInt();
		if (sv.type()==StructuralVariantType::BND)
		{
			sv.setStart1(coord);
		}
		else
		{
			sv.setStart1(coord);
			sv.setEnd1(coord);
		}
	}
	if (manualSvEndIsValid())
	{
		int coord = manual_sv_end.toInt();
		if (sv.type()==StructuralVariantType::BND)
		{
			sv.setEnd1(coord);
		}
		else
		{
			sv.setStart2(coord);
			sv.setEnd2(coord);
		}
	}
	if (manualSvGenoIsValid())
	{
		sv.setGenotype(annotation_headers, manual_sv_genotype=="hom" ? "1/1" : "0/1");
	}
	if (manualSvStartBndIsValid()) sv.setStart2(manual_sv_start_bnd.toInt());
	if (manualSvEndBndIsValid()) sv.setEnd2(manual_sv_end_bnd.toInt());

	//update gene list if coordinates changed
	if (manualSvStartIsValid() || manualSvEndIsValid() || manualSvStartBndIsValid() || manualSvEndBndIsValid())
	{
		GeneSet genes;
		BedFile regions = sv.affectedRegion(false);
		for (int i=0; i<regions.count(); ++i)
		{
			const BedLine& reg = regions[i];
			genes << db.genesOverlapping(reg.chr(), reg.start(), reg.end(), 5000);
		}
		sv.setGenes(annotation_headers, genes);
	}
}

bool ReportVariantConfiguration::manualReAllele1IsValid() const
{
	if (manual_re_allele1.isEmpty()) return false;

	bool ok = false;
	int value = manual_re_allele1.toInt(&ok);
	if (!ok) return false;

	return value>=0;
}

bool ReportVariantConfiguration::manualReAllele2IsValid() const
{
	if (manual_re_allele2.isEmpty()) return false;

	bool ok = false;
	int value = manual_re_allele2.toInt(&ok);
	if (!ok) return false;

	return value>=0;
}

void ReportVariantConfiguration::updateRe(RepeatLocus& re) const
{
	if (manualReAllele1IsValid())
	{
		re.setAllele1(manual_re_allele1.toUtf8());
	}
	if (manualReAllele2IsValid())
	{
		re.setAllele2(manual_re_allele2.toUtf8());
	}
}

QStringList ReportVariantConfiguration::getTypeOptions()
{
	static QStringList types = NGSD().getEnum("report_configuration_variant", "type");
	return types;
}

QStringList ReportVariantConfiguration::getInheritanceModeOptions()
{
	static QStringList modes = NGSD().getEnum("report_configuration_variant", "inheritance");
	return modes;
}

QStringList ReportVariantConfiguration::getClassificationOptions()
{
	static QStringList modes = NGSD().getEnum("report_configuration_cnv", "class");
	return modes;
}

QStringList ReportVariantConfiguration::getRnaInfoOptions()
{
	static QStringList modes = NGSD().getEnum("report_configuration_variant", "rna_info");
	return modes;
}


/*************************************** ReportConfiguration ***************************************/

ReportConfiguration::ReportConfiguration()
	: variant_config_()
	, created_by_(LoginManager::userLogin())
	, created_at_(QDateTime::currentDateTime())
	, last_updated_by_()
	, last_updated_at_(QDateTime())
	, finalized_by_()
	, finalized_at_(QDateTime())
{
}

const QList<ReportVariantConfiguration>& ReportConfiguration::variantConfig() const
{
	return variant_config_;
}

QList<int> ReportConfiguration::variantIndices(VariantType type, bool only_selected, QString report_type) const
{
	QList<int> output;

	foreach(const ReportVariantConfiguration& var_conf, variant_config_)
	{
		if (var_conf.variant_type!=type) continue;
		if (only_selected && !var_conf.showInReport()) continue;
		if (!report_type.isNull() && report_type!="all" && var_conf.report_type!=report_type) continue;

		output << var_conf.variant_index;
	}

	std::sort(output.begin(), output.end());

	return output;
}

bool ReportConfiguration::exists(VariantType type, int index) const
{
	foreach(const ReportVariantConfiguration& var_conf, variant_config_)
	{
		if (var_conf.variant_index==index && var_conf.variant_type==type) return true;
	}

	return false;
}

const ReportVariantConfiguration& ReportConfiguration::get(VariantType type, int index) const
{
	foreach(const ReportVariantConfiguration& var_conf, variant_config_)
	{
		if (var_conf.variant_index==index && var_conf.variant_type==type) return var_conf;
	}

	THROW(ArgumentException, "Report configuration not found for variant with index '" + QString::number(index) + "'!");
}

OtherCausalVariant ReportConfiguration::otherCausalVariant()
{
	return other_causal_variant_;
}

void ReportConfiguration::setOtherCausalVariant(const OtherCausalVariant& causal_variant)
{
	other_causal_variant_ = causal_variant;
	emit variantsChanged();
}

void ReportConfiguration::set(const ReportVariantConfiguration& config)
{
	bool updated_existing = false;
	for (int i=0; i<variant_config_.count(); ++i)
	{
		const ReportVariantConfiguration& var_conf = variant_config_[i];
		if (var_conf.variant_index==config.variant_index && var_conf.variant_type==config.variant_type)
		{
			variant_config_[i] = config;
			updated_existing = true;
			break;
		}
	}

	if (!updated_existing)
	{
		variant_config_ << config;
		sortByPosition();
	}

	emit variantsChanged();
}

void ReportConfiguration::remove(VariantType type, int index)
{
	for (int i=0; i<variant_config_.count(); ++i)
	{
		const ReportVariantConfiguration& var_conf = variant_config_[i];
		if (var_conf.variant_index==index && var_conf.variant_type==type)
		{
			variants_to_delete_.append(var_conf);
			variant_config_.removeAt(i);
			break;
		}
	}

	emit variantsChanged();
}

const QList<ReportVariantConfiguration>&ReportConfiguration::variantsToDelete() const
{
	return variants_to_delete_;
}

void ReportConfiguration::clearDeletionList()
{
	variants_to_delete_.clear();
}

QSet<int> ReportConfiguration::getVariantConfigIds(VariantType type)
{
	QSet<int> ids;
	foreach (const ReportVariantConfiguration& var_conf, variant_config_)
	{
		if (var_conf.variant_type == type) ids.insert(var_conf.id);
	}
	return ids;
}

QString ReportConfiguration::createdBy() const
{
	return created_by_;
}

void ReportConfiguration::setCreatedBy(QString user_name)
{
	created_by_ = user_name;
}

QDateTime ReportConfiguration::createdAt() const
{
	return created_at_;
}

void ReportConfiguration::setCreatedAt(QDateTime time)
{
	created_at_ = time;
}

QString ReportConfiguration::lastUpdatedBy() const
{
	return last_updated_by_;
}

QDateTime ReportConfiguration::lastUpdatedAt() const
{
	return last_updated_at_;
}

QString ReportConfiguration::finalizedBy() const
{
	return finalized_by_;
}

QDateTime ReportConfiguration::finalizedAt() const
{
	return finalized_at_;
}

bool ReportConfiguration::isFinalized() const
{
	return !finalized_by_.isEmpty();
}

QString ReportConfiguration::history() const
{
	QStringList output;

	output << "The report configuration was created by " + created_by_ + " on " + created_at_.toString("dd.MM.yyyy") + " at " + created_at_.toString("hh:mm:ss") + ".";
	if (last_updated_by_!="") output << "It was last updated by " + last_updated_by_ + " on " +  last_updated_at_.toString("dd.MM.yyyy") + " at " + last_updated_at_.toString("hh:mm:ss") + ".";
	if (finalized_by_!="") output << "It was finalized by " + finalized_by_ + " on " +  finalized_at_.toString("dd.MM.yyyy") + " at " + finalized_at_.toString("hh:mm:ss") + ".";

	return output.join("\n");
}

QString ReportConfiguration::variantSummary() const
{
	//count by type and causal
	int c_small = 0;
	int c_small_causal = 0;
	int c_cnv = 0;
	int c_cnv_causal = 0;
	int c_sv = 0;
	int c_sv_causal = 0;
	int c_re = 0;
	int c_re_causal = 0;
	foreach(const ReportVariantConfiguration& entry, variant_config_)
	{
		if (entry.variant_type==VariantType::SNVS_INDELS)
		{
			++c_small;
			if (entry.causal) ++c_small_causal;
		}
		else if (entry.variant_type==VariantType::CNVS)
		{
			++c_cnv;
			if (entry.causal) ++c_cnv_causal;
		}
		else if (entry.variant_type==VariantType::SVS)
		{
			++c_sv;
			if (entry.causal) ++c_sv_causal;
		}
		else if (entry.variant_type==VariantType::RES)
		{
			++c_re;
			if (entry.causal) ++c_re_causal;
		}
	}

	QStringList output;
	output << ("small variants: " + QString::number(c_small));
	if (c_small_causal>0) output.last().append(" (" + QString::number(c_small_causal) + " causal)");
	output << ("CNVs: " + QString::number(c_cnv));
	if (c_cnv_causal>0) output.last().append(" (" + QString::number(c_cnv_causal) + " causal)");
	output << ("SVs: " + QString::number(c_sv));
	if (c_sv_causal>0) output.last().append(" (" + QString::number(c_sv_causal) + " causal)");
	output << ("REs: " + QString::number(c_re));
	if (c_re_causal>0) output.last().append(" (" + QString::number(c_re_causal) + " causal)");
	if (other_causal_variant_.isValid())
	{
		output << "other causal variant: 1";
	}
	else
	{
		output << "other causal variant: 0";
	}

	return output.join("\n");
}

void ReportConfiguration::sortByPosition()
{
	std::sort(variant_config_.begin(), variant_config_.end(), [](const ReportVariantConfiguration& a, const ReportVariantConfiguration& b){return a.variant_index < b.variant_index;});
}

