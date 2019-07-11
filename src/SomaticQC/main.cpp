#include "BedFile.h"
#include "ToolBase.h"
#include "Helper.h"
#include "Statistics.h"
#include "Exceptions.h"
#include <QFileInfo>
#include <QDir>
#include "Settings.h"
#include "Log.h"
#include "NGSHelper.h"
#include "GeneSet.h"
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
		addInfile("tumor_bam", "Input tumor BAM file.", false, true);
		addInfile("normal_bam", "Input normal BAM file.", false, true);
		addInfile("somatic_vcf", "Input somatic VCF file.", false, true);
		//optional
		addOutfile("out", "Output qcML file. If unset, writes to STDOUT.", true, true);
		addInfileList("links","Files that appear in the link part of the qcML file.",true);
		addInfile("target_bed", "Target file used for tumor and normal experiment.", true);
		addInfile("target_exons","Bed file containing target exons, neccessary for TMB calculation. Please provide a file that contains the coordinates of all exons in the reference genome. If not set, algorithm assumes target file is already intersected with exonic coordinates.",true);
		addInfile("blacklist","Bed file containing regions which shall be blacklisted in TMB calculation.",true);
		addInfile("tsg_bed","Bed file containing regions of tumor suppressor genes.",true);
		addInfile("ref", "Reference genome FASTA file. If unset 'reference_genome' from the 'settings.ini' file is used.", true, false);
		addFlag("skip_plots", "Skip plots (intended to increase speed of automated tests).");
		setExtendedDescription(QStringList() << "SomaticQC integrates the output of the other QC tools and adds several metrics specific for tumor-normal pairs." << "All tools produce qcML, a generic XML format for QC of -omics experiments, which we adapted for NGS.");
		addEnum("build", "Genome build used to generate the input.", true, QStringList() << "hg19" << "hg38", "hg19");

		//changelog
		changeLog(2018,  7, 11, "Added build switch for hg38 support.");
		changeLog(2017,  7, 28, "Added somatic allele frequency histogram and tumor estimate.");
		changeLog(2017,  1, 16, "Increased speed for mutation profile, removed genome build switch.");
		changeLog(2016,  8, 25, "Version used in the application note.");
	}

	virtual void main()
	{
		// init
		QString out = getOutfile("out");
		QString tumor_bam = getInfile("tumor_bam");
		QString normal_bam = getInfile("normal_bam");
		QString somatic_vcf = getInfile("somatic_vcf");
		QString target_bed = getInfile("target_bed");
		QString target_exons = getInfile("target_exons");
		QString blacklist = getInfile("blacklist");
		QString tsg_bed = getInfile("tsg_bed");
		QString ref = getInfile("ref");
		if(ref.isEmpty())	ref = Settings::string("reference_genome");
		if (ref=="") THROW(CommandLineParsingException, "Reference genome FASTA unset in both command-line and settings.ini file!");
		QStringList links = getInfileList("links");
		bool skip_plots = getFlag("skip_plots");
		QString build = getEnum("build");

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

		//Construct target region for TMB calculation
		BedFile target_bed_file;
		target_bed_file.load(target_bed);


		QCCollection metrics;
		metrics = Statistics::somatic(build, tumor_bam, normal_bam, somatic_vcf, ref, target_bed_file, skip_plots);
		QCValue tmb = Statistics::somatic_tmb(somatic_vcf,target_exons,target_bed,tsg_bed,blacklist);
		metrics.insert(tmb);


		//store output
		QString parameters = "";
		if(!target_bed.isEmpty())	parameters += " -target_bed " + target_bed;	// targeted Seq
		if(!blacklist.isEmpty())	parameters += " -blacklist" + blacklist;
		if(!tsg_bed.isEmpty())		parameters += " -tsg_bed " + tsg_bed;
		if(!target_exons.isEmpty()) parameters += " -target_exons" + target_exons;
		metrics.storeToQCML(out, QStringList(), parameters, QMap< QString, int >(), metadata);
	}
};

#include "main.moc"

int main(int argc, char *argv[])
{
	ConcreteTool tool(argc, argv);
	return tool.execute();
}

