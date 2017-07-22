#include "ToolBase.h"
#include "NGSHelper.h"
#include "BasicStatistics.h"
#include "Histogram.h"
#include <QDebug>

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
		setDescription("Checks a sample for contamination with another sample.");
		addInfile("in", "Input BAM files.", false, true);
		//optional
		addInt("min_cov",  "Minimum coverage to consider a SNP for the analysis (BAM input).",  true,  30);
		addFlag("debug", "Enable debug output.");
		changeLog(2017,  7, 22, "Initial commit.");
	}

	virtual void main()
	{
		//init
		int min_cov = getInt("min_cov");
		bool debug = getFlag("debug");

		//open BAM
		BamReader reader;
		NGSHelper::openBAM(reader, getInfile("in"));

		//calcualate frequency histogram
		Histogram hist(0, 1, 0.05);
		int passed = 0;
		double passed_depth_sum = 0.0;
		VariantList snps = NGSHelper::getSNPs();
		for(int i=0; i<snps.count(); ++i)
		{
			Pileup pileup = NGSHelper::getPileup(reader, snps[i].chr(), snps[i].start());
			int depth = pileup.depth(false);
			if (depth<min_cov) continue;

			double freq = pileup.frequency(snps[i].ref()[0], snps[i].obs()[0]);

			//skip non-informative snps
			if (!BasicStatistics::isValidFloat(freq)) continue;

			++passed;
			passed_depth_sum += depth;

			hist.inc(freq);
		}

		//output
		QTextStream stream(stdout);
		stream << passed << " of " << snps.count() << " SNPs passed quality filters\n";
		stream << "Average depth of passed SNPs: " << QString::number(passed_depth_sum/passed,'f', 2) << "\n";

		double off = 0.0;
		for (int i=1; i<=5; ++i) off += hist.binValue(i, true);
		for (int i=14; i<=18; ++i) off += hist.binValue(i, true);
		stream << "Percentage SNPs with AF 0.05-0.30 and 0.70-0.95: " << QString::number(off, 'f', 2) <<"\n";

		//debug output
		if (debug)
		{
			stream << "\nAF histogram\n";
			hist.print(stream, "", 2, 0);
		}
	}
};

#include "main.moc"

int main(int argc, char *argv[])
{
	ConcreteTool tool(argc, argv);
	return tool.execute();
}
