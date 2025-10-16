#include "BedFile.h"
#include "ToolBase.h"
#include "Helper.h"
#include "Statistics.h"
#include "Exceptions.h"
#include <QFileInfo>
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
		setDescription("Calculates QC metrics for cfDNA samples.");
		addInfile("bam", "Input BAM/CRAM file.", false, true);
		addInfile("cfdna_panel", "Input BED file containing the (personalized) cfDNA panel.", false, true);
		//optional
		addOutfile("out", "Output qcML file. If unset, writes to STDOUT.", true, true);
		addInfile("tumor_bam", "Input tumor BAM/CRAM file for sample similarity.", true, true);
		addInfileList("related_bams", "BAM files of related cfDNA samples to compute sample similarity.", true, true);
		addInfile("error_rates", "Input TSV containing umiVar error rates.", true, true);
		addEnum("build", "Genome build used to generate the input.", true, QStringList() << "hg19" << "hg38", "hg38");
		addInfile("ref", "Reference genome FASTA file. If unset 'reference_genome' from the 'settings.ini' file is used.", true, false);
		addInt("min_mapq", "Set minimal mapping quality (default:0)", true, 0);
		addFlag("txt", "Writes TXT format instead of qcML.");
		addInt("threads", "The number of threads used for coverage calculation.", true, 1);

		//changelog
		changeLog(2022,  9, 16, "Added 'threads' parameter.");
		changeLog(2021, 10, 22, "Initial version.");
		changeLog(2021, 12, 3, "Added correllation between cfDNA samples.");
	}

	virtual void main()
	{
		// init
		QString bam = getInfile("bam");
		QString cfdna_panel_path = getInfile("cfdna_panel");
		QString out = getOutfile("out");
		QString tumor_bam = getInfile("tumor_bam");
		QStringList related_bams = getInfileList("related_bams");
		QString umivar_error_rate_file = getInfile("error_rates");
		int min_mapq = getInt("min_mapq");
		GenomeBuild build = stringToBuild(getEnum("build"));
		QString ref = getInfile("ref");
		if(ref.isEmpty())	ref = Settings::string("reference_genome", true);
		if (ref=="") THROW(CommandLineParsingException, "Reference genome FASTA unset in both command-line and settings.ini file!");
		int threads = getInt("threads");

		//set depth threshold
		int required_depth = 250;


		//split panel in ID and monitoring SNPs (gene/hotspot regions)
		BedFile cfdna_panel;
		cfdna_panel.load(cfdna_panel_path, false);
		BedFile id_snps, monitoring_snps;

		for (int i = 0; i < cfdna_panel.count(); ++i)
		{
			const BedLine& line = cfdna_panel[i];
			// if no annotation is available threat BED line as monitoring
			if ((line.annotations().size() > 0) && (line.annotations().at(0).startsWith("SNP_for_sample_identification:")))
			{
				id_snps.append(line);
			}
			else
			{
				monitoring_snps.append(line);
			}
		}
		id_snps.clearAnnotations();
		monitoring_snps.clearAnnotations();

		// compute coverage on monitoring SNPs
		Statistics::avgCoverage(monitoring_snps, bam, min_mapq, threads, 3, ref);

		//compute average depth
		double monitoring_avg_depth = 0.0;
		int covered_monitoring_snps = 0;
		for (int i = 0; i < monitoring_snps.count(); ++i)
		{
			const BedLine& monitoring_pos = monitoring_snps[i];
			double pos_depth = Helper::toDouble(monitoring_pos.annotations().at(0));
			monitoring_avg_depth += pos_depth * monitoring_pos.length();
			// count good covered regions
			if (pos_depth >= required_depth) covered_monitoring_snps++;
		}
		//normalize
		monitoring_avg_depth /= monitoring_snps.count();

		// compute coverage on ID SNPs
		Statistics::avgCoverage(id_snps, bam, min_mapq, threads, 3, ref);

		//compute average depth
		double id_avg_depth = 0.0;
		int covered_id_snps = 0;
		for (int i = 0; i < id_snps.count(); ++i)
		{
			const BedLine& id_pos = id_snps[i];
			double pos_depth = Helper::toDouble(id_pos.annotations().at(0));
			id_avg_depth +=  pos_depth * id_pos.length();
			// count good covered regions
			if (pos_depth >= required_depth) covered_id_snps++;
		}
		//normalize
		id_avg_depth /= id_snps.count();


		//extend cfDNA panel
		cfdna_panel.extend(60);
		SampleSimilarity::VariantGenotypes cfdna_genotype_data = SampleSimilarity::genotypesFromBam(build, bam, 30, 2000, false, cfdna_panel, ref);

		// compute similarity
		float tumor_correlation = 0.0;
		if (!tumor_bam.isEmpty())
		{
			//get genotype data
			SampleSimilarity::VariantGenotypes tumor_genotype_data = SampleSimilarity::genotypesFromBam(build, tumor_bam, 30, 2000, false, cfdna_panel, ref);
			SampleSimilarity sample_similarity;
			sample_similarity.calculateSimilarity(cfdna_genotype_data, tumor_genotype_data);
			tumor_correlation = sample_similarity.sampleCorrelation();

		}


		// compute similarity between cfDNA samples
		QStringList related_correlation;
		foreach (const QString& related_bam, related_bams)
		{
			SampleSimilarity::VariantGenotypes genotype_data = SampleSimilarity::genotypesFromBam(build, related_bam, 30, 2000, false, cfdna_panel, ref);
			SampleSimilarity sample_similarity;
			sample_similarity.calculateSimilarity(cfdna_genotype_data, genotype_data);
			related_correlation.append(QFileInfo(related_bam).baseName() + ":" + QString::number(sample_similarity.sampleCorrelation(), 'f', 2));
		}


		// parse umiVar error rates
		QMap<QString, double> umivar_error_rates;
		if (!umivar_error_rate_file.isEmpty())
		{
			QSharedPointer<QFile> error_rate_fp = Helper::openFileForReading(umivar_error_rate_file, false);
			while(!error_rate_fp->atEnd())
			{
				QString line = error_rate_fp->readLine().trimmed();

				//skip header
				if (line.startsWith("ER")) continue;

				//parse line
				QStringList columns = line.split("\t");
				double error_rate = std::numeric_limits<double>::quiet_NaN();
				if (columns.at(0).trimmed() != "NA")
				{
					error_rate = Helper::toDouble(columns.at(0), "Error rate");
				}
				QString duplication_rate = columns.at(4).trimmed();
				umivar_error_rates.insert(duplication_rate, error_rate);
			}
			error_rate_fp->close();

		}



		// create qcML

		// metadata
		QList<QCValue> metadata;
		QMap<QString,int> precision_overwrite;
		metadata << QCValue("source file", QFileInfo(bam).fileName(), "", "QC:1000005");
		if (!tumor_bam.isEmpty()) metadata << QCValue("source file", QFileInfo(tumor_bam).fileName() + " (tumor)", "", "QC:1000005");
		foreach (const QString& related_bam, related_bams)
		{
			metadata << QCValue("source file", QFileInfo(related_bam).fileName() + " (related cfDNA)", "", "QC:1000005");
		}

		metadata << QCValue("linked file", QFileInfo(cfdna_panel_path).fileName(), "", "QC:1000006");

		if (!umivar_error_rate_file.isEmpty()) metadata << QCValue("linked file", QFileInfo(umivar_error_rate_file).fileName(), "", "QC:1000006");

		QCCollection metrics;
		metrics.insert(QCValue("monitoring variant read depth", monitoring_avg_depth, "", "QC:2000077"));

		metrics.insert(QCValue("monitoring variant count", monitoring_snps.count(), "", "QC:2000079"));
		metrics.insert(QCValue("250x coverage monitoring variant percentage", 100.0 * (float) covered_monitoring_snps/monitoring_snps.count(), "", "QC:2000080"));
		metrics.insert(QCValue("ID variant count", id_snps.count(), "", "QC:2000081"));
		if (id_snps.count() > 0)
		{
			metrics.insert(QCValue("ID variant read depth", id_avg_depth, "", "QC:2000078"));
			metrics.insert(QCValue("250x coverage ID variant percentage", 100.0 * (float) covered_id_snps/id_snps.count(), "", "QC:2000082"));
		}


		if (!tumor_bam.isEmpty())
		{
			metrics.insert(QCValue("cfDNA-tumor correlation", tumor_correlation, "", "QC:2000083"));
		}

		if (!related_correlation.isEmpty())
		{
			metrics.insert(QCValue("cfDNA-cfDNA correlation", related_correlation.join(", "), "", "QC:2000084"));
		}

		if(!umivar_error_rate_file.isEmpty())
		{
			foreach (const QString& key, umivar_error_rates.keys())
			{
				if (key == "1x") metrics.insert(QCValue("umiVar error rate 1-fold duplication", umivar_error_rates.value(key), "", "QC:2000085"));
				else if(key == "2x") metrics.insert(QCValue("umiVar error rate 2-fold duplication", umivar_error_rates.value(key), "", "QC:2000086"));
				else if(key == "3x") metrics.insert(QCValue("umiVar error rate 3-fold duplication", umivar_error_rates.value(key), "", "QC:2000087"));
				else if(key == "4x") metrics.insert(QCValue("umiVar error rate 4-fold duplication", umivar_error_rates.value(key), "", "QC:2000088"));
			}

			// set specific precision for error values
			precision_overwrite.insert("umiVar error rate 1-fold duplication", 8);
			precision_overwrite.insert("umiVar error rate 2-fold duplication", 8);
			precision_overwrite.insert("umiVar error rate 3-fold duplication", 8);
			precision_overwrite.insert("umiVar error rate 4-fold duplication", 8);

		}

		//store output
		QString parameters = "";
		if(!tumor_bam.isEmpty())	parameters += " -tumor_bam " + tumor_bam;
		if(!related_bams.isEmpty())
		{
			parameters += " -related_bams";
			foreach (const QString& related_bam, related_bams)
			{
				parameters += " " + QFileInfo(related_bam).fileName();
			}
		}
		if(!umivar_error_rate_file.isEmpty()) parameters += " -error_rates " + umivar_error_rate_file;

		if (getFlag("txt"))
		{
			QStringList output;
			metrics.appendToStringList(output);
			Helper::storeTextFile(Helper::openFileForWriting(out, true), output);
		}
		else
		{
			metrics.storeToQCML(out, QStringList(), parameters, precision_overwrite, metadata);
		}
	}
};

#include "main.moc"

int main(int argc, char *argv[])
{
	ConcreteTool tool(argc, argv);
	return tool.execute();
}

