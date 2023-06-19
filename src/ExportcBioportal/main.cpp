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
		addInfile("in", "Input TSV file with samples (tumor, normal, rna) to be exported.", false);
		addString("study_name", "Name for the study.", false);
		addString("study_ident", "Identifier for the study.", false);
		addString("study_desc", "Description for the study.", false);
		addString("cancer_type", "Cancer type of samples in the study.", false);
		addString("reference", "Cancer type of samples in the study.", false);
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
