#include "BedFile.h"
#include "ToolBase.h"
#include "Helper.h"
#include "Statistics.h"
#include "Exceptions.h"
#include <QFileInfo>
#include <QDir>
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
		addInfile("tumor_bam", "Input tumor BAM/CRAM file for sample similarity.", true);
		addEnum("build", "Genome build used to generate the input.", true, QStringList() << "hg19" << "hg38", "hg19");
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
		QStringList links = getInfileList("links");
		//TODO: make parameter
		int min_mapq = 0;


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

		// compute coverage on ID SNPs
		Statistics::avgCoverage(id_snps, bam, min_mapq, false, true, 3, ref);


		// compute coverage on monitoring SNPs
		Statistics::avgCoverage(monitoring_snps, bam, min_mapq, false, true, 3, ref);

		// compute similarity


		// create qcML
	}
};

#include "main.moc"

int main(int argc, char *argv[])
{
	ConcreteTool tool(argc, argv);
	return tool.execute();
}

