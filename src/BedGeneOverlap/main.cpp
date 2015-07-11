#include "BedFile.h"
#include "ToolBase.h"
#include "ChromosomalIndex.h"
#include "Exceptions.h"
#include "Helper.h"
#include "Settings.h"

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
		setDescription("Calculates how much of each overlapping gene is covered.");
		//optional
		addInfile("in", "Input BED file. If unset, reads from STDIN.", true);
		addOutfile("out", "Output TSV file. If unset, writes to STDOUT.", true);
		addInfile("db", "The database file to use. A BED file containing all exons with gene names. If unset 'ccds_merged' from the 'settings.ini' file is used.", true);
	}

	virtual void main()
	{
		//load DB file
		QString db_file = getInfile("db");
		if (db_file=="") db_file = Settings::string("ccds_merged");

		BedFile db;
		db.load(db_file);
		if (!db.isMergedAndSorted()) THROW(ArgumentException, "DB file must be merged and sorted!");

		//create DB index
		ChromosomalIndex<BedFile> db_idx(db);

		//load input file
		BedFile file;
		file.load(getInfile("in"));
		file.intersect(db);
		file.merge();

		//count bases for each gene
		QMap<QString, int> counts;
		for(int i=0; i<file.count(); ++i)
		{
			BedLine& line = file[i];

			//get gene names
			QStringList genes;
			QVector<int> matches = db_idx.matchingIndices(line.chr(), line.start(), line.end());
			foreach(int index, matches)
			{
				genes.append(db[index].annotations().at(0).split(","));
			}

			//trim, sort, make uniq
			for(int j=0; j<genes.count(); ++j)
			{
				genes[j] = genes[j].trimmed();
			}
			genes.sort();
			genes.removeDuplicates();

			//count
			foreach(QString gene, genes)
			{
				if (counts.contains(gene))
				{
					counts[gene] += line.length();
				}
				else
				{
					counts[gene] = line.length();
				}
			}
		}

		//determine size of each gene
		QMap<QString, int> sizes;
		for(int i=0; i<db.count(); ++i)
		{
			BedLine& line = db[i];
			QStringList genes = line.annotations().at(0).split(",");

			//trim, sort, make uniq
			for(int j=0; j<genes.count(); ++j)
			{
				genes[j] = genes[j].trimmed();
			}
			genes.sort();
			genes.removeDuplicates();

			foreach(QString gene, genes)
			{
				if (sizes.contains(gene))
				{
					sizes[gene] += line.length();
				}
				else
				{
					sizes[gene] = line.length();
				}
			}
		}


		//create output
		QStringList output;
		output.append("#gene\tsize\toverlap\tpercentage");
		for(QMap<QString, int>::iterator it=counts.begin(); it!=counts.end(); ++it)
		{
			QString gene = it.key();
			if (gene=="") gene = "n/a";
			int count = it.value();
			int size = sizes[it.key()];
			output.append(gene + "\t" + QString::number(size) + "\t" + QString::number(count) + "\t" + QString::number(100.0*count/size, 'f', 2));
		}
		output.sort();
		Helper::storeTextFile(getOutfile("out"), output, true);
	}
};

#include "main.moc"

int main(int argc, char *argv[])
{
	ConcreteTool tool(argc, argv);
	return tool.execute();
}
