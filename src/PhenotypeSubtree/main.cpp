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
		setDescription("Returns all sub-phenotype of a given phenotype.");

		addString("in", "HPO phenotype identifier, e.g. HP:0002066.", false);
		//optional
		addOutfile("out", "Output TSV file with phenotypes identifiers (column 1) and names (column 2). If unset, writes to STDOUT.", true);
		addFlag("test", "Uses the test database instead of on the production database.");

		changeLog(2020,  5, 26, "First version.");
	}


	virtual void main()
	{
		//init
		NGSD db(getFlag("test"));
		QString in = getString("in");
		QString out = getOutfile("out");

		//convert
		PhenotypeList child_terms = db.phenotypeChildTerms(db.phenotypeIdByAccession(in.toLatin1()), true);

		//write output
		QSharedPointer<QFile> out_stream = Helper::openFileForWriting(out, true);
		foreach(const Phenotype& pheno, child_terms)
		{
			out_stream->write(pheno.accession() + "\t" + pheno.name() + "\n");
		}
	}
};

#include "main.moc"

int main(int argc, char *argv[])
{
	ConcreteTool tool(argc, argv);
	return tool.execute();
}
