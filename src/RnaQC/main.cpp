#include "BedFile.h"
#include "ToolBase.h"
#include "Helper.h"
#include "Statistics.h"
#include "Exceptions.h"
#include <QFileInfo>
#include <QDir>
#include <SampleSimilarity.h>
#include "Settings.h"

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
		addInfile("housekeeping_genes", "BED file containing the exon region of housekeeping genes.", false, true);
//		addInfile("gene_region", "BED file containing the gene region of all covered genes.", false, true);
//		addInfile("exon_region", "BED file containing the exon region of all covered genes.", false, true);


		//optional
		addOutfile("out", "Output qcML file. If unset, writes to STDOUT.", true, true);
		addInfile("rna_counts", "TSV file containing read counts by gene.", true, true);
		addInfile("splicing", "TSV file containing spliced reads by gene.", true, true);
		addInfile("expression", "TSV file containing RNA expression.", true, true);
//		addEnum("build", "Genome build used to generate the input.", true, QStringList() << "hg19" << "hg38", "hg38");
		addInfile("ref", "Reference genome FASTA file. If unset 'reference_genome' from the 'settings.ini' file is used.", true, false);
		addInt("min_mapq", "Set minimal mapping quality (default:0)", true, 1);
		addFlag("txt", "Writes TXT format instead of qcML.");


		//changelog
		changeLog(2022, 4, 27, "Initial version.");
	}

	virtual void main()
	{
		// init
		QString bam = getInfile("bam");
		BedFile housekeeping_genes;
		housekeeping_genes.load(getInfile("housekeeping_genes"));
//		BedFile gene_region;
//		gene_region.load(getInfile("gene_region"));
//		BedFile exon_region;
//		exon_region.load(getInfile("exon_genes"));
		QString out = getOutfile("out");
		QString rna_counts = getInfile("rna_counts");
		QString splicing = getInfile("splicing");
		QString expression = getInfile("expression");
		int min_mapq = getInt("min_mapq");
		QString ref = getInfile("ref");
		if(ref.isEmpty())	ref = Settings::string("reference_genome", true);
		if (ref=="") THROW(CommandLineParsingException, "Reference genome FASTA unset in both command-line and settings.ini file!");

//		QCCollection rna_qc;
		QTime timer;
		timer.start();
//		qDebug() << "Compute mapping QC...";
		QCCollection rna_qc = Statistics::mapping_housekeeping(housekeeping_genes, bam, ref, min_mapq);
//		qDebug() << "..done" << Helper::elapsedTime(timer);

		if (!rna_counts.trimmed().isEmpty())
		{
//			qDebug() << "Parse covered genes...";
			//get number of covered genes and outlier
			double tpm_threshold = 10.0;
			int n_covered_genes = getNumberOfCoveredGenes(rna_counts, tpm_threshold);
			rna_qc.insert(QCValue("covered gene count", n_covered_genes, "Number of covered genes (TPM >= 10.0)", "QC:2000109"));
//			qDebug() << "..done" << Helper::elapsedTime(timer);
		}

		if (!splicing.trimmed().isEmpty())
		{
//			qDebug() << "Parse aberrant spliced genes...";
			//get aberrant spliced genes
			double aberrant_gene_threshold = 0.05;
			int n_aberrant_genes = getNumberOfAberrantGenes(splicing, aberrant_gene_threshold);
			rna_qc.insert(QCValue("aberrant spliced gene count", n_aberrant_genes, "Number of aberrant spliced genes (>= 5%)", "QC:2000110"));
//			qDebug() << "..done"  << Helper::elapsedTime(timer);
		}

		if (!expression.trimmed().isEmpty())
		{
//			qDebug() << "Parse outlier genes...";
			//get outlier
			double zscore_threshold = 3;
			int n_outlier_genes = getNumberOfOutlierGenes(expression, zscore_threshold);
			rna_qc.insert(QCValue("outlier gene count", n_outlier_genes, "Number of outlier genes (zscore >= 3.0)", "QC:2000111"));
//			qDebug() << "..done"  << Helper::elapsedTime(timer);
		}

		// TODO: get intronic/exonic read fraction

		// create qcML
		// metadata
		QList<QCValue> metadata;
		QString parameters = "";
		metadata << QCValue("source file", QFileInfo(bam).fileName(), "", "QC:1000005");
		parameters += " -bam " + bam;
		if(!rna_counts.trimmed().isEmpty())
		{
			metadata << QCValue("source file", QFileInfo(rna_counts).fileName(), " (RNA counts)", "QC:1000005");
			parameters += " -rna_counts " + rna_counts;
		}
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


	int getNumberOfCoveredGenes(const QString& file_path, double tpm_threshold)
	{
		QStringList covered_genes;

		TSVFileStream tsv_file(file_path);
		int idx_gene = tsv_file.colIndex("gene_name", true);
		int idx_tpm = tsv_file.colIndex("tpm", true);

		while (!tsv_file.atEnd())
		{
			QByteArrayList tsv_line = tsv_file.readLine();
			double tpm = Helper::toDouble(tsv_line.at(idx_tpm), "TPM value");
			if (tpm >= tpm_threshold)
			{
				covered_genes.append(tsv_line.at(idx_gene));
			}
		}

		return covered_genes.length();
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
			double tpm = Helper::toDouble(tsv_line.at(idx_aberrant_frac), "Aberrant spliced gene fraction");
			if (tpm >= aberrant_gene_threshold)
			{
				aberrant_genes.append(tsv_line.at(idx_gene));
			}
		}

		return aberrant_genes.length();
	}

	int getNumberOfOutlierGenes(const QString& file_path, double zscore_threshold)
	{
		QStringList outlier_genes;

		TSVFileStream tsv_file(file_path);
		int idx_gene = tsv_file.colIndex("gene_name", true);
		int idx_zscore = tsv_file.colIndex("zscore", true);

		while (!tsv_file.atEnd())
		{
			QByteArrayList tsv_line = tsv_file.readLine();
			QByteArray zscore_str = tsv_line.at(idx_zscore);
			// skip n/a entries
			if(zscore_str == "n/a") continue;
			double zscore = std::fabs(Helper::toDouble(zscore_str, "ZScore"));
			if (zscore >= zscore_threshold)
			{
				outlier_genes.append(tsv_line.at(idx_gene));
			}
		}

		return outlier_genes.length();
	}
};

#include "main.moc"

int main(int argc, char *argv[])
{
	ConcreteTool tool(argc, argv);
	return tool.execute();
}

