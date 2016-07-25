#include "BedFile.h"
#include "ToolBase.h"
#include "Helper.h"
#include "Statistics.h"
#include "Exceptions.h"
#include "SampleCorrelation.h"
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
		setDescription("Calculates QC metrics based on tumor-normal pairs.");
		addInfile("tumor_bam", "Input tumor bam file.", false, true);
		addInfile("normal_bam", "Input normal bam file.", false, true);
		addInfile("somatic_vcf", "Input somatic vcf file.", false, true);
		//optional
		addFlag("count", "Count and print tripletts for current reference genome. No qcML file will generated.");
		addInfile("target_bed", "Target file used for tumor and normal experiment.", true);
		addOutfile("out", "Output qcML file. If unset, writes to STDOUT.", true);
	}

	virtual void main()
	{
		//init
		QString out = getOutfile("out");
		QString tumor_bam = getInfile("tumor_bam");
		QString normal_bam = getInfile("normal_bam");
		QString somatic_vcf = getInfile("somatic_vcf");
		QString target_bed = getInfile("target_bed");
		bool count = getFlag("count");

		if(!count)
		{
			QCCollection metrics;
			metrics = Statistics::somatic(tumor_bam, normal_bam, somatic_vcf, target_bed);

			// add linked files
			QStringList qcml_files;
			QDir out_dir = QFileInfo(out).absoluteDir();
			QString tumor_path_qcml = out_dir.relativeFilePath( QFileInfo(tumor_bam).absolutePath() ) + "/" + QFileInfo(tumor_bam).baseName();
			QString normal_path_qcml = out_dir.relativeFilePath( QFileInfo(normal_bam).absolutePath() ) + "/" + QFileInfo(normal_bam).baseName();
			QString tumor_path = QFileInfo(tumor_bam).path() + "/" + QFileInfo(tumor_bam).baseName();
			QString normal_path = QFileInfo(normal_bam).path() + "/" + QFileInfo(normal_bam).baseName();

			if(QFileInfo(tumor_path + "_stats_fastq.qcML").isFile())	qcml_files.append(tumor_path_qcml + "_stats_fastq.qcML");
			if(QFileInfo(tumor_path + "_stats_map.qcML").isFile())	qcml_files.append(tumor_path_qcml + "_stats_map.qcML");
			if(QFileInfo(normal_path + "_stats_fastq.qcML").isFile())	qcml_files.append(normal_path_qcml + "_stats_fastq.qcML");
			if(QFileInfo(normal_path + "_stats_map.qcML").isFile())	qcml_files.append(normal_path_qcml + "_stats_map.qcML");

			//store output
			QString parameters = "";
			if(!target_bed.isEmpty())	parameters += "-target_bed " + target_bed;
			metrics.storeToQCML(out, QStringList() << tumor_bam << normal_bam << somatic_vcf, parameters, QMap< QString, int >(), qcml_files);
		}

		//
		if(count)
		{
			QStringList sig = (QStringList() << "C" << "T");
			QStringList nuc = (QStringList() << "A" << "C" << "G" << "T");
			QList < QString > codons;
			QList < long long > counts;
			foreach(QString r, sig)
			{
				QString c = r;
				foreach(QString rr, nuc)
				{
					QString co = rr + c;
					foreach(QString rrr, nuc)
					{
						codons.append(co + rrr);
						counts.append(0);
					}
				}
			}


			FastaFileIndex reference(Settings::string("reference_genome"));
			int bin = 50000000;
			for(int i=0; i<reference.names().count(); ++i)
			{
				Chromosome chr = reference.names().at(i);

				if(!chr.isNonSpecial()) continue;

				int chrom_length = reference.lengthOf(chr);
				for(int j=1; j<=chrom_length; j+=bin)
				{
					int start = j;
					int length = bin;
					if(start>1)	//make bins overlap
					{
						start -= 2;
						length += 2;
					}
					if((start+length-1)>chrom_length)	length = (chrom_length - start + 1);
					Sequence seq = reference.seq(chr,start,length,true);
					for(int k=0; k<codons.count(); ++k)
					{
						counts[k] += seq.count(codons[k].toUpper().toLatin1());
					}
				}
			}
			for(int i=0; i<codons.count(); ++i)
			{
				qDebug() << "reference genome" << codons[i] << QString::number(counts[i]);
			}
		}
	}
};

#include "main.moc"

int main(int argc, char *argv[])
{
	ConcreteTool tool(argc, argv);
	return tool.execute();
}

