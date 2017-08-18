#include "ToolBase.h"
#include "Helper.h"
#include "Exceptions.h"
#include "BedFile.h"
#include "NGSHelper.h"
#include "NGSD.h"
#include "Histogram.h"
#include "GeneSet.h"
#include "TSVFileStream.h"

#include <QVector>
#include <QFileInfo>
#include <QDir>

class SampleCorrelation;

//Sample representation
struct SampleData
{
	SampleData()
    {
    }

	QByteArray name; //file name
	bool noref;

	QVector<float> doc; //coverage data (normalized: divided by mean)
	float doc_mean; //mean coverage before normalizazion (aterwards it is 1.0)
	float doc_stdev; //stdev of coverage after normalization

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
		, correlation(-1)
	{
	}

	SampleCorrelation(QSharedPointer<SampleData> s, float c)
		: sample(s)
		, correlation(c)
	{
	}

	QSharedPointer<SampleData> sample;
	float correlation;
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
    {
    }

    Chromosome chr; //chromosome
    int start; //start position
	QByteArray start_str; //start position as string
    int end; //end position
	QByteArray end_str; //end position as string
	int index; //exon index (needed to access DOC data arrays of samples)

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
        addInfileList("in", "Input TSV files (one per sample) containing coverage data (chr, start, end, avg_depth).", false, true);
        addOutfile("out", "Output TSV file containing the detected CNVs.", false, true);
		//optional
		addInfileList("in_noref", "Input TSV files like 'in' but not used as reference (e.g. tumor samples).", true, true);
		addInt("n", "The number of most similar samples to consider.", true, 20);
        addInfile("exclude", "BED file with regions to exclude from the analysis.", true, true);
        addFloat("min_z", "Minimum z-score for CNV seed detection.", true, 4.0);
		addFloat("ext_min_z", "Minimum z-score for CNV extension around seeds.", true, 2.0);
		addFloat("ext_gap_span", "Percentage of orignal region size that can be spanned while merging nearby regions (0 disables it).", true, 20.0);
        addFloat("sam_min_depth", "QC: Minimum average depth of a sample.", true, 40.0);
		addFloat("sam_min_corr", "QC: Minimum correlation of sample to constructed reference sample.", true, 0.95);
		addInt("sam_corr_regs", "Maximum number of regions used for sample correlation calculation (to speed up comparisons for exoms etc.).", true, 20000);
        addFloat("reg_min_cov", "QC: Minimum (average) absolute depth of a target region.", true, 20.0);
		addFloat("reg_min_ncov", "QC: Minimum (average) normalized depth of a target region.", true, 0.01);
        addFloat("reg_max_cv", "QC: Maximum coefficient of variation (median/mad) of target region.", true, 0.3);
		addFlag("anno", "Enable annotation of gene names to regions (needs the NGSD database).");
		addFlag("test", "Uses test database instead of production database for annotation.");
		addString("debug", "Writes debug information for the sample matching the given name (or for all samples if 'ALL' is given).", true, "");
		addString("seg", "Writes a SEG file for the sample matching the given name (used for visualization in IGV).", true);

