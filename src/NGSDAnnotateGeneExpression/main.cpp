#include "ToolBase.h"
#include "NGSD.h"
#include "Exceptions.h"
#include "Helper.h"
#include "TSVFileStream.h"
#include <QTextStream>
#include <QFileInfo>
#include <QElapsedTimer>


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
		setDescription("Annotates a GSvar file with RNA expression data.");
		addInfile("in", "Input GSvar file of DNA sample.", false, true);
		addOutfile("out", "Output GSvar file.", false, true);
		addString("rna_ps", "Processed sample name of the associated .", false);

		//optional
		QStringList valid_cohort = QStringList() << "RNA_COHORT_GERMLINE" << "RNA_COHORT_GERMLINE_PROJECT" << "RNA_COHORT_SOMATIC";
		addEnum("cohort_strategy", "Determines which samples are used as reference cohort.", true, valid_cohort, "RNA_COHORT_GERMLINE");
		addFlag("test", "Uses the test database instead of on the production database.");

		changeLog(2022, 6, 14, "Initial commit.");
	}

	virtual void main()
	{
		//init
		QString in = getInfile("in");
		QString out = getOutfile("out");
		QString rna_ps_name = getString("rna_ps");
		QString cohort_strategy_str = getEnum("cohort_strategy");
		RnaCohortDeterminationStategy cohort_strategy;
		if (cohort_strategy_str == "RNA_COHORT_GERMLINE")
		{
			cohort_strategy = RNA_COHORT_GERMLINE;
		}
		else if(cohort_strategy_str == "RNA_COHORT_GERMLINE_PROJECT")
		{
			cohort_strategy = RNA_COHORT_GERMLINE_PROJECT;
		}
		else if(cohort_strategy_str == "RNA_COHORT_SOMATIC")
		{
			cohort_strategy = RNA_COHORT_SOMATIC;
		}
		else
		{
			THROW(ArgumentException, "Invalid cohort strategy '" + cohort_strategy_str + "' given!");
		}

		NGSD db(getFlag("test"));

		//get expression values
		QString ps_id = db.processedSampleId(rna_ps_name);
		QMap<QByteArray, double> expression = db.getGeneExpressionValuesOfSample(ps_id, true);

		//get cohort stats
		ProcessedSampleData ps_data = db.getProcessedSampleData(ps_id);
		QString s_id = db.sampleId(rna_ps_name);
		SampleData s_data = db.getSampleData(s_id);
		int sys_id = db.processingSystemId(ps_data.processing_system);
		QSet<int> rna_cohort_ps_ids;
		QMap<QByteArray, ExpressionStats> expression_stats = db.calculateCohortExpressionStatistics(sys_id, s_data.tissue,  rna_cohort_ps_ids, ps_data.project_name, ps_id, cohort_strategy);


		//parse GSvar file
		VariantList gsvar;
		gsvar.load(in);

		//init columns
		int idx_genes = gsvar.annotationIndexByName("gene");
		gsvar.addAnnotationIfMissing("tpm", "Gene expression strength in transcripts-per-million.");
		gsvar.addAnnotationIfMissing("expr_log2fc", "Relative gene expression as log2 FC (log2 tpm).");
		gsvar.addAnnotationIfMissing("expr_zscore", "Relative gene expression as z-score (log2 tpm)");
		int idx_tpm = gsvar.annotationIndexByName("tpm");
		int idx_log2fc = gsvar.annotationIndexByName("expr_log2fc");
		int idx_zscore = gsvar.annotationIndexByName("expr_zscore");

		for (int i = 0; i < gsvar.count(); ++i)
		{
			Variant& var = gsvar[i];
			QByteArrayList genes = var.annotations().at(idx_genes).split(',');
			QByteArrayList tpm_list;
			QByteArrayList log2fc_list;
			QByteArrayList zscore_list;

			//get annotation for all genes
			foreach (const QByteArray& gene, genes)
			{
				int gene_id = db.geneId(gene);

				if(gene_id < 0)
				{
					tpm_list << "";
					log2fc_list << "";
					zscore_list << "";
				}
				else
				{
					QByteArray symbol = db.geneSymbol(gene_id);
					double tpm = expression.value(symbol);
					double log2p1tpm = std::log2(tpm + 1);
					ExpressionStats gene_stats = expression_stats.value(symbol);
					double log2fc = log2p1tpm - std::log2(gene_stats.mean + 1);
					double zscore = (log2p1tpm - gene_stats.mean_log2) / gene_stats.stddev_log2;
//					double p_value = 1 + std::erf(- std::abs(zscore) / std::sqrt(2));

					tpm_list << QByteArray::number(tpm);
					log2fc_list << QByteArray::number(log2fc);
					zscore_list << QByteArray::number(zscore);
				}
			}

			var.annotations()[idx_tpm] = tpm_list.join(',');
			var.annotations()[idx_log2fc] = log2fc_list.join(',');
			var.annotations()[idx_zscore] = zscore_list.join(',');
		}


		//store annotated GSvar file
		gsvar.store(out);

	}

};

#include "main.moc"

int main(int argc, char *argv[])
{
	ConcreteTool tool(argc, argv);
	return tool.execute();
}
