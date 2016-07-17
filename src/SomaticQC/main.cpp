#include "BedFile.h"
#include "ToolBase.h"
#include "Helper.h"
#include "Statistics.h"
#include "Exceptions.h"
#include "SampleCorrelation.h"
#include <QFileInfo>

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
		addInfile("target_bed", "Target file used for tumor and normal experiment.", true);
		addOutfile("out", "Output qcML file. If unset, writes to STDOUT.", true);
	}

	virtual void main()
	{
		//init
		QString tumor_bam = getInfile("tumor_bam");
		QString tumor_path = QFileInfo(tumor_bam).path() + "/" + QFileInfo(tumor_bam).baseName();
		QString normal_bam = getInfile("normal_bam");
		QString normal_path = QFileInfo(normal_bam).path() + "/" + QFileInfo(normal_bam).baseName();
		QString somatic_vcf = getInfile("somatic_vcf");
		QString target_bed = getInfile("target_bed");

		QCCollection metrics;
		metrics = Statistics::somatic(tumor_bam, normal_bam, somatic_vcf, target_bed);

		QStringList qcml;
		if(QFileInfo(tumor_path + "_stats_fastq.qcML").isFile())	qcml.append(tumor_path + "_stats_fastq.qcML");
		if(QFileInfo(tumor_path + "_stats_map.qcML").isFile())	qcml.append(tumor_path + "_stats_map.qcML");
		if(QFileInfo(normal_path + "_stats_fastq.qcML").isFile())	qcml.append(normal_path + "_stats_fastq.qcML");
		if(QFileInfo(normal_path + "_stats_map.qcML").isFile())	qcml.append(normal_path + "_stats_map.qcML");

		//store output
		QString out = getOutfile("out");
		metrics.storeToQCML(out, QStringList() << tumor_bam << normal_bam << somatic_vcf << qcml, "");
	}
};

#include "main.moc"

int main(int argc, char *argv[])
{
	ConcreteTool tool(argc, argv);
	return tool.execute();
}

