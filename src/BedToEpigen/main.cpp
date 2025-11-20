#include "ToolBase.h"
#include "Helper.h"
#include "Chromosome.h"
#include <QMap>
#include <QDebug>
#include <VersatileFile.h>

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
		setDescription("Converts a modkit BED file to a Epigen TSV file.");
		addInfile("id_file", "Input CSV file containing Illumina CpG IDs.", false, true);
		addString("sample", "Sample name used in output file header.", false);
		//optional
		addInfile("in", "Input modkit (bgzipped) BED file. If unset, read from STDIN.", true);
		addOutfile("out", "Output FASTA file. If unset, writes to STDOUT.", true);
	}

	virtual void main()
	{
		//load CpG IDs
		QMap<QByteArray,QByteArray> cpg_ids;
		QString id_file = getInfile("id_file");
		QSharedPointer<QFile> id_file_p = Helper::openFileForReading(id_file);
		bool header_parsed = false;
		QMap<QByteArray,int> header_items;
		while(!id_file_p->atEnd())
		{
			QByteArray line = id_file_p->readLine();

			//skip empty lines
			if (line.trimmed().isEmpty()) continue;

			if (!header_parsed)
			{
				//header
				if (line.startsWith("IlmnID"))
				{
					QByteArrayList header_line = line.split(',');
					for (int i = 0; i < header_line.size(); ++i)
					{
						header_items.insert(header_line.at(i), i);
					}

					header_parsed = true;
				}
				continue;
			}
			else
			{
				//data line
				QByteArrayList parts = line.split(',');

				//parse line
				QByteArray id = parts.at(header_items.value("Name"));
				QByteArray strand = parts.at(header_items.value("Strand_FR"));
				Chromosome chr(parts.at(header_items.value("CHR")));
				if (!chr.isValid()) continue; //ignore invalid chromosomes
				QByteArray chr_str = chr.strNormalized(true);
				QByteArray pos = parts.at(header_items.value("MAPINFO"));
				QByteArray species = parts.at(header_items.value("Species"));
				QByteArray build = parts.at(header_items.value("Genome_Build"));

				//ignore non-human/non-GRCh38
				if (species != "Human")
				{
					qDebug() << "Skipping id " + id + " (non-human)!";
					continue;
				}
				if (build != "GRCh38")
				{
					qDebug() << "Skipping id " + id + " (not GRCh38)!";
					continue;
				}

				// fix mapping info for reverse CpG
				if (strand == "R") pos = QByteArray::number(pos.toInt() + 1);

				//store pos->id mapping
				QByteArray key = chr_str + ":" + pos + "_" + strand;
				cpg_ids.insert(key, id);
			}
		}

		//debug:
//		QSharedPointer<QFile> debug_file = Helper::openFileForWriting(getOutfile("out")+"_debug.txt", false);
//		QTextStream out_debug(debug_file.data());
//		for (auto it = cpg_ids.begin(); it != cpg_ids.end(); ++it)
//		{
//			out_debug<< it.key() << "\t" << it.value() << "\n";
//		}


		//read input and write converted CpGs to output
		QSharedPointer<QFile> outfile = Helper::openFileForWriting(getOutfile("out"), true);
		QTextStream out(outfile.data());
		//write header
		out << "ID_REF\t" << getString("sample") << "\n";

		//QMap<QByteArray, float> methylation;
		VersatileFile infile(getInfile("in"));
		infile.open();
		while(!infile.atEnd())
		{
			QByteArray line = infile.readLine(true);

			//skip empty lines
			if (line.isEmpty()) continue;

			//skip comment lines
			if (line.startsWith('#')) continue;

			//parse line
			QByteArrayList parts = line.split('\t');
			QByteArray chr = parts.at(0);
			QByteArray pos = parts.at(2);
			QByteArray type = parts.at(3);
			if (type != "m") continue; // ignore non-5mC methylation
			QByteArray strand = parts.at(5);
			if (strand == "+") strand = "F";
			else strand = "R";
			float methylation_frac = Helper::toDouble(parts.at(10), "Methylation fraction", chr + ":" + pos + "_" + strand) / 100.0;

			QByteArray cpg_position = chr + ":" + pos + "_" + strand;
			QByteArray cpg_id = cpg_ids.value(cpg_position, "");

			if (cpg_id == "") continue; //no matching illumina ID found -> ignoring CpG
			out << cpg_id << "\t" << QByteArray::number(methylation_frac, 'f', 5) << "\n";
		}
	}
};

#include "main.moc"

int main(int argc, char *argv[])
{
	ConcreteTool tool(argc, argv);
	return tool.execute();
}

