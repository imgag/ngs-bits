#include "BedFile.h"
#include "ToolBase.h"
#include "BedpeFile.h"

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
		setDescription("Converts a BEDPE file into BED file.");

		addInfile("in", "Input BEDPE file.", false);
		addOutfile("out", "Output BED file.", false);
		//optional
		addFlag("merge", "Merge the resulting BED file.");
		changeLog(2020, 4, 27, "Initial commit.");
	}

	virtual void main()
	{
		//init
		QString in = getInfile("in");
		QString out = getOutfile("out");
		bool merge = getFlag("merge");

		//load BEDPE file
		BedpeFile in_file;
		in_file.load(in);


		//create output BED file
		BedFile out_file;


		// convert file
		for(int i=0; i<in_file.count(); ++i)
		{
			BedpeLine bedpe_line = in_file[i];
			BedFile sv_region = bedpe_line.affectedRegion();

			for(int j=0; j<sv_region.count(); ++j)
			{
				BedLine bed_line = sv_region[j];
				bed_line.annotations() << BedpeFile::typeToString(bedpe_line.type());
				out_file.append(bed_line);
			}
		}

		// post-processing
		out_file.sort();
		if (merge) out_file.merge();

		// write result to file
		out_file.store(out);
	}

};

#include "main.moc"

int main(int argc, char *argv[])
{
	ConcreteTool tool(argc, argv);
	return tool.execute();
}
