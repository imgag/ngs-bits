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
		setDescription("Annotates GC content fraction to regions in a BED file.");
		//optional
		addInfile("in", "Input BED file. If unset, reads from STDIN.", true);
		addOutfile("out", "Output BED file. If unset, writes to STDOUT.", true);
		addInfile("ref", "Reference genome FASTA file. If unset, 'reference_genome' from the 'settings.ini' file is used.", true, false);
		addInt("extend", "Bases to extend around the input region for calculating the GC content.", true, 0);
		addFlag("clear", "Clear all annotations present in the input file.");
	}

	virtual void main()
	{
		//init
		int extend = getInt("extend");
		bool clear = getFlag("clear");

		//open refererence genome file
		QString ref_file = getInfile("ref");
		if (ref_file=="") ref_file = Settings::string("reference_genome");
		FastaFileIndex reference(ref_file);

		//load input
		BedFile file;
		file.load(getInfile("in"));
		if (clear)
		{
			file.clearAnnotations();
		}

		//annotate
		for (int i=0; i<file.count(); ++i)
		{
			BedLine& r = file[i];
			Sequence seq = reference.seq(r.chr(), r.start()-extend, r.length()+2*extend, true);
			int gc = 0;
			int at = 0;
			for(int j=0; j<seq.length(); ++j)
			{
				if (seq[j]=='G' || seq[j]=='C') ++gc;
				else if (seq[j]=='A' || seq[j]=='T') ++at;
			}
			if (gc+at==0)
			{
				r.annotations().append("n/a");
			}
			else
			{
				r.annotations().append(QByteArray::number((double)gc/(gc+at), 'f', 4));
			}
		}

		//store output
		file.store(getOutfile("out"));
	}
};

#include "main.moc"

int main(int argc, char *argv[])
{
	ConcreteTool tool(argc, argv);
	return tool.execute();
}

