#include "ToolBase.h"
#include "Helper.h"
#include "Exceptions.h"
#include "BedFile.h"
#include "NGSHelper.h"
#include "Histogram.h"
#include "GeneSet.h"
#include "TSVFileStream.h"
#include "Settings.h"
#include "VcfFile.h"

#include <QVector>
#include <QFileInfo>
#include <QDir>
#include <cmath>

struct SampleCorrelation;

//Sample representation
struct SampleData
{
	SampleData()
    {
    }

	QByteArray name; //file name

	QVector<float> doc; //coverage data (normalized: divided by mean)
	float doc_mean; //mean coverage before normalizazion (afterwards it is 1.0)

	QVector<SampleCorrelation> correl_all; //correlation with all samples (-1.0 for self-correlation)

	QVector<float> ref; //reference sample (mean of 'n' most similar samples)
	QVector<float> ref_stdev; //reference sample standard deviation (deviation of 'n' most similar samples)
	float ref_correl; //correlation of sample to reference sample

	QByteArray qc; //QC warning flag
};

//Sample correlation helper
struct SampleCorrelation
{

	SampleCorrelation()
		: sample(nullptr)
		, rmsd_inv(-1)
	{
	}

	SampleCorrelation(QSharedPointer<SampleData> s, float c)
		: sample(s)
		, rmsd_inv(c)
	{
	}

	QSharedPointer<SampleData> sample;
	float rmsd_inv;
};

//Exon/region representation
struct ExonData
{
    ExonData()
        : start(-1)
		, start_str()
        , end(-1)
		, end_str()
		, index(-1)
		, is_par(false)
		, is_cnp(false)
		, gc(-1.0)
		, annotations()
    {
    }

    Chromosome chr; //chromosome
    int start; //start position
	QByteArray start_str; //start position as string
    int end; //end position
	QByteArray end_str; //end position as string
	int index; //exon index (needed to access DOC data arrays of samples)
	int is_par; //flag indicating if the region lies inside the pseudoautosomal region of X chromosome (normalization with autosomes)
	int is_cnp; //flag indicating if the region lies inside a known copy-number-polymorphism region (i.e. probably not pathogenic)
	float gc; //GC content
	QList<QSet<QByteArray>> annotations; //annotation sets, one for each annotation file given.

	float median; //median normalized DOC value
	float mad; //MAD of normalized DOC values

	QByteArray qc; //QC warning flag

	QByteArray toString() const
	{
		return chr.str() + ":" + start_str + "-" + end_str;
	}

    bool operator<(const ExonData& rhs) const
    {
        if (chr<rhs.chr) return true;
        else if (chr>rhs.chr) return false;
        else if (start==rhs.start) return end<rhs.end;
        else return start<rhs.start;
    }
};

//CNV test data
struct ResultData
{
    ResultData()
		: sample()
		, exon()
        , z(0.0)
        , copies(2)
    {
    }

	ResultData(const QSharedPointer<SampleData>& s, const QSharedPointer<ExonData>& e, float z_score)
		: sample(s)
		, exon(e)
        , z(z_score)
        , copies(2)
    {
    }

	QSharedPointer<SampleData> sample;
	QSharedPointer<ExonData> exon;
	float z; //z-score
	short copies; //estimated CN
};

//Range of subsequent exons with same copy number trend (closed interval)
struct Range
{
	enum Type {INS, DEL};

	Range(const QSharedPointer<SampleData>& sa, int s, int e, Type t)
		: sample(sa)
		, start(s)
		, end(e)
		, type(t)
	{
	}

	int size() const
	{
		return end - start + 1;
	}

	QSharedPointer<SampleData> sample;
	int start;
	int end;
	Type type; //flag if deletion
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
		setDescription("CNV detection from targeted resequencing data using non-matched control samples.");
		setExtendedDescription(QStringList() << "Note CnvHunter is no longer maintained and used. For CNV calling have a look at ClinCNV: https://github.com/imgag/ClinCNV.");

		addInfileList("in", "Input TSV files (one per sample) containing coverage data (chr, start, end, avg_depth). If only one file is given, each line in this file is interpreted as an input file path.", false, true);
        addOutfile("out", "Output TSV file containing the detected CNVs.", false, true);
		//optional
		addInt("n", "The number of most similar samples to use for reference construction.", true, 30);
        addFloat("min_z", "Minimum z-score for CNV seed detection.", true, 4.0);
		addFloat("ext_min_z", "Minimum z-score for CNV extension around seeds.", true, 2.0);
		addFloat("ext_gap_span", "Percentage of orignal region size that can be spanned while merging nearby regions (0 disables it).", true, 20.0);
        addFloat("sam_min_depth", "QC: Minimum average depth of a sample.", true, 40.0);
		addFloat("sam_min_corr", "QC: Minimum correlation of sample to constructed reference sample.", true, 0.95);
		addInt("sam_corr_regs", "Maximum number of regions used for sample correlation calculation (to speed up comparisons for exoms etc.).", true, 20000);
        addFloat("reg_min_cov", "QC: Minimum (average) absolute depth of a target region.", true, 20.0);
		addFloat("reg_min_ncov", "QC: Minimum (average) normalized depth of a target region.", true, 0.01);
		addFloat("reg_max_cv", "QC: Maximum coefficient of variation (median/mad) of target region.", true, 0.4);
		addString("debug", "Writes debug information for the sample matching the given name (or for all samples if 'ALL' is given).", true, "");
		addString("seg", "Writes a SEG file for the sample matching the given name (used for visualization in IGV).", true);
		addString("par", "Comma-separated list of pseudo-autosomal regions on the X chromosome.", true, "1-2699520,154931044-155270560");
		addInfile("cnp_file", "BED file containing copy-number-polymorphism (CNP) regions. They are excluded from the normalization/correlation calculation. E.g use the CNV map from http://dx.doi.org/10.1038/nrg3871.", true);
		addInfileList("annotate", "List of BED files used for annotation. Each file adds a column to the output file. The base filename is used as colum name and 4th column of the BED file is used as annotation value.", true);
		addInt("gc_window", "Moving median GC-content normalization window size (disabled by default).", true, -1);
		addInt("gc_extend", "Moving median GC-content normalization extension around target region.", true, 0);
		addInfile("ref", "Reference genome FASTA file. If unset, 'reference_genome' from the 'settings.ini' file is used.", true, false);

