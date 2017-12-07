#include "ToolBase.h"
#include "VariantList.h"
#include "VariantFilter.h"
#include "Helper.h"
#include "NGSHelper.h"
#include "BasicStatistics.h"

#include <QTextStream>
#include <QFileInfo>
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
		addInfile("in", "Input variant list in VCF or GSvar format.", false);
		addOutfile("out", "Output TSV file with ROH regions.", false);
		//optional
		addInfileList("annotate", "List of BED files used for annotation. Each file adds a column to the output file. The base filename is used as colum name and 4th column of the BED file is used as annotation value.", true);
		addInt("var_min_dp", "Minimum variant depth ('DP'). Variants with lower depth are excluded from the analysis.", true, 20);
		addFloat("var_min_q", "Minimum variant quality. Variants with lower depth are excluded from the analysis.", true, 30);
		addString("var_af_keys", "Annotation keys of allele frequency values (comma-separated).", true, "GNOMAD_AF,T1000GP_AF,EXAC_AF");
		addFloat("roh_min_q", "Minimum Q score of ROH regions.", true, 30.0);
		addInt("roh_min_markers", "Minimum marker count of ROH regions.", true, 20);
		addFloat("roh_min_size", "Minimum size in Kb of ROH regions.", true, 20.0);
		addFloat("ext_merker_perc", "Percentage of ROH markers that can be spanned when merging ROH regions .", true, 1.0);
		addFloat("ext_size_perc", "Percentage of ROH size that can be spanned when merging ROH regions.", true, 50.0);
		addFlag("inc_chrx", "Include chrX into the analysis. Excluded by default.");

		changeLog(2017, 11, 29, "Added 'inc_chrx' flag.");
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

		QStringList annotations;

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

			RohRegion region{var_info[start].chr, var_info[start].pos, var_info[end].pos, start, end, 0, QStringList()};
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
		bool inc_chrx = getFlag("inc_chrx");

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

			//skip gonosomes
			if (!v.chr().isAutosome() && !(inc_chrx && v.chr().isX()))
			{
				continue;
			}

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

		//filter regions
		int roh_min_markers = getInt("roh_min_markers");
		auto it = std::remove_if(regions.begin(), regions.end(), [roh_min_markers](const RohRegion& reg){return reg.sizeMarkers()<roh_min_markers;});
		regions.erase(it, regions.end());
		float roh_min_size = getFloat("roh_min_size") * 1000.0;
		it = std::remove_if(regions.begin(), regions.end(), [roh_min_size](const RohRegion& reg){return reg.sizeBases()<roh_min_size;});
		regions.erase(it, regions.end());

		//annotate regions
		QStringList annotate = getInfileList("annotate");
		foreach(QString anno, annotate)
		{
			BedFile anno_file;
			anno_file.load(anno);
			anno_file.sort();
			ChromosomalIndex<BedFile> anno_index(anno_file);
			for (int i=0; i<regions.count(); ++i)
			{
				QSet<QString> annos;
				QVector<int> indices = anno_index.matchingIndices(regions[i].chr, regions[i].start_pos, regions[i].end_pos);
				foreach(int index, indices)
				{
					if (anno_file[index].annotations().isEmpty())
					{
						annos.insert("yes");
					}
					else
					{
						annos.insert(anno_file[index].annotations()[0]);
					}
				}
				QStringList annos_sorted = annos.toList();
				std::sort(annos_sorted.begin(), annos_sorted.end());
				regions[i].annotations << annos_sorted.join(',');
			}
		}

		//ROH output
		QSharedPointer<QFile> file = Helper::openFileForWriting(getOutfile("out"));
		QTextStream outstream(file.data());
		outstream << "#chr\tstart\tend\tnumber of markers\thet markers\tsize [kB]\tQ score";
		foreach(QString anno, annotate)
		{
			outstream << '\t' << QFileInfo(anno).baseName();
		}
		outstream << '\n';
		double size_sum = 0.0;
		foreach(const RohRegion& reg, regions)
		{
			size_sum += reg.sizeBases();

			outstream << reg.chr.str() << '\t';
			outstream << QString::number(reg.start_pos) << '\t';
			outstream << QString::number(reg.end_pos) << '\t';
			outstream << QString::number(reg.sizeMarkers()) << '\t';
			outstream << QString::number(reg.het_count) << '\t';
			outstream << QString::number(reg.sizeBases()/1000.0, 'f', 2) << '\t';
			outstream << QString::number(reg.qScore(var_info), 'f', 2);
			if (annotate.count())
			{
				outstream << '\t' << reg.annotations.join("\t");
			}
			outstream << '\n';
		}

		//statistics output
		out << "=== Statistics output ===" << endl;
		out << "ROH count after filters: " << regions.count() << endl;
		out << "Overall ROH size sum: " << QString::number(size_sum/1000000.0 ,'f', 2) << "Mb" << endl;
	}
};

#include "main.moc"

int main(int argc, char *argv[])
{
    ConcreteTool tool(argc, argv);
    return tool.execute();
}

