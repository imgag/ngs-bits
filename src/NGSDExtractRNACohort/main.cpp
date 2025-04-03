#include "ToolBase.h"
#include "NGSD.h"
#include "Exceptions.h"
#include "Helper.h"
#include "TSVFileStream.h"
#include "GeneSet.h"
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
		setDescription("Creates a table with gene expression values for a given set of genes and cohort");
		addString("ps", "Processed sample name on which the cohort is calculated.", false);
		addInfile("genes", "Text file containing gene names which should be included in the table. (1 gene per line.)", false);


		//optional
		addInfile("sample_expression", "TSV file containing gene expression for processed sample (required if processed sample data hasn't been imported to the database yet)", true);
		addOutfile("out", "Output TSV file. If unset, writes to STDOUT.", true);
		QStringList valid_cohort = QStringList() << "RNA_COHORT_GERMLINE" << "RNA_COHORT_GERMLINE_PROJECT" << "RNA_COHORT_SOMATIC";
		addEnum("cohort_strategy", "Determines which samples are used as reference cohort.", true, valid_cohort, "RNA_COHORT_GERMLINE");
		addFlag("test", "Uses the test database instead of on the production database.");

		changeLog(2022, 7, 21, "Initial commit.");
	}

	virtual void main()
	{
		//init
		QString ps_name = getString("ps");
		QString gene_file = getInfile("genes");
		QString expression_file = getInfile("sample_expression");

		QString out = getOutfile("out");

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

		//get cohort stats
		QString ps_id = db.processedSampleId(ps_name);
		ProcessedSampleData ps_data = db.getProcessedSampleData(ps_id);
		QString s_id = db.sampleId(ps_name);
		SampleData s_data = db.getSampleData(s_id);
		int sys_id = db.processingSystemId(ps_data.processing_system);

		//get list of genes
		GeneSet genes = GeneSet::createFromFile(gene_file);

		QMap<QByteArray, double> sample_expression;
		if (!expression_file.isEmpty())
		{
			//read expression from file
			TSVFileStream tsv(expression_file);
			int gene_id_idx = tsv.colIndex("gene_id", true);
			int tpm_idx = tsv.colIndex("tpm", true);

			while(!tsv.atEnd())
			{
				QList<QByteArray> parts = tsv.readLine();
				//skip empty lines
				if (parts.count()==0) continue;

				sample_expression.insert(parts.at(gene_id_idx).trimmed(), Helper::toDouble(parts.at(tpm_idx)));
			}

		}


		//get cohort
        QVector<int> cohort = db.getRNACohort(sys_id, s_data.tissue, ps_data.project_name, ps_id, cohort_strategy, "genes").values().toVector();
		std::sort(cohort.rbegin(), cohort.rend());

		//remove given ps_id if manual file is provided
		if(!expression_file.isEmpty())
		{
			cohort.removeAll(ps_id.toInt());
		}

		//get ensembl ids
		auto gene_ensembl_mapping = db.getGeneEnsemblMapping();

		// open output file and write annotated expression values to file
		QSharedPointer<QFile> output_file = Helper::openFileForWriting(out, true);
		QTextStream output_stream(output_file.data());

		//init comment and header
		output_stream << "##cohort_strategy=" << cohort_strategy_str << "\n";

		QStringList ps_names;
		foreach (int ps_id, cohort)
		{
			ps_names << db.processedSampleName(QString::number(ps_id));
		}
		output_stream << "#gene_id\t";
		if(!expression_file.isEmpty())
		{
			output_stream << ps_name << "\t";
		}
		output_stream << ps_names.join("\t") << "\n";

		//iterate over gene list
        for (const QByteArray& gene : genes)
		{
			QVector<double> expression_values = db.getGeneExpressionValues(gene, cohort);
			QByteArrayList expression_values_str;
			foreach (double expr, expression_values)
			{
				if(expr != expr)
				{
					expression_values_str << "";
				}
				else
				{
					expression_values_str << QByteArray::number(expr);
				}
			}
			output_stream << gene_ensembl_mapping.value(gene);

			// add expression from file
			if(!expression_file.isEmpty())
			{
				output_stream << "\t" << sample_expression.value(gene_ensembl_mapping.value(gene));
			}

			output_stream << "\t" << expression_values_str.join("\t") << "\n";
		}
		output_stream.flush();
		output_file->close();

	}

};

#include "main.moc"

int main(int argc, char *argv[])
{
	ConcreteTool tool(argc, argv);
	return tool.execute();
}
