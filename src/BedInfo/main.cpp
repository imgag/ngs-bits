#include "BedFile.h"
#include "ToolBase.h"
#include "Statistics.h"
#include "Exceptions.h"
#include "Helper.h"
#include <QTextStream>
#include <QFile>
#include <QFileInfo>

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
		setDescription("Prints information about a (merged) BED file.");
		//optional
		addInfile("in", "Input BED file. If unset, reads from STDIN.", true);
		addOutfile("out", "Output file. If unset, writes to STDOUT.", true);
		addFlag("nomerge", "If set, the input is not merged before printing statistics.");
		addFlag("filename", "If set, prints the input file name before each line.");
		addInfile("fai", "If set, checks that the maximum position for each chromosome is not exceeded.", true);
	}

	virtual void main()
	{
		QString in = getInfile("in");
		BedFile file;
		file.load(in);

		QCCollection stats = Statistics::region(file, !getFlag("nomerge"));

		QString filename = "";
		if (getFlag("filename"))
		{
			filename = QFileInfo(in).fileName() + ": ";
		}

		//output
		QSharedPointer<QFile> outfile = Helper::openFileForWriting(getOutfile("out"), true);
		QTextStream out(outfile.data());
        out << filename << "Regions    : " << stats.value("roi_fragments").toString() << Qt::endl;
        out << filename << "Bases      : " << stats.value("roi_bases").toString(0) << Qt::endl;
        out << filename << "Chromosomes: " << stats.value("roi_chromosomes").toString() << Qt::endl;
        out << filename << Qt::endl;
        out << filename << "Is sorted  : " << stats.value("roi_is_sorted").toString() << Qt::endl;
        out << filename << "Is merged  : " << stats.value("roi_is_merged").toString() << Qt::endl;
        out << filename << Qt::endl;
        out << filename << "Fragment size (min)  : " << stats.value("roi_fragment_min").toString() << Qt::endl;
        out << filename << "Fragment size (max)  : " << stats.value("roi_fragment_max").toString() << Qt::endl;
        out << filename << "Fragment size (mean) : " << stats.value("roi_fragment_mean").toString() << Qt::endl;
        out << filename << "Fragment size (stdev): " << stats.value("roi_fragment_stdev").toString() << Qt::endl;

		//optional: check position bounds
		if (getInfile("fai")!="")
		{
            out << filename << Qt::endl;

			//load maxima
			QMap<int, int> max;
			QStringList fai = Helper::loadTextFile(getInfile("fai"), true, '~', true);
			foreach(QString line, fai)
			{
				QStringList parts = line.split("\t");
				if (parts.count()<2) continue;
				bool ok = false;
				int value = parts[1].toInt(&ok);
				if (!ok) continue;
				max[Chromosome(parts[0]).num()] = value;
			}

			//check maxima
			for (int i=0; i<file.count(); ++i)
			{
				BedLine& line = file[i];
				if (!max.contains(line.chr().num()))
				{
					THROW(ArgumentException, "Chromsome '" + line.chr().str() + "' not contained in FASTA index file '" + getInfile("fai") + "'!");
				}
				if (line.end()>max[line.chr().num()])
				{
                    out << filename << "Warning: maximum position " << max[line.chr().num()] << " exceeded for region " << line.chr().str() << ":" << line.start() << "-" << line.end() << Qt::endl;
				}
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
