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
		setDescription("Calculates QC metrics for cfDNA samples.");
		addInfile("bam", "Input BAM/CRAM file.", false, true);
		addInfile("cfdna_panel", "Input BED file containing the (personalized) cfDNA panel.", false, true);
		//optional
		addOutfile("out", "Output qcML file. If unset, writes to STDOUT.", true, true);
		addInfile("tumor_bam", "Input tumor BAM/CRAM file for sample similarity.", true, true);
		addEnum("build", "Genome build used to generate the input.", true, QStringList() << "hg19" << "hg38", "hg19");
		addInfile("ref", "Reference genome FASTA file. If unset 'reference_genome' from the 'settings.ini' file is used.", true, false);
		addString("ref_cram", "Reference genome for CRAM support (mandatory if CRAM is used). If set, it is used for tumor and normal file.", true);

		//changelog
		changeLog(2021, 10, 22, "Initial version.");
	}

	virtual void main()
	{
		// init
		QString bam = getInfile("bam");
		QString cfdna_panel_path = getInfile("cfdna_panel");
		QString out = getOutfile("out");
		QString tumor_bam = getInfile("tumor_bam");
		GenomeBuild build = stringToBuild(getEnum("build"));
		QString ref = getInfile("ref");
		if(ref.isEmpty())	ref = Settings::string("reference_genome", true);
		if (ref=="") THROW(CommandLineParsingException, "Reference genome FASTA unset in both command-line and settings.ini file!");
		//TODO: make parameter
		int min_mapq = 0;
		//TODO: determine good threshold
		int required_depth = 250;

		//TODO: remove
		QTextStream std_out(stdout);


		//split panel in ID and monitoring SNPs (gene/hotspot regions)
		BedFile cfdna_panel;
		cfdna_panel.load(cfdna_panel_path, false);
		BedFile id_snps, monitoring_snps;

		for (int i = 0; i < cfdna_panel.count(); ++i)
		{
			const BedLine& line = cfdna_panel[i];
			if (line.annotations().at(0).startsWith("SNP_for_sample_identification:"))
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
		Statistics::avgCoverage(monitoring_snps, bam, min_mapq, false, true, 3, ref);

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

//		qDebug() << "Monitoring SNP avg_depth:" << monitoring_avg_depth;
//		qDebug() << required_depth << "x covered monitoring SNP:" << covered_monitoring_snps << "/" << monitoring_snps.count();

		// compute coverage on ID SNPs
		Statistics::avgCoverage(id_snps, bam, min_mapq, false, true, 3, ref);

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

//		qDebug() << "ID SNP avg_depth:" << id_avg_depth;
//		qDebug() << required_depth << "x covered ID SNP:" << covered_id_snps << "/" << id_snps.count();

		std_out << bam << "\t"
				<< QString::number(monitoring_avg_depth) << "\t"
				<< QString::number(covered_monitoring_snps) << "\t"
				<< QString::number(id_avg_depth) << "\t"
				<< QString::number(covered_id_snps) << endl;


//		// compute similarity
//		float correlation = 0.0;
//		if (!tumor_bam.isEmpty())
//		{
//			//TODO: extend cfDNA panel

//			//get genotype data
//			SampleSimilarity::VariantGenotypes cfdna_genotype_data = SampleSimilarity::genotypesFromBam(build, bam, 30, 2000, false, cfdna_panel, ref);
//			SampleSimilarity::VariantGenotypes tumor_genotype_data = SampleSimilarity::genotypesFromBam(build, tumor_bam, 30, 2000, false, cfdna_panel, ref);
//			SampleSimilarity sample_similarity;
//			sample_similarity.calculateSimilarity(cfdna_genotype_data, tumor_genotype_data);
//			correlation = sample_similarity.sampleCorrelation();

//			qDebug() << "Sample similarity: " << correlation;
//		}




//		// create qcML

//		// metadata
//		QList<QCValue> metadata;
//		metadata << QCValue("source file", QFileInfo(bam).fileName(), "", "QC:1000005");
//		metadata << QCValue("source file", QFileInfo(tumor_bam).absoluteFilePath() + " (tumor)", "", "QC:1000005");
//		metadata << QCValue("linked file", QFileInfo(cfdna_panel_path).fileName(), "", "QC:1000006");

//		QCCollection metrics;
//		metrics.insert(QCValue("monitoring variant read depth", monitoring_avg_depth, "", "QC:2000077"));
//		metrics.insert(QCValue("ID variant read depth", id_avg_depth, "", "QC:2000078"));
//		metrics.insert(QCValue("monitoring variant count", monitoring_snps.count(), "", "QC:2000079"));
//		metrics.insert(QCValue("250x coverage monitoring variant count", covered_monitoring_snps, "", "QC:2000080"));
//		metrics.insert(QCValue("ID variant count", id_snps.count(), "", "QC:2000081"));
//		metrics.insert(QCValue("250x coverage ID variant count", covered_id_snps, "", "QC:2000082"));

////		Statistics::addQcValue(metrics, "QC:2000077", "monitoring variant count", monitoring_snps.count());
////		Statistics::addQcValue(metrics, "QC:2000078", "250x coverage monitoring variant count", covered_monitoring_snps);
////		Statistics::addQcValue(metrics, "QC:2000079", "ID variant count", id_snps.count());
////		Statistics::addQcValue(metrics, "QC:2000080", "250x coverage ID variant count", id_snps.count());

//		if (!tumor_bam.isEmpty())
//		{
//			metrics.insert(QCValue("cfDNA-tumor correlation", correlation, "", "QC:2000083"));
////			Statistics::addQcValue(metrics, "QC:2000081", "cfDNA-tumor correlation", correlation);
//		}

//		//store output
//		QString parameters = "";
//		if(!tumor_bam.isEmpty())	parameters += " -tumor_bam " + tumor_bam;
//		metrics.storeToQCML(out, QStringList(), parameters, QMap< QString, int >(), metadata);

	}
};

#include "main.moc"

int main(int argc, char *argv[])
{
	ConcreteTool tool(argc, argv);
	return tool.execute();
}

