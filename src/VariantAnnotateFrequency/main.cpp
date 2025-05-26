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
		addInfile("in", "Input variant list to annotate in GSvar format.", false, true);
		addInfile("bam", "Input BAM/CRAM file.", false, true);
		addOutfile("out", "Output variant list file in GSvar format.", false, true);
		//optional
		addFlag("depth", "Annotate an additional column containing the depth.");
		addFlag("mapq0", "Annotate an additional column containing the percentage of mapq 0 reads.");
		addString("name", "Column header prefix in output file.", true, "");
		addInfile("ref", "Reference genome FASTA file. If unset 'reference_genome' from the 'settings.ini' file is used.", true, false);
		addFlag("long_read", "Support long reads (> 1kb).");

		changeLog(2025,   5, 21, "Added long-read support.");
		changeLog(2020,  11, 27, "Added CRAM support.");
	}

	virtual void main()
	{
		//init
		bool depth = getFlag("depth");
		bool mapq0 = getFlag("mapq0");
		QString ref_file = getInfile("ref");
		if (ref_file=="") ref_file = Settings::string("reference_genome", true);
		if (ref_file=="") THROW(CommandLineParsingException, "Reference genome FASTA unset in both command-line and settings.ini file!");
		bool long_read = getFlag("long_read");

		//load input
		VariantList input;
		input.load(getInfile("in"));
		BamReader reader(getInfile("bam"), ref_file);

		//determine frequencies and depths
		FastaFileIndex reference(ref_file);
		for (int i=0; i<input.count(); ++i)
		{
			Variant& variant = input[i];
			VariantDetails tmp = reader.getVariantDetails(reference, variant, long_read);

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