		changeLog(2017, 8,  17, "Added down-sampleing if input to speed up sample correlation (sam_corr_regs parameter).");
		changeLog(2017, 8,  15, "Added allele frequency for each region to TSV output.");
		changeLog(2016, 10, 24, "Added copy-number variant size to TSV output and added optional SEG output file.");
		changeLog(2016, 9,   1, "Sample and region information files are now always written.");
		changeLog(2016, 8,  23, "Added merging of large CNVs that were split to several regions due to noise.");
		changeLog(2016, 8,  21, "Improved log output (to make parameter optimization easier).");
	}

	//TODO: replace fun_* by BasicStatistics (rewrite the functions as template functions)
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

	float fun_stdev(const QVector<float>& data, float mean)
	{
		if (data.count()==0) THROW(StatisticsException, "Cannot calculate standard deviation on empty data array.");

		double output = 0.0;
		for (int i=0; i<data.count(); ++i)
		{
			output += pow(data[i]-mean, 2.0);
		}
		return sqrt(output/data.count());
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

	float fun_q1(const QVector<float>& data)
	{
		if (data.count()==0) THROW(StatisticsException, "Cannot calculate q1 on empty data array!");

		return data[data.count()/4];
	}

	float fun_q3(const QVector<float>& data)
	{
		if (data.count()==0) THROW(StatisticsException, "Cannot calculate q3 on empty data array!");

		return data[3*data.count()/4];
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

	float fun_bound(float value, float lower_bound, float upper_bound)
	{
		if (value<lower_bound) return lower_bound;
		if (value>upper_bound) return upper_bound;
		return value;
	}

	void storeSampleInfo(QString out, const QVector<QSharedPointer<SampleData>>& samples, const QVector<QSharedPointer<SampleData>>& samples_removed, const QHash<QSharedPointer<SampleData>, int>& cnvs_sample, const QVector<ResultData>& results)
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
		outstream << "#sample\tref_sample\tdoc_mean\tref_correl\tz_score_mad\tcnvs\tqc_info" << endl;
		foreach(const QSharedPointer<SampleData>& sample, samples)
        {
			outstream << sample->name << "\t" << (sample->noref ? "no" : "yes") << "\t" << QByteArray::number(sample->doc_mean, 'f', 1) << "\t" << QByteArray::number(sample->ref_correl, 'f', 3) << "\t" << QByteArray::number(z_scores[sample], 'f', 3) << "\t" << cnvs_sample[sample] << "\t" << sample->qc << endl;
        }
        foreach(const QSharedPointer<SampleData>& sample, samples_removed)
        {
			outstream << sample->name << "\t" << (sample->noref ? "no" : "yes") << "\t" << QByteArray::number(sample->doc_mean, 'f', 1) << "\t" << QByteArray::number(sample->ref_correl, 'f', 3) << "\t-\t-\t" << sample->qc << endl;
        }
    }

	void storeRegionInfo(QString out, const QVector<QSharedPointer<ExonData>>& exons, const QVector<QSharedPointer<ExonData>>& exons_removed, const QHash<QSharedPointer<ExonData>, int>& cnvs_exon)
    {
        QSharedPointer<QFile> file = Helper::openFileForWriting(out.left(out.size()-4) + "_regions.tsv");
        QTextStream outstream(file.data());
		outstream << "#region\tsize\tndoc_median\tndoc_mad\tndoc_cv\tcnvs\tqc_info" << endl;

        //contruct sorted array
        QVector<QSharedPointer<ExonData>> tmp;
		foreach(const QSharedPointer<ExonData>& exon, exons) tmp.append(exon);
		foreach(const QSharedPointer<ExonData>& exon, exons_removed) tmp.append(exon);
        std::sort(tmp.begin(), tmp.end(), [](const QSharedPointer<ExonData>& a, const QSharedPointer<ExonData>& b){return *(a.data()) < *(b.data());} );
        foreach(const QSharedPointer<ExonData>& exon, tmp)
        {
			outstream << exon->toString() << "\t" << (exon->end-exon->start) << "\t" << QByteArray::number(exon->median, 'f', 3) << "\t" << QByteArray::number(exon->mad, 'f', 3) << "\t" << QByteArray::number(exon->mad/exon->median, 'f', 2) << "\t";
            if (exon->qc.isEmpty())
            {
                outstream << cnvs_exon[exon];
            }
            else
            {
                outstream << "-";
            }
            outstream << "\t" << exon->qc << endl;
        }
    }

	void storeDebugInfo(QSharedPointer<SampleData> debug_sample, QString out, const QVector<QSharedPointer<SampleData>>& samples, const QVector<ResultData>& results)
    {
		//write header
		QSharedPointer<QFile> file = Helper::openFileForWriting(out.left(out.size()-4) + "_debug.tsv");
        QTextStream outstream(file.data());
		outstream << "#sample\tregion\tcopy_number\tz_score\tndoc\tref_ndoc\tref_ndoc_stdev\tlog2_ratio" << endl;

		//write sample correlations
		foreach(const QSharedPointer<SampleData>& sample, samples)
		{
			if (sample==debug_sample)
			{
				outstream << "##correlation of " << sample->name << " to other samples:" << endl;
				for (int i=0; i<samples.count()-1; ++i)
				{
					outstream << "##" << (i+1) << "\t" << sample->correl_all[i].sample->name << "\t" << sample->correl_all[i].correlation << endl;
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
				outstream << r.sample->name << "\t" << r.exon->toString() << "\t" << r.copies << "\t" << QByteArray::number(r.z, 'f', 2) << "\t" << QByteArray::number(ncov, 'f', 3) << "\t" << QByteArray::number(ncov_ref, 'f', 3) << "\t" << QByteArray::number(r.sample->ref_stdev[r.exon->index], 'f', 3) << "\t" << QByteArray::number(log_ratio, 'f', 2) << endl;
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

	void storeSegFile(QSharedPointer<SampleData> sample, QString out, const QVector<ResultData>& results, const QVector<QSharedPointer<ExonData>>& exons_removed)
	{
		//write header
		QSharedPointer<QFile> file = Helper::openFileForWriting(out.left(out.size()-4) + ".seg");
		QTextStream outstream(file.data());
		outstream << "#type=GENE_EXPRESSION" << endl;
		outstream << "#track graphtype=heatmap name=\"" + sample->name+ " CN z-score\" midRange=-2.5:2.5 color=0,0,255 altColor=255,0,0 viewLimits=-5:5 maxHeightPixels=80:80:80" << endl;
		outstream << "ID	chr	start	end	log2-ratio	copy-number	z-score" << endl;

		//write valid region details
		foreach(const ResultData& r, results)
		{
			if(r.sample==sample)
			{
				float ncov = r.sample->doc[r.exon->index];
				float ncov_ref = r.sample->ref[r.exon->index];
				float log_ratio = log2(ncov/ncov_ref);
				outstream << "\t" << r.exon->chr.str() << "\t" << r.exon->start << "\t" << r.exon->end << "\t" << QByteArray::number(log_ratio, 'f', 2) << "\t" << r.copies << "\t" << QByteArray::number(r.z, 'f', 2) << endl;
			}
		}

		//write invalid regions
		foreach(const QSharedPointer<ExonData>& exon, exons_removed)
		{
			outstream << "\t" << exon->chr.str() << "\t" << exon->start << "\t" << exon->end << "\tQC failed\tQC failed\t0.0" << endl;
		}
	}

	GeneSet geneNames(QSharedPointer<NGSD>& db, const QSharedPointer<ExonData>& exon)
	{
		static QHash<QByteArray, GeneSet> cache;

		//check cache first
		QByteArray reg = exon->toString();
		if (cache.contains(reg))
		{
			return cache[reg];
		}

		//get genes from NGSD
		GeneSet tmp = db->genesOverlapping(exon->chr, exon->start, exon->end, 20);
		cache.insert(reg, tmp);
		return tmp;
	}

	void storeResultAsTSV(const QList<Range>& ranges, const QVector<ResultData>& results, QString filename, bool anno, bool test, const QHash<QSharedPointer<ExonData>, int>& cnvs_exon, int sample_count)
    {
		QSharedPointer<QFile> out = Helper::openFileForWriting(filename);
		QTextStream outstream(out.data());
		QSharedPointer<NGSD> db(anno ? new NGSD(test) : nullptr);

        //header
		outstream << "#chr\tstart\tend\tsample\tsize\tregion_count\tregion_copy_numbers\tregion_zscores\tregion_cnv_af\tregion_coordinates" << (anno ? "\tgenes" : "") << endl;
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

			//print output
			QSharedPointer<ExonData> start = results[range.start].exon;
			QSharedPointer<ExonData> end = results[range.end].exon;
			outstream << start->chr.str() << "\t" << start->start << "\t" << end->end << "\t" << range.sample->name << "\t" << (end->end-start->start+1) << "\t" << copies.count() << "\t" << copies.join(",") << "\t" << zscores.join(",") << "\t" << cnv_counts.join(",") << "\t" << coords.join(",");

			//annotation
			if (anno)
			{
				outstream << '\t';

				GeneSet genes;
				for (int j=range.start; j<=range.end; ++j)
				{
					genes.insert(geneNames(db, results[j].exon));
				}
				outstream << genes.join(",");
			}

			outstream << endl;
		}
    }

	float calculateZ(const QSharedPointer<SampleData>& sample, int e)
    {
		if(sample->ref_stdev[e]==0.0f || sample->ref[e]==0.0f)
        {
			return std::numeric_limits<float>::quiet_NaN();
        }

		return fun_bound((sample->doc[e]-sample->ref[e]) / sample->ref_stdev[e], -10.0f, 10.0f);
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
			int size = exons[e]->end-exons[e]->start;
			if (exons[e]->chr.isAutosome())
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

	bool previousExists(const QVector<ResultData>& results, int i)
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
		float mad = 1.428f * fun_mad(counts, median);

        //historgam
        outstream << "CNVs per sample histogram:" << endl;
		double max = median + 3.0*mad;
		Histogram hist(0.0, max, max/20);
        for (int s=0; s<samples.count(); ++s)
        {
            hist.inc(cnvs_sample[samples[s]], true);
        }
        hist.print(outstream, "  ", 2, 0);
        outstream << endl;
    }

	void printZScoreDistribution(const QVector<ResultData>& results, QTextStream& outstream)
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

	void printCorrelationStatistics(const QVector<QSharedPointer<SampleData>>& samples, QTextStream& outstream)
	{
		//extract correlation to n nearest samples
		const int n = std::min(samples.count()-1, 30);
		QVector<QVector<float>> corrs;
		corrs.resize(n);
		for (int s=0; s<samples.count(); ++s)
		{
			for (int i=0; i<n; ++i)
			{
				float corr = samples[s]->correl_all[i].correlation;
				if (corr!=-1)
				{
					corrs[i].append(corr);
				}
			}
		}

		//print output
		outstream << "Sample correlation for different 'n' values:" << endl;
		for (int i=0; i<n; ++i)
		{
			std::sort(corrs[i].begin(), corrs[i].end());
			float median = fun_median(corrs[i]);
			float q1 = fun_q1(corrs[i]);
			float q3 = fun_q3(corrs[i]);
			outstream << QByteArray::number(i+1).rightJustified(4, ' ') << ": q3=" << QByteArray::number(q3, 'f',3) << " median=" << QByteArray::number(median, 'f', 3) << " q1=" << QByteArray::number(q1, 'f',3) << endl;
		}
		outstream << endl;
	}

    virtual void main()
    {
        //init
		QString debug = getString("debug");
		QString seg = getString("seg");
		QStringList in = getInfileList("in");
		QStringList in_noref = getInfileList("in_noref");
		QString out = getOutfile("out");
        if (!out.endsWith(".tsv")) THROW(ArgumentException, "Output file name has to end with '.tsv'!");
		QString exclude = getInfile("exclude");
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

		//timing
		QTime timer;
		timer.start();
		QList<QByteArray> timings;

		//load exon list
		QVector<QSharedPointer<ExonData>> exons;
		{
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

		//load input (and check input)
		QVector<QSharedPointer<SampleData>> samples;
		QStringList in_all;
		in_all << in << in_noref;
		for (int i=0; i<in_all.count(); ++i)
        {
            //init
			QSharedPointer<SampleData> sample(new SampleData());
			sample->name = QFileInfo(in_all[i]).baseName().toLatin1();
			sample->noref = (i>=in.count());
			sample->doc.reserve(exons.count());

			//depth-of-coverage data
			TSVFileStream stream(in_all[i]);
			int line_count = 0;
			while(!stream.atEnd())
            {
				QByteArrayList parts = stream.readLine();
				if (parts.count()<4)
				{
					THROW(FileParseException, "Coverage file " + in_all[i] + " contains line with less then four elements: " + parts.join('\t'));
				}
				if (parts[0]!=exons[line_count]->chr.str() || parts[1]!=exons[line_count]->start_str || parts[2]!=exons[line_count]->end_str)
				{
					THROW(FileParseException, "Coverage file " + in_all[i] + " contains different regions than reference file " + in_all[0] + ". Expected " + exons[line_count]->toString() + ", got " + parts[0] + ":" + parts[1] + "-" + parts[2] + ".");
				}
				bool ok = false;
				float value = parts[3].toFloat(&ok);
				if (!ok) THROW(ArgumentException, "Could not convert coverage value '" + parts[3] + "' to float.");

				sample->doc.append(value);
				++line_count;
            }

			//check exon count
			if (line_count!=exons.count()) THROW(FileParseException, "Coverage file " + in_all[i] + " contains more/less regions than reference file " + in_all[0] + ". Expected " + QByteArray::number(exons.count()) + ", got " + QByteArray::number(line_count) + ".");

			samples.append(sample);
		}

		timings.append("loading input: " + Helper::elapsedTime(timer));
		timer.restart();

		//count gonosome regions
		outstream << "=== normalizing depth-of-coverage data ===" << endl;
        int c_chrx = 0;
        int c_chry = 0;
        int c_chro = 0;
        for (int e=0; e<exons.count(); ++e)
        {
			if (exons[e]->chr.isX())
            {
                ++c_chrx;
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
		outstream << "number of autosomal regions: " << c_auto << endl;
		outstream << "number of regions on chrX: " << c_chrx << endl;
		outstream << "number of regions on chrY: " << c_chry << " (ignored)" << endl;
        outstream << "number of regions on other chromosomes: " << c_chro << " (ignored)" << endl << endl;

        //normalize DOC by mean (for autosomes/gonosomes separately)
		for (int s=0; s<samples.count(); ++s)
        {
			//calculate mean depths
			QPair<float, float> tmp = weightedMean(exons, samples[s]);
			float mean_auto = tmp.first;
			float mean_chrx = tmp.second;

            //normalize
            for (int e=0; e<exons.count(); ++e)
            {
				if (exons[e]->chr.isAutosome() && mean_auto>0)
                {
					samples[s]->doc[e] /= mean_auto;
                }
				else if (exons[e]->chr.isX() && mean_chrx>0)
                {
					samples[s]->doc[e] /= mean_chrx;
                }
                else
                {
					samples[s]->doc[e] = 0;
                }
            }

            //store mean and stdev for later
			samples[s]->doc_mean = (c_chrx>c_auto) ? mean_chrx : mean_auto;
			if (!fun_isValidFloat(samples[s]->doc_mean))
			{
				THROW(ProgrammingException, "Mean depth of coverage (DOC) is invalid for sample '" + samples[s]->name + "': " + QByteArray::number(samples[s]->doc_mean));
			}
			samples[s]->doc_stdev = fun_stdev(samples[s]->doc, 1.0);
			if (!fun_isValidFloat(samples[s]->doc_stdev))
			{
				THROW(ProgrammingException, "Standard deviation of depth of coverage (DOC) is invalid for sample '" + samples[s]->name + "': " + QByteArray::number(samples[s]->doc_stdev));
			}

            //flag low-depth samples
			if (samples[s]->doc_mean < sam_min_depth)
            {
				samples[s]->qc += "avg_depth=" + QByteArray::number(samples[s]->doc_mean) + " ";
            }
			if (c_chrx>0 && mean_chrx<5)
			{
				samples[s]->qc += "avg_depth_chrx=" + QByteArray::number(mean_chrx) + " ";
			}
			if (c_auto>0 && mean_auto<5)
			{
				samples[s]->qc += "avg_depth_autosomes=" + QByteArray::number(mean_auto) + " ";
			}
		}
		timings.append("normalizing data: " + Helper::elapsedTime(timer));
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

        //load excluded regions file
        BedFile excluded;
		if(exclude!="") excluded.load(exclude);

        //region QC
		outstream << "=== checking for bad regions ===" << endl;
		int c_bad_region = 0;
		QVector<float> tmp;
        for (int e=0; e<exons.count(); ++e)
		{
			tmp.resize(0);
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
			float mad = 1.428f * fun_mad(tmp, median);

			if (median<reg_min_ncov) exons[e]->qc += "ncov<" + QByteArray::number(reg_min_ncov) + " ";
			if (median*avg_abs_cov<reg_min_cov) exons[e]->qc += "cov<" + QByteArray::number(reg_min_cov) + " ";
			if (mad/median>reg_max_cv) exons[e]->qc += "cv>" + QByteArray::number(reg_max_cv)+ " ";
			if (exclude!="" && excluded.overlapsWith(exons[e]->chr, exons[e]->start, exons[e]->end)) exons[e]->qc += "excluded ";
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
					//skip non-reference samples
					if (samples[j]->noref)
					{
						samples[i]->correl_all.append(SampleCorrelation(samples[j], -1.0f));
						continue;
					}

					float sum = 0.0;
					int steps = std::max(1, exons.count() / sam_corr_regs);
					for(int e=0; e<exons.count(); e += steps)
                    {
						sum += (samples[i]->doc[e]-1.0f) * (samples[j]->doc[e]-1.0f);
					}
					samples[i]->correl_all.append(SampleCorrelation(samples[j], sum / samples[i]->doc_stdev / samples[j]->doc_stdev / exons.count() * steps));
                }
			}

			//sort by correlation (reverse)
			std::sort(samples[i]->correl_all.begin(), samples[i]->correl_all.end(), [](const SampleCorrelation& a, const SampleCorrelation& b){return a.correlation > b.correlation;});
		}
		printCorrelationStatistics(samples, outstream);
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
				tmp.resize(0);
				for (int i=0; i<samples.count()-1; ++i)
				{
					if (samples[s]->correl_all[i].sample->qc.isEmpty()) //do not use bad QC samples
                    {
						float value = samples[s]->correl_all[i].sample->doc[e];
						if (value>=0.25f*exon_median && value<=1.75f*exon_median) //do not use extreme outliers
                        {
							tmp.append(value);
                        }
                    }
					if (tmp.count()==n) break;
                }
				if (tmp.count()==n)
                {
					std::sort(tmp.begin(), tmp.end());
					float median = fun_median(tmp);
					samples[s]->ref.append(median);
					std::for_each(tmp.begin(), tmp.end(), [median](float& value){ value = fabsf(value-median); });
					std::sort(tmp.begin(), tmp.end());
					float stdev = 1.428f * fun_median(tmp);
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
		QVector<ResultData> results;
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

		//store result files
		storeResultAsTSV(ranges, results, out, getFlag("anno"), getFlag("test"), cnvs_exon, samples.count());
		storeSampleInfo(out, samples, samples_removed, cnvs_sample, results);
		storeRegionInfo(out, exons, exons_removed, cnvs_exon);
		if (debug!="")
		{
			QSharedPointer<SampleData> sample;
			if (debug!="ALL") sample = sampleByName(debug.toLatin1(), samples, samples_removed, false);

			storeDebugInfo(sample, out, samples, results);
		}
		if (seg!="")
		{
			QSharedPointer<SampleData> sample = sampleByName(seg.toLatin1(), samples, samples_removed, true);
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
		int c_valid = in_all.count() - c_bad_sample;
		outstream << "=== statistics ===" << endl;
		outstream << "invalid regions: " << c_bad_region << " of " << (exons.count() + c_bad_region) << endl;
		outstream << "invalid samples: " << c_bad_sample << " of " << in_all.count() << endl;
		outstream << "mean correlation of samples to reference: " << QByteArray::number(corr_sum/c_valid, 'f', 4) << endl;
		float size_sum = 0;
		foreach(const Range& range, ranges)
		{
			size_sum += range.size();
		}
		outstream << "number of CNV events: " << ranges.count() << " (consisting of " << size_sum << " regions)" << endl;
		outstream << "mean regions per CNV event: " << QByteArray::number(size_sum/ranges.count(), 'f', 2) << endl;
		outstream << "mean CNV events per sample per 100 regions: " << QByteArray::number(1.0f*ranges.count()/c_valid/(exons.count()/100.0), 'f', 4) << endl;
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

