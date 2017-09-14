#include "BedFile.h"
#include "ToolBase.h"
#include "NGSHelper.h"
#include "BasicStatistics.h"
#include "api/BamReader.h"
#include "FastaFileIndex.h"
#include "Settings.h"
#include <cmath>

using namespace BamTools;

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
		setDescription("Annotates a variant list with variant frequencies from a BAM file.");
		addInfile("in", "Input variant list to annotate in TSV format.", false, true);
		addInfile("bam", "BAM file of second sample.", false, true);
		addOutfile("out", "Output TSV file.", false, true);
		//optional
		addFlag("depth", "Annotate an additional column containing the depth.");
		addFlag("mapq0", "Annotate an additional column containing the percentage of mapq 0 reads.");
		addString("name", "Column header prefix in output file.", true, "");
		addInfile("ref", "Reference genome FASTA file. If unset 'reference_genome' from the 'settings.ini' file is used.", true, false);
	}

	virtual void main()
	{
		//init
		bool depth = getFlag("depth");
		bool mapq0 = getFlag("mapq0");
		QString ref_file = getInfile("ref");
		if (ref_file=="") ref_file = Settings::string("reference_genome");

		//load input
		VariantList input;
		input.load(getInfile("in"));
		BamReader reader;
		NGSHelper::openBAM(reader, getInfile("bam"));

		//determine frequencies and depths
		FastaFileIndex reference(ref_file);
		for (int i=0; i<input.count(); ++i)
		{
			Variant& variant = input[i];
			VariantDetails tmp = NGSHelper::getVariantDetails(reader, reference, variant);

			//annotate variant
			if (tmp.depth==0 || !BasicStatistics::isValidFloat(tmp.frequency))
			{
				variant.annotations().append("n/a");
			}
			else
			{
				variant.annotations().append(QByteArray::number(tmp.frequency, 'f', 4));
			}
			if (depth)
			{
				variant.annotations().append(QByteArray::number(tmp.depth));
			}
			if (mapq0)
			{
				variant.annotations().append(QByteArray::number(tmp.mapq0_frac, 'f', 2));
			}
		}

		//store
		QString prefix = "";
		QString name = getString("name");
		if (name!="") prefix = name + "_";
		input.annotations().append(VariantAnnotationHeader(prefix + "freq"));
		input.annotationDescriptions().append(VariantAnnotationDescription(prefix + "freq", "Variant frequency.", VariantAnnotationDescription::FLOAT));
		if (depth)
		{
			input.annotations().append(VariantAnnotationHeader(prefix + "depth"));
			input.annotationDescriptions().append(VariantAnnotationDescription(prefix + "depth", "Sequencing depth at the variant position.",VariantAnnotationDescription::INTEGER));
		}
		if (mapq0)
		{
			input.annotations().append(VariantAnnotationHeader(prefix + "mapq0_frac"));
			input.annotationDescriptions().append(VariantAnnotationDescription(prefix + "mapq0_frac", "Fraction of reads with mapping quality 0 at the variant position.",VariantAnnotationDescription::INTEGER));
		}
		input.store(getOutfile("out"));
	}
};

#include "main.moc"

int main(int argc, char *argv[])
{
	ConcreteTool tool(argc, argv);
	return tool.execute();
}