		changeLog(2018, 5,  14, "Added option to specify input files in single input file.");
		changeLog(2017, 9,   4, "Added GC normalization.");
		changeLog(2017, 8,  29, "Updated default values of parameters 'n' and 'reg_max_cv' based on latest benchmarks.");
		changeLog(2017, 8,  28, "Added generic annotation mechanism for annotation from BED files.");
		changeLog(2017, 8,  24, "Added copy-number-polymorphisms regions input file ('cnp_file' parameter).");
		changeLog(2017, 8,  17, "Added down-sampling of input to speed up sample correlation ('sam_corr_regs' parameter).");
		changeLog(2017, 8,  15, "Added allele frequency for each region to TSV output.");
		changeLog(2016, 10, 24, "Added copy-number variant size to TSV output and added optional SEG output file.");
		changeLog(2016, 9,   1, "Sample and region information files are now always written.");
		changeLog(2016, 8,  23, "Added merging of large CNVs that were split to several regions due to noise.");
		changeLog(2016, 8,  21, "Improved log output (to make parameter optimization easier).");
	}

	void movingMedianNormalization(QVector<float>& values, const QVector<QPair<int, double>>& order, int window_size)
	{
		//check input
		if (values.count() != order.count())
		{
			THROW(StatisticsException, "Value and order count not the same for median normalization!");
		}

		//init
		if (window_size%2==0) ++window_size;
		const int hws = window_size/2;

		//construct baseline
		QVector<float> baseline;
		baseline.reserve(order.count());
		QVector<float> tmp;
		for (int i=0; i<window_size; ++i)
		{
			tmp.append(values[order[i].first]);
		}
		std::sort(tmp.begin(), tmp.end());
		baseline.fill(tmp[hws], hws+1);
		for (int i=hws+1; i<order.count()-hws; ++i)
		{
			float val_before = values[order[i-hws-1].first];
			tmp.erase(std::lower_bound(tmp.begin(), tmp.end(), val_before));

			float val_after = values[order[i+hws].first];
			tmp.insert(std::lower_bound(tmp.begin(), tmp.end(), val_after), val_after);

			baseline.append(tmp[hws]);
		}
		while(baseline.count()<order.count())
		{
			baseline.append(baseline.last());
		}

		//determine maximum
		float max = *(std::max_element(baseline.begin(), baseline.end()));

		//scale up by GC baseline
		for (int i=0; i<order.count(); ++i)
		{
			values[order[i].first] *= max / baseline[i];
		}
	}

	float fun_median(const QVector<float>& data)
	{
		if (data.count()==0)
		{

			THROW(StatisticsException, "Cannot calculate median on empty data array!");
		}

		if (data.count()%2==0)
		{
			return 0.5f * (data[data.count()/2] + data[data.count()/2-1]);
		}
		else
		{
			return data[data.count()/2];
		}
	}

	float fun_correlation(const QVector<float>& x, const QVector<float>& y)
	{
		if (x.count()!=y.count())
		{
			THROW(StatisticsException, "Cannot calculate correlation of data arrays with different length!");
		}
		if (x.count()==0)
		{
			THROW(StatisticsException, "Cannot calculate correlation of data arrays with zero length!");
		}

		const double x_mean = std::accumulate(x.begin(), x.end(), 0.0) / x.count();
		const double y_mean = std::accumulate(y.begin(), y.end(), 0.0) / x.count();

		double sum = 0.0;
		double x_square_sum = 0.0;
		double y_square_sum = 0.0;
		for(int i=0; i<x.count(); ++i)
		{
			sum += (x[i]-x_mean) * (y[i]-y_mean);
			x_square_sum += pow(x[i]-x_mean, 2.0);
			y_square_sum += pow(y[i]-y_mean, 2.0);
		}

		return sum / sqrt(x_square_sum/x.count()) / sqrt(y_square_sum/x.count()) / x.count();
	}

	float fun_mad(const QVector<float>& data, float median)
	{
		QVector<float> devs;
		devs.reserve(data.count());
		foreach(float value, data)
		{
			devs.append(fabsf(value-median));
		}
		std::sort(devs.begin(), devs.end());
		return fun_median(devs);
	}

	bool fun_isValidFloat(float value)
	{
		if (value != value)
		{
			return false;
		}
		if (value > std::numeric_limits<float>::max())
		{
			return false;
		}
		if (value < -std::numeric_limits<float>::max())
		{
			return false;
		}

		return true;
	}

	void storeSampleInfo(QString out, const QVector<QSharedPointer<SampleData>>& samples, const QVector<QSharedPointer<SampleData>>& samples_removed, const QHash<QSharedPointer<SampleData>, int>& cnvs_sample, const QList<ResultData>& results)
    {
        //init
        QSharedPointer<QFile> file = Helper::openFileForWriting(out.left(out.size()-4) + "_samples.tsv");
        QTextStream outstream(file.data());

        //calcualte z-score statistics for each sample
		QHash<QSharedPointer<SampleData>, float> z_scores;
        QSharedPointer<SampleData> curr;
		QVector<float> zs;
        for (int r=0; r<results.count(); ++r)
        {
            if (results[r].sample!=curr)
            {
                if (!curr.isNull())
                {
					z_scores[curr] = fun_mad(zs, 0);
                    zs.clear();
                }
                curr = results[r].sample;
            }

            zs.append(results[r].z);
		}
		z_scores[curr] = fun_mad(zs, 0);

        //store file
		outstream << "#sample\tdoc_mean\tref_correl\tz_score_mad\tcnvs\tqc_info" << "\n";
		foreach(const QSharedPointer<SampleData>& sample, samples)
        {
			outstream << sample->name << "\t" << QByteArray::number(sample->doc_mean, 'f', 1) << "\t" << QByteArray::number(sample->ref_correl, 'f', 3) << "\t" << QByteArray::number(z_scores[sample], 'f', 3) << "\t" << cnvs_sample[sample] << "\t" << sample->qc << "\n";
        }
        foreach(const QSharedPointer<SampleData>& sample, samples_removed)
        {
			outstream << sample->name << "\t" << QByteArray::number(sample->doc_mean, 'f', 1) << "\t" << QByteArray::number(sample->ref_correl, 'f', 3) << "\tnan\tnan\t" << sample->qc << "\n";
        }
    }

	void storeRegionInfo(QString out, const QVector<QSharedPointer<ExonData>>& exons, const QVector<QSharedPointer<ExonData>>& exons_removed, const QHash<QSharedPointer<ExonData>, int>& cnvs_exon)
    {
        QSharedPointer<QFile> file = Helper::openFileForWriting(out.left(out.size()-4) + "_regions.tsv");
        QTextStream outstream(file.data());
		outstream << "#region\tsize\tgc_content\tndoc_median\tndoc_mad\tndoc_cv\tcnvs\tqc_info\toverlaps_cnp_region" << "\n";

        //contruct sorted array
        QVector<QSharedPointer<ExonData>> tmp;
		foreach(const QSharedPointer<ExonData>& exon, exons) tmp.append(exon);
		foreach(const QSharedPointer<ExonData>& exon, exons_removed) tmp.append(exon);
        std::sort(tmp.begin(), tmp.end(), [](const QSharedPointer<ExonData>& a, const QSharedPointer<ExonData>& b){return *(a.data()) < *(b.data());} );
        foreach(const QSharedPointer<ExonData>& exon, tmp)
        {
			outstream << exon->toString() << "\t" << (exon->end-exon->start) << "\t" << QByteArray::number(exon->gc, 'f', 2) << "\t" << QByteArray::number(exon->median, 'f', 3) << "\t" << QByteArray::number(exon->mad, 'f', 3) << "\t" << QByteArray::number(exon->mad/exon->median, 'f', 2) << "\t";
            if (exon->qc.isEmpty())
            {
                outstream << cnvs_exon[exon];
            }
            else
            {
				outstream << "nan";
            }
			outstream << "\t" << exon->qc << "\t" << (exon->is_cnp ? "yes" : "") << "\n";
        }
    }

	void storeDebugInfo(QSharedPointer<SampleData> debug_sample, QString out, const QVector<QSharedPointer<SampleData>>& samples, const QList<ResultData>& results)
    {
		//write header
		QSharedPointer<QFile> file = Helper::openFileForWriting(out.left(out.size()-4) + "_debug.tsv");
        QTextStream outstream(file.data());
		outstream << "#sample\tregion\tcopy_number\tz_score\tndoc\tref_ndoc\tref_ndoc_stdev\tlog2_ratio" << "\n";

		//write sample correlations
		foreach(const QSharedPointer<SampleData>& sample, samples)
		{
			if (sample==debug_sample)
			{
				outstream << "##RMSD of " << sample->name << " to other samples:" << "\n";
				for (int i=0; i<samples.count()-1; ++i)
				{
					outstream << "##" << (i+1) << "\t" << sample->correl_all[i].sample->name << "\t" << sample->correl_all[i].rmsd_inv << "\n";
				}
			}
		}

		//write details
        foreach(const ResultData& r, results)
		{
			if(debug_sample.isNull() || r.sample==debug_sample)
			{
				float ncov = r.sample->doc[r.exon->index];
				float ncov_ref = r.sample->ref[r.exon->index];
				float log_ratio = log2(ncov/ncov_ref);
				outstream << r.sample->name << "\t" << r.exon->toString() << "\t" << r.copies << "\t" << QByteArray::number(r.z, 'f', 2) << "\t" << QByteArray::number(ncov, 'f', 3) << "\t" << QByteArray::number(ncov_ref, 'f', 3) << "\t" << QByteArray::number(r.sample->ref_stdev[r.exon->index], 'f', 3) << "\t" << QByteArray::number(log_ratio, 'f', 2) << "\n";
			}
        }
    }

	QSharedPointer<SampleData> sampleByName(QByteArray name, const QVector<QSharedPointer<SampleData>>& samples, const QVector<QSharedPointer<SampleData>>& samples_removed, bool removed_samples_ok)
	{
		//find samples (QC ok)
		foreach(const QSharedPointer<SampleData>& sample, samples)
		{
			if (sample->name==name)
			{
				return sample;
			}
		}

		//find samples (QC fail)
		foreach(const QSharedPointer<SampleData>& sample, samples_removed)
		{
			if (sample->name==name)
			{
				if (!removed_samples_ok) THROW(CommandLineParsingException, "Given sample '" + name + "' failed QC check: " + sample->qc);
				return sample;
			}
		}

		//not found
		QByteArrayList sample_names;
		foreach(const QSharedPointer<SampleData>& sample, samples)
		{
			sample_names.append(sample->name);
		}
		foreach(const QSharedPointer<SampleData>& sample, samples_removed)
		{
			sample_names.append(sample->name);
		}
		THROW(CommandLineParsingException, "Given sample name '" + name + "' is invalid. Valid names are: " + sample_names.join(", "));
	}

	void storeSegFile(QSharedPointer<SampleData> sample, QString out, const QList<ResultData>& results, const QVector<QSharedPointer<ExonData>>& exons_removed)
	{
		//write header
		QSharedPointer<QFile> file = Helper::openFileForWriting(out.left(out.size()-4) + ".seg");
		QTextStream outstream(file.data());
		outstream << "#type=GENE_EXPRESSION" << "\n";
		outstream << "#track graphtype=points name=\"" + sample->name+ " CN z-score\" midRange=-2.5:2.5 color=0,0,255 altColor=255,0,0 viewLimits=-5:5 maxHeightPixels=80:80:80" << "\n";
		outstream << "ID	chr	start	end	log2-ratio	copy-number	z-score" << "\n";

		//write valid region details
		foreach(const ResultData& r, results)
		{
			if(r.sample==sample)
			{
				float ncov = r.sample->doc[r.exon->index];
				float ncov_ref = r.sample->ref[r.exon->index];
				float log_ratio = log2(ncov/ncov_ref);
				outstream << "\t" << r.exon->chr.str() << "\t" << r.exon->start << "\t" << r.exon->end << "\t" << QByteArray::number(log_ratio, 'f', 2) << "\t" << r.copies << "\t" << QByteArray::number(r.z, 'f', 2) << "\n";
			}
		}

		//write invalid regions
		foreach(const QSharedPointer<ExonData>& exon, exons_removed)
		{
			outstream << "\t" << exon->chr.str() << "\t" << exon->start << "\t" << exon->end << "\tQC failed\tQC failed\t0.0" << "\n";
		}
	}

	void storeResultAsTSV(const QList<Range>& ranges, const QList<ResultData>& results, QString filename, QStringList annotate, const QHash<QSharedPointer<ExonData>, int>& cnvs_exon, int sample_count, int& regions_overlapping_cnp_regions)
    {
		QSharedPointer<QFile> out = Helper::openFileForWriting(filename);
		QTextStream outstream(out.data());

        //header
		outstream << "#chr\tstart\tend\tsample\tsize\tregion_count\tregion_copy_numbers\tregion_zscores\tregion_cnv_af\tregion_coordinates\toverlaps_cnp_region";
		foreach(QString anno, annotate)
		{
			outstream << "\t" << QFileInfo(anno).baseName();
		}
		outstream << "\n";

		//content
		for (int r=0; r<ranges.count(); ++r)
        {
			const Range& range = ranges[r];
            if (!range.sample->qc.isEmpty()) continue;

			//get copy-number, z-scores, coordinates and genes for adjacent regions
			QByteArrayList copies;
			QByteArrayList zscores;
			QByteArrayList coords;
			QByteArrayList cnv_counts;
			for (int j=range.start; j<=range.end; ++j)
            {
				copies.append(QByteArray::number(results[j].copies));
				zscores.append(QByteArray::number(results[j].z, 'f', 2));
				coords.append(results[j].exon->toString());
				cnv_counts.append(QByteArray::number(1.0f * cnvs_exon[results[j].exon] / sample_count, 'f', 3));
			}

			//check for CNP data
			bool overlaps_cnp_region = false;
			for (int r=range.start; r<=range.end; ++r)
			{
				if (results[r].exon->is_cnp)
				{
					overlaps_cnp_region = true;
					++regions_overlapping_cnp_regions;
				}
			}

			//print output
			QSharedPointer<ExonData> start = results[range.start].exon;
			QSharedPointer<ExonData> end = results[range.end].exon;
			outstream << start->chr.str() << "\t" << start->start << "\t" << end->end << "\t" << range.sample->name << "\t" << (end->end-start->start+1) << "\t" << copies.count() << "\t";
			outstream << copies.join(",") << "\t" << zscores.join(",") << "\t" << cnv_counts.join(",") << "\t" << coords.join(",") << "\t" << (overlaps_cnp_region ? "yes" : "");

			//annotation
			for (int i=0; i<annotate.count(); ++i)
			{
				QSet<QByteArray> set;
				for (int j=range.start; j<=range.end; ++j)
				{
					foreach(const QByteArray& anno, results[j].exon->annotations[i])
					{
						set << VcfFile::decodeInfoValue(anno.trimmed()).toUtf8();
					}
				}

				QList<QByteArray> list = set.toList();
				std::sort(list.begin(), list.end());
				outstream << "\t" << list.join(",");
			}

			//end of line
			outstream << "\n";
		}
    }

	float calculateZ(const QSharedPointer<SampleData>& sample, int e)
    {
		if(sample->ref_stdev[e]==0.0f || sample->ref[e]==0.0f)
        {
			return std::numeric_limits<float>::quiet_NaN();
        }

		return BasicStatistics::bound((sample->doc[e]-sample->ref[e]) / sample->ref_stdev[e], -10.0f, 10.0f);
    }

	short calculateCopies(const QSharedPointer<SampleData>& s, const QSharedPointer<ExonData>& e)
	{
		float copies = 2.0f*s->doc[e->index]/s->ref[e->index];
		if (copies<0.2f) return 0;
		else if (copies<1.0f) return 1;
		else return roundf(copies);
    }

	QPair<float, float> weightedMean(const QVector<QSharedPointer<ExonData>>& exons, const QSharedPointer<SampleData>& sample)
    {
		double wsum_auto = 0.0;
		double wsum_chrx = 0.0;
		double size_sum_auto = 0.0;
		double size_sum_chrx = 0.0;
		for (int e=0; e<exons.count(); ++e)
		{
			if (exons[e]->is_cnp) continue;

			int size = exons[e]->end-exons[e]->start;
			if (exons[e]->chr.isAutosome() ||  exons[e]->is_par)
			{
				wsum_auto += (double)(sample->doc[e]) * size;
				size_sum_auto += size;
			}
			else if (exons[e]->chr.isX())
			{
				wsum_chrx += (double)(sample->doc[e]) * size;
				size_sum_chrx += size;
			}
		}

		return qMakePair(size_sum_auto==0.0 ? 0.0 : wsum_auto/size_sum_auto, size_sum_chrx==0.0 ? 0.0 : wsum_chrx/size_sum_chrx);
    }

	bool previousExists(const QList<ResultData>& results, int i)
    {
        //no previous result
        if (i==0) return false;
        //not same sample
		if (results[i-1].sample!=results[i].sample) return false;
        //not same chromosome
		if (results[i].exon->chr!=results[i-1].exon->chr) return false;

        return true;
    }

	void printRegionDistributionCV(const QVector<QSharedPointer<ExonData>>& exons, QTextStream& outstream)
	{
		outstream << "Region coefficient of variation (normalized depth of coverage) histogram:" << endl;
		Histogram hist(0.0, 0.5, 0.05);
		for (int e=0; e<exons.count(); ++e)
		{
			hist.inc(exons[e]->mad/exons[e]->median, true);
		}
        hist.print(outstream, "  ", 2, 0);
		outstream << endl;
	}

	void printSampleDistributionCNVs(const QVector<QSharedPointer<SampleData>>& samples, const QHash<QSharedPointer<SampleData>, int>& cnvs_sample, QTextStream& outstream)
	{
        //determine mean/stdev of CNV counts
		QVector<float> counts;
        for (auto it=cnvs_sample.cbegin(); it!=cnvs_sample.cend(); ++it)
        {
            counts.append(it.value());
        }
		std::sort(counts.begin(), counts.end());
		float median = fun_median(counts);
		float mad = 1.482f * fun_mad(counts, median);

        //historgam
		outstream << "CNVs per sample histogram:" << endl;
		double max = median + 3.0*mad;
		max = ceil(max / 20.0) * 20.0; //round to next highest number that can be devided by 20
		Histogram hist(0.0, max, max/20);
        for (int s=0; s<samples.count(); ++s)
        {
			hist.inc(cnvs_sample[samples[s]], true);
        }
		hist.print(outstream, "  ", 0, 0);
		outstream << endl;
    }

	void printCnvRegionCountDistribution(QList<Range> ranges, QTextStream& outstream)
	{
		outstream << "Region count of CNV events histogram:" << endl;
		Histogram hist(0.0, 15.0, 1.0);
		foreach(const Range& range, ranges)
		{
			hist.inc(range.size(), true);
		}
		hist.print(outstream, "  ", 0, 0);
		outstream << endl;
	}

	void printZScoreDistribution(const QList<ResultData>& results, QTextStream& outstream)
	{
		outstream << "Overall z-score histogram:" << endl;
		Histogram hist(-6, 6, 1.0);
		for (int r=0; r<results.count(); ++r)
		{
			hist.inc(results[r].z, true);
		}
		hist.print(outstream, "  ", 0, 0, true);
		outstream << endl;
	}

	void printSampleDistributionCorrelation(const QVector<QSharedPointer<SampleData>>& samples, QTextStream& outstream)
	{
		outstream << "Reference sample correlation histogram:" << endl;
		Histogram hist(0.8, 1.0, 0.02);
        for (int s=0; s<samples.count(); ++s)
        {
			hist.inc(samples[s]->ref_correl, true);
        }
        hist.print(outstream, "  ", 2, 0, false);
		outstream << endl;
    }

    virtual void main()
    {
        //init
		QString debug = getString("debug");
		QString seg = getString("seg");
		QStringList in = getInfileList("in");
		if (in.count()==1)
		{
			in = Helper::loadTextFile(in[0], true, '#', true);
		}
		QString out = getOutfile("out");
		if (!out.endsWith(".tsv")) THROW(ArgumentException, "Output file name has to end with '.tsv'!");
        QTextStream outstream(stdout);
        int n = getInt("n");
		if (in.count()<n+1) THROW(ArgumentException, "At least n+1 input files are required! Got " + QByteArray::number(in.count()) + "!");
		float min_z = getFloat("min_z");
		float ext_min_z = getFloat("ext_min_z");
		float ext_gap_span = getFloat("ext_gap_span");
		float reg_min_ncov = getFloat("reg_min_ncov");
		float reg_min_cov = getFloat("reg_min_cov");
		float reg_max_cv = getFloat("reg_max_cv");
		float sam_min_corr = getFloat("sam_min_corr");
		float sam_min_depth = getFloat("sam_min_depth");
		int sam_corr_regs = getInt("sam_corr_regs");
		QString par = getString("par");
		QString cnp_file = getInfile("cnp_file");
		QStringList annotate = getInfileList("annotate");
		int gc_window = getInt("gc_window");
		int gc_extend = getInt("gc_extend");

		//timing
		QTime timer;
		timer.start();
		QList<QByteArray> timings;

		//load exon list
		QVector<QSharedPointer<ExonData>> exons;
		{
			//load pseudoautosomal regions
			BedFile par_regs;
			QStringList parts = par.split(',');
			foreach(QString part, parts)
			{
				QStringList parts2 = part.split('-');
				if (parts2.count()==2)
				{
					int start = Helper::toInt(parts2[0], "PAR region start");
					int end = Helper::toInt(parts2[1], "PAR region end");
					par_regs.append(BedLine("chrX", start, end));
				}
			}
			ChromosomalIndex<BedFile> par_index(par_regs);

			//load CNP list
			BedFile cnp_regs;
			if (!cnp_file.isEmpty()) cnp_regs.load(cnp_file);
			ChromosomalIndex<BedFile> cnp_regs_index(cnp_regs);


			//load input data
			TSVFileStream stream(in[0]);
			while(!stream.atEnd())
			{
				QByteArrayList parts = stream.readLine();
				if (parts.count()<4) THROW(FileParseException, "Coverage file " + in[0] + " contains line with less then four elements: " + parts.join('\t'));
				QSharedPointer<ExonData> ex(new ExonData());
				ex->chr = parts[0];
				ex->start_str = parts[1];
				bool ok = false;
				ex->start = ex->start_str.toInt(&ok);
				if (!ok) THROW(ArgumentException, "Could not convert coverage value '" + ex->start_str + "' to float.");
				ex->end_str = parts[2];
				ex->end = ex->end_str.toInt(&ok);
				if (!ok) THROW(ArgumentException, "Could not convert coverage value '" + ex->end_str + "' to float.");
				ex->index = exons.count();
				ex->is_par = ex->chr.isX() && par_index.matchingIndex(ex->chr, ex->start, ex->end)!=-1;
				ex->is_cnp = (!cnp_file.isEmpty() && cnp_regs_index.matchingIndex(ex->chr, ex->start, ex->end)!=-1);

				//check that exons are sorted according to chromosome and start position
				if (exons.count()!=0 && ex->chr==exons.last()->chr)
				{
					if(ex->start<exons.last()->start)
					{
						THROW(FileParseException, "Exons not sorted according to chromosome/position! " + ex->chr.str() + ":" + QByteArray::number(ex->start) + " after " + ex->chr.str() + ":" + QByteArray::number(exons.last()->start) + "!");
					}
				}

				//append exon to data
				exons.append(ex);
			}
		}

		//annotate regions (generic)
		foreach(QString anno, annotate)
		{
			BedFile anno_file;
			anno_file.load(anno);
			if (!anno_file.isSorted()) anno_file.sort();
			ChromosomalIndex<BedFile> anno_index(anno_file);
			for (int i=0; i<exons.count(); ++i)
			{
				QSet<QByteArray> annos;
				QVector<int> indices = anno_index.matchingIndices(exons[i]->chr, exons[i]->start, exons[i]->end);
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
				exons[i]->annotations.append(annos);
			}
		}

		//annotate regions (GC content)
		if (gc_window>=0)
		{
			QString ref_file = getInfile("ref");
			if (ref_file=="") ref_file = Settings::string("reference_genome", true);
			if (ref_file=="") THROW(CommandLineParsingException, "Reference genome FASTA unset in both command-line and settings.ini file!");
			if (ref_file!="")
			{
				FastaFileIndex reference(ref_file);

				//determine GC content of exons
				for (int e=0; e<exons.count(); ++e)
				{
					Sequence seq = reference.seq(exons[e]->chr, exons[e]->start+1-gc_extend, exons[e]->end-exons[e]->start+1+2*gc_extend, true);
					int gc = 0;
					int at = 0;
					for(int i=0; i<seq.length(); ++i)
					{
						if (seq[i]=='G' || seq[i]=='C') ++gc;
						else if (seq[i]=='A' || seq[i]=='T') ++at;
					}
					exons[e]->gc = (double)gc/(gc+at);
				}
			}
		}


		//load input (and check input)
		QVector<QSharedPointer<SampleData>> samples;
		for (int i=0; i<in.count(); ++i)
        {
            //init
			QSharedPointer<SampleData> sample(new SampleData());
			sample->name = QFileInfo(in[i]).baseName().toUtf8();
			sample->doc.reserve(exons.count());

			//depth-of-coverage data
			TSVFileStream stream(in[i]);
			int line_count = 0;
			while(!stream.atEnd())
            {
				QByteArrayList parts = stream.readLine();
				if (parts.count()<4)
				{
					THROW(FileParseException, "Coverage file " + in[i] + " contains line with less then four elements: " + parts.join('\t'));
				}
				if (parts[0]!=exons[line_count]->chr.str() || parts[1]!=exons[line_count]->start_str || parts[2]!=exons[line_count]->end_str)
				{
					THROW(FileParseException, "Coverage file " + in[i] + " contains different regions than reference file " + in[0] + ". Expected " + exons[line_count]->toString() + ", got " + parts[0] + ":" + parts[1] + "-" + parts[2] + ".");
				}
				bool ok = false;
				float value = parts[3].toFloat(&ok);
				if (!ok) THROW(ArgumentException, "Could not convert coverage value '" + parts[3] + "' to float.");

				sample->doc.append(value);
				++line_count;
            }

			//check exon count
			if (line_count!=exons.count()) THROW(FileParseException, "Coverage file " + in[i] + " contains more/less regions than reference file " + in[0] + ". Expected " + QByteArray::number(exons.count()) + ", got " + QByteArray::number(line_count) + ".");

			samples.append(sample);
		}


		//count gonosome regions
		outstream << "=== normalizing depth-of-coverage data ===" << endl;
        int c_chrx = 0;
        int c_chry = 0;
        int c_chro = 0;
        for (int e=0; e<exons.count(); ++e)
        {
			if (exons[e]->chr.isX())
			{
				if (!exons[e]->is_par)
				{
					++c_chrx;
				}
            }
			else if (exons[e]->chr.isY())
			{
				++c_chry;
			}
			else if (!exons[e]->chr.isAutosome())
            {
                ++c_chro;
            }
        }
        int c_auto = exons.count() - c_chrx - c_chry -c_chro;
		outstream << "number of autosomal regions (including PAR): " << c_auto << endl;
		outstream << "number of regions on chrX (excluding PAR): " << c_chrx << endl;
		outstream << "number of regions on chrY (ignored): " << c_chry << endl;
		outstream << "number of regions on other chromosomes (ignored): " << c_chro << endl << endl;

		//calculate and store mean DOC (for autosomes/gonosomes separately)
		for (int s=0; s<samples.count(); ++s)
		{
			QPair<float, float> tmp = weightedMean(exons, samples[s]);
			float mean_auto = tmp.first;
			float mean_chrx = tmp.second;

			//store mean
			samples[s]->doc_mean = (c_chrx>c_auto) ? mean_chrx : mean_auto;
			if (!fun_isValidFloat(samples[s]->doc_mean))
			{
				THROW(ProgrammingException, "Mean depth of coverage (DOC) is invalid for sample '" + samples[s]->name + "': " + QByteArray::number(samples[s]->doc_mean));
			}

			//flag low-depth samples
			if (samples[s]->doc_mean < sam_min_depth)
            {
				samples[s]->qc += "avg_depth=" + QByteArray::number(samples[s]->doc_mean) + " ";
            }
			if (c_chrx>0 && mean_chrx < sam_min_depth)
			{
				samples[s]->qc += "avg_depth_chrx=" + QByteArray::number(mean_chrx) + " ";
			}
			if (c_auto>0 && mean_auto < sam_min_depth)
			{
				samples[s]->qc += "avg_depth_autosomes=" + QByteArray::number(mean_auto) + " ";
			}
		}

		timings.append("loading input: " + Helper::elapsedTime(timer));
		timer.restart();

		//check that we have enough high-quality samples
		int high_quality_samples = 0;
		for (int s=0; s<samples.count(); ++s)
		{
			high_quality_samples += samples[s]->qc.isEmpty();
		}
		if (high_quality_samples==0)
		{
			QStringList tmp;
			for (int s=0; s<samples.count(); ++s)
			{
				tmp << samples[s]->name + ": " + samples[s]->qc;
			}
			THROW(ArgumentException, "All samples have failed QC:\n" + tmp.join("\n"));
		}

		//normalize DOC by GC content
		if (gc_window>0)
		{
			//determine GC content of exons
			QVector<QPair<int, double>> gc_indices;
			gc_indices.reserve(exons.count());
			for (int e=0; e<exons.count(); ++e)
			{
				gc_indices.append(qMakePair(e, exons[e]->gc));
			}

			//sort exons by gc content
			std::sort(gc_indices.begin(), gc_indices.end(), [](const QPair<int, double>& a, const QPair<int, double>& b){ return a.second < b.second ;});

			//normalize samples
			for (int s=0; s<samples.count(); ++s)
			{
				movingMedianNormalization(samples[s]->doc, gc_indices, gc_window);
			}
		}
		timings.append("normalizing data (GC content): " + Helper::elapsedTime(timer));
		timer.restart();

		//normalize by mean DOC (for autosomes/gonosomes separately)
		for (int s=0; s<samples.count(); ++s)
		{
			//calculate mean depths
			QPair<float, float> tmp = weightedMean(exons, samples[s]);
			float mean_auto = tmp.first;
			float mean_chrx = tmp.second;

			//normalize
			for (int e=0; e<exons.count(); ++e)
			{
				if ((exons[e]->chr.isAutosome() || exons[e]->is_par) && mean_auto>0)
				{
					samples[s]->doc[e] /= mean_auto;
				}
				else if (exons[e]->chr.isX() && !exons[e]->is_par && mean_chrx>0)
				{
					samples[s]->doc[e] /= mean_chrx;
				}
				else
				{
					samples[s]->doc[e] = 0;
				}
			}
		}
		timings.append("normalizing data (mean): " + Helper::elapsedTime(timer));
		timer.restart();

		//calculate overall average depth (of good samples)
		float avg_abs_cov = 0.0f;
		int samples_valid = 0;
		for (int s=0; s<samples.count(); ++s)
		{
            if (samples[s]->qc.isEmpty())
			{
				avg_abs_cov += samples[s]->doc_mean;
				++samples_valid;
			}
		}
		avg_abs_cov /= samples_valid;

        //region QC
		outstream << "=== checking for bad regions ===" << endl;
		int c_bad_region = 0;

        for (int e=0; e<exons.count(); ++e)
		{
			QVector<float> tmp;
			tmp.reserve(samples.count());
			for (int s=0; s<samples.count(); ++s)
            {
                if (samples[s]->qc.isEmpty())
                {
                    //check that DOC data for good samples is ok
					if (!fun_isValidFloat(samples[s]->doc[e]))
                    {
						THROW(ProgrammingException, "Normalized coverage value is invalid for sample '" + samples[s]->name + "' in exon '" + exons[e]->toString() + "' (" + QByteArray::number(samples[s]->doc[e]) + ")");
					}
					tmp.append(samples[s]->doc[e]);
                }
			}
            std::sort(tmp.begin(), tmp.end());
			float median = fun_median(tmp);
			float mad = 1.482f * fun_mad(tmp, median);

			if (median<reg_min_ncov) exons[e]->qc += "ncov<" + QByteArray::number(reg_min_ncov) + " ";
			if (median*avg_abs_cov<reg_min_cov) exons[e]->qc += "cov<" + QByteArray::number(reg_min_cov) + " ";
			if (mad/median>reg_max_cv) exons[e]->qc += "cv>" + QByteArray::number(reg_max_cv)+ " ";
			if (exons[e]->chr.isY()) exons[e]->qc += "chrY ";
			exons[e]->median = median;
			exons[e]->mad = mad;
            if (!exons[e]->qc.isEmpty())
			{
                ++c_bad_region;
            }
        }
        outstream << "bad regions: " << c_bad_region << " of " << exons.count() << endl << endl;
        printRegionDistributionCV(exons, outstream);
		timings.append("detecting bad regions: " + Helper::elapsedTime(timer));
		timer.restart();

		//determine exons to use for correlation (not inside copy-number-polymorphism regions, not chrMT or chrY)
		QVector<int> exon_indices;
		exon_indices.reserve(exons.count());
		for(int i=0; i<exons.count(); ++i)
		{
			if (!exons[i]->is_cnp && (exons[i]->chr.isAutosome() || exons[i]->chr.isX()))
			{
				exon_indices.append(i);
			}
		}

		//downsample exons if too many
		int steps = std::max(1, exon_indices.count() / sam_corr_regs);
		if (steps>1)
		{
			QVector<int> tmp = exon_indices;
			exon_indices.clear();

			for(int i=0; i<tmp.count(); i += steps)
			{
				exon_indices.append(tmp[i]);
			}
		}

		//calculate correlation between all samples
		for (int i=0; i<samples.count(); ++i)
		{
			samples[i]->correl_all.reserve(samples.count());

            //calculate correlation to all other samples
			for (int j=0; j<samples.count(); ++j)
            {
                if (i==j)
                {
					samples[i]->correl_all.append(SampleCorrelation(samples[j], -1.0f));
                }
                else
				{
					//calculdate inverse of RMSD as proxy for correlation (faster to calculate and gives better results)
					double rmsd = 0.0;
					for(int k=0; k<exon_indices.count(); ++k)
                    {
						const int e = exon_indices[k];
						rmsd += std::pow(samples[j]->doc[e]-samples[i]->doc[e], 2);
					}
					rmsd = 1.0 / std::sqrt(rmsd/exon_indices.count());
					samples[i]->correl_all.append(SampleCorrelation(samples[j], rmsd));
				}
			}

			//sort by correlation (reverse)
			std::sort(samples[i]->correl_all.begin(), samples[i]->correl_all.end(), [](const SampleCorrelation& a, const SampleCorrelation& b){return a.rmsd_inv > b.rmsd_inv;});
		}
		timings.append("calculating sample correlations: " + Helper::elapsedTime(timer));
		timer.restart();

        //construct reference from 'n' most similar samples
		outstream << "=== checking for bad samples ===" << endl;
		int c_bad_sample = 0;
		for (int s=0; s<samples.count(); ++s)
		{
			//reserve space
			samples[s]->ref.reserve(exons.count());
			samples[s]->ref_stdev.reserve(exons.count());

			//calcualte reference mean and stddev for each exon
			for (int e=0; e<exons.count(); ++e)
			{
				float exon_median = exons[e]->median;
				QVector<float> tmp;
				tmp.reserve(n);
				for (int i=0; i<samples.count()-1; ++i)
				{
					//do not use bad QC samples
					if (!samples[s]->correl_all[i].sample->qc.isEmpty()) continue;

					//do not use extreme outliers
					float value = samples[s]->correl_all[i].sample->doc[e];
					if (value<0.25f*exon_median || value>1.75f*exon_median) continue;

					tmp.append(value);

					if (tmp.count()==n) break;
                }
				if (tmp.count()==n)
				{
					std::sort(tmp.begin(), tmp.end());
					float median = fun_median(tmp);
					samples[s]->ref.append(median);
					std::for_each(tmp.begin(), tmp.end(), [median](float& value){ value = fabsf(value-median); });
					std::sort(tmp.begin(), tmp.end());
					float stdev = 1.482f * fun_median(tmp);
					samples[s]->ref_stdev.append(std::max(stdev, 0.1f*median));
                }
				else
				{
					samples[s]->ref.append(exon_median);
					samples[s]->ref_stdev.append(0.3f*exon_median);
                }
            }
			samples[s]->ref_correl = fun_correlation(samples[s]->doc, samples[s]->ref);


            //flag samples with bad correlation
			if (samples[s]->ref_correl<sam_min_corr)
            {
				samples[s]->qc += "corr=" + QByteArray::number(samples[s]->ref_correl, 'f', 3) + " ";
            }

            //print all bad samples (also those which were flagged as bad before, e.g. because os too low avg depth)
            if (!samples[s]->qc.isEmpty())
            {
				++c_bad_sample;
            }
		}
		outstream << "bad samples: " << c_bad_sample << " of " << samples.count() << endl << endl;
        printSampleDistributionCorrelation(samples, outstream);
		timings.append("constructing reference samples: " + Helper::elapsedTime(timer));
		timer.restart();

        //remove bad samples
        QVector<QSharedPointer<SampleData>> samples_removed;
        int to = 0;
		for (int s=0; s<samples.count(); ++s)
        {
            if(samples[s]->qc.isEmpty())
            {
				if (to!=s) samples[to] = samples[s];
                ++to;
            }
            else
            {
                samples_removed.append(samples[s]);
            }
        }
		samples.resize(to);

		//remove bad regions
        QVector<QSharedPointer<ExonData>> exons_removed;
		to = 0;
		for (int e=0; e<exons.count(); ++e)
		{
            if(exons[e]->qc.isEmpty())
			{
				if (to!=e)
				{
					exons[to] = exons[e];
					exons[to]->index = to;
					for (int s=0; s<samples.count(); ++s)
					{
						samples[s]->doc[to] = samples[s]->doc[e];
						samples[s]->ref[to] = samples[s]->ref[e];
						samples[s]->ref_stdev[to] = samples[s]->ref_stdev[e];
					}
				}
				++to;
			}
            else
            {
                exons_removed.append(exons[e]);
            }
		}
		exons.resize(to);
		for (int s=0; s<samples.count(); ++s)
		{
			samples[s]->doc.resize(to);
			samples[s]->ref.resize(to);
			samples[s]->ref_stdev.resize(to);
		}
		timings.append("removing bad samples: " + Helper::elapsedTime(timer));
		timer.restart();

        //detect CNVs from DOC data
		outstream << "=== CNV seed detection ===" << endl;
		int index = 0;
		QList<Range> ranges;
		QList<ResultData> results;
		results.reserve(exons.count() * samples.count());
		for (int s=0; s<samples.count(); ++s)
        {
            for (int e=0; e<exons.count(); ++e)
            {
                //detect CNVs
				float z = calculateZ(samples[s], e);
				ResultData res(samples[s], exons[e], z);
                if (
                        z<=-min_z //statistical outlier (del)
                        || z>=min_z //statistical outlier (dup)
						|| (samples[s]->ref[e]>=reg_min_ncov && samples[s]->ref[e]*avg_abs_cov>=reg_min_cov && samples[s]->doc[e]<0.1f*samples[s]->ref[e]) //region with homozygous deletion which is not detected by statistical outliers
                        )
				{
					res.copies = calculateCopies(samples[s], exons[e]);

                    //warn if there is something wrong with the copy number estimation
					if (res.copies==2)
                    {
						outstream << "  WARNING: Found z-score outlier (" << z << ") with estimated copy-number (i.e. rounded ratio) equal to 2!" << endl;
					}
					else
					{
						ranges.append(Range(samples[s], index, index, res.copies<2 ? Range::DEL : Range::INS));
					}
				}
				results.append(res);
				++index;
			}
        }
		outstream << "detected " << ranges.count() << " seed regions" << endl << endl;
		timings.append("CNV seed detection: " + Helper::elapsedTime(timer));
		printZScoreDistribution(results, outstream);
		timer.restart();

        //extending initial CNVs in both directions
		outstream << "=== CNV extension ===" << endl;
		int c_extended = 0;
		for (int r=0; r<ranges.count(); ++r)
		{
			Range& range = ranges[r];
			const Chromosome& range_chr = results[range.start].exon->chr;

            //extend to left
			int i=range.start-1;
            while(i>0 && results[i].copies==2)
            {
                const ResultData& curr = results[i];
				if (curr.sample!=range.sample) break; //same sample
				if (curr.exon->chr!=range_chr) break; //same chromosome
				int copies = calculateCopies(curr.sample, curr.exon);
				if (range.type==Range::DEL) //del
				{
					if (curr.z>-ext_min_z) break;
					if (copies>=2) break;
				}
				else //dup
				{
					if (curr.z<ext_min_z) break;
					if (copies<=2) break;
				}

                results[i].copies = copies;
				range.start = i;
                //outstream << "    EX LEFT " << data[curr.s].filename << " " << exons[curr.e].name << " " << curr.z << " " << copies << endl;
				++c_extended;
                --i;
            }

            //extend to right
			i=range.end+1;
            while(i<results.count() && results[i].copies==2)
            {
                const ResultData& curr = results[i];
				if (curr.sample!=range.sample) break; //same sample
				if (curr.exon->chr!=range_chr) break; //same chromosome
				int copies = calculateCopies(curr.sample, curr.exon);
				if (range.type==Range::DEL) //same CNV type (del)
				{
					if (curr.z>-ext_min_z) break;
					if (copies>=2) break;
				}
				else //same CNV type (dup)
				{
					if (curr.z<ext_min_z) break;
					if (copies<=2) break;
				}

                results[i].copies = copies;
				range.end = i;
                //outstream << "    EX RIGH " << data[curr.s].filename << " " << exons[curr.e].name << " " << curr.z << " " << copies << endl;
				++c_extended;
                ++i;
            }
        }
		outstream << "extended seeds to " << c_extended << " additional regions" << endl << endl;
		timings.append("CNV seed extension: " + Helper::elapsedTime(timer));
		timer.restart();

		//merge adjacent ranges
		outstream << "=== merging adjacent CNV regions to larger events ===" << endl;
		int c_ranges_before_merge = ranges.count();
		for (int r=ranges.count()-2; r>=0; --r)
		{
			Range& first = ranges[r];
			Range& second = ranges[r+1];
			if(first.type!=second.type) continue; //same type (ins/del)
			if(first.end!=second.start-1) continue; //subsequent exons
			if(first.sample!=second.sample) continue; //same sample
			if(results[first.start].exon->chr!=results[second.start].exon->chr) continue; //same chromosome

			first.end = second.end;
			ranges.removeAt(r+1);
		}

		//merge adjacent regions to bridge gaps with CN=2
		if (ext_gap_span>0)
		{
			int regs_before = 0;
			int regs_after = 1;
			while(regs_before!=regs_after)
			{
				regs_before = ranges.count();
				for (int r=ranges.count()-2; r>=0; --r)
				{
					Range& first = ranges[r];
					Range& second = ranges[r+1];
					if(first.type!=second.type) continue; //same type (ins/del)
					if(first.sample!=second.sample) continue; //same sample
					if(results[first.start].exon->chr!=results[second.start].exon->chr) continue; //same chromosome
					const int dist = second.start-first.end-1;
					if (dist>ext_gap_span/100.0*(first.size() + second.size())) continue; //gap not too big

					//check that no region with the wrong trend is in between
					bool skip = false;
					for (int i=first.end+1; i<second.start; ++i)
					{
						if (first.type==Range::INS && results[i].z<0.0)
						{
							skip = true;
							break;
						}
						if (first.type==Range::DEL && results[i].z>0.0)
						{
							skip = true;
							break;
						}
					}
					if (skip) continue;

					//update estimated copy number
					for (int i=first.end+1; i<second.start; ++i)
					{
						results[i].copies = calculateCopies(results[i].sample, results[i].exon);
					}

					//merge ranges
					first.end = second.end;
					ranges.removeAt(r+1);
				}
				regs_after = ranges.count();
			}
		}
		outstream << "merged " << c_ranges_before_merge << " to " << ranges.count() << " ranges" << endl << endl;
		timings.append("CNV merging: " + Helper::elapsedTime(timer));
		timer.restart();

		//count CNVs per sample/region
		QHash<QSharedPointer<ExonData>, int> cnvs_exon;
		QHash<QSharedPointer<SampleData>, int> cnvs_sample;
		for (int r=0; r<ranges.count(); ++r)
		{
			cnvs_sample[ranges[r].sample] += 1;
			for (int i=ranges[r].start; i<=ranges[r].end; ++i)
			{
				if (results[i].copies!=2)
				{
					cnvs_exon[results[i].exon] += 1;
				}
			}
		}
        printSampleDistributionCNVs(samples, cnvs_sample, outstream);

		//print region count of CNV events
		printCnvRegionCountDistribution(ranges, outstream);

		//store result files
		int regions_overlapping_cnp_regions = 0;
		storeResultAsTSV(ranges, results, out, annotate, cnvs_exon, samples.count(), regions_overlapping_cnp_regions);
		storeSampleInfo(out, samples, samples_removed, cnvs_sample, results);
		storeRegionInfo(out, exons, exons_removed, cnvs_exon);
		if (debug!="")
		{
			QSharedPointer<SampleData> sample;
			if (debug!="ALL") sample = sampleByName(debug.toUtf8(), samples, samples_removed, false);

			storeDebugInfo(sample, out, samples, results);
		}
		if (seg!="")
		{
			QSharedPointer<SampleData> sample = sampleByName(seg.toUtf8(), samples, samples_removed, true);
			if (sample->qc.isEmpty())
			{
				storeSegFile(sample, out, results, exons_removed);
			}
			else
			{
				outstream << "  WARNING: Skipping SEG file creation because sample '" + seg + "' failed QC check: " + sample->qc << endl;
			}
		}

		//print statistics
		float corr_sum = 0;
		foreach(const QSharedPointer<SampleData>& sample, samples)
        {
            if (sample->qc.isEmpty())
            {
				corr_sum += sample->ref_correl;
            }
        }
		int c_valid = in.count() - c_bad_sample;
		outstream << "=== statistics ===" << endl;
		outstream << "invalid regions: " << c_bad_region << " of " << (exons.count() + c_bad_region) << endl;
		outstream << "invalid samples: " << c_bad_sample << " of " << in.count() << endl;
		outstream << "mean correlation of samples to reference: " << QByteArray::number(corr_sum/c_valid, 'f', 4) << endl;
		long long size_sum = 0;
		foreach(const Range& range, ranges)
		{
			size_sum += range.size();
		}
		outstream << "number of CNV events: " << ranges.count() << " (consisting of " << size_sum << " regions)" << endl;
		outstream << "mean regions per CNV event: " << QByteArray::number((double)size_sum/ranges.count(), 'f', 2) << endl;
		outstream << "mean CNV events per sample per 1000 regions: " << QByteArray::number(1.0f*ranges.count()/c_valid/(exons.count()/1000.0), 'f', 4) << endl;
		outstream << "CNV regions overlapping CNP regions: " << QByteArray::number(100.0f * regions_overlapping_cnp_regions/size_sum, 'f', 2) << '%' << endl;
        outstream << endl;

        //print timing output
		timings.append("statistics and writing output: " + Helper::elapsedTime(timer));
		outstream << "=== timing ===" << endl;
		foreach(const QByteArray& line, timings)
		{
			outstream << line << endl;
		}
    }
};

#include "main.moc"

int main(int argc, char *argv[])
{
    ConcreteTool tool(argc, argv);
    return tool.execute();
}

