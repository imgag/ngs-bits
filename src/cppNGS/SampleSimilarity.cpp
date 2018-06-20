#include "SampleSimilarity.h"
#include "Exceptions.h"
#include "Statistics.h"
#include "BasicStatistics.h"
#include "NGSHelper.h"

void SampleSimilarity::calculateFromVcf(QString& in1, QString& in2, int window, bool include_gonosomes)
{
	//load input files
	VariantList file1;
	file1.load(in1);
	VariantList file2;
	file2.load(in2);

	//get genotype column indices
	int col_geno1 = file1.annotationIndexByName("genotype", true, false);
	int col_geno2 = file2.annotationIndexByName("genotype", true, false);
	if (col_geno1==-1 || col_geno2==-1) //VCF format
	{
		col_geno1 = file1.annotationIndexByName("GT", true, false);
		col_geno2 = file2.annotationIndexByName("GT", true, false);
	}
	if (col_geno1==-1 || col_geno2==-1)
	{
		THROW(FileParseException, "Could not determine genotype column ('genotype' for GSvar, 'GT' for VCF).");
	}

	//calculate overlap / correlation
	int c_ol = 0;
	int c_ibs2 = 0;
	QVector<double> geno1;
	QVector<double> geno2;
	ChromosomalIndex<VariantList> file2_idx(file2);
	for (int i=0; i<file1.count(); ++i)
	{
		Variant& v1 = file1[i];

		//skip variants not on autosomes
		if(!v1.chr().isAutosome() && !include_gonosomes) continue;

		int start = v1.start();
		int end = v1.end();

		//indel => fuzzy position search
		if (!v1.isSNV())
		{
			start -= window;
			end += window;
		}

		QVector<int> matches = file2_idx.matchingIndices(v1.chr(), start, end);
		foreach(int index, matches)
		{
			Variant& v2 = file2[index];
			if (v1.ref()==v2.ref() && v1.obs()==v2.obs())
			{
				++c_ol;

				double freq1 = genoToDouble(v1.annotations()[col_geno1]);
				geno1.append(freq1);
				double freq2 = genoToDouble(v2.annotations()[col_geno2]);
				geno2.append(freq2);

				if (freq1==1.0 && freq2==1.0) ++c_ibs2;

				break;
			}
		}
	}

	//abort if no overlap
	if (geno1.count()==0 || geno2.count()==0)
	{
		THROW(ArgumentException, "Zero overlap between variant lists!")
	}

	//count overall number of variants
	if (include_gonosomes)
	{
		no_variants1_ = file1.count();
		no_variants2_ = file2.count();
	}
	else
	{
		int tmp1 = 0;
		for (int i=0; i<file1.count(); ++i)
		{
			tmp1 += file1[i].chr().isAutosome();
		}
		no_variants1_ = tmp1;

		int tmp2 = 0;
		for (int i=0; i<file2.count(); ++i)
		{
			tmp2 += file2[i].chr().isAutosome();
		}
		no_variants2_ = tmp2;

	}
	int max_count = std::min(no_variants1_, no_variants2_);
	ol_perc_ = 100.0 * c_ol / max_count;
	sample_correlation_ = BasicStatistics::correlation(geno1, geno2);
	ibs0_perc_ = std::numeric_limits<double>::quiet_NaN();
	ibs2_perc_ = 100.0 * c_ibs2 / max_count;

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

void SampleSimilarity::calculateFromBam(QString& in1, QString& in2, int min_cov, int max_snps, bool include_gonosomes, QString roi_file)
{
	VariantList snps;
	if (!roi_file.trimmed().isEmpty())
	{
		BedFile roi;
		roi.load(roi_file);
		snps = NGSHelper::getKnownVariants(true, 0.2, 0.8, &roi);
	}
	else
	{
		snps = NGSHelper::getKnownVariants(true, 0.2, 0.8);
	}

	//open BAM readers
	BamReader r1(in1);
	BamReader r2(in2);

	//calcualate frequencies
	QVector<double> freq1;
	freq1.reserve(max_snps);
	QVector<double> freq2;
	freq2.reserve(max_snps);
	int c_ibs0 = 0;
	int c_ibs2 = 0;
	for(int i=0; i<snps.count(); ++i)
	{
		if (!snps[i].chr().isAutosome() && !include_gonosomes) continue;

		Pileup p1 = r1.getPileup(snps[i].chr(), snps[i].start());
		if (p1.depth(false)<min_cov) continue;

		Pileup p2 = r2.getPileup(snps[i].chr(), snps[i].start());
		if (p2.depth(false)<min_cov) continue;

		QChar ref = snps[i].ref()[0];
		QChar obs = snps[i].obs()[0];
		double p1_freq = p1.frequency(ref, obs);
		double p2_freq = p2.frequency(ref, obs);

		//skip non-informative snps
		if (!BasicStatistics::isValidFloat(p1_freq) || !BasicStatistics::isValidFloat(p2_freq)) continue;

		freq1.append(p1_freq);
		freq2.append(p2_freq);

		if ((p1_freq>0.9 && p2_freq>0.9) || (p1_freq<0.1 && p2_freq<0.1)) ++c_ibs2;
		if ((p1_freq>0.9 && p2_freq<0.1) || (p1_freq<0.1 && p2_freq>0.9)) ++c_ibs0;

		if (freq1.count()==max_snps) break;
	}

	//abort if no overlap
	if (freq1.count()==0)
	{
		no_variants1_ = 0;
		no_variants2_ = 0;
		ol_perc_ = std::numeric_limits<double>::quiet_NaN();
		sample_correlation_ = std::numeric_limits<double>::quiet_NaN();
		ibs0_perc_ = std::numeric_limits<double>::quiet_NaN();
		ibs2_perc_ = std::numeric_limits<double>::quiet_NaN();

		messages_.append("Could not calulate genotype correlation!");
	}
	else
	{
		no_variants1_ = freq1.count();
		no_variants2_ = freq2.count();
		ol_perc_ = std::numeric_limits<double>::quiet_NaN();
		ibs0_perc_ = 100.0 * c_ibs0 / freq1.count();
		ibs2_perc_ = 100.0 * c_ibs2 / freq1.count();
		sample_correlation_ = BasicStatistics::correlation(freq1, freq2);
	}
}

double SampleSimilarity::genoToDouble(const QString& geno)
{
	//GSvar format
	if (geno=="hom") return 1.0;
	if (geno=="het") return 0.5;

	//VCF format
	if (geno=="1/1" || geno=="1|1") return 1.0;
	if (geno=="0/1" || geno=="0|1" || geno=="./1" || geno==".|1" || geno=="1/0" || geno=="1|0" || geno=="1/." || geno=="1|.") return 0.5;
	if (geno=="0/0" || geno=="0|0") return 0.0;

	THROW(ArgumentException, "Invalid genotype '" + geno + "' in input file.");
}
