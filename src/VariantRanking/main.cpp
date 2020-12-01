#include "ToolBase.h"
#include "VariantScores.h"
#include "NGSD.h"
#include "Log.h"

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
		setDescription("Annotatates variants in GSvar format with a score/rank.");
		addInfile("in", "Input variant list in GSvar format.", false);
		addString("hpo_ids", "Comma-separated list of HPO identifiers.", false);
		addOutfile("out", "Output variant list in GSvar format.", false);
		addEnum("algorithm", "Algorithm used for ranking.", true, VariantScores::algorithms() , "GSvar_v1");
		addFlag("add_explaination", "Add a third column with an explaination how that score was calculated.");
		addFlag("test", "Uses the test database instead of on the production database.");

		changeLog(2020, 11, 20, "Initial commit.");
	}

	virtual void main()
	{
		//init
		QStringList hpo_ids = getString("hpo_ids").split(",");
		QString algorithm = getEnum("algorithm");
		bool add_explaination = getFlag("add_explaination");
		NGSD db(getFlag("test"));

		//load variants
		QString in = getInfile("in");
		VariantList variants;
		variants.load(in);

		//get HPO regions from NGSD
		QHash<QByteArray, BedFile> gene2region_cache;
		QHash<Phenotype, BedFile> phenotype_rois;
		foreach(QString hpo_id, hpo_ids)
		{
			hpo_id = hpo_id.trimmed();
			if (hpo_id.isEmpty()) continue;
			
			//determine phenotype
			Phenotype pheno;
			try
			{
				pheno = db.phenotypeByAccession(hpo_id.toLatin1());
			}
			catch(Exception& e)
			{
				Log::warn(e.message());
				continue;
			}
			
			//pheno > genes > roi
			BedFile roi;
			GeneSet genes = db.phenotypeToGenes(pheno, true);
			foreach(const QByteArray& gene, genes)
			{
				if (!gene2region_cache.contains(gene))
				{
					gene2region_cache[gene] = db.geneToRegions(gene, Transcript::ENSEMBL, "gene", true);
				}
				roi.add(gene2region_cache[gene]);
			}
			roi.extend(5000);
			roi.merge();

			phenotype_rois[pheno] = roi;
		}

		//rank
		VariantScores::Result result = VariantScores::score(algorithm, variants, phenotype_rois);
		VariantScores::annotate(variants, result, add_explaination);

		//store result
		variants.store(getOutfile("out"));
	}
};

#include "main.moc"

int main(int argc, char *argv[])
{
	ConcreteTool tool(argc, argv);
	return tool.execute();
}
