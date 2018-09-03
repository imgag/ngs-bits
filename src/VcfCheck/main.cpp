#include "ToolBase.h"
#include "Settings.h"
#include "FastaFileIndex.h"
#include "Exceptions.h"
#include "Helper.h"

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
		setDescription("Checks a VCF file for error!");
		//optional
		addInfile("in", "Input VCF file. If unset, reads from STDIN.", true, true);
		addOutfile("out", "Output file. If unset, writes to STDOUT.", true, true);
		addInt("lines", "Number of lines to check in the VCF file (unlimited if 0)", true, 1000);
		addInfile("ref", "Reference genome FASTA file. If unset 'reference_genome' from the 'settings.ini' file is used.", true, false);
		changeLog(2018, 9, 3, "Initial implementation.");
	}

	void printInfo(QSharedPointer<QFile> out, QByteArray message)
	{
		out->write(message.trimmed() + "\n");
	}

	void printWarning(QSharedPointer<QFile> out, QByteArray message, int l, QByteArray line)
	{
		out->write("WARNING: " + message.trimmed() + " - in line " + QByteArray::number(l+1) + ":\n  " + line.trimmed() + "\n");
	}

	void printError(QSharedPointer<QFile> out, QByteArray message, int l, QByteArray line)
	{
		out->write("ERROR: " + message.trimmed() + " - in line " + QByteArray::number(l+1) + ":\n  " + line.trimmed() + "\n");
		exit(1);
	}

	virtual void main()
	{
		//init
		QString in = getInfile("in");
		QSharedPointer<QFile> in_p = Helper::openFileForReading(in, true);
		QString out = getOutfile("out");
		QSharedPointer<QFile> out_p = Helper::openFileForWriting(out, true);
		int lines = getInt("lines");
		if (lines<=0) lines = std::numeric_limits<int>::max();

		//open refererence genome file (to check reference bases)
		QString ref_file = getInfile("ref");
		if (ref_file=="") ref_file = Settings::string("reference_genome");
		if (ref_file=="") THROW(CommandLineParsingException, "Reference genome FASTA unset in both command-line and settings.ini file!");
		FastaFileIndex reference(ref_file);

		//perform checks
		int l = 0;
		while(!in_p->atEnd() && l<lines)
		{
			QByteArray line = in_p->readLine();

			//skip empty lines
			if (line.trimmed().isEmpty()) continue;

			//check: format line
			if (l==0 && !line.startsWith("##fileformat=VCFv4."))
			{
				printError(out_p, "First line must be 'fileformat' line!", l, line);
			}

			if (line.startsWith("##")) //meta data
			{
				//TODO check INFO header/duplicates/used

				//TODO check FORMAT header/duplicates/used

				//TODO check FILTER header/duplicates/used

			}
			else if (line.startsWith("#")) //header
			{
				QByteArrayList parts = line.split('\t');

				//check no more header lines after this one

			}
			else //content
			{
				QByteArrayList parts = line.split('\t');

				//check: reference base

				//check number of samples is ok

			}

			++l;
		}

		printInfo(out_p, "Finished - checked " + QByteArray::number(l+1) + " lines.");
    }
};

#include "main.moc"

int main(int argc, char *argv[])
{
	ConcreteTool tool(argc, argv);
	return tool.execute();
}

