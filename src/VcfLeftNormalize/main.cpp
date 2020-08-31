#include "ToolBase.h"
#include "Helper.h"
#include "Exceptions.h"
#include "VcfFile.h"
#include "Settings.h"
#include <QFile>
#include <QTextStream>
#include <QList>

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
		setDescription("Normalizes all variants and shifts indels to the left in a VCF file. Multi-allelic and complex variant are not changed!");
		//optional
		addInfile("in", "Input VCF file. If unset, reads from STDIN.", true, true);
		addOutfile("out", "Output VCF or VCF or VCF.GZ file. If unset, writes to STDOUT.", true, true);
		addInfile("ref", "Reference genome FASTA file. If unset 'reference_genome' from the 'settings.ini' file is used.", true, false);
		addInt("compression_level", "Output VCF compression level from 1 (fastest) to 9 (best compression). If unset, an unzipped VCF is written.", true, Z_NO_COMPRESSION);

		changeLog(2020, 8, 12, "Added parameter '-compression_level' for compression level of output vcf files.");
		changeLog(2016, 06, 24, "Initial implementation.");
	}

	virtual void main()
	{
		//open refererence genome file
		QString ref_file = getInfile("ref");
		if (ref_file=="") ref_file = Settings::string("reference_genome", true);
		if (ref_file=="") THROW(CommandLineParsingException, "Reference genome FASTA unset in both command-line and settings.ini file!");

		//open input/output streams
		QString in = getInfile("in");
		QString out = getOutfile("out");
		if(in!="" && in==out)
		{
			THROW(ArgumentException, "Input and output files must be different when streaming!");
		}

		VcfFile vcf_file;
		vcf_file.load(in);
		vcf_file.leftNormalize(ref_file);

		int compression_level = getInt("compression_level");
		vcf_file.store(out, true, compression_level);
    }
};

#include "main.moc"

int main(int argc, char *argv[])
{
	ConcreteTool tool(argc, argv);
	return tool.execute();
}

