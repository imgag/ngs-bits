#include "ToolBase.h"
#include "VariantList.h"
#include "VariantFilter.h"
#include "Helper.h"

#include <QTextStream>
#include <QDebug>

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
		setDescription("ROH detection based on a variant list.");
		addInfile("in", "Input variant list in VCF or GSvar format.", true);
		addInfile("roi", "Target region BED file with regions where variant calling was performed to generate the 'in' file.", true);
		addOutfile("out", "Output TSV file with ROH regions.", true);

		//optional
		addInt("min_dp", "Minimum depth. Variants with lower depth are excluded from the analysis.", true, 20);
		addFloat("min_q", "Minimum quality score. Variants with lower depth are excluded from the analysis.", true, 30);

		//TODO changeLog(2017, 11, 20, "First version.");
	}

	//Input data struct
	struct VariantInfo
	{
		//position
		Chromosome chr;
		int pos;

		//genotype
		bool geno_hom;

		//allele frequency in public databases
		float af;
	};

	//algorithm implementation
	static QStringList calculate(const QList<VariantInfo>& var_info)
	{
		QStringList output;

		return output;
	}

/*
TODO:
- special handling gonosomes
- shrink test data (a few chromosomes only?) and test ROI
- optimize quality cutoffs based on variants that are het on chrX of males (AF, DP, MQM, ...)
*/

	virtual void main()
	{
		//init
		QTextStream out(stdout);
		int min_dp = getInt("min_dp");
		float min_q = getFloat("min_q");

		//load variant list
		VariantList vl;
		vl.load(getInfile("in"));
		out << "Input variants: " << vl.count() << endl;

		//filter by target region (to get rid of off-target calls)
		BedFile roi;
		roi.load(getInfile("roi"));
		VariantFilter roi_filter(vl);
		roi_filter.flagByRegions(roi);
		roi_filter.removeFlagged();
		out << "Input variants in ROI: " << vl.count() << endl;

		//convert to datastructure used in algorithm
		int i_qual = vl.annotationIndexByName("QUAL");
		int i_dp = -1;
		for(int i=0; i<vl.annotationDescriptions().count(); ++i)
		{
			if(vl.annotationDescriptions()[i].sampleSpecific() && vl.annotationDescriptions()[i].name()=="DP")
			{
				i_dp = i;
			}
		}
		if (i_dp==-1) THROW(ArgumentException, "Could not find 'DP' annotation in variant list!");


		int i_gt = vl.annotationIndexByName("GT");
		QList<VariantInfo> var_info;
		for(int i=0; i<vl.count(); ++i)
		{
			const Variant& v = vl[i];

			//skip low quality variants
			bool ok = true;
			if (vl[i].annotations().at(i_dp).toInt(&ok) < min_dp) continue;
			if (!ok) THROW(ArgumentException, "Could not convert 'DP' value of variant " + v.toString() + " to integer");
			if (vl[i].annotations().at(i_qual).toDouble(&ok) < min_q) continue;
			if (!ok) THROW(ArgumentException, "Could not convert 'QUAL' value of variant " + v.toString() + " to double");

			//determine if homozygous
			QByteArray genotype = vl[i].annotations().at(i_gt);
			bool geno_hom = (genotype=="1/1" || genotype=="1|1");

			//determine database AF
			float af = 0.5;

			var_info.append(VariantInfo{v.chr(), v.start(), geno_hom, af});
		}
		out << "Variants passing QC filters: " << var_info.count() << endl;


		//find ROH regions
		QStringList output = calculate(var_info);

		//write output
		Helper::storeTextFile(getOutfile("out"), output);

	}
};

#include "main.moc"

int main(int argc, char *argv[])
{
    ConcreteTool tool(argc, argv);
    return tool.execute();
}

