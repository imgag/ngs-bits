#include "ToolBase.h"
#include "VariantList.h"
#include "Helper.h"
#include "api/BamReader.h"
#include "Pileup.h"
#include "NGSHelper.h"
#include "BasicStatistics.h"
#include <QFileInfo>
#include <QTextStream>

using namespace BamTools;

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
		setDescription("Estimates the tumor content using the median of the top-n somatic varaints.");

		addInfile("tu", "Somatic variant list.", false);
		addInfile("tu_bam", "Tumor tissue BAM file.", false);
		addInfile("no_bam", "Normal tissue BAM file.", false);
		//optional
		addOutfile("out", "Output TXT file. If unset, writes to STDOUT.", true);
		addInt("min_depth", "Minmum depth in tumor and normal sample to consider a variant.", true, 30);
		addFloat("max_somatic", "Maximum frequency in normal sample to consider a variant somatic.", true, 0.01);
		addInt("n", "Minimal number of somatic autosomal heterocygous variants to calculate the tumor content.", true, 10);
	}

	virtual void main()
	{
		//init
		VariantList variants;
		variants.load(getInfile("tu"));
		BamReader tu_bam;
		NGSHelper::openBAM(tu_bam, getInfile("tu_bam"));
		BamReader no_bam;
		NGSHelper::openBAM(no_bam, getInfile("no_bam"));

		QSharedPointer<QFile> outfile = Helper::openFileForWriting(getOutfile("out"), true);
		QTextStream out(outfile.data());

		int min_depth = getInt("min_depth");
		double max_somatic = getFloat("max_somatic");
		int n = getInt("n");

		int count_snps = 0;
		int count_auto = 0;
		int count_depth = 0;
		int count_somatic = 0;
		int count_het = 0;

		//process variants
		QVector<double> freqs;
		for (int i=0; i<variants.count(); ++i)
		{
			const Variant& v = variants[i];

			if (!v.isSNV()) continue;
			++count_snps;

			if (!v.chr().isAutosome()) continue;
			++count_auto;

			Pileup pileup_tu = NGSHelper::getPileup(tu_bam, v.chr(), v.start());
			if (pileup_tu.depth(true) < min_depth) continue;

			Pileup pileup_no = NGSHelper::getPileup(no_bam, v.chr(), v.start());
			if (pileup_no.depth(true) < min_depth) continue;
			++count_depth;

			double no_freq = pileup_no.frequency(v.ref()[0], v.obs()[0]);
			if (!BasicStatistics::isValidFloat(no_freq) || no_freq >= max_somatic) continue;
			++count_somatic;

			double tu_freq = pileup_tu.frequency(v.ref()[0], v.obs()[0]);
			if (!BasicStatistics::isValidFloat(tu_freq) || tu_freq > 0.6) continue;
			++count_het;

			freqs.append(tu_freq);
		}

		//print sample names
		out << "somatic variant list  : " << QFileInfo(getInfile("tu")).fileName() << endl;
		out << "normal tissue BAM file: " << QFileInfo(getInfile("no_bam")).fileName() << endl;
		out << "tumor tissue BAM file : " << QFileInfo(getInfile("tu_bam")).fileName() << endl;
		out << endl;

		//sort data
		std::sort(freqs.begin(), freqs.end());
		QString freq_string = "  considered top-" + QString::number(n) + " variant frequencies of: ";
		foreach(double f, freqs)
		{
			freq_string.append(" " + QString::number(f, 'f', 2));
		}

		//print tumor content estimate
		if (freqs.count()>=n)
		{
			freqs = freqs.mid(freqs.count()-n);
			out << "estimated tumor content: " << QString::number(BasicStatistics::median(freqs, false)*200, 'f', 0) << "%" << endl;
		}
		else
		{
			out << "estimated tumor content: n/a (too few variants)" << endl;
		}

		//print processing info
		out << "" << endl;
		out << "INDELs" << endl;
		out << "  total: " << (variants.count() - count_snps) << endl;
		out << "SNPs" << endl;
		out << "  total: " << count_snps << endl;
		out << "  autosomal: " << count_auto << endl;
		out << "  with depth > " << min_depth << ": " << count_depth << endl;
		out << "  with normal sample variant frequency < " << max_somatic << ": " << count_somatic << endl;
		out << "  with tumor sample variant frequency <= 0.6: " << count_het << endl;
		out << freq_string;
	}
};

#include "main.moc"

int main(int argc, char *argv[])
{
	ConcreteTool tool(argc, argv);
	return tool.execute();
}
