#include "BedFile.h"
#include "ToolBase.h"
#include "Helper.h"
#include "Statistics.h"
#include "Exceptions.h"
#include <QFileInfo>
#include <QDir>
#include <SampleSimilarity.h>
#include "Settings.h"

struct GeneCount
{
	int n_outlier_genes;
	int n_covered_genes;
};

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
		setDescription("Calculates QC metrics for RNA samples.");
		addInfile("bam", "Input BAM/CRAM file.", false, true);

		//optional
		addInfile("housekeeping_genes", "BED file containing the exon region of housekeeping genes.", true, true);
		addOutfile("out", "Output qcML file. If unset, writes to STDOUT.", true, true);
		addInfile("splicing", "TSV file containing spliced reads by gene.", true, true);
		addInfile("expression", "TSV file containing RNA expression.", true, true);
		addInfile("ref", "Reference genome FASTA file. If unset 'reference_genome' from the 'settings.ini' file is used.", true, false);
		addInt("min_mapq", "Set minimal mapping quality (default:0)", true, 1);
		addFlag("txt", "Writes TXT format instead of qcML.");


		//changelog
		changeLog(2022, 4, 27, "Initial version.");
		changeLog(2022, 5, 12, "Changed TPM cutoffs.");
		changeLog(2022, 7, 12, "Made housekeeping genes optional.");
	}

	virtual void main()
	{
		// init
		QString bam = getInfile("bam");
		QString housekeeping_genes = getInfile("housekeeping_genes");
		QString out = getOutfile("out");
		QString splicing = getInfile("splicing");
		QString expression = getInfile("expression");
		int min_mapq = getInt("min_mapq");
		QString ref = getInfile("ref");
		if(ref.isEmpty())	ref = Settings::string("reference_genome", true);
		if (ref=="") THROW(CommandLineParsingException, "Reference genome FASTA unset in both command-line and settings.ini file!");


		QCCollection rna_qc;

		if(!housekeeping_genes.trimmed().isEmpty())
		{
			//get qc stats of housekeeping genes
			BedFile housekeeping_genes_bed;
			housekeeping_genes_bed.load(housekeeping_genes);
			rna_qc = Statistics::mapping_housekeeping(housekeeping_genes_bed, bam, ref, min_mapq);
		}


		if (!splicing.trimmed().isEmpty())
		{
			//get aberrant spliced genes
			double aberrant_gene_threshold = 0.05;
			int n_aberrant_genes = getNumberOfAberrantGenes(splicing, aberrant_gene_threshold);
			rna_qc.insert(QCValue("aberrant spliced gene count", n_aberrant_genes, "Number of aberrant spliced genes (>= 5%)", "QC:2000110"));
		}

		if (!expression.trimmed().isEmpty())
		{
			//get outlier
			double zscore_threshold = 3;
			double tpm_threshold = 1.0;
			GeneCount gene_count = parseGeneExpression(expression, zscore_threshold, tpm_threshold);
			rna_qc.insert(QCValue("outlier gene count", gene_count.n_outlier_genes, "Number of outlier genes (zscore >= 3.0)", "QC:2000111"));
			rna_qc.insert(QCValue("covered gene count", gene_count.n_covered_genes, "Number of covered genes (TPM >= 1.0)", "QC:2000109"));
		}

		// create qcML
		// metadata
		QList<QCValue> metadata;
		QString parameters = "";
		metadata << QCValue("source file", QFileInfo(bam).fileName(), "", "QC:1000005");
		parameters += " -bam " + bam;
		if(!splicing.trimmed().isEmpty())
		{
			metadata << QCValue("source file", QFileInfo(splicing).fileName(), " (splicing)", "QC:1000005");
			parameters += " -splicing " + splicing;
		}
		if(!expression.trimmed().isEmpty())
		{
			metadata << QCValue("source file", QFileInfo(expression).fileName(), " (expression)", "QC:1000005");
			parameters += " -expression " + expression;
		}
		metadata << QCValue("linked file", QFileInfo(getInfile("housekeeping_genes")).fileName(), " (housekeeping genes)", "QC:1000006");


		if (getFlag("txt"))
		{
			QStringList output;
			rna_qc.appendToStringList(output);
			Helper::storeTextFile(Helper::openFileForWriting(out, true), output);
		}
		else
		{
			rna_qc.storeToQCML(out, QStringList(), parameters, QMap<QString, int>(), metadata);
		}


	}

	int getNumberOfAberrantGenes(const QString& file_path, double aberrant_gene_threshold)
	{
		QStringList aberrant_genes;

		TSVFileStream tsv_file(file_path);
		int idx_gene = tsv_file.colIndex("symbol", true);
		int idx_aberrant_frac = tsv_file.colIndex("aberrant_frac", true);

		while (!tsv_file.atEnd())
		{
			QByteArrayList tsv_line = tsv_file.readLine();
			double aberrant_frac = Helper::toDouble(tsv_line.at(idx_aberrant_frac), "Aberrant spliced gene fraction");
			if (aberrant_frac >= aberrant_gene_threshold)
			{
				aberrant_genes.append(tsv_line.at(idx_gene));
			}
		}

		return aberrant_genes.length();
	}

	GeneCount parseGeneExpression(const QString& file_path, double zscore_threshold, double tpm_threshold)
	{
		GeneCount gene_count;
		gene_count.n_outlier_genes = 0;
		gene_count.n_covered_genes = 0;

		TSVFileStream tsv_file(file_path);
		int idx_zscore = tsv_file.colIndex("zscore", true);
		int idx_tpm = tsv_file.colIndex("tpm", true);

		while (!tsv_file.atEnd())
		{
			QByteArrayList tsv_line = tsv_file.readLine();

			//parse zScore (outlier genes)
			QByteArray zscore_str = tsv_line.at(idx_zscore);
			// skip n/a entries
			if((zscore_str != "n/a") && (!zscore_str.trimmed().isEmpty()))
			{
				double zscore = std::fabs(Helper::toDouble(zscore_str, "ZScore"));
				if (zscore >= zscore_threshold)
				{
					gene_count.n_outlier_genes++;
				}
			}
			//parse covered genes
			double tpm = Helper::toDouble(tsv_line.at(idx_tpm), "TPM value");
			if (tpm >= tpm_threshold)
			{
				gene_count.n_covered_genes++;
			}
		}


		return gene_count;
	}
};

#include "main.moc"

int main(int argc, char *argv[])
{
	ConcreteTool tool(argc, argv);
	return tool.execute();
}

