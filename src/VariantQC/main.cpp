#include "ToolBase.h"
#include "Statistics.h"
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
		setDescription("Calculates QC metrics on variant lists.");
		addInfile("in", "Input variant list in VCF format.", false, false);
		//optional
		addFlag("ignore_filter", "Ignore filter entries, i.e. consider variants that did not pass filters.");
		addOutfile("out", "Output qcML file. If unset, writes to STDOUT.", true);
		addFlag("txt", "Writes TXT format instead of qcML.");
		addFlag("longread", "Adds LongRead specific QC values (e.g. phasing information)");
		addOutfile("phasing_bed", "Output BED file containing phasing blocks with id. (requires parameter '-longread')", true);

		//changelog
		changeLog(2023,  9, 21, "Added parameter 'longread' to add longread specific QC values.");
		changeLog(2020,  8,  7, "VCF files only as input format for variant list.");
		changeLog(2018,  9, 12, "Now supports VEP CSQ annotations (no longer support SnpEff ANN annotations).");
		changeLog(2017,  1,  5, "Added 'ignore_filter' flag.");
	}

	virtual void main()
	{
		//load variant list
        VcfFile vl;
		QString filename = getInfile("in");
		vl.load(filename);

		//calculate metrics
		QCCollection metrics = Statistics::variantList(vl, !getFlag("ignore_filter"));

		//calculate phasing metrics
		if (getFlag("longread"))
		{
			BedFile phasing_blocks;
			QCCollection phasing_metrics = Statistics::phasing(vl, !getFlag("ignore_filter"), phasing_blocks);
			metrics.insert(phasing_metrics);

			//store BED file (optional)
			if (!getOutfile("phasing_bed").isEmpty()) phasing_blocks.store(getOutfile("phasing_bed"), false);
		}

		//store output
		if (getFlag("txt"))
		{
			QStringList output;
			metrics.appendToStringList(output);
			Helper::storeTextFile(Helper::openFileForWriting(getOutfile("out"), true), output);
		}
		else
		{
			metrics.storeToQCML(getOutfile("out"), QStringList() << getInfile("in"), "");
		}
	}
};

#include "main.moc"

int main(int argc, char *argv[])
{
	ConcreteTool tool(argc, argv);
	return tool.execute();
}

