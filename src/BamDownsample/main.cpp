#include "ToolBase.h"
#include "NGSHelper.h"
#include "Helper.h"
#include "BasicStatistics.h"
#include "BamWriter.h"
#include <QTime>

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
		setDescription("Downsamples a BAM file to the given percentage of reads.");
		addInfile("in", "Input BAM file.", false, true);
		addFloat("percentage", "Percentage of reads to keep.", false);
		addOutfile("out", "Output BAM file.", false, true);
		//optional
		addFlag("test", "Test mode: fix random number generator seed and write kept read names to STDOUT.");
	}

	virtual void main()
	{
		//init
		QTextStream out(stdout);
		bool test = getFlag("test");
		srand(test ? 1 : QTime::currentTime().msec());
		double percentage = getFloat("percentage");
		if (percentage<=0 || percentage>=100) THROW(CommandLineParsingException, "Invalid percentage " + QString::number(percentage) +"!");

		BamReader reader(getInfile("in"));

		BamWriter writer(getOutfile("out"));
		writer.writeHeader(reader);

		//process alignments
		int c_se = 0;
		int c_se_pass = 0;
		int c_pe = 0;
		int c_pe_pass = 0;

		BamAlignment al;
		QHash<QByteArray, BamAlignment> al_cache;
		while (reader.getNextAlignment(al))
		{
			//skip secondary and supplementary alignments
			if(al.isSecondaryAlignment() || al.isSupplementaryAlignment()) continue;

			if(!al.isPaired()) //single-end reads
			{
				++c_se;
				if (Helper::randomNumber(0, 100)<percentage)
				{
					++c_se_pass;
					writer.writeAlignment(al);
					if (test) out << "KEPT SE: " << al.name() << endl;
				}
			}
			else //paired-end reads
			{
				QByteArray name = al.name();
				if (!al_cache.contains(name)) //mate not seen yet => cache
				{
					al_cache.insert(name, al);
				}
				else //mate seen => decide if pair is written
				{
					++c_pe;
					if (Helper::randomNumber(0, 100)<percentage)
					{
						++c_pe_pass;
						writer.writeAlignment(al_cache.take(name));
						writer.writeAlignment(al);
						if (test) out << "KEPT PE: " << name << endl;
					}
					else
					{
						al_cache.remove(name);
					}
				}
			}
		}

		//write debug output
		out << "SE reads                    : " << c_se << endl;
		out << "SE reads (written)          : " << c_se_pass << endl;
		out << "PE reads                    : " << c_pe << endl;
		out << "PE reads (written)          : " << c_pe_pass << endl;
		out << "PE reads unmatched (skipped): " << al_cache.size() << endl;
	}
};

#include "main.moc"

int main(int argc, char *argv[])
{
	ConcreteTool tool(argc, argv);
	return tool.execute();
}
