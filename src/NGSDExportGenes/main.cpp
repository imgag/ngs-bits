#include "Exceptions.h"
#include "ToolBase.h"
#include "NGSD.h"
#include "Helper.h"
#include "Exceptions.h"
#include "Settings.h"
#include <QSqlQuery>
#include <QSqlRecord>
#include <QDir>

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
		setDescription("Lists genes from NGSD.");
		addOutfile("out", "Output TSV file. If unset, writes to STDOUT.", true);
		//optional
		addFlag("test", "Uses the test database instead of on the production database.");

		changeLog(2018,  5,  3, "First version");
		changeLog(2019,  9,  20, "Added several columns with gene details.");
	}

	virtual void main()
	{
		//init
		NGSD db(getFlag("test"));

		//write header
		QSharedPointer<QFile> output = Helper::openFileForWriting(getOutfile("out"), true);
		output->write("#symbol\t");
		output->write("HGNC id\t");
		output->write("type\t");
		output->write("name\t");
		output->write("transcripts coding (ensembl)\t");
		output->write("transcripts non-coding (ensembl)\t");
		output->write("genomAD oe (syn)\t");
		output->write("genomAD oe (mis)\t");
		output->write("genomAD oe (lof)\t");
		output->write("inheritance\t");
		output->write("HPO terms\n");

		//write content
		SqlQuery query = db.getQuery();
		query.exec("SELECT g.symbol, g.hgnc_id, g.type, g.name, (SELECT COUNT(*) FROM gene_transcript gt WHERE gene_id=g.id AND gt.source='ensembl' AND gt.start_coding IS NOT NULL) as trans, "
				   "(SELECT COUNT(*) FROM gene_transcript gt WHERE gene_id=g.id AND gt.source='ensembl' AND gt.start_coding IS NULL) as trans_nc FROM gene g ORDER BY g.symbol ASC");
		while (query.next())
		{
			QByteArray gene_symbol = query.value("symbol").toByteArray();
			output->write(gene_symbol + "\t");
			output->write("HGNC:" + query.value("hgnc_id").toByteArray() + "\t");
			output->write(query.value("type").toByteArray() + "\t");
			output->write(query.value("name").toByteArray() + "\t");
			output->write(query.value("trans").toByteArray() + "\t");
			output->write(query.value("trans_nc").toByteArray() + "\t");
			//gene info
			GeneInfo gene_info = db.geneInfo(gene_symbol);
			output->write(gene_info.oe_syn.replace("n/a", "").toLatin1() + "\t");
			output->write(gene_info.oe_mis.replace("n/a", "").toLatin1() + "\t");
			output->write(gene_info.oe_lof.replace("n/a", "").toLatin1() + "\t");
			output->write(gene_info.inheritance.replace("n/a", "").toLatin1() + "\t");
			//HPO terms
			QByteArrayList hpos;
			QList<Phenotype> phenos = db.phenotypes(gene_symbol);
			foreach(const Phenotype& pheno, phenos)
			{
				hpos << pheno.toString();
			}
			output->write(hpos.join(",") + "\n");
		}
	}
};

#include "main.moc"

int main(int argc, char *argv[])
{
	ConcreteTool tool(argc, argv);
	return tool.execute();
}
