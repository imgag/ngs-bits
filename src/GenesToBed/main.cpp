#include "Exceptions.h"
#include "ToolBase.h"
#include "Helper.h"
#include "Log.h"
#include "BedFile.h"
#include "Settings.h"
#include <QSet>
#include <QFile>
#include <QTextStream>

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
		setDescription("Converts a text file with gene names to a BED file.");
		addInfile("in", "Input TXT file with one gene symbol per line.", false, true);
		addOutfile("out", "Output BED file.", false, true);
		QStringList modes;
        modes << "gene" << "splice" << "exon" << "ccds";
        addEnum("mode", "Mode: gene = the gene in UCSC, splice = all splice variants in UCSC, exon = all coding exons of all splice variants in UCSC, ccds = all coding exons of all splice variants in CCDS.", false, modes);
		//optional
		addInfile("db_ccds", "The CCDS flat file. If unset 'ccds_joined' from the 'settings.ini' file is used.", true, true);
		addInfile("db_ucsc", "The UCSC flat file. If unset 'kgxref_joined' from the 'settings.ini' file is used.", true, true);
	}

	virtual void main()
	{
		//load input data
		QMap<QString, QSet<QString> >  genes_uc; //gene names and chromosomes on which they were found.
		QStringList genes = Helper::loadTextFile(getInfile("in"), true, '#', true);
		foreach(const QString& gene, genes)
		{
			genes_uc.insert(gene.toUpper(), QSet<QString>());
		}

		//determine DB file to use
		QString mode = getEnum("mode");
		QString db_file;
		if (mode=="ccds")
		{
			db_file = getInfile("db_ccds");
			if (db_file=="") db_file = Settings::string("ccds_joined");
		}
		else
		{
			db_file = getInfile("db_ucsc");
			if (db_file=="") db_file = Settings::string("kgxref_joined");
		}

		/*
		====UCSC====
		#ID	chr	cds_start	cds_end	exon_starts	exon_ends	gene_name
		uc001aaa.3	chr1	11873	14409	11873,12612,13220,	12227,12721,14409,	DDX11L1
		====CCDS====
		#ID	chr	start	end	ccds_starts	ccds_ends	gene_names
		CCDS55494.1	chrX	131513693	131547537	131513693,131516205,131518690,131520688,131524874,131526170,131540255,131547510,	131513705,131516300,131518726,131520839,131525111,131526362,131540420,131547537,	MBNL3
		*/
		//load DB data and create output
		QScopedPointer<QFile> file(Helper::openFileForReading(db_file));
		QTextStream stream(file.data());
		BedFile output;
        //if gene level is wanted, we have to collect all lines of a gene and find min first/ max last base at the end
        QMap <QString, QString> gene_to_chrom;
        QMap <QString, int> gene_to_start;
        QMap <QString, int> gene_to_end;
        while(!stream.atEnd())
		{
			//skip empty and header lines
			QString line = stream.readLine();
			if(line.trimmed().length()==0 || line.startsWith("#")) continue;

			QStringList parts = line.split("\t");
			if (parts.count()<7)
			{
				Log::warn("Database '" + db_file + "' contains invalid line: " + line);
				continue;
			}
			QStringList gene_names = parts[6].split(",");
			foreach(const QString& gene, gene_names)
			{
				QString gene_uc = gene.trimmed().toUpper();

				if (!genes_uc.contains(gene_uc)) continue;

				//store the chromosome we found the gene on
				genes_uc[gene_uc].insert(parts[1]);
                if (mode=="gene")
                {
                    gene_to_chrom[gene] = parts[1];
                    if ((!(gene_to_start.contains(gene)))||(parts[2].toInt()+1<gene_to_start[gene]))
                    {
                        gene_to_start[gene]=parts[2].toInt()+1;
                    }
                    if ((!(gene_to_end.contains(gene)))||(parts[3].toInt()>gene_to_end[gene]))
                    {
                        gene_to_end[gene]=parts[3].toInt();
                    }
                }
                else if (mode=="splice")
                {
					output.append(BedLine(parts[1],  parts[2].toInt()+1, parts[3].toInt(), QStringList(gene)));
				}
				else //mode: exon/ccds
                {
					QStringList annos(gene);
					QStringList starts = parts[4].split(",", QString::SkipEmptyParts);
					QStringList ends = parts[5].split(",", QString::SkipEmptyParts);
					for (int i=0; i<starts.count(); ++i)
					{
						output.append(BedLine(parts[1], starts[i].toInt()+1, ends[i].toInt(), annos));
					}
				}
			}
		}
        if (mode=="gene")//collect resulting gene level data
        {
            foreach (QString gene, gene_to_chrom.keys())
            {
                output.append(BedLine(gene_to_chrom[gene],  gene_to_start[gene], gene_to_end[gene], QStringList(gene)));
            }
        }
		output.sort(true);
		output.store(getOutfile("out"));

		//warnings if genes were not found or found on several chromosomes
		QStringList found_several;
		QStringList found_not;
		QMap<QString, QSet<QString> >::iterator it = genes_uc.begin();
		while (it != genes_uc.end())
		{
			if (it.value().count()>1)
			{
				found_several.append(it.key());
			}
			if (it.value().count()==0)
			{
				found_not.append(it.key());
			}

			++it;
		}
		if (found_several.count()!=0)
		{
			Log::warn(QString::number(found_several.count()) + " genes found on several chromosomes: " + found_several.join(", "));
		}
		if (found_not.count()!=0)
		{
			Log::warn(QString::number(found_not.count()) + " genes not found: " + found_not.join(", "));
		}
	}
};

#include "main.moc"

int main(int argc, char *argv[])
{
	ConcreteTool tool(argc, argv);
	return tool.execute();
}
