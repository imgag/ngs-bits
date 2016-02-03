#include "ToolBase.h"
#include "Helper.h"
#include "NGSD.h"
#include "Exceptions.h"
#include <QFile>


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
		//optional
		addInfile("in", "Input TXT file with one gene symbol per line. If unset, reads from STDIN.", true, true);
		addOutfile("out", "Output TXT file with approved gene symbols. If unset, writes to STDOUT.", true);
		addFlag("test", "Uses the test database instead of on the production database.");
	}

	virtual void main()
	{
		//init
		QTextStream messages(stderr);
		NGSD db(getFlag("test"));
		SqlQuery q_gene = db.getQuery();
		q_gene.prepare("SELECT id FROM gene WHERE symbol=:1");
		SqlQuery q_prev = db.getQuery();
		q_prev.prepare("SELECT g.symbol FROM gene g, gene_alias ga WHERE g.id=ga.gene_id AND ga.symbol=:1 AND ga.type='previous'");
		SqlQuery q_syn = db.getQuery();
		q_syn.prepare("SELECT g.symbol FROM gene g, gene_alias ga WHERE g.id=ga.gene_id AND ga.symbol=:1 AND ga.type='synonym'");

		//process input
		QSharedPointer<QFile> in = Helper::openFileForReading(getInfile("in"), true);
		QSharedPointer<QFile> out = Helper::openFileForWriting(getOutfile("out"), true);
		while(!in->atEnd())
		{
			QByteArray gene = in->readLine().trimmed().toUpper();

			//skip empty/comment lines
			if (gene.isEmpty() || gene[0]=='#') continue;

			//approved
			q_gene.bindValue(0, gene);
			q_gene.exec();
			if (q_gene.size()==1)
			{
				out->write(gene + '\n');
				continue;
			}

			//previous
			q_prev.bindValue(0, gene);
			q_prev.exec();
			if (q_prev.size()==1)
			{
				q_prev.next();
				QByteArray gene2 = q_prev.value(0).toByteArray();
				out->write(gene2 + '\n');
				messages << "Notice: Gene name '" << gene + "' is a previous approved name. Replacing it by '" <<  gene2 << "'!\n";
				continue;
			}
			else if(q_prev.size()>1)
			{
				messages << "Warning: Gene name '" << gene << "' is a previous name for more than one gene.\n";
				out->write(gene + '\n');
				continue;
			}

			//synonymous
			q_syn.bindValue(0, gene);
			q_syn.exec();
			if (q_syn.size()==1)
			{
				q_syn.next();
				QByteArray gene2 = q_syn.value(0).toByteArray();
				out->write(gene2 + '\n');
				messages << "Notice: Gene name '" << gene + "' is a synonymous name. Replacing it by '" <<  gene2 << "'!\n";
				continue;
			}
			else if(q_syn.size()>1)
			{
				messages << "Warning: Gene name '" << gene << "' is a synonymous name for more than one gene.\n";
				out->write(gene + '\n');
				continue;
			}

			messages << "Warning: Gene name '" << gene << "' not found in HGNC database.\n";
			out->write(gene + '\n');
		}
	}
};

#include "main.moc"

int main(int argc, char *argv[])
{
	ConcreteTool tool(argc, argv);
	return tool.execute();
}
