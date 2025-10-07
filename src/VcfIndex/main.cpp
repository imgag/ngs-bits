#include "ToolBase.h"
#include "Exceptions.h"
#include <htslib/tbx.h>

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
		setDescription("Indexes a VCF file.");
		addInfile("in", "Input VCF.GZ format. Must be compressed with bgzip.", false);
		//optional
		addEnum("type", "Index type.", true, QStringList() << "CSI" << "TBI", "CSI");
		addInt("m", "Set minimal interval size for CSI indices to 2^INT.", 9); //the tabix default of 14 ofen produces indices that are slower than TBI with annotating genomes, so we use smaller bins.
		addInt("threads", "Number of threads to use.", 4);
		addFlag("hts_version", "Prints used htlib version and exits.");

		changeLog(2025, 10,  7, "First version.");
	}

	virtual void main()
	{
		//version output
		if (getFlag("hts_version"))
		{
			QTextStream stream(stdout);
			stream << "htslib version: " << hts_version() << "\n";
			return;
		}

		//init
		QByteArray in = getInfile("in").toUtf8();
		QByteArray type = getEnum("type").toUtf8();
		int m = getInt("m");
		if (type=="TBI") m = 0;
		int threads = getInt("threads");

		// Build the index
		QByteArray in_index = in + "." + type.toLower();
		hts_set_log_level(HTS_LOG_DEBUG);
		int ret = tbx_index_build3(in.data(), in_index.data(), m, threads, &tbx_conf_vcf);
		if (ret<0) THROW(Exception, "Could not create the index");
	}
};

#include "main.moc"

int main(int argc, char *argv[])
{
	ConcreteTool tool(argc, argv);
	return tool.execute();
}
