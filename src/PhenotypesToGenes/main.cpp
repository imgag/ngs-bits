#include "ToolBase.h"
#include "NGSD.h"

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
		setDescription("Converts a phenotype list to a list of matching genes.");
		setExtendedDescription(QStringList() << "For each given HPO term, the genes associated with the term itself and the genes associated with any sub-term are returned.");

		addString("in", "Input file, containing one HPO term identifier per line, e.g. HP:0002066. Text after the identifier is ignored. If unset, reads from STDIN.", true);
		addOutfile("out", "Output TSV file with genes (column 1) and matched phenotypes (column 2). If unset, writes to STDOUT.", true);
		addFlag("test", "Uses the test database instead of on the production database.");
		addFlag("ignore_invalid", "Ignores invalid HPO identifiers instead of throwing an error.");
		addFlag("ignore_non_phenotype", "Ignores HPO identifiers that are sub-terms of 'Mode of inheritance' or 'Frequency'");

		changeLog(2020, 11, 23, "Added parameter 'ignore_invalid'.");
		changeLog(2020,  5, 24, "First version.");
	}


	virtual void main()
	{
		//init
		NGSD db(getFlag("test"));
		QString in = getString("in");
		QString out = getOutfile("out");
		bool ignore_invalid = getFlag("ignore_invalid");
		bool ignore_non_phenotype = getFlag("ignore_non_phenotype");

		//get HPO IDs
		QStringList hpo_ids;
		QSharedPointer<QFile> in_stream = Helper::openFileForReading(in, true);
		while(!in_stream->atEnd())
		{
			QString line = in_stream->readLine().trimmed();
			if (line.isEmpty() || line.startsWith('#')) continue;
			hpo_ids << line.left(10);
		}
		hpo_ids.sort();
		hpo_ids.removeDuplicates();

		//convert to phenotypes
		QMap<QByteArray, PhenotypeList> genes2phenotypes;
		foreach(const QString& hpo_id, hpo_ids)
		{
			int id = db.phenotypeIdByAccession(hpo_id.toLatin1(), false);
			if (id==-1)
			{
				if (ignore_invalid)
				{
					continue;
				}
				else
				{
					THROW(ArgumentException, "Cannot find HPO phenotype with accession '" + hpo_id + "' in NGSD!");
				}
			}
			GeneSet genes = db.phenotypeToGenes(id, true, ignore_non_phenotype);
			foreach(const QByteArray& gene, genes)
			{
				genes2phenotypes[gene] << db.phenotype(id);
			}
		}

		//write output
		QSharedPointer<QFile> out_stream = Helper::openFileForWriting(out, true);
		for (auto it=genes2phenotypes.begin(); it!=genes2phenotypes.end(); ++it)
		{
			QByteArrayList tmp;
			foreach(const Phenotype& pheno, it.value())
			{
				tmp << pheno.accession();
			}
			out_stream->write(it.key() + "\t" + tmp.join(", ") + "\n");
		}
	}

};

#include "main.moc"

int main(int argc, char *argv[])
{
	ConcreteTool tool(argc, argv);
	return tool.execute();
}
