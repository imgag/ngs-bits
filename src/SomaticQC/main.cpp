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
		setDescription("Calculates QC metrics based on tumor-normal pairs.");
		addInfile("tumor_bam", "Input tumor bam file.", false, true);
		addInfile("normal_bam", "Input normal bam file.", false, true);
		addInfile("somatic_vcf", "Input somatic vcf file.", false, true);
		//optional
		addOutfile("out", "Output qcML file. If unset, writes to STDOUT.", true);
		addInfile("target_bed", "Target file used for tumor and normal experiment.", true);
		addInfileList("links","Files that appear in the link part of the qcML file.",true);
		addEnum("genome_build", "Genome build. Option count uses the refence genome taken from the settings file and counts all trippletts on the fly.",true,QStringList({"hg19","hg38","count"}),"hg19");
		setExtendedDescription(QStringList() << "SomaticQC integrates the output of the other QC tools "
							   << "and adds several metrics specific for tumor-normal pairs. All tools produce qcML, "
							   << "a generic XML format for QC of -omics experiments, which we adapted for NGS."
							   );
	}

	virtual void main()
	{
		//init
		QString out = getOutfile("out");
		QString tumor_bam = getInfile("tumor_bam");
		QString normal_bam = getInfile("normal_bam");
		QString somatic_vcf = getInfile("somatic_vcf");
		QString target_bed = getInfile("target_bed");
		QStringList links = getInfileList("links");
		QString genome_build = getEnum("genome_build");

		// convert linked files to relative paths
		QDir out_dir = QFileInfo(out).absoluteDir();
		for(int i=0;i<links.length();++i)
		{
			if(!QFileInfo(links[i]).isFile())
			{
				Log::error("Could not find file " + links[i] + ". Skipping.");
				continue;
			}
			links[i] = out_dir.relativeFilePath( QFileInfo(links[i]).absolutePath() ) + "/" + QFileInfo(links[i]).fileName();
		}

		QCCollection metrics;
		metrics = Statistics::somatic(tumor_bam, normal_bam, somatic_vcf, genome_build, target_bed);

		//store output
		QString parameters = "-build " + genome_build + ", ";
		if(!target_bed.isEmpty())	parameters += "-target_bed " + target_bed;
		metrics.storeToQCML(out, QStringList() << tumor_bam << normal_bam << somatic_vcf, parameters, QMap< QString, int >(), links);
	}
};

#include "main.moc"

int main(int argc, char *argv[])
{
	ConcreteTool tool(argc, argv);
	return tool.execute();
}

