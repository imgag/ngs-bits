#include "BedFile.h"
#include "ToolBase.h"
#include "NGSHelper.h"
#include "BasicStatistics.h"
#include "FastaFileIndex.h"
#include "Settings.h"
#include "Exceptions.h"
#include <cmath>

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
		setDescription("Annotates a variant list with variant frequencies from a BAM/CRAM file.");
		addInfile("in", "Input variant list to annotate in VCF or GSvar format.", false, true);
		addInfile("bam", "Input BAM/CRAM file.", false, true);
		addOutfile("out", "Output variant list file name (VCF or GSvar).", false, true);
		//optional
		addInfile("ref", "Reference genome FASTA file. If unset 'reference_genome' from the 'settings.ini' file is used.", true, false);
		addString("ref_cram", "Reference genome for CRAM support (mandatory if CRAM is used).", true);

		changeLog(2021, 06, 24, "Initial version.");
	}

	virtual void main()
	{
		//init
		QString ref_file = getInfile("ref");
		if (ref_file=="") ref_file = Settings::string("reference_genome", true);
		if (ref_file=="") THROW(CommandLineParsingException, "Reference genome FASTA unset in both command-line and settings.ini file!");

		//load input
		VariantList input;
		input.load(getInfile("in"));
		BamReader reader(getInfile("bam"), getString("ref_cram"));

		//factorials cache
		BasicStatistics::precalculateFactorials();

		//iterate over input variants and calculate ASE probability
		FastaFileIndex reference(ref_file);
		for (int i=0; i<input.count(); ++i)
		{
			Variant& variant = input[i];
			VariantDetails tmp = reader.getVariantDetails(reference, variant);

			//annotate variant
			if (tmp.depth==0 || !BasicStatistics::isValidFloat(tmp.frequency))
			{
				variant.annotations().append("n/a (no coverage)");
				variant.annotations().append(QByteArray::number(tmp.depth));
				variant.annotations().append("n/a (no coverage)");
				variant.annotations().append("n/a (no coverage)");
				continue;
			}

			int alt_obs = tmp.depth * tmp.frequency;
			int trials = tmp.depth;
			int success = alt_obs;


			if (variant.annotations()[0] != "het")
			{
				variant.annotations().append(QByteArray::number(tmp.frequency, 'f', 4));
				variant.annotations().append(QByteArray::number(tmp.depth));
				variant.annotations().append(QByteArray::number(alt_obs));
				variant.annotations().append("n/a (non-het)");
				continue;
			}

			//for high coverage (~160x), downsample alternative observation and depth
			while (!BasicStatistics::isValidFloat(BasicStatistics::factorial(trials)))
			{
				trials /= 2;
				success /= 2;
			}

			//calculate binomial test p-value, one-sided, with p=0.5
			int failure = trials - success;
			double pval = 0.0;

			int limit = success;
			if (tmp.frequency > 0.5)
			{
				limit = failure;
			}

			for (int x=0; x <= limit; ++x)
			{
				pval += std::pow(0.5, trials) * BasicStatistics::factorial(trials) / BasicStatistics::factorial(x) / BasicStatistics::factorial(trials-x);
			}

			//for p=0.5, p-value for two-sided test is 2 * p-value for one sided test
			if (BasicStatistics::isValidFloat(pval))
			{
				pval = std::min(1.0, 2. * pval);
			}

			variant.annotations().append(QByteArray::number(tmp.frequency, 'f', 4));
			variant.annotations().append(QByteArray::number(tmp.depth));
			variant.annotations().append(QByteArray::number(alt_obs));
			variant.annotations().append(QByteArray::number(pval, 'f', 4));

		}

		//output
		input.annotations().append(VariantAnnotationHeader("ASE_af"));
		input.annotationDescriptions().append(VariantAnnotationDescription("ASE_freq", "Expressed variant allele frequency.", VariantAnnotationDescription::FLOAT));
		input.annotations().append(VariantAnnotationHeader("ASE_depth"));
		input.annotationDescriptions().append(VariantAnnotationDescription("ASE_depth", "Sequencing depth at the variant position.", VariantAnnotationDescription::INTEGER));
		input.annotations().append(VariantAnnotationHeader("ASE_alt"));
		input.annotationDescriptions().append(VariantAnnotationDescription("ASE_alt", "Expressed variant alternative observation count.", VariantAnnotationDescription::INTEGER));
		input.annotations().append(VariantAnnotationHeader("ASE_pval"));
		input.annotationDescriptions().append(VariantAnnotationDescription("ASE_pval", "Binomial test p-value.", VariantAnnotationDescription::FLOAT));
		input.store(getOutfile("out"));
	}
};

#include "main.moc"

int main(int argc, char *argv[])
{
	ConcreteTool tool(argc, argv);
	return tool.execute();
}
