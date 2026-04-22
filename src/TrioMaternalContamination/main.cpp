#include "ToolBase.h"
#include "VcfFile.h"
#include "BamReader.h"
#include "NGSHelper.h"

enum Member
{
	FATHER,
	MOTHER,
	CHILD
};

struct VariantInfo
{
	QString bam;
	QMap<QByteArray, double> variants;

	VariantInfo(){};
	VariantInfo(QString in_file_name_)
	{
		bam = in_file_name_;
	}
};

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
		setDescription("Detects maternal contamination of a child using SNPs from parents.");
		setExtendedDescription(QStringList() << "Determines the percentage of heterozygous SNPs passed on to the child from mother/father." << "This percentage should be similar for mother/father. If it is much higher for the mother, maternal contamination is likely.");
		addInfile("bam_m", "Input BAM/CRAM file of mother.", false);
		addInfile("bam_f", "Input BAM/CRAM file of father.", false);
		addInfile("bam_c", "Input BAM/CRAM file of child.", false);

		//optional
		addInt("min_depth", "Minimum depth for calling SNPs.", true, 3);
		addInt("min_alt_count", "Minimum number of alternate observations for calling a SNP.", true, 1);
		addEnum("build", "Genome build used to generate the input.", true, QStringList() << "hg19" << "hg38", "hg38");
		addOutfile("out", "Output file. If unset, writes to STDOUT.", true);
		addInfile("ref", "Reference genome for CRAM support (mandatory if CRAM is used).", true);

		changeLog(2025,   5, 30, "Code refactoring and speed-up.");
		changeLog(2020,  11, 27, "Added CRAM support.");
		changeLog(2020,   6,  18, "Initial version of the tool.");
	}

	virtual void main()
    {
		QTextStream out(stdout);

        //init
		QString bam_m = getInfile("bam_m");
		QString bam_f = getInfile("bam_f");
		QString bam_c = getInfile("bam_c");
		const QString ref_file = getInfile("ref");
		int min_depth = getInt("min_depth");
		int min_alt_count = getInt("min_alt_count");

		GenomeBuild build = stringToBuild(getEnum("build"));
		VcfFile snps = NGSHelper::getKnownVariants(build, true);
		QSet<QByteArray> homozygous_variants;

		//get relevant SNPs allele frequencies from BAMs
		QMap<Member, VariantInfo> trio;
		trio.insert(Member::MOTHER, VariantInfo(bam_m));
		trio.insert(Member::FATHER, VariantInfo(bam_f));
		trio.insert(Member::CHILD, VariantInfo(bam_c));
		for(auto it=trio.begin(); it!=trio.end(); ++it)
		{
			BamReader reader(it.value().bam, ref_file);
			for (int i=0; i<snps.count(); ++i)
			{
				const  VcfLine& v = snps[i];
				if (!v.isSNV(true)) continue;
				if (!v.chr().isAutosome()) continue;

				//keep only variants of minimum depth
				Pileup pileup_tu = reader.getPileup(v.chr(), v.start());
				if (pileup_tu.depth(true) < min_depth) continue;

				for(const Sequence& alt : v.alt())
				{
					long long count = pileup_tu.countOf(alt[0]);
					double frequency = pileup_tu.frequency(v.ref()[0], alt[0]);

					//do not keep homozygous variants
					QByteArray tag = v.toString();
					if(frequency==1)
					{
						homozygous_variants.insert(tag);
						continue;
					}
					if(homozygous_variants.contains(tag)) continue;

					//only keep variants with a minimum base count
					if(count < min_alt_count) continue;

					it.value().variants[v.toString()] = frequency;
				}
			}
		}

		//delete homozygous variants from all samples
		for(auto it=trio.begin(); it!=trio.end(); ++it)
		{
			for (auto variant_it = it.value().variants.begin(); variant_it != it.value().variants.end(); )
			{
				if(homozygous_variants.contains(variant_it.key()))
				{
					variant_it = it.value().variants.erase(variant_it);
				}
				else
				{
					++variant_it;
				}
			}
		}

		//count mother variants
		double mother_variants = 0;
		double variants_of_mother_in_child = 0;
		for(auto it=trio[Member::MOTHER].variants.begin(); it!=trio[Member::MOTHER].variants.end(); ++it)
		{
			if(trio[Member::FATHER].variants.contains(it.key())) continue;
			++mother_variants;

			if(trio[Member::CHILD].variants.contains(it.key()))
			{
				++variants_of_mother_in_child;
			}
		}
		double perc_mother = 100.0 * variants_of_mother_in_child / mother_variants;

		//count father variants
		double father_variants = 0;
		double variants_of_father_in_child = 0;
		for(auto it=trio[Member::FATHER].variants.begin(); it!=trio[Member::FATHER].variants.end(); ++it)
		{
			if(trio[Member::MOTHER].variants.contains(it.key())) continue;
			++father_variants;

			if(trio[Member::CHILD].variants.contains(it.key()))
			{
				++variants_of_father_in_child;
			}
		}
		double perc_father = 100.0 * variants_of_father_in_child / father_variants;

		//write output
		QSharedPointer<QFile> file = Helper::openFileForWriting(getOutfile("out"), true);
		QTextStream stream(file.data());
		stream << "Variants in mother: " << mother_variants << "\n"
			   << "Percentage of variants from mother passed to child: " << QString::number(perc_mother, 'f', 2) << "\n"
			   << "Variants in father: " << father_variants << "\n"
			   << "Percentage of variants from father passed to child: " << QString::number(perc_father, 'f', 2) << "\n"
			   << "Ratio mother/father: " << QString::number(perc_mother/perc_father, 'f', 2) << "\n";
	}
};

#include "main.moc"

int main(int argc, char *argv[])
{
	ConcreteTool tool(argc, argv);
	return tool.execute();
}

