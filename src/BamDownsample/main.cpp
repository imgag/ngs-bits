#include "ToolBase.h"
#include "api/BamReader.h"
#include "api/BamWriter.h"
#include "api/BamAlgorithms.h"
#include "NGSHelper.h"
#include "Helper.h"
#include <QTime>
#include <QString>

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
		setDescription("Downsamples a given BAM file to the given percentage of reads.");
		addInfile("in", "Input BAM file.", false, true);
		addInt("percentage", "Percentage of reads from the input to the output file.", false, true);
		addOutfile("out", "Output BAM file.", false, true);
		addFlag("test", "fixed seed for random and text output");
	}

	void write_statistics(QString filename, int reads_count, QStringList downsampled_read_names)
	{
		QStringList output;
		output << "read count:\t"<<QString::number(reads_count);
		output << "downsampled reads:\t"<<QString::number(downsampled_read_names.size());
		foreach(QString read_name,downsampled_read_names)
		{
			output << read_name;
		}

		Helper::storeTextFile(Helper::openFileForWriting(filename, true,false), output);
	}

	virtual void main()
	{
		//step 1: init
		bool test =getFlag("test");
		if (test)
		{
			qsrand(1);
		}
		else
		{
			qsrand(QTime::currentTime().msec());
		}
		QTextStream out(stderr);
		BamReader reader;
		NGSHelper::openBAM(reader, getInfile("in"));
		BamWriter writer;
		writer.Open(getOutfile("out").toStdString(), reader.GetConstSamHeader(), reader.GetReferenceData());
		int reads_count=0;
		QStringList downsampled_read_names;

		//step 2: iterate through reads, write specified percentage of reads (+mate) to output bam
		BamAlignment al;
		QHash<QString, BamAlignment> al_map;
		while (reader.GetNextAlignment(al))
		{
			if(!al.IsPrimaryAlignment()) continue;
			reads_count++;
			if((!al.IsPaired())&&(qrand()%100)<getInt("percentage"))//if single end and should be saved
			{
				writer.SaveAlignment(al);
				downsampled_read_names.append(QString::fromStdString(al.Name));
				continue;
			}

			if((al_map.contains(QString::fromStdString(al.Name))))//if paired end and mate has been seen already
			{
				if((qrand()%100)<getInt("percentage"))//if the pair should be saved
				{
					BamAlignment mate;
					mate = al_map.take(QString::fromStdString(al.Name));
					writer.SaveAlignment(al);
					writer.SaveAlignment(mate);
					downsampled_read_names.append(QString::fromStdString(al.Name));
					downsampled_read_names.append(QString::fromStdString(mate.Name));
					continue;
				}
				else//if the pair should not be saved
				{
					al_map.take(QString::fromStdString(al.Name));
					continue;
				}
			}
			else//if paired end and mate has not been seen yet
			{
				al_map.insert(QString::fromStdString(al.Name), al);
				continue;
			}
		}

		//write output text if test case
		if (test)
		{
			write_statistics("out/BamDownsample_out1.txt",reads_count,downsampled_read_names);
		}

		//done
		reader.Close();
		writer.Close();
	}

};
#include "main.moc"

int main(int argc, char *argv[])
{
	ConcreteTool tool(argc, argv);
	return tool.execute();
}
