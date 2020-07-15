#include "ToolBase.h"
#include "VcfFileHandler.h"
#include "Helper.h"

#include <QTextStream>
#include <QFileInfo>
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
		setDescription("ROH detection based on a variant list.");
		setExtendedDescription(QStringList() << "Runs of homozygosity (ROH) are detected based on the genotype annotations in the VCF file."
												"Based on the allele frequency of the contained variants, each ROH is assigned an estimated likelyhood to be observed by chance (Q score).");
		addInfile("in", "Input variant list in VCF format.", false);
		addOutfile("out", "Output TSV file with ROH regions.", false);
		//optional
		addInfileList("annotate", "List of BED files used for annotation. Each file adds a column to the output file. The base filename is used as colum name and 4th column of the BED file is used as annotation value.", true);
		addInt("var_min_dp", "Minimum variant depth ('DP'). Variants with lower depth are excluded from the analysis.", true, 20);
		addFloat("var_min_q", "Minimum variant quality. Variants with lower quality are excluded from the analysis.", true, 30);
		addString("var_af_keys", "Comma-separated allele frequency info field names in 'in'.", true, "");
		addString("var_af_keys_vep", "Comma-separated VEP CSQ field names of allele frequency annotations in 'in'.", true, "");
		addFloat("roh_min_q", "Minimum Q score of output ROH regions.", true, 30.0);
		addInt("roh_min_markers", "Minimum marker count of output ROH regions.", true, 20);
		addFloat("roh_min_size", "Minimum size in Kb of output ROH regions.", true, 20.0);
		addFloat("ext_marker_perc", "Percentage of ROH markers that can be spanned when merging ROH regions .", true, 1.0);
		addFloat("ext_size_perc", "Percentage of ROH size that can be spanned when merging ROH regions.", true, 50.0);
		addFlag("inc_chrx", "Include chrX into the analysis. Excluded by default.");

		changeLog(2019, 11, 21, "Added support for parsing AF data from any VCF info field (removed 'af_source' parameter).");
		changeLog(2019,  3, 12, "Added support for input variant lists that are not annotated with VEP. See 'af_source' parameter.");
		changeLog(2018,  9, 12, "Now supports VEP CSQ annotations (no longer support SnpEff ANN annotations).");
		changeLog(2017, 12, 07, "Added generic annotation feature.");
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
	void mergeRohs(QList<RohRegion>& raw, const QList<VariantInfo>& var_info, double ext_marker_perc, double ext_size_perc)
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
				int het_count = 0;
				for (int j=raw[i].end_index+1; j<raw[i+1].start_index; ++j)
				{
					het_count += !var_info[j].geno_hom;
				}
				if (het_count>1 && het_count > ext_marker_perc / 100.0 * (raw[i].sizeMarkers() + raw[i+1].sizeMarkers())) continue;

				//not too far apart (bases)
				if (raw[i+1].start_pos - raw[i].end_pos > ext_size_perc / 100.0 * (raw[i].sizeBases() + raw[i+1].sizeBases())) continue;

				//merge
				raw[i].end_index = raw[i+1].end_index;
				raw[i].end_pos = raw[i+1].end_pos;
				raw[i].het_count += raw[i+1].het_count + het_count;

				raw.removeAt(i+1);
				i-=1;
				merged = true;
			}
		}
	}

	virtual void main()
	{
		//init
		QTime timer;
		timer.start();
		QTextStream out(stdout);
		bool inc_chrx = getFlag("inc_chrx");

		//load variant list
		 VcfFileHandler vl;
		vl.load(getInfile("in"));

		out << "=== Loading input data ===" << endl;
		out << "Variants in VCF: " << vl.count() << endl;

		//determine quality indices
		if (!vl.formatIDs().contains("DP")) THROW(ArgumentException, "Could not find 'DP' annotation in vcf header!");

		QVector<int> csq_af_indices;
		QByteArray tmp = getString("var_af_keys_vep").toLatin1().trimmed();
		if (!tmp.isEmpty())
		{
			QByteArrayList var_af_keys_vep =tmp.split(',');
			foreach(const QByteArray& key, var_af_keys_vep)
			{
				csq_af_indices << vl.vcfHeader().vepIndexByName(key);
			}
		}

		//convert variant list to data structure
		int var_min_dp = getInt("var_min_dp");
		float var_min_q = getFloat("var_min_q");
		int vars_hom = 0;
		int vars_known = 0;
		QList<VariantInfo> var_info;
		for(int i=0; i<vl.count(); ++i)
		{
			const  VCFLine& v = vl[i];

			//skip gonosomes
			if (!v.chr().isAutosome() && !(inc_chrx && v.chr().isX()))
			{
				continue;
			}

			//skip low quality variants
			bool ok = true;
			int dp_value = v.sample(0, "DP").toInt(&ok);
			if (!ok) THROW(ArgumentException, "Could not convert 'DP' value of variant " + v.variantToString() + " to integer.");
			if (dp_value < var_min_dp) continue;
			int qual_value = v.qual();
			if (qual_value < var_min_q) continue;

			//determine if homozygous
			QByteArray genotype = v.sample(0, "GT");
			bool geno_hom = (genotype=="1/1" || genotype=="1|1");
			if (geno_hom) ++vars_hom;

			//determine database AF
			bool var_known = false;
			float af = 0.01;

			tmp = getString("var_af_keys").toLatin1().trimmed();
			QByteArrayList var_af_keys = tmp.split(',');
			foreach(const QByteArray& var, var_af_keys)
			{
				bool ok = false;
				float af_new = v.info(var).toFloat(&ok);
				if (!ok) continue;
				if (af_new>0.0) var_known = true;
				af = std::max(af, af_new);
			}

			foreach(int index, csq_af_indices)
			{
				QByteArrayList annos = v.vepAnnotations(index);
				foreach(const QByteArray& anno, annos)
				{
					float af_new = anno.toFloat();
					if (af_new>0.0) var_known = true;
					af = std::max(af, af_new);
				}
			}

			if (var_known) ++vars_known;

			var_info.append(VariantInfo{v.chr(), v.start(), geno_hom, af});
		}
		out << "Variants passing QC filters: " << var_info.count() << endl;
		double hom_perc = 100.0*vars_hom/var_info.count();
		out << "Variants homozygous: " << QByteArray::number(hom_perc, 'f', 2) << "%" << endl;
		out << "Variants with AF annoation greater zero: " << QByteArray::number(100.0*vars_known/var_info.count(), 'f', 2) << "%" << endl;
		out << endl;

		//detect raw ROHs
		out << "=== Detecting ROHs ===" << endl;
		float roh_min_q = getFloat("roh_min_q");
		QList<RohRegion> regions = calculateRawRohs(var_info, roh_min_q);
		out << "Raw ROH count: " << regions.count() << endl;

		//merge raw ROHs
		double ext_marker_perc = getFloat("ext_marker_perc");
		double ext_size_perc = getFloat("ext_size_perc");
		mergeRohs(regions, var_info, ext_marker_perc, ext_size_perc);
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
			if (!anno_file.isSorted()) anno_file.sort();
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
		outstream << "#chr\tstart\tend\tnumber of markers\thet markers\tsize [Kb]\tQ score";
		foreach(QString anno, annotate)
		{
			outstream << '\t' << QFileInfo(anno).baseName();
		}
		outstream << '\n';
		foreach(const RohRegion& reg, regions)
		{
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
		out << "Overall ROH count: " << regions.count() << endl;
		int count_a = 0;
		double sum_a = 0.0;
		int count_b = 0;
		double sum_b = 0.0;
		int count_c = 0;
		double sum_c = 0.0;
		foreach(const RohRegion& reg, regions)
		{
			int bases = reg.sizeBases();
			if (bases <500000)
			{
				++count_a;
				sum_a += bases;
			}
			else if (bases <1500000)
			{
				++count_b;
				sum_b += bases;
			}
			else
			{
				++count_c;
				sum_c += bases;
			}
		}
		out << "Overall ROH size sum: " << QString::number((sum_a+sum_b+sum_c)/1000000.0 ,'f', 2) << "Mb" << endl;
		out << "Class A: <0.5 Mb" << endl;
		out << "Class A ROH count: " << count_a << endl;
		out << "Class A ROH size sum: " << QString::number(sum_a/1000000.0 ,'f', 2) << "Mb" << endl;
		out << "Class B: >=0.5 Mb and <1.5 Mb" << endl;
		out << "Class B ROH count: " << count_b << endl;
		out << "Class B ROH size sum: " << QString::number(sum_b/1000000.0 ,'f', 2) << "Mb" << endl;
		out << "Class C: >=1.5 Mb" << endl;
		out << "Class C ROH count: " << count_c << endl;
		out << "Class C ROH size sum: " << QString::number(sum_c/1000000.0 ,'f', 2) << "Mb" << endl;
		out << endl;


		//debug output
		out << "=== Debug output ===" << endl;
		out << "Time: " << Helper::elapsedTime(timer) << endl;
	}
};

#include "main.moc"

int main(int argc, char *argv[])
{
    ConcreteTool tool(argc, argv);
    return tool.execute();
}

