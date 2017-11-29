#include "ToolBase.h"
#include "VariantList.h"
#include "VariantFilter.h"
#include "Helper.h"
#include "NGSHelper.h"
#include "BasicStatistics.h"

#include <QTextStream>
#include <QDebug>
#include <cmath>

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
		setDescription("ROH detection based on a variant list annotated with AF values.");
		addInfile("in", "Input variant list in VCF or GSvar format.", true);
		addOutfile("out", "Output TSV file with ROH regions.", true);
		//optional
		addInt("var_min_dp", "Minimum variant depth ('DP'). Variants with lower depth are excluded from the analysis.", true, 20);
		addFloat("var_min_q", "Minimum variant quality. Variants with lower depth are excluded from the analysis.", true, 30);
		addString("var_af_keys", "Annotation keys of allele frequency values (comma-separated).", true, "GNOMAD_AF,T1000GP_AF,EXAC_AF");
		addFloat("roh_min_q", "Minimum Q score of ROH regions.", true, 30.0);
		addInt("roh_min_markers", "Minimum marker count of ROH regions.", true, 20);
		addFloat("roh_min_size", "Minimum size in Kb of ROH regions.", true, 20.0);
		addFloat("ext_merker_perc", "Percentage of ROH markers that can be spanned when merging ROH regions .", true, 1.0);
		addFloat("ext_size_perc", "Percentage of ROH size that can be spanned when merging ROH regions.", true, 50.0);

		changeLog(2017, 11, 21, "First version.");
	}

	//Input data struct of the algorithm
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

	//Output data struct of the algorithm
	struct RohRegion
	{
		Chromosome chr;
		int start_pos;
		int end_pos;

		int start_index;
		int end_index;

		int het_count;

		//returns the region
		QString toString() const
		{
			return chr.str() + ":" + QString::number(start_pos) + "-" + QString::number(end_pos);
		}

		//returns the marker count
		int sizeMarkers() const
		{
			return end_index-start_index+1;
		}

		//returns the size in bases
		int sizeBases() const
		{
			return end_pos-start_pos;
		}

		//returns the probability to observe the event as Q score
		double qScore(const QList<VariantInfo>& var_info) const
		{
			double p = 1.0;
			for (int i=start_index; i<=end_index; ++i)
			{
				p *= std::pow(var_info[i].af, 2);
			}

			//max of 10000, also because of numeric overflow
			double q_score = -10.0 * log10(p);
			if (q_score>10000.0)
			{
				q_score = 10000;
			}
			return q_score;
		}
	};

	//raw ROH detection
	static QList<RohRegion> calculateRawRohs(const QList<VariantInfo>& var_info, double roh_min_q)
	{
		QList<RohRegion> output;
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

			RohRegion region{var_info[start].chr, var_info[start].pos, var_info[end].pos, start, end, 0};
			if (region.qScore(var_info)>=roh_min_q)
			{
				output << region;
			}
		}

		return output;
	}

	//ROH merging
	void mergeRohs(QList<RohRegion>& raw, double ext_merker_perc, double ext_size_perc)
	{
		bool merged = true;
		while(merged)
		{
			merged = false;
			for (int i=0; i<raw.count()-1; ++i)
			{
				//same chr
				if (raw[i].chr!=raw[i+1].chr) continue;

				//not too far apart (markers)
				int index_diff = raw[i+1].start_index-raw[i].end_index-1;
				if (index_diff>1 && index_diff > ext_merker_perc / 100.0 * (raw[i].sizeMarkers() + raw[i+1].sizeMarkers())) continue;

				//not too far apart (bases)
				if (raw[i+1].start_pos - raw[i].end_pos > ext_size_perc / 100.0 * (raw[i].sizeBases() + raw[i+1].sizeBases())) continue;

				//merge
				raw[i].end_index = raw[i+1].end_index;
				raw[i].end_pos = raw[i+1].end_pos;
				raw[i].het_count += raw[i+1].het_count + index_diff;

				raw.removeAt(i+1);
				i-=1;
				merged = true;
			}
		}
	}

	virtual void main()
	{
		//init
		QTextStream out(stdout);

		//load variant list
		VariantList vl;
		vl.load(getInfile("in"));

		out << "=== Loading input data ===" << endl;
		out << "Variants in VCF: " << vl.count() << endl;

		//determine required annotation indices
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
		QStringList var_af_keys = getString("var_af_keys").split(",", QString::SkipEmptyParts);
		QVector<int> var_af_indices;
		foreach(QString key, var_af_keys)
		{
			var_af_indices << vl.annotationIndexByName(key);
		}

		//convert variant list to data structure
		int var_min_dp = getInt("var_min_dp");
		float var_min_q = getFloat("var_min_q");
		int vars_hom = 0;
		int vars_known = 0;
		int i_gt = vl.annotationIndexByName("GT");
		QList<VariantInfo> var_info;
		for(int i=0; i<vl.count(); ++i)
		{
			const Variant& v = vl[i];

			//skip low quality variants
			bool ok = true;
			if (vl[i].annotations().at(idx_dp).toInt(&ok) < var_min_dp) continue;
			if (!ok) THROW(ArgumentException, "Could not convert 'DP' value of variant " + v.toString() + " to integer.");
			if (vl[i].annotations().at(idx_qual).toDouble(&ok) < var_min_q) continue;
			if (!ok) THROW(ArgumentException, "Could not convert 'QUAL' value of variant " + v.toString() + " to double.");

			//determine if homozygous
			QByteArray genotype = vl[i].annotations().at(i_gt);
			bool geno_hom = (genotype=="1/1" || genotype=="1|1");
			if (geno_hom) ++vars_hom;

			//determine database AF
			bool var_known = false;
			float af = 0.01;
			foreach(int index, var_af_indices)
			{
				float af_new = v.annotations()[index].toFloat(&ok);
				if (!ok) continue;
				af = std::max(af, af_new);
				if (af_new>0.0) var_known = true;
			}
			if (var_known) ++vars_known;

			var_info.append(VariantInfo{v.chr(), v.start(), geno_hom, af});
		}
		out << "Variants passing QC filters: " << var_info.count() << endl;
		double hom_perc = 100.0*vars_hom/var_info.count();
		out << "Variants homozygous: " << QByteArray::number(hom_perc, 'f', 2) << "%" << endl;
		out << "Variants with AF annoation: " << QByteArray::number(100.0*vars_known/var_info.count(), 'f', 2) << "%" << endl;
		out << endl;

		//detect raw ROHs
		out << "=== Detecting ROHs ===" << endl;
		float roh_min_q = getFloat("roh_min_q");
		QList<RohRegion> regions = calculateRawRohs(var_info, roh_min_q);
		out << "Raw ROH count: " << regions.count() << endl;

		//merge raw ROHs
		double ext_merker_perc = getFloat("ext_merker_perc");
		double ext_size_perc = getFloat("ext_size_perc");
		mergeRohs(regions, ext_merker_perc, ext_size_perc);
		out << "Merged ROH count: " << regions.count() << endl;
		out << endl;

		//filter and write output
		int roh_min_markers = getInt("roh_min_markers");
		float roh_min_size = getFloat("roh_min_size");
		double size_sum_kb = 0.0;
		QStringList output;
		output << "#chr\tstart\tend\tnumber of markers\thet markers\tsize [kB]\tQ score";
		foreach(const RohRegion& reg, regions)
		{
			int markers = reg.sizeMarkers();
			if (markers<roh_min_markers) continue;
			double size_kb = reg.sizeBases()/1000.0;
			if (size_kb<roh_min_size) continue;

			size_sum_kb += size_kb;

			QString line;
			line += reg.chr.str() + "\t";
			line += QString::number(reg.start_pos) + "\t";
			line += QString::number(reg.end_pos) + "\t";
			line += QString::number(markers) + "\t";
			line += QString::number(reg.het_count) + "\t";
			line += QString::number(size_kb, 'f', 2) + "\t";
			line += QString::number(reg.qScore(var_info), 'f', 2);
			output << line;
		}
		Helper::storeTextFile(getOutfile("out"), output);
		out << "=== Statistics output ===" << endl;
		out << "ROH count after filters: " << (output.count()-1) << endl;
		out << "Overall ROH size sum: " << QString::number(size_sum_kb/1000.0 ,'f', 2) << "Mb" << endl;
	}
};

#include "main.moc"

int main(int argc, char *argv[])
{
    ConcreteTool tool(argc, argv);
    return tool.execute();
}

