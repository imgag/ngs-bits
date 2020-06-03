#include "BedFile.h"
#include "ToolBase.h"
#include "FastaFileIndex.h"
#include "Helper.h"
#include "Settings.h"
#include <QTextStream>

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
		setDescription("Converts a BED file to a FASTA file.");
		addInfile("in", "Input BED file.", false, true);
		//optional
		addOutfile("out", "Output FASTA file. If unset, writes to STDOUT.", true);
		addInfile("ref", "Reference genome FASTA file. If unset 'reference_genome' from the 'settings.ini' file is used.", true, false);
	}

	virtual void main()
	{
		//open refererence genome file
		QString ref_file = getInfile("ref");
		if (ref_file=="") ref_file = Settings::string("reference_genome", true);
		if (ref_file=="") THROW(CommandLineParsingException, "Reference genome FASTA unset in both command-line and settings.ini file!");
		FastaFileIndex reference(ref_file);

		//load input
		BedFile file;
		file.load(getInfile("in"));

		//store output
		QSharedPointer<QFile> outfile = Helper::openFileForWriting(getOutfile("out"), true);
		QTextStream out(outfile.data());
		for (int i=0;i<file.count();++i)
		{
			const BedLine& line = file[i];
			out << ">" << line.chr().str() << ":" << line.start() << "-" << line.end() << "\n";
			out << reference.seq(line.chr(), line.start(), line.length(), false) << "\n";
		}
	}
};

#include "main.moc"

int main(int argc, char *argv[])
{
	ConcreteTool tool(argc, argv);
	return tool.execute();
}

