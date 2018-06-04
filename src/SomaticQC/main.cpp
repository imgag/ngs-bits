#include "BedFile.h"
#include "ToolBase.h"
#include "Helper.h"
#include "Statistics.h"
#include "Exceptions.h"
#include "SampleCorrelation.h"
#include <QFileInfo>
#include <QDir>
#include "Settings.h"
#include "Log.h"
#include "NGSHelper.h"
#include <vector>


class ConcreteTool
		: public ToolBase
{
	Q_OBJECT

public:
	ConcreteTool(int& argc, char *argv[])
		: ToolBase(argc, argv)
	{
	}

	QList<QString> extractExperimentInfo(QString bam, QString tag)
	{
		QStringList platforms;
		QStringList devices;
		QStringList enrichments;

		BamReader reader(bam);
		foreach(QByteArray line, reader.headerLines())
		{
			if(line.startsWith("@RG"))
			{
				QString platform = "unknown_platform";
				QString device = "unknown_device";
				QString enrichment = "unknown_enrichment";
				QByteArrayList parts = line.split('\t');
				foreach(QByteArray part, parts)
				{
					if (part.startsWith("PL:"))
					{
						platform = part.mid(3).trimmed();
					}
					if (part.startsWith("PM:"))
					{
						device = part.mid(3).trimmed();
					}
					if (part.startsWith("en:"))
					{
						enrichment = part.mid(3).trimmed();
					}
				}
				platforms << platform;
				devices << device;
				enrichments << enrichment;
			}
		}

		return QList<QString>({"QC:?", "experiment", platforms.join(":") + ", " + devices.join(":") + ", " + enrichments.join(":") + " (" + tag + ")"});
	}

	virtual void setup()
	{
		setDescription("Calculates QC metrics based on tumor-normal pairs.");
		addInfile("tumor_bam", "Input tumor bam file.", false, true);
		addInfile("normal_bam", "Input normal bam file.", false, true);
		addInfile("somatic_vcf", "Input somatic vcf file.", false, true);
		addOutfile("out", "Output qcML file. If unset, writes to STDOUT.", false, true);
		//optional
		addInfileList("links","Files that appear in the link part of the qcML file.",true);
		addInfile("target_bed", "Target file used for tumor and normal experiment.", true);
		addInfile("ref_fasta", "Reference fasta file. If unset the reference file from the settings file will be used.", true);
		addFlag("skip_plots", "Skip plots (intended to increase speed of automated tests).");
		setExtendedDescription(QStringList() << "SomaticQC integrates the output of the other QC tools and adds several metrics specific for tumor-normal pairs. The genome build form the settings file will be used during calcuation of QC metrics. All tools produce qcML, a generic XML format for QC of -omics experiments, which we adapted for NGS.");

		//changelog
		changeLog(2017, 7, 28, "Added somatic allele frequency histogram and tumor estimate.");
		changeLog(2017, 1, 16, "Increased speed for mutation profile, removed genome build switch.");
		changeLog(2016, 8, 25, "Version used in the application note.");
	}

	virtual void main()
	{
		// init
		QString out = getOutfile("out");
		QString tumor_bam = getInfile("tumor_bam");
		QString normal_bam = getInfile("normal_bam");
		QString somatic_vcf = getInfile("somatic_vcf");
		QString target_bed = getInfile("target_bed");
		QString ref_fasta = getInfile("ref_fasta");
		if(ref_fasta.isEmpty())	ref_fasta = Settings::string("reference_genome");
		if (ref_fasta=="") THROW(CommandLineParsingException, "Reference genome FASTA unset in both command-line and settings.ini file!");
		QStringList links = getInfileList("links");
		bool skip_plots = getFlag("skip_plots");

		// metadata
		QList<QList<QString>> metadata;
		metadata += QList<QString>({"QC:1000005","source file",QFileInfo(tumor_bam).fileName() + " (tumor)"});
		metadata += QList<QString>({"QC:1000005","source file",QFileInfo(normal_bam).fileName() + " (normal)"});
		metadata += QList<QString>({"QC:1000005","source file",QFileInfo(somatic_vcf).fileName()});
		metadata += extractExperimentInfo(tumor_bam, "tumor");
		metadata += extractExperimentInfo(normal_bam, "normal");

		// metadata - add linked files as relative paths
		QDir out_dir = QFileInfo(out).absoluteDir();
		for(int i=0;i<links.length();++i)
		{
			if(!QFileInfo(links[i]).isFile())
			{
				Log::error("Could not find file " + links[i] + ". Skipping.");
				continue;
			}
			QString rel = out_dir.relativeFilePath( QFileInfo(links[i]).absolutePath() );
			if(!rel.isEmpty())	 rel += "/";
			metadata += QList<QString>({"QC:1000006","linked file",rel + QFileInfo(links[i]).fileName()});
		}

		// calculate somatic QC metrics
		QCCollection metrics;
		metrics = Statistics::somatic(tumor_bam, normal_bam, somatic_vcf, ref_fasta, target_bed, skip_plots);

		//store output
		QString parameters = "";
		if(!target_bed.isEmpty())	parameters += "-target_bed " + target_bed;	// targeted Seq
		metrics.storeToQCML(out, QStringList(), parameters, QMap< QString, int >(), metadata);
	}
};

#include "main.moc"

int main(int argc, char *argv[])
{
	ConcreteTool tool(argc, argv);
	return tool.execute();
}

