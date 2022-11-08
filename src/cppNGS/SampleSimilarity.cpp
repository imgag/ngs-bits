#include "SampleSimilarity.h"
#include "Exceptions.h"
#include "BasicStatistics.h"
#include "NGSHelper.h"

SampleSimilarity::VariantGenotypes SampleSimilarity::genotypesVcf(const VcfFile& variants, const QString& filename, bool include_gonosomes, bool skip_multi)
{
	//vcf file must have only one sample to parse the correct genotype
	if(variants.sampleIDs().count() > 1)
	{
		THROW(FileParseException, "The genotype can not be determined correctly for a VCF line with multiple samples. File name:  " + filename + " .");
	}

	if (!variants.formatIDs().contains("GT"))
	{
		THROW(FileParseException, "Could not determine genotype column for variant list " + filename);
	}

	VariantGenotypes output;
	for (int i=0; i<variants.count(); ++i)
	{
		const VcfLine& variant = variants[i];

		//skip variants not on autosomes
		if(!variant.chr().isAutosome() && !include_gonosomes) continue;

		//skip multi-allelic variants
		if (variant.isMultiAllelic() && skip_multi)
		{
			continue;
		}
		else if(variant.isMultiAllelic())
		{
			THROW(ArgumentException, "Can not handle multiallelic variants.");
		}

		output[strToPointer(variant.toString())] = genoToDouble(variant.formatValueFromSample("GT"));
	}

	return output;
}

SampleSimilarity::VariantGenotypes SampleSimilarity::genotypesGSvar(VariantList variants, QString filename, bool include_gonosomes)
{
	//determine genotype column
	int geno_col = -1;
	QList<int> affected_cols = variants.getSampleHeader().sampleColumns(true);
	if (affected_cols.count()==1)
	{
		geno_col = affected_cols[0];
	}
	if (geno_col==-1)
	{
		THROW(FileParseException, "Could not determine genotype column for variant list " + filename);
	}

	//get index of consequence column
	int i_cons = variants.annotationIndexByName("coding_and_splicing", true, true);

	//convert data
	VariantGenotypes output;
	for (int i=0; i<variants.count(); ++i)
	{
		Variant& variant = variants[i];

		//skip variants not on autosomes
		if(!variant.chr().isAutosome() && !include_gonosomes) continue;

		//skip MODIFIER impact variants (only rare or known pathogenic intronic/intergenic are stored in GSvar files for WGS)
		QByteArray consequence_text = variant.annotations()[i_cons];
		if(!consequence_text.contains(":HIGH:") && !consequence_text.contains(":MODERATE:") && !consequence_text.contains(":LOW:")) continue;

		output[strToPointer(variant.toString())] = genoToDouble(variant.annotations()[geno_col]);
	}

	return output;
}

SampleSimilarity::VariantGenotypes SampleSimilarity::genotypesBam(const VcfFile& snps, BamReader& reader, int min_cov, int max_snps, bool include_gonosomes)
{
	VariantGenotypes output;
	for(int i=0; i<snps.count(); ++i)
	{
		const Chromosome& chr = snps[i].chr();
		int pos = snps[i].start();

		if (!chr.isAutosome() && !include_gonosomes) continue;

		Pileup pileup = reader.getPileup(chr, pos);
		if (pileup.depth(false)<min_cov) continue;

		QChar ref = snps[i].ref()[0];
		QChar obs = snps[i].alt(0)[0];
		double frequency = pileup.frequency(ref, obs);

		//skip non-informative snps
		if (!BasicStatistics::isValidFloat(frequency)) continue;

		output[strToPointer(chr.strNormalized(false) + ":" + QString::number(pos) + " " + ref + ">" + obs)] = frequency;

		if (output.count()>=max_snps) break;
	}

	return output;
}

SampleSimilarity::VariantGenotypes SampleSimilarity::genotypesFromVcf(QString filename, bool include_gonosomes, bool skip_multi, const BedFile& roi)
{
	VcfFile variants;
	variants.load(filename, roi, false);

	//vcf file must have only one sample to parse the correct genotype
	if(variants.sampleIDs().count() > 1)
	{
		THROW(FileParseException, "The genotype can not be determined correctly for a VCF line with multiple samples. File name:  " + filename + " .");
	}

	if (!variants.formatIDs().contains("GT"))
	{
		THROW(FileParseException, "Could not determine genotype column for variant list " + filename);
	}

	VariantGenotypes output = genotypesVcf(variants, filename, include_gonosomes, skip_multi);

	return output;
}

SampleSimilarity::VariantGenotypes SampleSimilarity::genotypesFromVcf(QString filename, bool include_gonosomes, bool skip_multi)
{
	VcfFile variants;
	variants.load(filename, false);

	//vcf file must have only one sample to parse the correct genotype
	if(variants.sampleIDs().count() > 1)
	{
		THROW(FileParseException, "The genotype can not be determined correctly for a VCF line with multiple samples. File name:  " + filename + " .");
	}

	if (!variants.formatIDs().contains("GT"))
	{
		THROW(FileParseException, "Could not determine genotype column for variant list " + filename);
	}

	VariantGenotypes output = genotypesVcf(variants, filename, include_gonosomes, skip_multi);

	return output;
}

