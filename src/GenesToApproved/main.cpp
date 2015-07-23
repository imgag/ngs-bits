#include "Exceptions.h"
#include "ToolBase.h"
#include "Helper.h"
#include "Log.h"
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
		setDescription("Replaces gene symbols by approved symbols using the HGNC database.");
		addInfile("in", "Input TXT file with one gene symbol per line.", false, true);
		//optional
		addOutfile("out", "Output TXT file with approved gene symbols. If unset, writes to STDOUT.", true);
		addInfile("db", "The HGNC flat file. If unset 'hgnc' from the 'settings.ini' file is used.", true, true);
	}

	virtual void main()
	{
		//init
		QSet<QString> approved;
		QMap<QString, QString> previous;
		QMap<QString, QString> synonyms;

		/*
		#HGNC ID	Approved Symbol	Approved Name	Status	Previous Symbols	Previous Names	Synonyms	Chromosome	Accession Numbers	RefSeq IDs
		HGNC:5	A1BG	alpha-1-B glycoprotein	Approved				19q		NM_130786
		HGNC:37133	A1BG-AS1	A1BG antisense RNA 1	Approved	NCRNA00181, A1BGAS, A1BG-AS	"non-protein coding RNA 181", "A1BG antisense RNA (non-protein coding)", "A1BG antisense RNA 1 (non-protein coding)"	FLJ23569	19q13.4	BC040926	NR_015380
		HGNC:24086	A1CF	APOBEC1 complementation factor	Approved			ACF, ASP, ACF64, ACF65, APOBEC1CF	10q21.1	AF271790	NM_014576
		*/
		//load HGNC DB file
		QString db_file = getInfile("db");
		if (db_file=="") db_file = Settings::string("hgnc");

		QScopedPointer<QFile> file(Helper::openFileForReading(db_file));
		QTextStream stream(file.data());
		while(!stream.atEnd())
		{
			QString line = stream.readLine().trimmed();

			//skip empty and header lines
			if(line.length()==0 || line.startsWith("#")) continue;

			//skip not approved symbols
			QStringList parts = line.split("\t");
			if (parts[3]!="Approved") continue;

			//get important infos
			QString gene = parts[1].trimmed();
			QString prev = parts[6].trimmed();
			QString syn = parts[8].trimmed();

			//popolate data structures
			approved.insert(gene.toUpper());
			if (prev!="")
			{
				parts = prev.split(",");
				for(int i=0; i<parts.count(); ++i)
				{
					QString gene2 = parts[i].trimmed().toUpper();
					if (gene2!="")
					{
						if (previous.contains(gene2))
						{
							previous.insert(gene2, previous[gene2] + "," + gene);
						}
						else
						{
							previous.insert(gene2, gene);
						}
					}
				}
			}
			if (syn!="")
			{
				parts = syn.split(",");
				for(int i=0; i<parts.count(); ++i)
				{
					QString gene2 = parts[i].trimmed().toUpper();
					if (gene2!="")
					{
						if (synonyms.contains(gene2))
						{
							synonyms.insert(gene2, synonyms[gene2] + "," + gene);
						}
						else
						{
							synonyms.insert(gene2, gene);
						}
					}
				}
			}
		}
		file->close();

		//process input
		QStringList input = Helper::loadTextFile(getInfile("in"), true, '#', true);
		QStringList output;
		QStringList message_prev;
		QStringList message_syn;
		QStringList message_unknown;
		foreach(const QString& gene, input)
		{
			//approved
			QString gene_uc = gene.toUpper();
			if (approved.contains(gene_uc))
			{
				output.append(gene);
				continue;
			}

			//previous
			QString prev = previous.value(gene_uc);
			if (prev!="")
			{
				if (prev.contains(","))
				{
					Log::warn("Gene '" + gene + "' is ambigous. It is a previous name for the following genes: " + prev );
				}
				else
				{
					output.append(prev);
					message_prev.append(gene);
				}
				continue;
			}

			//synonymous
			QString syn = synonyms.value(gene_uc);
			if (syn!="")
			{
				if (syn.contains(","))
				{
					Log::warn("Gene '" + gene + "' is ambigous. It is a synonyme for the following genes: " + syn );
				}
				else
				{
					output.append(syn);
					message_syn.append(gene);
				}
				continue;
			}

			//not found
			message_unknown.append(gene);
		}

		//store output file
		Helper::storeTextFile(getOutfile("out"), output, true);

		//console output
		if (message_prev.count()>0)
		{
			Log::info(QString::number(message_prev.count()) + " previous symbols replaced: " + message_prev.join(", "));
		}
		if (message_syn.count()>0)
		{
			Log::info(QString::number(message_syn.count()) + " synonymous symbols replaced: " + message_syn.join(", "));
		}
		if (message_unknown.count()>0)
		{
			Log::warn(QString::number(message_unknown.count()) + " symbols not found in HGNC: " + message_unknown.join(", "));
		}
	}
};

#include "main.moc"

int main(int argc, char *argv[])
{
	ConcreteTool tool(argc, argv);
	return tool.execute();
}
