#include "ToolBase.h"
#include "BasicStatistics.h"
#include "FastaFileIndex.h"
#include "Settings.h"
#include "VariantList.h"
#include "BamReader.h"

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
		setDescription("Annotates a variant list with variant frequencies from a RNA BAM/CRAM file. Used to check for allele-specific expression.");
		addInfile("in", "Input variant list to annotate in GSvar format.", false, true);
		addInfile("bam", "Input BAM/CRAM file.", false, true);
		addOutfile("out", "Output variant list file name (VCF or GSvar).", false, true);
		//optional
		addInfile("ref", "Reference genome FASTA file. If unset 'reference_genome' from the 'settings.ini' file is used.", true, false);

		changeLog(2021, 06, 24, "Initial version.");
	}

	//binomial distribution density
	double binom(int x, int n, double p)
	{
		return std::pow(p, x) * std::pow(1-p, n-x) * BasicStatistics::factorial(n) / BasicStatistics::factorial(x) / BasicStatistics::factorial(n-x);
	}

	//binomial test p-value
	double binomtest_p(int x, int n, double p)
	{
		//for high coverage (~160x), downsample alternative observation and depth
		while (!BasicStatistics::isValidFloat(BasicStatistics::factorial(n)))
		{
			x /= 2;
			n /= 2;
		}

		double pval = 0;

		//sum of all probabilities <= prob_x
		double prob_x = binom(x, n, p);
		for (int i=0; i<=n; ++i)
		{
			double prob_i = binom(i, n, p);
			if (prob_i <= prob_x)
			{
				pval += prob_i;
			}
		}
		return pval;
	}

	//binomial test p-value for p=0.5
	double binomtest_p05(int x, int n)
	{
		//calculate binomial test p-value, one-sided, with p=0.5
		double pval = 0.0;

		//if observed p <= 0.5, use range 0..x
		//if observed p > 0.5, use range 0..(n-x)
		double obs_p = 1.0 * x / n;
		int limit = obs_p <= 0.5 ? x : n -x;

		for (int x=0; x <= limit; ++x)
		{
			pval += binom(x, n, 0.5);
		}

		//for p=0.5, p-value for two-sided test is 2 * p-value for one sided test
		if (BasicStatistics::isValidFloat(pval))
		{
			pval = std::min(1.0, 2. * pval);
		}

		return pval;
	}

	virtual void main()
	{
		//init
		QString ref_file = getInfile("ref");
		if (ref_file=="") ref_file = Settings::string("reference_genome", true);
		if (ref_file=="") THROW(CommandLineParsingException, "Reference genome FASTA unset in both command-line and settings.ini file!");

		//factorials cache
		BasicStatistics::precalculateFactorials();

		//load DNA variants
		VariantList input;
		input.load(getInfile("in"));

		//reader for RNA BAM file
		BamReader reader(getInfile("bam"), ref_file);

		//somatic: find tumor_af column, germline: find by sample name
		bool somatic = input.type(false) == SOMATIC_PAIR || input.type(false) == SOMATIC_SINGLESAMPLE;
		QString col_name = somatic ? "tumor_af" : input.mainSampleName();
		int col_idx = input.annotationIndexByName(col_name);

		//iterate over input variants and calculate ASE probability
		FastaFileIndex reference(ref_file);
		for (int i=0; i<input.count(); ++i)
		{
			Variant& variant = input[i];
			VariantDetails tmp = reader.getVariantDetails(reference, variant, false);

			//no coverage
			if (tmp.depth==0 || !BasicStatistics::isValidFloat(tmp.frequency))
			{
				variant.annotations().append("n/a (no coverage)");
				variant.annotations().append(QByteArray::number(tmp.depth));
				variant.annotations().append("n/a (no coverage)");
				variant.annotations().append("n/a (no coverage)");
				continue;
			}

			//output string for p-value
			QByteArray pval_str;

			//germline, skip non-het variants
			if (!somatic && variant.annotations()[col_idx] != "het")
			{
				pval_str = "n/a (non-het)";
			}
			else {
				//use p=0.5 (germline het), or p=tumor_af (somatic)
				double prob = somatic ? Helper::toDouble(variant.annotations()[col_idx]) : 0.5;
				double pval = binomtest_p(tmp.obs, tmp.depth, prob);
				pval_str = QByteArray::number(pval, 'f', 4);
			}

			//append values
			variant.annotations().append(QByteArray::number(tmp.frequency, 'f', 4));
			variant.annotations().append(QByteArray::number(tmp.depth));
			variant.annotations().append(QByteArray::number(tmp.obs));
			variant.annotations().append(pval_str);

		}

		//add annotation headers
		input.annotations().append(VariantAnnotationHeader("ASE_af"));
		input.annotationDescriptions().append(VariantAnnotationDescription("ASE_af", "Expressed variant allele frequency.", VariantAnnotationDescription::FLOAT));
		input.annotations().append(VariantAnnotationHeader("ASE_depth"));
		input.annotationDescriptions().append(VariantAnnotationDescription("ASE_depth", "Sequencing depth at the variant position.", VariantAnnotationDescription::INTEGER));
		input.annotations().append(VariantAnnotationHeader("ASE_alt"));
		input.annotationDescriptions().append(VariantAnnotationDescription("ASE_alt", "Expressed variant alternative observation count.", VariantAnnotationDescription::INTEGER));
		input.annotations().append(VariantAnnotationHeader("ASE_pval"));
		input.annotationDescriptions().append(VariantAnnotationDescription("ASE_pval", "Binomial test p-value.", VariantAnnotationDescription::FLOAT));
		input.addCommentLine("##VariantAnnotateASE_BAM=" + getInfile("bam"));
		//write output
		input.store(getOutfile("out"));
	}
};

#include "main.moc"

int main(int argc, char *argv[])
{
	ConcreteTool tool(argc, argv);
	return tool.execute();
}
