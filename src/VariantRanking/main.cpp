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
		setDescription("Rankes small variants in the context of a patients phenotype using an evidence-based model.");
		addInfile("in", "Input variant list in GSvar format.", false);
		addString("hpo_ids", "Comma-separated list of HPO identifiers.", false);
		addOutfile("out", "Output variant list in GSvar format with rank/score columns.", false);
		addEnum("algorithm", "Algorithm used for ranking.", false, VariantScores::algorithms());
		//optional
		addFlag("add_explanation", "Add a third output column with an explanation how that score was calculated.");
		addFlag("skip_ngsd_classifications", "Do not use variant classifications from NGSD.");
		addFlag("test", "Uses the test database instead of on the production database.");
		addFlag("debug", "Adds debug output to STDOUT.");

		changeLog(2026, 11,  2, "Implementation of v3.");
		changeLog(2023,  5,  5, "Implementation of v2.");
		changeLog(2020, 11, 20, "Implementation of v1.");
	}

	virtual void main()
	{
		//init
		QStringList hpo_ids = getString("hpo_ids").split(",");
		QString algorithm = getEnum("algorithm");
		bool add_explanation = getFlag("add_explanation");
		NGSD db(getFlag("test"));
		bool debug = getFlag("debug");
		QTextStream out(stdout);

		//load variants
		QElapsedTimer timer;
		timer.start();
		QString in = getInfile("in");
		VariantList variants;
		variants.load(in);
		if (debug)
		{
			out << "Loading GSvar took: " << Helper::elapsedTime(timer) << Qt::endl;
			timer.restart();
		}

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
			for (const QByteArray& gene : std::as_const(genes))
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
		if (debug)
		{
			out << "Getting HPO regions from NGSD took: " << Helper::elapsedTime(timer) << Qt::endl;
			timer.restart();
		}

		//rank
		VariantScores::Parameters parameters;
		parameters.use_ngsd_classifications = !getFlag("skip_ngsd_classifications");
		VariantScores::Result result = VariantScores::score(algorithm, variants, phenotype_rois, parameters);
		VariantScores::annotate(variants, result, add_explanation);
		if (debug)
		{
			out << "Ranking took: " << Helper::elapsedTime(timer) << Qt::endl;
			timer.restart();
		}

		//store result
		variants.store(getOutfile("out"));
		if (debug)
		{
			out << "Storing GSvar took: " << Helper::elapsedTime(timer) << Qt::endl;
			timer.restart();
		}
	}
};

#include "main.moc"

int main(int argc, char *argv[])
{
	ConcreteTool tool(argc, argv);
	return tool.execute();
}

/*
//TODO Marc
- splicing prediction or variant effect
- score by transcript, not by gene
	- extra score for MANE?
- check for recurring variants in top 10 that are not flagged as causal
*/