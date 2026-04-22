#include "BedFile.h"
#include "ToolBase.h"
#include <QFileInfo>
#include "BamReader.h"
#include <QTextStream>
#include "Helper.h"
#include "Exceptions.h"

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
		setDescription("Extracts base counts and depth in the given regions from a BAM/CRAM files.");
		addInfileList("bam", "Input BAM/CRAM file(s).", false);
		//optional
		addInfile("in", "Input BED file. If unset, reads from STDIN.", true);
		addOutfile("out", "Output TSV file. If unset, writes to STDOUT.", true);
		addInfile("ref", "Reference genome for CRAM support (mandatory if CRAM is used).", true);
		addInt("min_mapq", "Minimum mapping quality.", true, 1);
		addInt("min_baseq", "Minimum base quality.", true, 25);
		addFlag("long_read", "Support long reads (> 1kb).");

		//changelog
		changeLog(2025,  3, 18, "Added long_read support.");
		changeLog(2020, 11, 27, "Added CRAM support.");
		changeLog(2020, 11, 27, "Added CRAM support.");
	}

	virtual void main()
	{
		//init
		QStringList bams = getInfileList("bam");
		int min_mapq = getInt("min_mapq");
		int min_baseq = getInt("min_baseq");
		bool long_read = getFlag("long_read");

		//open output stream
		QString out = getOutfile("out");
		QSharedPointer<QFile> outfile = Helper::openFileForWriting(out, true);
		QTextStream outstream(outfile.data());
		outstream << "#chr\tstart\tend\tsample\tA\tC\tG\tT\ttotal\n";

		//open BAM files
		QList<QSharedPointer<BamReader>> bams_open;
		const QString ref_string = getInfile("ref");
		foreach(QString bam, bams)
		{
			bams_open.append(QSharedPointer<BamReader>(new BamReader(bam, ref_string)));
		}

		//extract base counts from BAMs
		BedFile file;
		file.load(getInfile("in"));
		for(int i=0; i<file.count(); ++i)
		{
			if(file[i].length()!=1)
			{
				THROW(ToolFailedException, "BED file contains region with length > 1, which is not supported: " + file[i].toString(true));
			}

			for(int j=0; j<bams.count(); ++j)
			{
				Pileup pileup = bams_open[j]->getPileup(file[i].chr(), file[i].end(), -1, min_mapq, long_read, min_baseq);
				outstream << file[i].toString(false)+"\t"+QFileInfo(bams[j]).baseName()+"\t"+QString::number(pileup.a())+"\t"+QString::number(pileup.c())+"\t"+QString::number(pileup.g())+"\t"+QString::number(pileup.t())+"\t"+QString::number(pileup.depth(false)) + "\n";
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

