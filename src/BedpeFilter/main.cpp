#include "BedFile.h"
#include "ToolBase.h"
#include "BedpeFile.h"
#include "ChromosomalIndex.h"

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
			roi.sort();
			ChromosomalIndex<BedFile> roi_idx(roi);

			// iterate through the file in reverse to don't screw up indices
			for(int i=bedpe_file.count()-1; i>=0; --i)
			{
				BedpeLine line = bedpe_file[i];

				//check if should be removed
				bool remove_sv = true;
				BedFile affected_region = line.affectedRegion();
				for(int j=0; j<affected_region.count(); ++j)
				{
					if (roi_idx.matchingIndices(affected_region[j].chr(), affected_region[j].start(), affected_region[j].end()).count()>0)
					{
						remove_sv = false;
					}
				}

				if (remove_sv) bedpe_file.removeAt(i);
			}
		}

		// write output
		bedpe_file.store(out);

	}

};

#include "main.moc"

int main(int argc, char *argv[])
{
	ConcreteTool tool(argc, argv);
	return tool.execute();
}
