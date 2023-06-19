#include "ToolBase.h"
#include "FastqFileStream.h"
#include "Helper.h"
#include <QFile>

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
		setDescription("Converts a FASTQ file to FASTA format.");
		addInfile("samples", "Input TSV file with samples (tumor, normal, rna) to be exported and their clinical data.", false);
		addInfile("study", "Input TSV file with Infos about the stuy that should be created.", false);
		addString("out", "Output folder that will contain all files for the cBioPortal study.", false);
	}

	virtual void main()
	{
		//TODO

		//gather necessary files

		//get variant lists

		//compare to Mainwindow export helper
	}
};



#include "main.moc"

int main(int argc, char *argv[])
{
	ConcreteTool tool(argc, argv);
	return tool.execute();
}
