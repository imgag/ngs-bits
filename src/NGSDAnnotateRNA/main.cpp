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
		addInfile("hpa_file", "TSV file containing the Human Protein Atlas (https://www.proteinatlas.org) to annotate gene expression", true);
		addFlag("update_genes", "Update annotated gene names with approved gene names from the NGSD");
		addFlag("test", "Uses the test database instead of on the production database.");

		changeLog(2022, 6, 9, "Initial commit.");
		changeLog(2022, 7, 13, "Added support for exons.");
		changeLog(2022, 8, 11, "Added HPA annotation support.");
		changeLog(2022, 8, 18, "Added ability to update gene names.");
		changeLog(2022, 9, 15, "Added annotation of transcript ids.");

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
		QString hpa_file_path = getInfile("hpa_file");
		bool update_genes = getFlag("update_genes");

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
		QMap<QByteArray, QByteArray> ensg_gene_mapping = db.getEnsemblGeneMapping();
		QMap<QByteArray, QByteArrayList> exon_transcript_mapping;
		QSet<int> cohort = db.getRNACohort(sys_id, s_data.tissue, ps_data.project_name, ps_id, cohort_strategy, mode.toUtf8());

		//remove this sample from cohort
		cohort.remove(ps_id.toInt());

		qDebug() << cohort;

		if (cohort.size() > 0)
		{
			if (mode == "genes")
			{
				expression_stats = db.calculateGeneExpressionStatistics(cohort, "", true);
			}
			else if(mode == "exons")
			{
				expression_stats = db.calculateExonExpressionStatistics(cohort);
				exon_transcript_mapping  = db.getExonTranscriptMapping();
			}
			else
			{
				THROW(ArgumentException, "Invalid mode '" + mode + "given!")
			}
		}

		//get HPA data
		bool hpa_annotation = false;
		QMap<QByteArray, double> hpa_data;
		if(!hpa_file_path.isEmpty())
		{
			if(mode != "genes") THROW(ArgumentException, "HPA annotation only supported for gene expression!");
			if(cohort_strategy != RNA_COHORT_SOMATIC) THROW(ArgumentException, "HPA annotation only supported for somatic samples!");

			//get hpa tissue
			QStringList sample_hpa_tissues = db.getValues(QByteArray() + "SELECT DISTINCT sdi.disease_info FROM sample s "
											 + "LEFT JOIN sample_relations sr ON s.id=sr.sample1_id OR s.id=sr.sample2_id "
											 + "LEFT JOIN sample_disease_info sdi ON sdi.sample_id=sr.sample1_id OR sdi.sample_id=sr.sample2_id "
											 + "WHERE s.id=:0 AND sdi.type='RNA reference tissue' AND (sr.relation='same sample' OR sr.relation IS NULL)", s_id);
			if (sample_hpa_tissues.size() == 0) THROW(DatabaseException, "No HPA reference tissue set for sample '" + ps_name + "'!");
			if (sample_hpa_tissues.size() > 1) THROW(DatabaseException, "Multiple HPA reference tissues set for sample '" + ps_name + "'! Cannot perform annotation.");

			//parse HPA file

			QSharedPointer<QFile> hpa_file = Helper::openFileForReading(hpa_file_path,false);

			//read header
			QByteArrayList hpa_file_header = hpa_file->readLine().replace("\n", "").replace("\r", "").split('\t');
			int ensg_idx = hpa_file_header.indexOf("Gene");
			if (ensg_idx < 0) THROW(FileParseException, "Column 'Gene' missing in HPA file!");
			int tissue_idx = hpa_file_header.indexOf("Tissue");
			if (tissue_idx < 0) THROW(FileParseException, "Column 'Tissue' missing in HPA file!");
			int tpm_idx = hpa_file_header.indexOf("TPM");
			if (tpm_idx < 0) THROW(FileParseException, "Column 'TPM' missing in HPA file!");

			while(!hpa_file->atEnd())
			{
				QByteArrayList line = hpa_file->readLine().split('\t');
				QByteArray ensg = line.at(ensg_idx).trimmed();
				QByteArray tissue = line.at(tissue_idx).trimmed();

				double tpm = Helper::toDouble(line.at(tpm_idx), "TPM (hpa_file)", ensg);

				if(tissue == sample_hpa_tissues.at(0))
				{
					hpa_data.insert(ensg, tpm);
				}
			}

			if(hpa_data.size() == 0) THROW(ArgumentException, "No HPA gene expression found for HPA reference tissue '" + sample_hpa_tissues.at(0) + "'! Please check if it is a valid HPA tissue.");

			hpa_annotation = true;
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
		if(mode == "exons") db_header << "transcript_id";
		db_header << "cohort_mean" << "log2fc" << "zscore" << "pval";
		QByteArrayList hpa_header;
		if(hpa_annotation) hpa_header << "hpa_tissue_tpm" << "hpa_tissue_log2tpm" << "hpa_sample_log2tpm" << "hpa_log2fc";
		QByteArrayList additional_headers = db_header + hpa_header;
		QByteArrayList additional_columns;
		QMap<QByteArray, int> column_indices;
		foreach (const QByteArray column_name, additional_headers)
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
		int i_value=-1, i_exon=-1;
		if (mode == "genes")
		{
			i_value =  input_file.colIndex("tpm", true);
		}
		else //exon
		{
			i_value =  input_file.colIndex("srpb", true);
			i_exon = input_file.colIndex("exon", true);
		}

		//ensg id
		int i_gene_id = input_file.colIndex("gene_id", true);

		//gene name
		int i_gene_name = -1;
		if (update_genes) i_gene_name = input_file.colIndex("gene_name", true);

		int i_tpm = -1;
		if(hpa_annotation) i_tpm = input_file.colIndex("tpm", true);

		//buffer for correaltion
		QVector<double> expression_values;
		QVector<double> mean_values;


		// iterate over input file and annotate each expression
		while (!input_file.atEnd())
		{
			//get line
			QByteArrayList tsv_line = input_file.readLine();
			//skip empty lines
			if (tsv_line.size() == 0) continue;

			//add empty additional columns
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

				//annotate transcript id
				if (mode == "exons")
				{
					QByteArray transcript_id;
					if (exon_transcript_mapping.contains(key))
					{
						QStringList buffer;
						foreach (const QByteArray& id, exon_transcript_mapping.value(key)) buffer << id;
						std::sort(buffer.begin(), buffer.end());
						transcript_id = buffer.join(",").toUtf8();
					}
					tsv_line[column_indices["transcript_id"]] = transcript_id;
				}

				//cohort mean
				double cohort_mean = gene_expression.mean;
				tsv_line[column_indices["cohort_mean"]] = QByteArray::number(cohort_mean);

				//log2fc
				double log2fc = log2p1_expr_value - std::log2(gene_expression.mean + 1);
				tsv_line[column_indices["log2fc"]] = QByteArray::number(log2fc);

				//zscore
				double zscore;
				if (gene_expression.stddev_log2 != 0)
				{
					zscore = (log2p1_expr_value - gene_expression.mean_log2) / gene_expression.stddev_log2;
				}
				else
				{
					zscore = std::numeric_limits<double>::quiet_NaN();
				}

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

			if(hpa_annotation)
			{
				QByteArray ensg = tsv_line.at(i_gene_id).trimmed();

				double sample_tpm = Helper::toDouble(tsv_line.at(i_tpm), "TPM", ensg);

				double sample_log2tpm = std::log2(sample_tpm + 1);
				tsv_line[column_indices["hpa_sample_log2tpm"]] = QByteArray::number(sample_log2tpm);

				if(hpa_data.contains(ensg))
				{
					double tissue_tpm = hpa_data.value(ensg);
					tsv_line[column_indices["hpa_tissue_tpm"]] = QByteArray::number(tissue_tpm);

					double tissue_log2tpm = std::log2(tissue_tpm + 1);
					tsv_line[column_indices["hpa_tissue_log2tpm"]] = QByteArray::number(tissue_log2tpm);

					double log2fc = sample_log2tpm - tissue_log2tpm;
					tsv_line[column_indices["hpa_log2fc"]] = QByteArray::number(log2fc);
				}

			}

			if(update_genes)
			{
				qDebug() << "index:" << i_gene_name;
				qDebug() << tsv_line;
				QByteArray old_gene_name = tsv_line.at(i_gene_name).trimmed();
				qDebug() << old_gene_name;
				if(!old_gene_name.isEmpty())
				{
					QByteArray approved_gene_name;
					QByteArray ensg_id = tsv_line.at(i_gene_id).trimmed();
					if(ensg_gene_mapping.contains(ensg_id))
					{
						//update by ENSG id:
						approved_gene_name = ensg_gene_mapping.value(ensg_id);
					}
					else
					{
						//update by NGSD or use old name if not updatable:
						approved_gene_name = db.geneToApproved(old_gene_name, true);
					}
					tsv_line[i_gene_name] = approved_gene_name;
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
