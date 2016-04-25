#include "SampleCorrelation.h"
#include "Exceptions.h"
#include "Statistics.h"
#include "BasicStatistics.h"
#include "NGSHelper.h"

#include "api/BamReader.h"
using namespace BamTools;

void SampleCorrelation::calculateFromVcf(QString& in1, QString& in2, int window)
{
	//load input files
	VariantList file1;
	file1.load(in1);
	VariantList file2;
	file2.load(in2);

	//get genotype column indices
	int col_geno1 = file1.annotationIndexByName("genotype", true, true);
	int col_geno2 = file2.annotationIndexByName("genotype", true, true);

	//calculate overlap / correlation
	int c_ol = 0;
	QVector<double> geno1;
	QVector<double> geno2;
	ChromosomalIndex<VariantList> file2_idx(file2);
	for (int i=0; i<file1.count(); ++i)
	{
		Variant& v1 = file1[i];
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
				geno1.append(genoToDouble(v1.annotations()[col_geno1]));
				geno2.append(genoToDouble(v2.annotations()[col_geno2]));
				break;
			}
		}
	}

	no_variants1_ = file1.count();
	no_variants2_ = file2.count();
	ol_perc_ = 100.0 * c_ol / std::min(no_variants1_, no_variants2_);
	sample_correlation_ = BasicStatistics::correlation(geno1, geno2);

	//calulate percentage with same genotype if correlation is not calculatable
	if (!BasicStatistics::isValidFloat(sample_correlation_))
	{
		double equal = 0.0;
		for (int i=0; i<geno1.count(); ++i)
		{
			equal += (geno1[i]==geno2[i]);
		}
		sample_correlation_ = equal / geno1.count();
		messages_.append("Note: Could not calulate the genotype correlation, calculated the fraction of matching genotypes instead.");
	}
}

void SampleCorrelation::calculateFromBam(QString& in1, QString& in2, int min_cov, int max_snps)
{
	VariantList snps = NGSHelper::getSNPs();

	//open BAM readers
	BamReader r1;
	NGSHelper::openBAM(r1, in1);
	BamReader r2;
	NGSHelper::openBAM(r2, in2);

	//calcualate frequencies
	QVector<double> freq1;
	freq1.reserve(max_snps);
	QVector<double> freq2;
	freq2.reserve(max_snps);
	for(int i=0; i<snps.count(); ++i)
	{
		//out << "SNP " << snp.chr << " " << QString::number(snp.pos) << endl;
		Pileup p1 = NGSHelper::getPileup(r1, snps[i].chr(), snps[i].start());
		//out << " D1: " <<p1.depth() << endl;
		if (p1.depth(false)<min_cov) continue;

		Pileup p2 = NGSHelper::getPileup(r2, snps[i].chr(), snps[i].start());
		//out << " D2: " <<p2.depth() << endl;
		if (p2.depth(false)<min_cov) continue;

		QChar ref = snps[i].ref()[0];
		QChar obs = snps[i].obs()[0];
		double p1_freq = p1.frequency(ref, obs);
		double p2_freq = p2.frequency(ref, obs);

		//skip non-informative snps
		if (!BasicStatistics::isValidFloat(p1_freq) || !BasicStatistics::isValidFloat(p2_freq)) continue;

		freq1.append(p1_freq);
		freq2.append(p2_freq);
		if (freq1.count()==max_snps) break;
	}

//	out << "Number of high-coverage SNPs: " << QString::number(high_cov) << " of " << QString::number(snps.count()) << " (max_snps: " << QString::number(max_snps) << ")" << endl;

	no_variants1_ = freq1.count();
	no_variants2_ = freq2.count();
	total_variants_ = snps.count();
	sample_correlation_ = BasicStatistics::correlation(freq1, freq2);
}

double SampleCorrelation::genoToDouble(const QString& geno)
{
	if (geno=="hom") return 1.0;
	if (geno=="het") return 0.5;

	THROW(ArgumentException, "Invalid genotype '" + geno + "' in input file.");
}
