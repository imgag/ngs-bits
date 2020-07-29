#include "BedFile.h"
#include "ToolBase.h"
#include "NGSHelper.h"
#include "Settings.h"
#include "VcfFileCheck.h"
#include "BedpeFile.h"
#include <QTextStream>
#include <QFileInfo>
#include <QElapsedTimer>

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
		setDescription("Filters a BEDPE file by region.");

		//optional
		addInfile("in", "Input BEDPE file. If unset, reads from STDIN.", true);
		addOutfile("out", "Output BEDPE file. If unset, writes to STDOUT.", true);
		addInfile("bed", "BED file that is used as ROI.", true);

		changeLog(2020, 6, 5, "Initial commit.");
	}

	virtual void main()
	{
		//init
		QString in = getInfile("in");
		QString bed = getInfile("bed");
		QString out = getOutfile("out");

		if (bed == "")
		{
			THROW(ArgumentException, "-bed parameter has to be provided")
		}


		//process BEDPE file
		BedpeFile bedpe_file;
		bedpe_file.load(in);

		if (bed != "")
		{
			//load roi file
			BedFile roi;
			roi.load(bed);

			// iterate through the file in reverse to don't screw up indices
			for(int i=bedpe_file.count()-1; i>=0; --i)
			{
				BedpeLine line = bedpe_file[i];
				BedFile affected_region = line.affectedRegion();
				bool remove_sv = true;

				for(int j=0; j<affected_region.count(); ++j)
				{
					if (roi.overlapsWith(affected_region[j].chr(), affected_region[j].start(), affected_region[j].end()))
					{
						remove_sv = false;
						break;
					}
				}

				if (remove_sv) bedpe_file.removeAt(i);
			}
		}

		// write output
		bedpe_file.toTSV(out);

	}

};

#include "main.moc"

int main(int argc, char *argv[])
{
	ConcreteTool tool(argc, argv);
	return tool.execute();
}
