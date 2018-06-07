#include "ToolBase.h"
#include "Helper.h"
#include "Exceptions.h"
#include "VariantList.h"
#include "BasicStatistics.h"
#include <QTextStream>

//TODO: remove MHC region? (chr6:28,477,797-33,448,354)
//TODO: also support chrX

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
		setDescription("UPD detection for trio variant data.");
		addInfile("in", "Input VCF file of trio.", false, true);
		addString("c", "Header name of child.", false);
		addString("f", "Header name of father.", false);
		addString("m", "Header name of mother.", false);
		addOutfile("out", "Output TSV file containing the detected UPDs.", false, true);
		//optional
		addInfile("exclude", "BED file with regions to exclude, e.g. copy-number variant regions.", true);
		addInt("var_min_dp", "Minimum depth (DP) of a variant (in all three samples).", true, 20);
		addFloat("var_min_q", "Minimum quality (QUAL) of a variant.", true, 30);
		addFlag("use_indels", "Also use InDels. The default is to use SNVs only.");
		addFloat("reg_min_kb", "Mimimum size in kilo-bases required for a UPD region.",  true, 100.0);
		addInt("reg_min_markers", "Mimimum number of UPD markers required in a region.",  true, 5);

		changeLog(2018,  6,  6, "Initial import.");
	}

	enum Genotype
	{
		HOM,
		HET,
		WT
	};

	enum UpdType
	{
		EXCLUDED,
		BIPARENTAL,
		UNINFORMATIVE,
		ISO,
		ISO_OR_HET
	};

	enum UpdSource
	{
		NONE,
		FATHER,
		MOTHER
	};

	struct VariantData
	{
		Chromosome chr;
		int start;
		int end;
		QByteArray ref;
		QByteArray obs;
		Genotype c;
		Genotype f;
		Genotype m;
		UpdType type = UNINFORMATIVE;
		UpdSource source = NONE;

		void determineType()
		{
			if ((c==HET && f==HOM &&  m==WT) || (c==HET && f==WT && m==HOM))
			{
				type = BIPARENTAL;
				source = NONE;
			}

			if ((c==HOM && f==HET && m==WT) || (c==WT && f==HET && m==HOM))
			{
				type = ISO;
				source = FATHER;
			}
			if ((c==HOM && f==WT &&  m==HET) || (c==WT && f==HOM &&  m==HET))
			{
				type = ISO;
				source = MOTHER;
			}

			if ((c==HOM && f==WT &&  m==HOM) || (c==WT && f==HOM &&  m==WT))
			{
				type = ISO_OR_HET;
				source = MOTHER;
			}
			if ((c==HOM && f==HOM &&  m==WT) || (c==WT && f==WT &&  m==HOM))
			{
				type = ISO_OR_HET;
				source = FATHER;
			}
		}
	};

	struct UpdRange
	{
		QList<VariantData>::const_iterator start;
		QList<VariantData>::const_iterator end;

		QByteArray sourceAsString() const
		{
			if(start->source==FATHER) return "father";
			if(start->source==MOTHER) return "mother";

			THROW(ArgumentException, "Invalid source NONE!");
		}

		QByteArray typeAsString() const
		{
			int c_iso = 0;
			for (auto it=start; it<end; ++it)
			{
				if (it->type==ISO) ++c_iso;
			}

			return c_iso>0 ? "ISO" : "ISO_OR_HET";
		}

		int sizeMarkers() const
		{
			return end-start;
		}

		int sizeBases() const
		{
			return (end-1)->end-start->start;
		}

		int countType(UpdType type) const
		{
			int c = 0;
			for (auto it=start; it<end; ++it)
			{
				if (it->type==type) ++c;
			}
			return c;
		}

		double qScore(double p_biparental, double p_upd, QTextStream& stream) const
		{
			stream << "RANGE " << sizeMarkers() << endl;
			int c_upd = countType(ISO) + countType(ISO_OR_HET);
			stream << "  c_upd=" << c_upd << endl;

			//likelyhood to find n variants without seeing any biparental inheritance
			int markers = sizeMarkers();
			double p_no_bip = std::pow(1-p_biparental, markers);
			stream << "  p_no_bip=" << p_no_bip << endl;

			//likelyhood to find n-1 UPD variants
			double p_n_upd = c_upd<2 ? 1.0 : BasicStatistics::matchProbability(p_upd, c_upd-1, markers-1);
			stream << "  p_n_upd=" << p_n_upd << endl;

			//calculate overall likelyhood
			double p = p_no_bip * p_n_upd;
			stream << "  p=" << p << endl;

			double q_score = -10.0 * std::log10(p);

			return q_score;
		}
	};

	static Genotype str2geno(QByteArray str)
	{
		str.replace("|", "/").replace(".", "0");

		if (str=="1/1") return HOM;
		if (str=="0/1" || str=="1/0") return HET;
		if (str=="0/0") return WT;

		THROW(ArgumentException, "Invalid string '" + str + "' for conversion to genotype!");
	}

	static QByteArray geno2str(Genotype geno)
	{
		if (geno==HOM) return "HOM";
		if (geno==HET) return "HET";
		if (geno==WT) return "WT";

		THROW(ArgumentException, "Invalid genotype for conversion to string!");
	}

	static QByteArray type2str(UpdType type)
	{
		if (type==EXCLUDED) return "EXCLUDED";
		if (type==BIPARENTAL) return "BIPARENTAL";
		if (type==UNINFORMATIVE) return "UNINFORMATIVE";
		if (type==ISO) return "ISO";
		if (type==ISO_OR_HET) return "ISO_OR_HET";

		THROW(ArgumentException, "Invalid UPD type for conversion to string!");
	}

    virtual void main()
    {
		//init
		QString c = getString("c");
		QString f = getString("f");
		QString m = getString("m");
		QString out = getOutfile("out");
		if (!out.endsWith(".tsv")) THROW(ArgumentException, "Output file name has to end with '.tsv'!");
		QString exclude = getInfile("exclude");
		int var_min_dp = getInt("var_min_dp");
		double var_min_q = getFloat("var_min_q");
		bool use_indels = getFlag("use_indels");
		int reg_min_markers = getInt("reg_min_markers");
		double reg_min_bases = 1000.0 * getFloat("reg_min_kb");
		BasicStatistics::precalculateFactorials();
		QTextStream stream(stdout);

		//load BED file to exclude
		BedFile exclude_regions;
		if (exclude!="")
		{
			exclude_regions.load(exclude);
		}
		ChromosomalIndex<BedFile> exclude_idx(exclude_regions);

		//load genotype data
		stream << "### Loading variant/genotype data ###" << endl;
		QList<VariantData> data;
		VariantList variants;
		variants.load(getInfile("in"));
		variants.sort();
		int i_qual = variants.annotationIndexByName("QUAL");
		int i_dp_c = variants.annotationIndexByName("DP", c);
		int i_dp_f = variants.annotationIndexByName("DP", f);
		int i_dp_m = variants.annotationIndexByName("DP", m);
		int i_gt_c = variants.annotationIndexByName("GT", c);
		int i_gt_f = variants.annotationIndexByName("GT", f);
		int i_gt_m = variants.annotationIndexByName("GT", m);
		int skip_chr = 0;
		int skip_qual = 0;
		int skip_dp = 0;
		int skip_indel = 0;
		int c_excluded = 0;
		for (int i=0; i<variants.count(); ++i)
		{
			const Variant& v = variants[i];

			//only autosomes
			if (!v.chr().isAutosome())
			{
				++skip_chr;
				continue;
			}

			//filter by quality
			bool ok;
			double qual = v.annotations()[i_qual].toDouble(&ok);
			if (!ok) THROW(ArgumentException, "Quality '" + v.annotations()[i_qual] + "' is no float - variant " + v.toString());
			if (qual<var_min_q)
			{
				++skip_qual;
				continue;
			}

			//filter by depth
			int dp1 = v.annotations()[i_dp_c].toInt(&ok);
			if (!ok) THROW(ArgumentException, "Depth '" + v.annotations()[i_dp_c] + "' is no integer - variant " + v.toString());
			int dp2 = v.annotations()[i_dp_f].toInt(&ok);
			if (!ok) THROW(ArgumentException, "Depth '" + v.annotations()[i_dp_f] + "' is no integer - variant " + v.toString());
			int dp3 = v.annotations()[i_dp_m].toInt(&ok);
			if (!ok) THROW(ArgumentException, "Depth '" + v.annotations()[i_dp_m] + "' is no integer - variant " + v.toString());
			if (dp1<var_min_dp || dp2<var_min_dp || dp3<var_min_dp)
			{
				++skip_dp;
				continue;
			}

			//filter indels
			if (!use_indels && !v.isSNV())
			{
				++skip_indel;
				continue;
			}

			VariantData var;
			var.chr = v.chr();
			var.start = v.start();
			var.end = v.end();
			var.ref = v.ref();
			var.obs = v.obs();
			var.c = str2geno(v.annotations()[i_gt_c]);
			var.f = str2geno(v.annotations()[i_gt_f]);
			var.m = str2geno(v.annotations()[i_gt_m]);

			//filter by exclude regions
			if (exclude_regions.count() && exclude_idx.matchingIndex(v.chr(), v.start(), v.end())!=-1)
			{
				var.type = EXCLUDED;
				var.source = NONE;
				++c_excluded;
			}
			else
			{
				var.determineType();
			}
			data << var;
		}
		stream << "Loaded " << data.count() << " of " << variants.count() << " variants" << endl;
		variants.clear();
		stream << "Skipped " << skip_chr << " variants not on autosomes" << endl;
		stream << "Skipped " << skip_qual << " variants because of low quality (<" << var_min_q << ")" << endl;
		stream << "Skipped " << skip_dp << " variants because of low depth (<" << var_min_dp << ")" << endl;
		stream << "Skipped " << skip_indel << " indels" << endl;
		stream << "Excluded " << c_excluded << " variants" << endl;
		stream << endl;

		//check medelian errors
		stream << "### Checking for mendelian errors ###" << endl;
		int err_f = 0;
		int err_m = 0;
		int err_fm = 0;
		foreach(const VariantData& entry, data)
		{
			if (entry.type==EXCLUDED) continue;

			if ((entry.f==HOM && entry.c==WT) || (entry.f==WT && entry.c==HOM)) ++err_f;
			if ((entry.m==HOM && entry.c==WT) || (entry.m==WT && entry.c==HOM)) ++err_m;
			if ((entry.m==HOM && entry.f==WT) || (entry.m==WT && entry.m==HOM)) ++err_fm;
		}
		stream << "Found " << err_f << " (" << QByteArray::number(100.0*err_f/data.count(), 'f', 2) << "%) mendelian errors for mother-child pair" << endl;
		stream << "Found " << err_m << " (" << QByteArray::number(100.0*err_m/data.count(), 'f', 2) << "%) mendelian errors for father-child pair" << endl;
		stream << "Found " << err_fm << " (" << QByteArray::number(100.0*err_fm/data.count(), 'f', 2) << "%) mendelian errors for father-mother pair (expected to be high)" << endl;
		stream << endl;

		stream << "### Performing statistics ###" << endl;
		int biparental = 0;
		int upd = 0;
		QMap<QByteArray, int> chr_upd;
		QMap<QByteArray, int> chr_var;
		foreach(const VariantData& entry, data)
		{
			if (entry.type==EXCLUDED) continue;

			if (entry.type==BIPARENTAL) ++biparental;

			if (entry.type==ISO || entry.type==ISO_OR_HET)
			{
				++upd;
				++chr_upd[entry.chr.str()];
			}

			++chr_var[entry.chr.str()];
		}
		double p_biparental = 1.0 * biparental / data.count();
		stream << "bi-parental variants: " << biparental <<  " (" << QByteArray::number(100.0*p_biparental, 'f', 3) << "%)" << endl;

		int max =  *std::max_element(chr_upd.begin(), chr_upd.end());
		QByteArray max_chr = chr_upd.key(max);
		double p_upd = 1.0 * (upd-max) / (data.count()-chr_var[max_chr]);
		stream << "UPD variants: " << (upd-max) <<  " (" << QByteArray::number(100.0*p_upd, 'f', 3) << "%) - excluded chromosome " << max_chr << " containing " << max <<  " (" << QByteArray::number(100.0*max/chr_var[max_chr], 'f', 3) << "%)" << endl;
		stream << endl;

		stream << "### Detecting UPDs ###" << endl;
		bool in_range = false;
		QList<UpdRange> ranges;
		UpdRange current_range;
		for (auto it=data.begin(); it!=data.end(); ++it)
		{

			if (in_range)
			{
				if (it->type==BIPARENTAL || it->type==EXCLUDED || it->chr!=current_range.start->chr || it+1==data.end())
				{
					ranges.append(current_range);
					in_range = false;
				}
				else if (it->type==ISO || it->type==ISO_OR_HET)
				{
					if (it->source == current_range.start->source)
					{
						current_range.end = it + 1;
					}
					else //different source
					{
						ranges.append(current_range);
						in_range = false;
					}
				}
			}

			if (!in_range) //not else because the because 'in_range' can change in the if-clause above
			{
				if (it->type==ISO || it->type==ISO_OR_HET)
				{
					current_range.start = it;
					current_range.end = it + 1;
					in_range = true;
				}
			}
		}
		stream << "Detected " << ranges.count() << " raw ranges" << endl;

		//perform sanity checks
		foreach(const UpdRange& range, ranges)
		{
			//check same chr
			if (range.start->chr!=range.end->chr)
			{
				THROW(ProgrammingException, "Range spans chromosome border")
			}
			//check same source
			for (auto it=range.start; it<range.end; ++it)
			{
				if (it->source!=NONE && it->source!=range.start->source)
				{
					THROW(ProgrammingException, "Range with mixed UPD source (father/mother)!")
				}
			}
		}

		//write output
		int c_passing  = 0;
		QSharedPointer<QFile> output = Helper::openFileForWriting(out);
		output->write("#chr\tstart\tend\tsize_kb\tsize_markers\tupd_markers\tsource\ttype\tq-score\n");
		foreach(const UpdRange& range, ranges)
		{
			if (range.sizeBases()<reg_min_bases) continue;
			int upd_markers = range.countType(ISO)+range.countType(ISO_OR_HET);
			if (upd_markers<reg_min_markers) continue;

			output->write(range.start->chr.str() + "\t" + QByteArray::number(range.start->start) + "\t" + QByteArray::number((range.end-1)->end)
						  + "\t" + QByteArray::number(range.sizeBases()/1000.0, 'f', 3)
						  + "\t" + QByteArray::number(range.sizeMarkers())
						  + "\t" + QByteArray::number(upd_markers)
						  + "\t" + range.sourceAsString()
						  + "\t" + range.typeAsString()
						  + "\t" + QByteArray::number(range.qScore(p_biparental, p_upd, stream), 'f', 2)
						  + "\n");

			++c_passing;
		}
		stream << "Written " << c_passing << " ranges that passes the filters" << endl;
	}
};

#include "main.moc"

int main(int argc, char *argv[])
{
    ConcreteTool tool(argc, argv);
    return tool.execute();
}

