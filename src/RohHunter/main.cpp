#include "ToolBase.h"
#include "VariantList.h"
#include "VariantFilter.h"
#include "Helper.h"
#include "NGSHelper.h"

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
		QTextStream out(stdout); //TODO

		QStringList output;
		const int count = var_info.count();
		int last_end = -1;
		while(true)
		{
			int start=last_end+1;
			while (start<count && !var_info[start].geno_hom)
			{
				++start;
			}
			if (start>=count) break;

			int end=start;
			while(end<count && var_info[end].geno_hom && var_info[end].chr==var_info[start].chr)
			{
				++end;
			}
			--end;

			last_end = end;
			double p = 1.0;
			for (int i=start; i<=end; ++i)
			{
				p *= var_info[i].af * var_info[i].af;
			}
			out << var_info[end].chr.str() << ":" << var_info[start].pos << "-" << var_info[end].pos << "\t" << (end-start+1) << "\t" << (var_info[end].pos-var_info[start].pos) << "\t" << (-10.0 * log10(p)) << endl;
		}

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
		out << "Variants in VCF: " << vl.count() << endl;

		//filter by target region (to get rid of off-target calls)
		BedFile roi;
		roi.load(getInfile("roi"));
		VariantFilter roi_filter(vl);
		roi_filter.flagByRegions(roi);
		roi_filter.removeFlagged();
		out << "Variants inside ROI: " << vl.count() << endl;

		//convert to datastructure used in algorithm
		int idx_qual = vl.annotationIndexByName("QUAL");
		int idx_dp = -1;
		for(int i=0; i<vl.annotationDescriptions().count(); ++i)
		{
			if(vl.annotationDescriptions()[i].sampleSpecific() && vl.annotationDescriptions()[i].name()=="DP")
			{
				idx_dp = i;
			}
		}
		if (idx_dp==-1) THROW(ArgumentException, "Could not find 'DP' annotation in variant list!");

		//load known variant list
		VariantList kv = NGSHelper::getKnownVariants(false, 0.0, 1.0, &roi);
		ChromosomalIndex<VariantList> kv_index(kv);
		int idx_af_kv = kv.annotationIndexByName("AF");

		//convert variant list to data structure
		int vars_hom = 0;
		int vars_known = 0;
		int i_gt = vl.annotationIndexByName("GT");
		QList<VariantInfo> var_info;
		for(int i=0; i<vl.count(); ++i)
		{
			const Variant& v = vl[i];

			//skip low quality variants
			bool ok = true;
			if (vl[i].annotations().at(idx_dp).toInt(&ok) < min_dp) continue;
			if (!ok) THROW(ArgumentException, "Could not convert 'DP' value of variant " + v.toString() + " to integer.");
			if (vl[i].annotations().at(idx_qual).toDouble(&ok) < min_q) continue;
			if (!ok) THROW(ArgumentException, "Could not convert 'QUAL' value of variant " + v.toString() + " to double.");

			//determine if homozygous
			QByteArray genotype = vl[i].annotations().at(i_gt);
			bool geno_hom = (genotype=="1/1" || genotype=="1|1");
			if (geno_hom) ++vars_hom;

			//determine database AF
			float af = 0.01;
			QVector<int> indices = kv_index.matchingIndices(v.chr(), v.start()-1, v.end()+1);
			foreach(int index, indices)
			{
				if (kv[index].start()==v.start() && kv[index].ref()==v.ref() && kv[index].obs()==v.obs())
				{
					af = std::max(af, kv[index].annotations()[idx_af_kv].toFloat(&ok));
					if (!ok) THROW(ProgrammingException, "Could not convert 'AF' value of known variant " + kv[index].toString() + " to double.");
					++vars_known;
				}
			}

			var_info.append(VariantInfo{v.chr(), v.start(), geno_hom, af});
		}
		out << "Variants passing QC filters: " << var_info.count() << endl;
		out << endl;
		out << "Variants homozygous: " << QByteArray::number(100.0*vars_hom/var_info.count(), 'f', 2) << "%" << endl;
		out << "Variants known: " << QByteArray::number(100.0*vars_known/var_info.count(), 'f', 2) << "%" << endl;

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

