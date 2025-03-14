#include "ToolBase.h"
#include "Helper.h"
#include "Exceptions.h"
#include "VariantList.h"
#include "VcfFile.h"
#include "BasicStatistics.h"
#include <QTextStream>
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
		setDescription("UPD detection from trio variant data.");
		addInfile("in", "Input VCF file of trio.", false, true);
		addString("c", "Header name of child.", false);
		addString("f", "Header name of father.", false);
		addString("m", "Header name of mother.", false);
		addOutfile("out", "Output TSV file containing the detected UPDs.", false, true);
		//optional
		addOutfile("out_informative", "Output IGV file containing informative variants.", true, true);
		addInfile("exclude", "BED file with regions to exclude, e.g. copy-number variant regions.", true);
		addInt("var_min_dp", "Minimum depth (DP) of a variant (in all three samples).", true, 20);
		addFloat("var_min_q", "Minimum quality (QUAL) of a variant.", true, 30);
		addFlag("var_use_indels", "Also use InDels. The default is to use SNVs only.");
		addFloat("ext_marker_perc", "Percentage of markers that can be spanned when merging adjacent regions .", true, 1.0);
		addFloat("ext_size_perc", "Percentage of base size that can be spanned when merging adjacent regions.", true, 20.0);
		addFloat("reg_min_kb", "Mimimum size in kilo-bases required for a UPD region.",  true, 1000.0);
		addInt("reg_min_markers", "Mimimum number of UPD markers required in a region.",  true, 15);
		addFloat("reg_min_q", "Mimimum Q-score required for a UPD region.",  true, 20.0);
		addFlag("debug", "Enable verbose debug output.");

		changeLog(2024,  6,  6, "Added optional output file containing informative variants.");
		changeLog(2020,  8,  7, "VCF files only as input format for variant list.");
		changeLog(2018,  6, 11, "First working version.");
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

		double hetMarkerPercentage() const
		{
			int c_het = 0;
			for (auto it=start; it<end; ++it)
			{
				bool het = it->c==HET;
				c_het += het;
			}
			return 100.0 * c_het / sizeMarkers();
		}

		int sizeMarkers() const
		{
			return end-start;
		}

		int sizeBases() const
		{
			return (end-1)->end-start->start+1;
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

		double qScore(double p_biparental, double p_upd) const
		{
			int c_upd = countType(ISO) + countType(ISO_OR_HET);

			//likelyhood to find n variants without seeing any biparental inheritance
			int markers = sizeMarkers();
			double p_no_bip = std::pow(1-p_biparental, markers);

			//likelyhood to find n-1 UPD variants
			double p_n_upd = c_upd<2 ? 1.0 : BasicStatistics::matchProbability(p_upd, c_upd-1, markers-1);

			//calculate overall likelyhood
			return -10.0 * log10(p_no_bip * p_n_upd);
		}

		QByteArray toString() const
		{
			return start->chr.str() + ":" + QByteArray::number(start->start) + "-" + QByteArray::number((end-1)->end) + " bases=" + QByteArray::number(sizeBases()) + " markers=" + QByteArray::number(sizeMarkers())+ " upd_markers=" + QByteArray::number(countType(ISO)+countType(ISO_OR_HET));
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

	QList<VariantData> loadVariants(QTextStream& stream)
	{
		QList<VariantData> output;

		QByteArray c = getString("c").toUtf8();
		QByteArray f = getString("f").toUtf8();
		QByteArray m = getString("m").toUtf8();
		int var_min_dp = getInt("var_min_dp");
		double var_min_q = getFloat("var_min_q");
		bool var_use_indels = getFlag("var_use_indels");

		//load BED file to exclude
		QString exclude = getInfile("exclude");
		BedFile exclude_regions;
		if (exclude!="")
		{
			exclude_regions.load(exclude);
		}
		ChromosomalIndex<BedFile> exclude_idx(exclude_regions);

		VcfFile variants;
        variants.load(getInfile("in"), true);
		variants.sort();
		int skip_chr = 0;
		int skip_qual = 0;
		int skip_dp = 0;
		int skip_indel = 0;
		int c_excluded = 0;

		for (int i=0; i<variants.count(); ++i)
		{
			const  VcfLine& v = variants[i];

			//only autosomes
			if (!v.chr().isAutosome())
			{
				++skip_chr;
				continue;
			}

			//filter by quality
			if (v.qual() < 0) THROW(ArgumentException, "Quality '" + QString::number(v.qual()) + "' is not given in " + v.toString(true));
			if (v.qual()<var_min_q)
			{
				++skip_qual;
				continue;
			}

			//filter by depth
			if(variants.vcfHeader().formatIdDefined("DP"))
			{
				QByteArray tmp = v.formatValueFromSample("DP", c);

				bool ok = true;
				int dp1 = (tmp.isEmpty() || tmp==".") ? 0 : tmp.toInt(&ok);
				if (!ok) THROW(ArgumentException, "Depth of child '" + tmp + "' is no integer - variant " + v.toString(true));
				tmp = v.formatValueFromSample("DP", f);
				int dp2 = (tmp.isEmpty() || tmp==".") ? 0 : tmp.toInt(&ok);
				if (!ok) THROW(ArgumentException, "Depth of father  '" + tmp + "' is no integer - variant " + v.toString(true));
				tmp = v.formatValueFromSample("DP", m);
				int dp3 = (tmp.isEmpty() || tmp==".") ? 0 : tmp.toInt(&ok);
				if (!ok) THROW(ArgumentException, "Depth of mother  '" + tmp + "' is no integer - variant " + v.toString(true));
				if (dp1<var_min_dp || dp2<var_min_dp || dp3<var_min_dp)
				{
					++skip_dp;
					continue;
				}
			}

			//filter indels
			if (!var_use_indels && !v.isSNV())
			{
				++skip_indel;
				continue;
			}

			VariantData entry;
			entry.chr = v.chr();
			entry.start = v.start();
			entry.end = v.end();
			entry.ref = v.ref();
			entry.obs = v.altString();
			if(variants.vcfHeader().formatIdDefined("GT"))
			{
				entry.c = str2geno(v.formatValueFromSample("GT", c));
				entry.f = str2geno(v.formatValueFromSample("GT", f));
				entry.m = str2geno(v.formatValueFromSample("GT", m));
			}

			//filter by exclude regions
			if (exclude_regions.count() && exclude_idx.matchingIndex(v.chr(), v.start(), v.end())!=-1)
			{
				entry.type = EXCLUDED;
				entry.source = NONE;
				++c_excluded;
			}
			else
			{
				entry.determineType();
			}
			output << entry;
		}
        stream << "Loaded " << output.count() << " of " << variants.count() << " variants" << QT_ENDL;
        stream << "Skipped " << skip_chr << " variants not on autosomes" << QT_ENDL;
        stream << "Skipped " << skip_qual << " variants because of low quality (<" << var_min_q << ")" << QT_ENDL;
        stream << "Skipped " << skip_dp << " variants because of low depth (<" << var_min_dp << ")" << QT_ENDL;
        stream << "Skipped " << skip_indel << " indels" << QT_ENDL;
        stream << "Excluded " << c_excluded << " variants" << QT_ENDL;

		return output;
	}

	void checkMendelianErrors(const QList<VariantData> data, QTextStream& stream)
	{
		int err_f = 0;
		int err_m = 0;
		int err_fm = 0;
		foreach(const VariantData& entry, data)
		{
			if (entry.type==EXCLUDED) continue;

			if ((entry.f==HOM && entry.c==WT) || (entry.f==WT && entry.c==HOM)) ++err_f;
			if ((entry.m==HOM && entry.c==WT) || (entry.m==WT && entry.c==HOM)) ++err_m;
			if ((entry.m==HOM && entry.f==WT) || (entry.m==WT && entry.f==HOM)) ++err_fm;
		}
        stream << "Found " << err_f << " (" << QByteArray::number(100.0*err_f/data.count(), 'f', 2) << "%) mendelian errors for father-child pair" << QT_ENDL;
        stream << "Found " << err_m << " (" << QByteArray::number(100.0*err_m/data.count(), 'f', 2) << "%) mendelian errors for mother-child pair" << QT_ENDL;
        stream << "Found " << err_fm << " (" << QByteArray::number(100.0*err_fm/data.count(), 'f', 2) << "%) mendelian errors for father-mother pair (expected to be high)" << QT_ENDL;
		if (err_f>err_fm || err_m>err_fm)
		{
            stream << "Error: Mendelian error rates suggest a sample swap!" << QT_ENDL;
			THROW(ArgumentException, "Mendelian error rates suggest a sample swap!");
		}
	}

	QList<UpdRange> detectRanges(QList<VariantData>& data, QTextStream& stream)
	{
		double debug = getFlag("debug");

		QList<UpdRange> output;

		bool in_range = false;
		UpdRange current_range;
		for (auto it=data.cbegin(); it!=data.cend(); ++it)
		{

			if (in_range)
			{
				if (it->type==BIPARENTAL || it->type==EXCLUDED || it->chr!=current_range.start->chr || it+1==data.cend())
				{
					output.append(current_range);
					in_range = false;
				}
				else if (it->type==ISO || it->type==ISO_OR_HET)
				{
					if (it->source==current_range.start->source)
					{
						current_range.end = it + 1;
					}
					else //different source
					{
						output.append(current_range);
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

        stream << "Detected " << output.count() << " raw ranges" << QT_ENDL;
		if (debug)
		{
			foreach(const UpdRange& range, output)
			{
                stream << "  Range: " << range.toString() << QT_ENDL;
			}
		}

		return output;
	}

	void mergeRanges(QList<UpdRange>& ranges, QTextStream& stream)
	{
		double ext_marker_perc = getFloat("ext_marker_perc");
		double ext_size_perc = getFloat("ext_size_perc");
		double debug = getFlag("debug");

		bool merged = true;
		while(merged)
		{
			merged = false;
			for (int i=0; i<ranges.count()-1; ++i)
			{
				//same chr and source
				if (ranges[i].start->chr!=ranges[i+1].start->chr) continue;
				if (ranges[i].start->source!=ranges[i+1].start->source) continue;


				double marker_diff = ranges[i+1].start - ranges[i].end;
				double marker_cutoff = ext_marker_perc / 100.0 * (ranges[i].sizeMarkers() + ranges[i+1].sizeMarkers());

				double base_diff = ranges[i+1].start->start - (ranges[i].end-1)->end;
				double base_cutoff = ext_size_perc / 100.0 * (ranges[i].sizeBases() + ranges[i+1].sizeBases());

				//merge
				if (marker_diff<marker_cutoff || base_diff<base_cutoff)
				{
					if (debug)
					{
                        stream << "Merging ranges:" << QT_ENDL;
                        stream << "  " << ranges[i].toString() << QT_ENDL;
                        stream << "  " << ranges[i+1].toString() << QT_ENDL;
                        stream << "  base_diff=" << base_diff << " marker_diff=" << marker_diff  << QT_ENDL;
					}

					ranges[i].end = ranges[i+1].end;
					ranges.removeAt(i+1);
					i-=1;
					merged = true;
				}
			}
		}

        stream << "Merged adjacent raw regions resulting in " << ranges.count() << " region(s)" << QT_ENDL;
	}

	void writeOutput(QList<UpdRange>& ranges, double p_biparental, double p_upd, QTextStream& stream)
	{
		QString out = getOutfile("out");
		if (!out.endsWith(".tsv")) THROW(ArgumentException, "Output file name has to end with '.tsv'!");
		int reg_min_markers = getInt("reg_min_markers");
		double reg_min_bases = 1000.0 * getFloat("reg_min_kb");
		double reg_min_q = getFloat("reg_min_q");
		bool debug = getFlag("debug");

		QSharedPointer<QFile> output = Helper::openFileForWriting(out);
		output->write("#chr\tstart\tend\tsize_kb\tsize_markers\tupd_markers\tsource\thet_percentage\tq-score\n");
		int c_passing  = 0;
		foreach(const UpdRange& range, ranges)
		{
			if (range.sizeBases()<reg_min_bases) continue;
			int upd_markers = range.countType(ISO)+range.countType(ISO_OR_HET);
			if (upd_markers<reg_min_markers) continue;
			double q_score = range.qScore(p_biparental, p_upd);
			if (q_score<reg_min_q) continue;

			output->write(range.start->chr.str() + "\t" + QByteArray::number(range.start->start) + "\t" + QByteArray::number((range.end-1)->end)
						  + "\t" + QByteArray::number(range.sizeBases()/1000.0, 'f', 3)
						  + "\t" + QByteArray::number(range.sizeMarkers())
						  + "\t" + QByteArray::number(upd_markers)
						  + "\t" + range.sourceAsString()
						  + "\t" + QByteArray::number(range.hetMarkerPercentage(), 'f', 2)
						  + "\t" + QByteArray::number(q_score, 'f', 2)
						  + "\n");

			++c_passing;

			if (debug)
			{
                stream << "  Range: " << range.toString() << QT_ENDL;
			}
		}
        stream << "Written " << c_passing << " ranges that pass the filters" << QT_ENDL;
	}

	void writeInformativeVariants(QList<VariantData>& data)
	{
		QString out = getOutfile("out_informative");
		if (out.isEmpty()) return;
		if (!out.endsWith(".igv")) THROW(ArgumentException, "Output file name for informative variants has to end with '.igv'!");

		QSharedPointer<QFile> output = Helper::openFileForWriting(out);
		output->write("#track graphtype=heatmap viewLimits=1:4 maxHeightPixels=80:80:80 color=0,0,255 altColor=255,0,0 midRange=2.5:2.5 midColor=255,255,255 windowingFunction=mean\n");
		output->write("Chromosome\tStart\tEnd\tFeature\tUPD variants\n");
		foreach(const VariantData& var, data)
		{
			QByteArray source = "";
			if (var.source==UpdSource::FATHER)
			{
				source = "father";
			}
			else if (var.source==UpdSource::MOTHER)
			{
				source = "mother";
			}
			else continue;

			QByteArray type = "";
			if (var.type==UpdType::ISO)
			{
				type = "iso";
			}
			else if (var.type==UpdType::ISO_OR_HET)
			{
				type = "het_or_iso";
			}
			else continue;

			QByteArray score = "";
			if (var.source==UpdSource::FATHER && var.type==UpdType::ISO) score = "4";
			if (var.source==UpdSource::FATHER && var.type==UpdType::ISO_OR_HET) score = "3";
			if (var.source==UpdSource::MOTHER && var.type==UpdType::ISO_OR_HET) score = "2";
			if (var.source==UpdSource::MOTHER && var.type==UpdType::ISO) score = "1";

			output->write(var.chr.strNormalized(true) + "\t" + QByteArray::number(var.start) + "\t" + QByteArray::number(var.start+1) + "\t" + source + " - " + type + "\t" + score + "\n");
		}
	}
	virtual void main()
	{
		//init
		BasicStatistics::precalculateFactorials();
		QTextStream stream(stdout);

		//load genotype data
        stream << "### Loading variant/genotype data ###" << QT_ENDL;
		QList<VariantData> data = loadVariants(stream);
        stream << QT_ENDL;

		//check medelian errors
        stream << "### Checking for mendelian errors ###" << QT_ENDL;
		checkMendelianErrors(data, stream);
        stream << QT_ENDL;

		//statistics
        stream << "### Performing statistics ###" << QT_ENDL;
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
        stream << "bi-parental variants: " << biparental <<  " (" << QByteArray::number(100.0*p_biparental, 'f', 3) << "%)" << QT_ENDL;

		int max =  *std::max_element(chr_upd.begin(), chr_upd.end());
		QByteArray max_chr = chr_upd.key(max);
		double p_upd = 1.0 * (upd-max) / (data.count()-chr_var[max_chr]);
        stream << "UPD variants: " << (upd-max) <<  " (" << QByteArray::number(100.0*p_upd, 'f', 3) << "%) - excluded chromosome " << max_chr << " containing " << max <<  " (" << QByteArray::number(100.0*max/chr_var[max_chr], 'f', 3) << "%)" << QT_ENDL;
        stream << QT_ENDL;

		//detection
        stream << "### Detecting UPDs ###" << QT_ENDL;
		QList<UpdRange> ranges = detectRanges(data, stream);
		mergeRanges(ranges, stream);

		//perform sanity checks
		foreach(const UpdRange& range, ranges)
		{
			for (auto it=range.start; it<range.end; ++it)
			{
				//check same source
				if (it->source!=NONE && it->source!=range.start->source)
				{
					THROW(ProgrammingException, "Range with mixed UPD source (father/mother)!")
				}
				//check same chr
				if (it->chr!=range.start->chr)
				{
					THROW(ProgrammingException, "Range spans chromosome border: " + range.start->chr.str() + " - " + it->chr.str());
				}
			}
		}

		//write output
		writeOutput(ranges, p_biparental, p_upd, stream);

		//write informative variant output
		writeInformativeVariants(data);
	}
};

#include "main.moc"

int main(int argc, char *argv[])
{
	ConcreteTool tool(argc, argv);
	return tool.execute();
}

