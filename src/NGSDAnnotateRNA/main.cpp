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
		setDescription("Annotates a RNA expression TSV file with cohort information.");
		addString("ps", "Processed sample name of the input file.", false);


		//optional
		addInfile("in", "Input TSV file. If unset, reads from STDIN.", true);
		addOutfile("out", "Output TSV file. If unset, writes to STDOUT.", true);
		QStringList valid_cohort = QStringList() << "RNA_COHORT_GERMLINE" << "RNA_COHORT_GERMLINE_PROJECT" << "RNA_COHORT_SOMATIC";
		addEnum("cohort_strategy", "Determines which samples are used as reference cohort.", true, valid_cohort, "RNA_COHORT_GERMLINE");
		addFlag("test", "Uses the test database instead of on the production database.");

		changeLog(2022, 06, 9, "Initial commit.");
	}

	virtual void main()
	{
		//init
		QString in = getInfile("in");
		QString out = getOutfile("out");
		QString ps_name = getString("ps");
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
		QSet<int> cohort_ps_ids;


		QMap<QByteArray, ExpressionStats> expression_stats = db.calculateCohortExpressionStatistics(sys_id, s_data.tissue,  cohort_ps_ids, ps_data.project_name, ps_id, cohort_strategy);

		//get ensg->symbol mapping
		QMap<QByteArray, QByteArray> ensg_gene_mapping = db.getEnsemblGeneMapping();


		//parse input file
		TSVFileStream input_file(in);
		QByteArrayList output_buffer;
		output_buffer.append(input_file.comments());

		// check if annotation columns already present and add to header
		QByteArrayList header = input_file.header();
		QByteArrayList db_header;
		db_header << "cohort_mean" << "log2fc" << "zscore" << "pvalue";
		QByteArrayList additional_columns;
		QMap<QByteArray, int> column_indices;
		foreach (const QByteArray column_name, db_header)
		{
			int index = header.indexOf(column_name);
			if (index < 0)
			{
				header.append(column_name);
				additional_columns << "";
				index = header.size() - 1;
			}
			column_indices.insert(column_name, index);
		}

		output_buffer << "#" + header.join("\t");

		// get indices for position
		int i_gene_id = input_file.colIndex("gene_id", true);
		int i_tpm =  input_file.colIndex("tpm", true);

		// iterate over input file and annotate each cnv
		while (!input_file.atEnd())
		{
			//get line
			QByteArrayList tsv_line = input_file.readLine();
			tsv_line += additional_columns;

			//annotate
			QByteArray ensg_id = tsv_line.at(i_gene_id).trimmed();

			if(ensg_gene_mapping.contains(ensg_id))
			{
				QByteArray symbol = ensg_gene_mapping.value(ensg_id);
				if(expression_stats.contains(symbol))
				{
					ExpressionStats gene_expression = expression_stats.value(symbol);
					double tpm = Helper::toDouble(tsv_line.at(i_tpm), "TPM", ensg_id);
					double log2p1tpm = std::log2(tpm + 1);

					//cohort mean
					double cohort_mean = gene_expression.mean;
					tsv_line[column_indices["cohort_mean"]] = QByteArray::number(cohort_mean);

					//log2fc
					double log2fc = log2p1tpm - std::log2(gene_expression.mean + 1);
					tsv_line[column_indices["log2fc"]] = QByteArray::number(log2fc);

					//zscore
					double zscore = (log2p1tpm - gene_expression.mean_log2) / gene_expression.stddev_log2;
					tsv_line[column_indices["zscore"]] = QByteArray::number(zscore);

					//pvalue
					double pvalue = 1 + std::erf(- std::abs(zscore) / std::sqrt(2));
					tsv_line[column_indices["pvalue"]] = QByteArray::number(pvalue);

				}
			}

			//write line to buffer
			output_buffer << tsv_line.join("\t");
		}

		// open output file and write annotated expression values to file
		QSharedPointer<QFile> output_file = Helper::openFileForWriting(out, true);
		QTextStream output_stream(output_file.data());

		foreach (QByteArray line, output_buffer)
		{
			output_stream << line << "\n";
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
