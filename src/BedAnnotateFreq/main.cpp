#include "BedFile.h"
#include "ToolBase.h"
#include "Statistics.h"
#include <QFileInfo>

#include "NGSHelper.h"
#include "api/BamReader.h"
#include <QTextStream>
#include "Helper.h"
#include "Exceptions.h"
using namespace BamTools;

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
		setDescription("Extracts base frequencies for given regions from BAMs files.");
		addInfileList("bam", "Input BAM file(s).", false);
		//optional
		addInfile("in", "Input BED file. If unset, reads from STDIN.", true);
		addOutfile("out", "Output TSV file. If unset, writes to STDOUT.", true);

	}

	virtual void main()
	{
		//init
		QStringList bams = getInfileList("bam");

		//open output stream
		QString out = getOutfile("out");
		QScopedPointer<QFile> outfile(Helper::openFileForWriting(out, true));
		QTextStream outstream(outfile.data());
		outstream << "#chr\tstart\tend\tsample\tA\tC\tG\tT\ttotal\n";

		//extract base counts form BAMs
		BedFile file;
		file.load(getInfile("in"));
		for(int i=0; i<file.count(); ++i)
		{
			if(file[i].length()!=1)
			{
				THROW(ToolFailedException, "BED file contains region with length > 1, which is not supported: " + file[i].toString());
			}

			foreach(QString bam, bams)
			{
				BamReader reader;
				NGSHelper::openBAM(reader, bam);
				Pileup pileup = NGSHelper::getPileup(reader, file[i].chr(), file[i].end());

				outstream << file[i].toString('\t')+"\t"+QFileInfo(bam).baseName()+"\t"+QString::number(pileup.a())+"\t"+QString::number(pileup.c())+"\t"+QString::number(pileup.g())+"\t"+QString::number(pileup.t())+"\t"+QString::number(pileup.depth(false)) + "\n";
			}
		}
	}
};

#include "main.moc"

int main(int argc, char *argv[])
{
	ConcreteTool tool(argc, argv);
	return tool.execute();
}

