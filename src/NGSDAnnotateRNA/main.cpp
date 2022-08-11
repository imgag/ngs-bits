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
		QStringList valid_modes = QStringList() << "genes" << "exons";
		addEnum("mode", "Determines if genes or exons should be annotated.", true, valid_modes, "genes");
		QStringList valid_cohort = QStringList() << "RNA_COHORT_GERMLINE" << "RNA_COHORT_GERMLINE_PROJECT" << "RNA_COHORT_SOMATIC";
		addEnum("cohort_strategy", "Determines which samples are used as reference cohort.", true, valid_cohort, "RNA_COHORT_GERMLINE");
		addOutfile("corr", "File path to output file containing the spearman correlation to cohort mean.", true);
		addFlag("test", "Uses the test database instead of on the production database.");

		changeLog(2022, 6, 9, "Initial commit.");
		changeLog(2022, 7, 13, "Added support for exons.");
	}

	virtual void main()
	{
		//init
		QString in = getInfile("in");
		QString out = getOutfile("out");
		QString ps_name = getString("ps");
		QString mode = getEnum("mode");
		QString cohort_strategy_str = getEnum("cohort_strategy");
		QString corr = getOutfile("corr");
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


		//get expression
		QMap<QByteArray, ExpressionStats> expression_stats;
		QMap<QByteArray, QByteArray> ensg_gene_mapping;
		QSet<int> cohort = db.getRNACohort(sys_id, s_data.tissue, ps_data.project_name, ps_id, cohort_strategy, mode.toUtf8());

		//remove this sample from cohort
		cohort.remove(ps_id.toInt());

		if (cohort.size() > 0)
		{
			if (mode == "genes")
			{
				expression_stats = db.calculateGeneExpressionStatistics(cohort);
				ensg_gene_mapping = db.getEnsemblGeneMapping();
			}
			else if(mode == "exons")
			{
				expression_stats = db.calculateExonExpressionStatistics(cohort);
			}
			else
			{
				THROW(ArgumentException, "Invalid mode '" + mode + "given!")
			}
		}

		//parse input file
		TSVFileStream input_file(in);
		QByteArrayList output_buffer;
		output_buffer.append(input_file.comments());

		//add cohort stats
		output_buffer.append("##cohort_strategy:" + cohort_strategy_str.toUtf8());
		output_buffer.append("##cohort_size:" + QByteArray::number(cohort.size()));
		int corr_line_number = -1;
		if(!corr.isEmpty() && cohort.size() > 0)
		{
			output_buffer.append("##correlation: placeholder");
			corr_line_number = output_buffer.size() - 1;
		}


		// check if annotation columns already present and add to header
		QByteArrayList header = input_file.header();
		QByteArrayList db_header;
		db_header << "cohort_mean" << "log2fc" << "zscore" << "pval";
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
		int i_value=-1, i_gene_id=-1, i_exon=-1;
		if (mode == "genes")
		{
			i_value =  input_file.colIndex("tpm", true);
			i_gene_id = input_file.colIndex("gene_id", true);
		}
		else //exon
		{
			i_value =  input_file.colIndex("srpb", true);
			i_exon = input_file.colIndex("exon", true);
		}

		//buffer for correaltion
		QVector<double> expression_values;
		QVector<double> mean_values;


		// iterate over input file and annotate each cnv
		while (!input_file.atEnd())
		{
			//get line
			QByteArrayList tsv_line = input_file.readLine();
			tsv_line += additional_columns;

			//annotate
			QByteArray key;
			if (mode == "genes")
			{
				QByteArray ensg_id = tsv_line.at(i_gene_id).trimmed();
				if(ensg_gene_mapping.contains(ensg_id)) key = ensg_gene_mapping.value(ensg_id);
			}
			else //exons
			{
				BedLine exon = BedLine::fromString(tsv_line.at(i_exon));
				key = exon.toString(true).toUtf8();
			}


			if((!key.isEmpty()) && (expression_stats.contains(key)))
			{
				ExpressionStats gene_expression = expression_stats.value(key);
				double expr_value = Helper::toDouble(tsv_line.at(i_value), "expression value");
				double log2p1_expr_value = std::log2(expr_value + 1);

				//cohort mean
				double cohort_mean = gene_expression.mean;
				tsv_line[column_indices["cohort_mean"]] = QByteArray::number(cohort_mean);

				//log2fc
				double log2fc = log2p1_expr_value - std::log2(gene_expression.mean + 1);
				tsv_line[column_indices["log2fc"]] = QByteArray::number(log2fc);

				//zscore
				double zscore = (log2p1_expr_value - gene_expression.mean_log2) / gene_expression.stddev_log2;
				tsv_line[column_indices["zscore"]] = QByteArray::number(zscore);

				//pvalue
				double pvalue = 1 + std::erf(- std::abs(zscore) / std::sqrt(2));
				tsv_line[column_indices["pval"]] = QByteArray::number(pvalue);

				if((expr_value > 0) && (cohort_mean > 0))
				{
					expression_values << expr_value;
					mean_values << cohort_mean;
				}

			}

			//write line to buffer
			output_buffer << tsv_line.join("\t");
		}


		//calculate correlation
		if(!corr.isEmpty() && cohort.size() > 0)
		{
			QVector<double> rank_sample = calculateRanks(expression_values);
			QVector<double> rank_means = calculateRanks(mean_values);
			double correlation = BasicStatistics::correlation(rank_sample, rank_means);

			//write correlation to file:
			QSharedPointer<QFile> correlation_file = Helper::openFileForWriting(corr, true);
			QTextStream correlation_stream(correlation_file.data());
			correlation_stream << QString::number(correlation) << "\n";
			correlation_stream.flush();
			correlation_file->close();

			output_buffer[corr_line_number] = "##correlation: " + QByteArray::number(correlation);
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

	QVector<double> calculateRanks(const QVector<double>& values)
	{
		QVector<double> sorted_values = values;
		std::sort(sorted_values.rbegin(), sorted_values.rend());
		QVector<double> ranks = QVector<double>(values.size());
		for (int i = 0; i < values.size(); ++i)
		{
			ranks[i] = sorted_values.indexOf(values.at(i)) + 1;
		}
		return ranks;
	}

};

#include "main.moc"

int main(int argc, char *argv[])
{
	ConcreteTool tool(argc, argv);
	return tool.execute();
}