SampleSimilarity::VariantGenotypes SampleSimilarity::genotypesFromGSvar(QString filename, bool include_gonosomes, const BedFile& roi)
{
	VariantList variants;
	variants.load(filename, roi);

	VariantGenotypes output = genotypesGSvar(variants, filename, include_gonosomes);

	return output;
}

SampleSimilarity::VariantGenotypes SampleSimilarity::genotypesFromGSvar(QString filename, bool include_gonosomes)
{
	VariantList variants;
	variants.load(filename);

	VariantGenotypes output = genotypesGSvar(variants, filename, include_gonosomes);

	return output;
}

SampleSimilarity::VariantGenotypes SampleSimilarity::genotypesFromBam(GenomeBuild build, const QString& filename, int min_cov, int max_snps, bool include_gonosomes, const BedFile& roi, const QString& ref_file)
{
	//get known SNP list
	VcfFile snps;
	snps = NGSHelper::getKnownVariants(build, true, roi, 0.2, 0.8);

	//open BAM
	BamReader reader(filename, ref_file);

	//get VariantGenotypes
	VariantGenotypes output = genotypesBam(snps, reader, min_cov, max_snps, include_gonosomes);

	return output;
}

SampleSimilarity::VariantGenotypes SampleSimilarity::genotypesFromBam(GenomeBuild build, const QString& filename, int min_cov, int max_snps, bool include_gonosomes, const QString& ref_file)
{
	//get known SNP list
	VcfFile snps;
	snps = NGSHelper::getKnownVariants(build, true, 0.2, 0.8);

	//open BAM
	BamReader reader(filename, ref_file);

	//get VariantGenotypes
	VariantGenotypes output = genotypesBam(snps, reader, min_cov, max_snps, include_gonosomes);

	return output;
}

void SampleSimilarity::calculateSimilarity(const VariantGenotypes& in1, const VariantGenotypes& in2)
{
	static QVector<double> geno1;
	geno1.clear();
	static QVector<double> geno2;
	geno2.clear();
	clear();

	//calculate overlap / correlation
	int c_ol = 0;
	int c_ibs2 = 0;
	int c_ibs0 = 0;
	for (auto it=in1.cbegin(); it!=in1.cend(); ++it)
	{
		float freq1 = it.value();
		float freq2 = in2.value(it.key(), -1.0);
		if (freq2==-1.0) continue;
		++c_ol;

		geno1.append(freq1);
		geno2.append(freq2);

		if ((freq1>0.9 && freq2>0.9) || (freq1<0.1 && freq2<0.1)) ++c_ibs2;
		if ((freq1>0.9 && freq2<0.1) || (freq1<0.1 && freq2>0.9)) ++c_ibs0;
	}

	//abort if no overlap
	if (geno1.count()==0 || geno2.count()==0)
	{
		messages_.append("Zero overlap between variant lists!");
		return;
	}

	//count overall number of variants
	no_variants1_ = in1.count();
	no_variants2_ = in2.count();
	int min_count = std::min(no_variants1_, no_variants2_);
	ol_perc_ = 100.0 * c_ol / min_count;
	ol_count_ = c_ol;
	sample_correlation_ = BasicStatistics::correlation(geno1, geno2);
	ibs2_perc_ = 100.0 * c_ibs2 / min_count;
	ibs0_perc_ = 100.0 * c_ibs0 / min_count;

	//calulate percentage with same genotype if correlation is not calculatable
	if (!BasicStatistics::isValidFloat(sample_correlation_))
	{
		double equal = 0.0;
		for (int i=0; i<geno1.count(); ++i)
		{
			equal += (geno1[i]==geno2[i]);
		}
		sample_correlation_ = equal / geno1.count();
		messages_.append("Could not calulate genotype correlation, calculated the fraction of matching genotypes instead.");
	}
}

void SampleSimilarity::clear()
{
	no_variants1_ = 0;
	no_variants2_ = 0;
	sample_correlation_ = std::numeric_limits<double>::quiet_NaN();
	ol_perc_ = std::numeric_limits<double>::quiet_NaN();
	ol_count_ = 0;
	ibs0_perc_ = std::numeric_limits<double>::quiet_NaN();
	ibs2_perc_ = std::numeric_limits<double>::quiet_NaN();
	messages_.clear();
}

float SampleSimilarity::genoToDouble(const QString& geno)
{
	//GSvar format
	if (geno=="hom") return 1.0;
	if (geno=="het") return 0.5;

	//VCF format
	if (geno=="1/1" || geno=="1|1") return 1.0;
	if (geno=="0/1" || geno=="0|1" || geno=="./1" || geno==".|1" || geno=="1/0" || geno=="1|0" || geno=="1/." || geno=="1|.") return 0.5;
	if (geno=="0/0" || geno=="0|0" || geno=="./0" || geno==".|0" || geno=="0/." || geno=="0|." || geno==".|.") return 0.0;

	THROW(ArgumentException, "Invalid genotype '" + geno + "' in input file.");
}

const QChar* SampleSimilarity::strToPointer(const QString& str)
{
	static QSet<QString> uniq;

	auto it = uniq.find(str);
	if (it==uniq.cend())
	{
		it = uniq.insert(str);
	}

	return it->constData();
}
