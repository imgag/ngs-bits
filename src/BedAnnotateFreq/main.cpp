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
		setDescription("Annotate variant frequencies on a per nucleotide level from bam files.");
		addInfile("in", "Input BED file containing regions (SNVs only).", false, true);
		addInfileList("bam", "Input BAM file(s).", false, true);
		addOutfile("out", "Output overview file.", false, true);
		addFlag("anom", "Also consider anomalous reads.");
	}

	virtual void main()
	{
		//load regions
		BedFile file;
		file.load(getInfile("in"));

		//extract nucleotide information from pileup
		QStringList bams = getInfileList("bam");
		QStringList outlines;
		outlines.append("#chr\tstart\tend\tsample\tA\tC\tG\tT\ttotal");	//set header
		for(int i=0;i<file.count();++i)
		{
			int start = file[i].start();
			int end = file[i].end();
			if(file[i].length() != 1) // throw error if not SNV
			{
				THROW(ToolFailedException, "BED file contains indels (end - start > 1)!");
			}

			foreach(QString bam, bams)
			{
				BamReader reader;
				NGSHelper::openBAM(reader,bam);	//open bam file
				Pileup pileup = NGSHelper::getPileup(reader, file[i].chr(), end);

				QString tmp = file[i].chr().str()+"\t"+QString::number(start)+"\t"+QString::number(end)+"\t"+QFileInfo(bam).baseName()+"\t"+QString::number(pileup.a())+"\t"+QString::number(pileup.c())+"\t"+QString::number(pileup.g())+"\t"+QString::number(pileup.t())+"\t"+QString::number(pileup.depth(false));
				outlines.append(tmp);
			}
		}

		//save
		QString out = getOutfile("out");
		QScopedPointer<QFile> outfile(Helper::openFileForWriting(out));
		QTextStream stream(outfile.data());
		foreach(const QString& line, outlines)
		{
			stream << line.toLatin1()+"\n";
		}
	}
};

#include "main.moc"

int main(int argc, char *argv[])
{
	ConcreteTool tool(argc, argv);
	return tool.execute();
}

