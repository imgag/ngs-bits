#include "ToolBase.h"
#include "Settings.h"
#include "Exceptions.h"
#include "Helper.h"
#include "VcfFile.h"

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
		setDescription("Checks a VCF file for errors.");
		setExtendedDescription(QStringList() << "Checks the input VCF file with SNVs and small InDels for errors and warnings." << "If the VEP-based CSQ annotation is present, it also checks that the Miso terms in the consequence field are valid.");
		//optional
		addInfile("in", "Input VCF file. If unset, reads from STDIN.", true, true);
		addOutfile("out", "Output file. If unset, writes to STDOUT.", true, true);
		addInt("lines", "Number of variant lines to check in the VCF file (unlimited if 0)", true, 5000);
		addInfile("ref", "Reference genome FASTA file. If unset 'reference_genome' from the 'settings.ini' file is used.", true, false);
		addFlag("info", "Add general information about the input file to the output.");
		addFlag("duplicates", "Check for variants occuring twice (input VCF needs to be sorted).");

		changeLog(2025,  5, 30, "Added parameter 'duplicates'.");
		changeLog(2019, 12, 13, "Added support for gzipped VCF files.");
		changeLog(2019, 12, 11, "Added check for invalid characters in INFO column.");
		changeLog(2018, 12, 3, "Initial implementation.");
	}

	virtual void main()
	{
		//init
		QString in = getInfile("in");

		QString out = getOutfile("out");
		QSharedPointer<QFile> out_p = Helper::openFileForWriting(out, true);
		QTextStream out_stream(out_p.data());

		bool info = getFlag("info");
		bool duplicates = getFlag("duplicates");

		int lines = getInt("lines");
		if (lines<=0) lines = std::numeric_limits<int>::max();

		QString ref_file = getInfile("ref");
		if (ref_file=="") ref_file = Settings::string("reference_genome", true);
		if (ref_file=="") THROW(CommandLineParsingException, "Reference genome FASTA unset in both command-line and settings.ini file!");

		//check
		if (!VcfFile::isValid(in, ref_file, out_stream, info, lines, duplicates))
		{
			THROW(ToolFailedException, "VCF check failed - see 'out' file for details!");
		}
    }
};

#include "main.moc"

int main(int argc, char *argv[])
{
	ConcreteTool tool(argc, argv);
	return tool.execute();
}

