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
		addFlag("add_explanation", "Add a third column with an explanation how that score was calculated.");
		addFlag("use_blacklist", "Use variant blacklist from settings.ini file.");
		addFlag("skip_ngsd_classifications", "Do not use variant classifications from NGSD.");
		addFlag("test", "Uses the test database instead of on the production database.");

		changeLog(2020, 11, 20, "Initial commit.");
	}

	virtual void main()
	{
		//init
		QStringList hpo_ids = getString("hpo_ids").split(",");
		QString algorithm = getEnum("algorithm");
		bool add_explanation = getFlag("add_explanation");
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
			int term_id = db.phenotypeIdByAccession(hpo_id.toUtf8(), false);
			if (term_id==-1)
			{
				Log::warn("No HPO phenotype with accession '" + hpo_id + "' found in NGSD!");
				continue;
			}
			
			//pheno > genes > roi
			BedFile roi;
			GeneSet genes = db.phenotypeToGenes(term_id, true);
            for (const QByteArray& gene : genes)
			{
				if (!gene2region_cache.contains(gene))
				{
					gene2region_cache[gene] = db.geneToRegions(gene, Transcript::ENSEMBL, "gene", true);
				}
				roi.add(gene2region_cache[gene]);
			}
			roi.extend(5000);
			roi.merge();

			phenotype_rois[db.phenotype(term_id)] = roi;
		}

		//rank
		VariantScores::Parameters parameters;
		parameters.use_blacklist = getFlag("use_blacklist");
		parameters.use_ngsd_classifications = !getFlag("skip_ngsd_classifications");
		VariantScores::Result result = VariantScores::score(algorithm, variants, phenotype_rois, parameters);
		VariantScores::annotate(variants, result, add_explanation);

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
