#include "BedFile.h"
#include "ToolBase.h"
#include "NGSHelper.h"
#include "Settings.h"
#include "NGSD.h"
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
		setDescription("Annotates BED file regions with gene names.");
		addInfile("in", "Input BED file. If unset, reads from STDIN.", true);
		addOutfile("out", "Output BED file. If unset, writes to STDOUT.", true);
		addInt("extend", "The number of bases to extend the gene regions before annotation.", true, 0);
		addFlag("test", "Uses the test database instead of on the production database.");
	}

	virtual void main()
	{
		//init
		int extend = getInt("extend");
		NGSD db(getFlag("test"));
		SqlQuery query = db.getQuery();
		query.prepare("SELECT DISTINCT g.symbol FROM gene g, gene_transcript gt WHERE g.id=gt.gene_id AND g.chromosome=:1 AND gt.start_coding IS NOT NULL AND ((gt.start_coding>=:2 AND gt.start_coding<=:3) OR (:4>=gt.start_coding AND :5<=gt.end_coding)) ORDER BY g.symbol");

		//load input
		BedFile file;
		file.load(getInfile("in"));

		//annotate
		for(int i=0; i<file.count(); ++i)
		{
			BedLine& line = file[i];

			QByteArray chr = line.chr().str();
			query.bindValue(0, chr.replace("chr", ""));
			query.bindValue(1, line.start() - extend);
			query.bindValue(2, line.end() + extend);
			query.bindValue(3, line.start() - extend);
			query.bindValue(4, line.start() - extend);
			query.exec();

			QStringList genes;
			while(query.next())
			{
				genes.append(query.value(0).toString());
			}

			if (line.annotations().empty()) line.annotations().append("");
			line.annotations()[0] = genes.join(", ");
		}

		file.store(getOutfile("out"));
	}
};

#include "main.moc"

int main(int argc, char *argv[])
{
	ConcreteTool tool(argc, argv);
	return tool.execute();
}
