#include "ToolBase.h"
#include "Helper.h"
#include "Exceptions.h"
#include "BasicStatistics.h"
#include "BedFile.h"
#include "NGSHelper.h"
#include "NGSD.h"
#include <QVector>
#include <QFileInfo>
#include <QDir>
#include "math.h"

//Sample Representation
struct SampleData
{
    SampleData()
        : cnvs(0)
        , cnvs_merged(0)
    {
    }

    QString name; //file name

    QVector<double> doc; //coverage data (normalized: divided by mean)
    double doc_mean; //mean coverage before normalizazion (aterwards it is 1.0)
    double doc_stdev; //stdev of coverage after normalization

    QVector< QPair<int, double> > correl_all; //correlation with all samples (-1.0 for self-correlation)

    QVector<double> ref; //reference sample (mean of 'n' most similar samples)
    QVector<double> ref_stdev; //reference sample standard deviation (deviation of 'n' most similar samples)
    double ref_correl; //correlation of sample to reference sample

    int cnvs; //number of CNVs
    int cnvs_merged; //number of CNVs (subsequent CNVs are counted as one)
    QString qc; //QC warning flag
};

//Exon/region representation
struct ExonData
{
    ExonData()
        : start(-1)
        , end(-1)
        , cnvs(0)
    {
    }

    QString name; //region name
    Chromosome chr; //chromosome
    int start; //start position
    int end; //end position

    double median; //median normalized DOC value
    double mad; //MAD of normalized DOC values

    int cnvs; //number of CNVs
    QString qc; //QC warning flag
};

//CNV test data
struct ResultData
{
    ResultData()
        : s(-1)
        , e(-1)
        , z(0.0)
        , copies(2)
    {
    }

    ResultData(int sample_index, int exon_index, double z_score)
        : s(sample_index)
        , e(exon_index)
        , z(z_score)
        , copies(2)
    {
    }

    int s; //sample index
    int e; //exon index
    double z; //z-score
    int copies; //Estimated genotype
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
        setDescription("Detects copy number variations from targeted resequencing data using non-matched control samples.");
        addInfileList("in", "Input TSV files (one per sample) containing coverage data (chr, start, end, avg_depth).", false, true);
        addOutfile("out", "Output TSV file containing the detected CNVs.", false, true);
		//optional
		addOutfile("out_reg", "If set, writes a BED file with region information (baq QC, excluded, good).", true, true);
        addInt("n", "The number of most similar samples to consider.", true, 20);
        addInfile("exclude", "BED file with regions to exclude from the analysis.", true, true);
        addFloat("min_z", "Minimum z-score for CNV seed detection.", true, 4.0);
		addFloat("ext_min_z", "Minimum z-score for CNV extension around seeds.", true, 2.0);
        addFloat("sam_min_depth", "QC: Minimum average depth of a sample.", true, 40.0);
        addFloat("sam_min_corr", "QC: Minimum correlation of sample to constructed reference sample.", true, 0.95);
        addInt("sam_max_cnvs", "QC: Maximum number of CNV events in a sample.", true, 30);
		addFloat("reg_min_cov", "QC: Minimum (average) absolute depth of a target region.", true, 20.0);
		addFloat("reg_min_ncov", "QC: Minimum (average) normalized depth of a target region.", true, 0.01);
		addFloat("reg_max_cv", "QC: Maximum coefficient of variation (median/mad) of target region.", true, 0.3);
        addFlag("verbose", "Enables verbose mode. Writes detail information files for samples, regions and results.");
		addFlag("anno", "Enable annotation of gene names to regions (needs the NGSD database).");
		addFlag("test", "Uses the test database instead of on the production database for annotation.");
    }

    void storeSampleInfo(const QVector<SampleData>& data)
    {
		QSharedPointer<QFile> out = Helper::openFileForWriting("samples.tsv");
        QTextStream outstream(out.data());
        outstream << "#sample\tdoc_mean\tref_correl\tcnvs\tcnvs_merged" << endl;
        foreach(const SampleData& sample, data)
        {
            if (sample.qc!="") continue;
            outstream << sample.name << "\t" << sample.doc_mean << "\t" << sample.ref_correl << "\t" << sample.cnvs << "\t" << sample.cnvs_merged << endl;
        }
    }

    void storeSampleCorrel(const QVector<SampleData>& data)
    {
		QSharedPointer<QFile> out = Helper::openFileForWriting("samples_correl_all.tsv");
        QTextStream outstream(out.data());
        outstream << "#sample\tdoc_mean\tref_correl\tqc" << endl;
        foreach(const SampleData& sample, data)
        {
            outstream << sample.name << "\t" << sample.doc_mean << "\t" << sample.ref_correl << "\t" << sample.qc << endl;
        }
    }

    void storeRegionInfo(const QVector<ExonData>& exons)
    {
		QSharedPointer<QFile> out = Helper::openFileForWriting("regions.tsv");
        QTextStream outstream(out.data());
        outstream << "#region\tsize\tmedian_ncov\tmad_ncov\tcv_ncov\tcnvs" << endl;
        foreach(const ExonData& exon, exons)
        {
            outstream << exon.name << "\t" << (exon.end-exon.start) << "\t" << exon.median << "\t" << exon.mad  << "\t" << (exon.mad/exon.median) << "\t" << exon.cnvs << endl;
        }
    }

    void storeRegionInfoBED(QString out_reg, const QVector<ExonData>& exons)
    {
        //BED format: chr, start, end, name, score, strand, thickstart, thickend
		QSharedPointer<QFile> out = Helper::openFileForWriting(out_reg);
        QTextStream outstream(out.data());
        outstream << "track name=\"CnvHunter region QC\" itemRgb=On visibility=4" << endl;
        foreach(const ExonData& exon, exons)
        {
            QString color;
            QString text;
            if (exon.qc=="")
            {
                color = "0,0,178";
                text += "qc=ok";
            }
            else
            {
                color = "255,0,0";
                text += "qc=" + exon.qc.trimmed().replace(" ", "_");
            }
            outstream << exon.chr.str() << "\t" << (exon.start-1) << "\t" << exon.end << "\t" << text << "\t" << QString::number(exon.median, 'f', 2) << "\t.\t" << (exon.start-1) << "\t" << (exon.end) << "\t" << color << endl;
        }
    }

    void storeResultInfo(const QVector<ResultData>& results, const QVector<SampleData>& data, const QVector<ExonData>& exons)
    {
		QSharedPointer<QFile> out = Helper::openFileForWriting("results.tsv");
        QTextStream outstream(out.data());
        outstream << "#sample\tregion\tcopy_number\tz_score\tdepth\tref_depth\tref_stdev" << endl;
        foreach(const ResultData& r, results)
        {
            outstream << data[r.s].name << "\t" << exons[r.e].name << "\t" << r.copies << "\t" << r.z << "\t" << data[r.s].doc[r.e] << "\t" << data[r.s].ref[r.e] << "\t" << data[r.s].ref_stdev[r.e] << endl;
        }
    }

	QStringList geneNames(const Chromosome& chr, int start, int end, bool test)
	{
		static QHash<QString, QStringList> cache;
		static NGSD db(test);

		//check cache
		QString reg = chr.str() + ":" + QString::number(start) + "-" + QString::number(end);
		if (cache.contains(reg))
		{
			return cache[reg];
		}

		//get genes from NGSD
		QStringList genes = db.genesOverlapping(chr, start, end, 20);
		cache.insert(reg, genes);
		return genes;
	}

	QPair<int, int> storeResultAsTSV(const QVector<ResultData>& results, const QVector<SampleData>& data, const QVector<ExonData>& exons, QString filename, QStringList comments, bool anno, bool test)
    {
		QSharedPointer<QFile> out = Helper::openFileForWriting(filename);
        QTextStream outstream(out.data());

		//comments
		foreach(QString comment, comments)
		{
			outstream << "##" << comment.trimmed() << endl;
		}
		//header
		outstream << "#chr\tstart\tend\tsample\tregion_count\tregion_copy_numbers\tregion_zscores\tregion_coordinates" << (anno ? "\tgenes" : "") << endl;

		QPair<int, int> output = qMakePair(0, 0);
        for (int i=0; i<results.count(); ++i)
        {
            if (results[i].copies==2) continue;
            if (data[results[i].s].qc!="") continue;

			//get copy-number,coordinates and genes for first region
            QStringList copies;
			QStringList zscores;
			QStringList coords;
			QStringList genes;

			const ExonData& exon = exons[results[i].e];
            copies.append(QString::number(results[i].copies));

			zscores.append(QString::number(results[i].z, 'f', 2));
			coords.append(exon.chr.str() + ":" + QString::number(exon.start) + "-" + QString::number(exon.end));
			if (anno) genes = geneNames(exon.chr, exon.start, exon.end, test);

			//get copy-number, z-scores, coordinates and genes for adjacent regions
            int j=i+1;
            while (j<results.count() &&
                   results[j].copies!=2 && //CNV
                   results[j-1].s==results[j].s && //same sample (qc is also ok then, we checked above)
				   exons[results[j-1].e].chr==exons[results[j].e].chr //same chromosome
                   )
            {
                copies.append(QString::number(results[j].copies));
				zscores.append(QString::number(results[j].z, 'f', 2));
				const ExonData& exon2 = exons[results[j].e];
				coords.append(exon2.chr.str() + ":" + QString::number(exon2.start) + "-" + QString::number(exon2.end));
				if (anno) genes += geneNames(exon2.chr, exon2.start, exon2.end, test);
                ++j;
			}

			//gene list: sort and remove duplicates
			if (anno)
			{
				genes.sort();
				genes.removeDuplicates();
			}

			//print output
			int end = exons[results[j-1].e].end;
			outstream << exon.chr.str() << "\t" << exon.start << "\t" << end << "\t" << data[results[i].s].name << "\t" << copies.count() << "\t" << copies.join(",") << "\t" << zscores.join(",") << "\t" << coords.join(",") << (anno ? "\t" + genes.join(",")  : "") << endl;

            //set index after CNV range (++i is performed in the loop => j-1)
            i=j-1;

            //count CNV events and regions
            output.first += 1;
            output.second += copies.count();
        }

        return output;
    }

    void storeNormalizedData(const QVector<SampleData>& data, const QVector<ExonData>& exons)
    {
		QSharedPointer<QFile> out = Helper::openFileForWriting("normalized.tsv");
        QTextStream outstream(out.data());
        outstream << "#region";
        foreach(const SampleData& sample, data)
        {
            if (sample.qc!="") continue;
            outstream << "\t" << sample.name;
        }
        outstream << endl;
        for (int e=0; e<exons.count(); ++e)
        {
            if (exons[e].qc!="") continue;
            outstream << exons[e].name;
            foreach(const SampleData& sample, data)
            {
                if (sample.qc!="") continue;
                outstream << "\t" << QString::number(sample.doc[e], 'f', 4);
            }
            outstream << endl;
        }
    }

    double calculateZ(const QVector<SampleData>& data, int s, int e)
    {
        if(data[s].ref_stdev[e]==0.0 || data[s].ref[e]==0.0)
        {
            return std::numeric_limits<double>::quiet_NaN();
        }

        return BasicStatistics::bound((data[s].doc[e]-data[s].ref[e]) / data[s].ref_stdev[e], -10.0, 10.0);
    }

    int calculateCopies(const QVector<SampleData>& data, int s, int e)
	{
		double copies = 2.0*data[s].doc[e]/data[s].ref[e];
		if (copies<0.2) return 0;
		else if (copies<1.0) return 1;
		else return round(copies);
    }

    double weightedMean(const QVector< QPair<double, int> >& data)
    {
        if (data.count()==0) return 0.0;

        double wsum = 0.0;
        double size = 0.0;
        for (int i=0; i<data.count(); ++i)
        {
            wsum += data[i].first * data[i].second;
            size += data[i].second;
        }

        return wsum / size;
    }

	bool previousExists(const QVector<ResultData>& results, const QVector<ExonData>& exons, int i)
    {
        //no previous result
        if (i==0) return false;
        //not same sample
        if (results[i-1].s!=results[i].s) return false;
        //not same chromosome
		if (exons[results[i].e].chr!=exons[results[i-1].e].chr) return false;

        return true;
    }

	virtual void writeRegionDistributionCV(const QVector<ExonData>& exons, QTextStream& outstream)
	{
		outstream << "Region CV (normalized depth of coverage) histogram:" << endl;
		QVector<int> counts(10, 0);
		for (int e=0; e<exons.count(); ++e)
		{
			double cv = exons[e].mad/exons[e].median;
			int bin = std::floor(cv/0.05);
			if (bin<0) bin = 0;
			if (bin>=10) bin = 9;
			++counts[bin];
		}
		for (int i=0; i<10; ++i)
		{
			outstream << "  " << QString::number(0.05*i, 'f', 2) << "-" << QString::number(0.05*(i+1), 'f', 2) << ": " << counts[i] << endl;
		}
		outstream << endl;
	}

	virtual void writeSampleDistributionCNVs(const QVector<SampleData>& data, QTextStream& outstream)
	{
		outstream << "Merged CNVs per sample histogram:" << endl;
		QVector<int> counts(21, 0);
		for (int s=0; s<data.count(); ++s)
		{
			int bin = data[s].cnvs_merged;
			if (bin>=21) bin = 20;
			++counts[bin];
		}
		for (int i=0; i<21; ++i)
		{
			outstream << "  " << i << ": " << counts[i] << endl;
		}
		outstream << endl;
	}

	virtual void writeSampleDistributionCorrelation(const QVector<SampleData>& data, QTextStream& outstream)
	{
		outstream << "Reference sample correlation histogram:" << endl;
		QVector<int> counts(10, 0);
		for (int s=0; s<data.count(); ++s)
		{
			double corr = 1.0 - data[s].ref_correl;
			int bin = std::floor(corr/0.02);
			if (bin<0) bin = 0;
			if (bin>=10) bin = 9;
			++counts[bin];
		}
		for (int i=0; i<10; ++i)
		{
			outstream << "  " << QString::number(1.0-(0.02*i), 'f', 2) << "-" << QString::number(1.0-(0.02*(i+1)), 'f', 2) << ": " << counts[i] << endl;
		}
		outstream << endl;
	}

    virtual void main()
    {
        //init
        bool verbose = getFlag("verbose");
        QStringList in = getInfileList("in");
        QString exclude = getInfile("exclude");
		QSharedPointer<QFile> out = Helper::openFileForWriting("", true);
        QTextStream outstream(out.data());
        int n = getInt("n");
        if (in.count()<n+1) THROW(ArgumentException, "At least n+1 input files are required! Got " + QString::number(in.count()) + "!");
        double min_z = getFloat("min_z");
		double ext_min_z = getFloat("ext_min_z");
        double reg_min_ncov = getFloat("reg_min_ncov");
		double reg_min_cov = getFloat("reg_min_cov");
		double reg_max_cv = getFloat("reg_max_cv");
        double sam_min_corr = getFloat("sam_min_corr");
        double sam_min_depth = getFloat("sam_min_depth");
        int sam_max_cnvs = getInt("sam_max_cnvs");
		QStringList comments;

        //load exon list
        QVector<ExonData> exons;
        QStringList file = Helper::loadTextFile(in[0], true, '#', true);
        foreach(const QString& line, file)
        {
            //create exon
            QStringList parts = line.split('\t');
            if (parts.count()<4) THROW(FileParseException, "Coverage file " + in[0] + " contains line with less then four elements: " + line);
            ExonData ex;
            ex.chr = parts[0].trimmed();
			ex.start = Helper::toInt(parts[1], "start position" , line);
			ex.end = Helper::toInt(parts[2], "end position" , line);
            ex.name = ex.chr.str() + ":" + QString::number(ex.start) + "-" + QString::number(ex.end);

            //check that exons are sorted according to chromosome and start position
            if (exons.count()!=0 && ex.chr==exons.last().chr)
            {
                if(ex.start<exons.last().start)
                {
                    THROW(FileParseException, "Exons not sorted according to chromosome/position! " + ex.chr.str() + ":" + QString::number(ex.start) + " after " + ex.chr.str() + ":" + QString::number(exons.last().start) + "!");
                }
            }

            //append exon to data
            exons.append(ex);
        }

        //load input (and check input)
        QVector<SampleData> data;
        data.resize(in.count());
        for (int i=0; i<in.count(); ++i)
        {
            //init
            data[i].name = QFileInfo(in[i]).baseName();
            data[i].doc.reserve(exons.count());

            //check exon count
            file = Helper::loadTextFile(in[i], true, '#', true);
            if (file.count()!=exons.count()) THROW(FileParseException, "Coverage file " + data[i].name + " contains more/less regions than reference file " + in[0] + ". Expected " + QString::number(exons.count()) + ", got " + QString::number(file.count()) + ".");

            //depth-of-coverage data
            for (int j=0; j<file.count(); ++j)
            {
                QStringList parts = file[j].split('\t');
                if (parts.count()<4) THROW(FileParseException, "Coverage file " + data[i].name + " contains line with less then four elements: " + file[j]);
                QString ex = parts[0].trimmed() + ":" + parts[1].trimmed() + "-" + parts[2].trimmed();
                if (ex!=exons[j].name) THROW(FileParseException, "Coverage file " + data[i].name + " contains different regions than reference file " + in[0] + ". Expected " + exons[j].name + ", got " + ex + ".");

				double value = Helper::toDouble(parts[3], "coverge value", file[j]);
				data[i].doc.append(value);
            }
        }

        //count gonosome regions
        outstream << "normalizing depth-of-coverage data..." << endl;
        int c_chrx = 0;
        int c_chry = 0;
        int c_chro = 0;
        for (int e=0; e<exons.count(); ++e)
        {
            if (exons[e].chr.isX())
            {
                ++c_chrx;
            }
            else if (exons[e].chr.isY())
            {
                ++c_chry;
            }
            else if (!exons[e].chr.isAutosome())
            {
                ++c_chro;
            }
        }
        int c_auto = exons.count() - c_chrx - c_chry -c_chro;
        outstream << "  number of autosomal regions: " << c_auto << endl;
        outstream << "  number of regions on chrX: " << c_chrx << endl;
        outstream << "  number of regions on chrY: " << c_chry << " (ignored)" << endl;
        outstream << "  number of regions on other chromosomes: " << c_chro << " (ignored)" << endl;

        //normalize DOC by mean (for autosomes/gonosomes separately)
        for (int s=0; s<data.count(); ++s)
        {
            //calculate means
            QVector< QPair<double, int> > doc_auto;
            doc_auto.reserve(c_auto);
            QVector< QPair<double, int> > doc_chrx;
            doc_chrx.reserve(c_chrx);
            for (int e=0; e<exons.count(); ++e)
            {
                if (exons[e].chr.isAutosome())
                {
                    doc_auto.append(qMakePair(data[s].doc[e], exons[e].end-exons[e].start));
                }
                else if (exons[e].chr.isX())
                {
                    doc_chrx.append(qMakePair(data[s].doc[e], exons[e].end-exons[e].start));
                }
            }
            double mean_chrx = weightedMean(doc_chrx);
			double mean_auto = weightedMean(doc_auto);

            //normalize
            for (int e=0; e<exons.count(); ++e)
            {
				if (exons[e].chr.isAutosome() && mean_auto>0)
                {
                    data[s].doc[e] /= mean_auto;
                }
				else if (exons[e].chr.isX() && mean_chrx>0)
                {
                    data[s].doc[e] /= mean_chrx;
                }
                else
                {
                    data[s].doc[e] = 0;
                }
            }

            //store mean and stdev for later
            data[s].doc_mean = (c_chrx>c_auto) ? mean_chrx : mean_auto;
			if (!BasicStatistics::isValidFloat(data[s].doc_mean))
			{
				THROW(ProgrammingException, "Mean depth of coverage (DOC) is invalid for sample '" + data[s].name + "': " + QString::number(data[s].doc_mean));
			}
			data[s].doc_stdev = BasicStatistics::stdev(data[s].doc, 1.0);
			if (!BasicStatistics::isValidFloat(data[s].doc_stdev))
			{
				THROW(ProgrammingException, "Standard deviation of depth of coverage (DOC) is invalid for sample '" + data[s].name + "': " + QString::number(data[s].doc_stdev));
			}

            //flag low-depth samples
			if (data[s].doc_mean < sam_min_depth)
            {
				data[s].qc += "avg_depth=" + QString::number(data[s].doc_mean) + " ";
            }
			if (c_chrx>0 && mean_chrx<5)
			{
				data[s].qc += "avg_depth_chrx=" + QString::number(mean_chrx) + " ";
			}
			if (c_auto>0 && mean_auto<5)
			{
				data[s].qc += "avg_depth_autosomes=" + QString::number(mean_auto) + " ";
			}
		}
        outstream << "done" << endl << endl;

		//calculate overall average depth (of good samples)
		QVector<double> tmp;
		tmp.reserve(data.count());
		for (int s=0; s<data.count(); ++s)
		{
			if (data[s].qc=="")
			{
				tmp.append(data[s].doc_mean);
			}
		}
		double avg_abs_cov = BasicStatistics::mean(tmp);

        //load excluded regions file
        BedFile excluded;
		if(exclude!="") excluded.load(exclude);

        //region QC
		outstream << "validating regions..." << endl;
        int c_bad_region = 0;
        for (int e=0; e<exons.count(); ++e)
        {
            tmp.resize(0);
            for (int s=0; s<data.count(); ++s)
            {
				if (data[s].qc=="")
                {
                    //check that DOC data for good samples is ok
                    if (!BasicStatistics::isValidFloat(data[s].doc[e]))
                    {
						THROW(ProgrammingException, "Normalized coverage value is invalid for sample '" + data[s].name + "' in exon '" + exons[e].name + "' (" + QString::number(data[s].doc[e]) + ")");
					}
                    tmp.append(data[s].doc[e]);
                }
            }
            std::sort(tmp.begin(), tmp.end());
            double median = BasicStatistics::median(tmp);
            double mad = 1.428 * BasicStatistics::mad(tmp, median);

            if (median<reg_min_ncov) exons[e].qc += "ncov<" + QString::number(reg_min_ncov) + " ";
			if (median*avg_abs_cov<reg_min_cov) exons[e].qc += "cov<" + QString::number(reg_min_cov) + " ";
			if (mad/median>reg_max_cv) exons[e].qc += "cv>" + QString::number(reg_max_cv)+ " ";
			if (exclude!="" && excluded.overlapsWith(exons[e].chr, exons[e].start, exons[e].end)) exons[e].qc += "excluded ";
			if (exons[e].chr.isY()) exons[e].qc += "chrY ";
            exons[e].median = median;
            exons[e].mad = mad;
            if (exons[e].qc!="")
			{
				comments << "bad region: " + exons[e].name + " " + exons[e].qc;
                ++c_bad_region;
            }
        }
		outstream << "bad regions: " << c_bad_region << " of " << exons.count() << endl << endl;
		comments << "bad regions: " + QString::number(c_bad_region) + " of " + QString::number(exons.count());
		writeRegionDistributionCV(exons, outstream);

        //write region info BED file
        QString out_reg = getOutfile("out_reg");
        if (out_reg!="")
        {
            storeRegionInfoBED(out_reg, exons);
        }

        //calculate correlation between all samples
        for (int i=0; i<data.count(); ++i)
        {
            //calculate correlation to all other samples
            for (int j=0; j<data.count(); ++j)
            {
                if (i==j)
                {
                    data[i].correl_all.append(qMakePair(j, -1.0));
                }
                else
                {
                    double sum = 0.0;
                    for(int e=0; e<exons.count(); ++e)
                    {
                        sum += (data[i].doc[e]-1.0) * (data[j].doc[e]-1.0);
					}
					data[i].correl_all.append(qMakePair(j, sum / data[i].doc_stdev / data[j].doc_stdev / exons.count()));
                }
            }

			//sort by correlation (reverse)
			std::sort(data[i].correl_all.begin(), data[i].correl_all.end(), [](const QPair<int, double> & a, const QPair<int, double> & b){return a.second > b.second;});
        }

        //construct reference from 'n' most similar samples
        outstream << "validating samples..." << endl;
        int c_bad_sample = 0;
        for (int s=0; s<data.count(); ++s)
		{
            for (int e=0; e<exons.count(); ++e)
            {
                double exon_median = exons[e].median;
                QVector<double> values;
                values.reserve(n);
                for (int i=0; i<data.count()-1; ++i)
                {
					int sidx = data[s].correl_all[i].first;
                    if (data[sidx].qc=="") //do not use bad QC samples
                    {
                        double value = data[sidx].doc[e];
                        if (value>=0.25*exon_median && value<=1.75*exon_median) //do not use extreme outliers
                        {
                            values.append(value);
                        }
                    }
                    if (values.count()==n) break;
                }
                if (values.count()==n)
                {
                    std::sort(values.begin(), values.end());
                    double median = BasicStatistics::median(values);
                    data[s].ref.append(median);
                    double stdev = 1.428 * BasicStatistics::mad(values, median);
                    data[s].ref_stdev.append(std::max(stdev, 0.1*median));
                }
                else
				{
                    data[s].ref.append(exon_median);
                    data[s].ref_stdev.append(0.3*exon_median);
                }
            }
            data[s].ref_correl = BasicStatistics::correlation(data[s].doc, data[s].ref);

            //flag samples with bad correlation
            if (data[s].ref_correl<sam_min_corr)
            {
                data[s].qc += "corr=" + QString::number(data[s].ref_correl, 'f', 3) + " ";
            }

            //print all bad samples (also those which were flagged as bad before, e.g. because os too low avg depth)
            if (data[s].qc!="")
            {
				++c_bad_sample;
				comments << "bad sample: " + data[s].name + " " + data[s].qc;
            }
        }
        outstream << "bad samples: " << c_bad_sample << " of " << data.count() << endl << endl;
		comments << "bad samples: " + QString::number(c_bad_sample) + " of " + QString::number(data.count());
        if (verbose) storeSampleCorrel(data);
		writeSampleDistributionCorrelation(data, outstream);

		//log 'n' most similar samples
		for (int s=0; s<data.count(); ++s)
		{
			QString sim_str;
			int sim_count = 0;
			for (int i=0; i<data[s].correl_all.count()-1; ++i)
			{
				int sidx = data[s].correl_all[i].first;
				if (data[sidx].qc!="") continue;

				sim_str += " " + data[sidx].name;
				++sim_count;
				if (sim_count==n) break;
			}
			comments << QString("ref samples of ") + data[s].name + " (corr=" + QString::number(data[s].ref_correl, 'f', 4) + "):" + sim_str;
		}

        //remove bad samples
        int to = 0;
        for (int s=0; s<data.count(); ++s)
        {
            if(data[s].qc=="")
            {
                if (to!=s) data[to] = data[s];
                ++to;
            }
        }
        data.resize(to);

        //remove bad regions
        for (int s=0; s<data.count(); ++s)
        {
            to = 0;
            for (int e=0; e<exons.count(); ++e)
            {
                if(exons[e].qc=="")
                {
                    if (to!=e)
                    {
                        data[s].doc[to] = data[s].doc[e];
                        data[s].ref[to] = data[s].ref[e];
                        data[s].ref_stdev[to] = data[s].ref_stdev[e];
                    }
                    ++to;
                }
            }
            data[s].doc.resize(to);
            data[s].ref.resize(to);
            data[s].ref_stdev.resize(to);
        }
        to = 0;
        for (int e=0; e<exons.count(); ++e)
        {
            if(exons[e].qc=="")
            {
                if (to!=e) exons[to] = exons[e];
                ++to;
            }
        }
        exons.resize(to);

        //detect CNVs from DOC data
        outstream << "initial CNV detection..." << endl;
        int detected = 0;
        QVector<ResultData> results;
        results.reserve(exons.count() * data.count());
        for (int s=0; s<data.count(); ++s)
        {
            for (int e=0; e<exons.count(); ++e)
            {
                //detect CNVs
                double z = calculateZ(data, s, e);
                ResultData res(s, e, z);
                if (
                        z<=-min_z //statistical outlier (del)
                        || z>=min_z //statistical outlier (dup)
						|| (data[s].ref[e]>=reg_min_ncov && data[s].ref[e]*avg_abs_cov>=reg_min_cov && data[s].doc[e]<0.1*data[s].ref[e]) //region with homozygous deletion which is not detected by statistical outliers
                        )
                {
                    data[s].cnvs += 1;
                    exons[e].cnvs += 1;
                    res.copies = calculateCopies(data, s, e);
                    ++detected;

                    //warn if there is something wrong with the copy number estimation
                    if (res.copies==2)
                    {
                        outstream << "  WARNING: Found z-score outlier (" << z << ") with estimated copy-number 2!" << endl;
                    }
                }
                results.append(res);
            }
        }
		outstream << "detected " << detected << " seed regions" << endl << endl;

        //extending initial CNVs in both directions
        outstream << "extending CNV seeds..." << endl;
        detected = 0;
        for (int r=0; r<results.count(); ++r)
        {
			const ResultData& seed = results[r];
			if (seed.copies==2) continue;
			bool is_del = results[r].copies<2;

            //outstream << "  " << data[seed.s].filename << " " << exons[seed.e].name << " " << seed.z << " " << seed.copies << endl;

            //extend to left
            int i=r-1;
            while(i>0 && results[i].copies==2)
            {
                const ResultData& curr = results[i];
				if (curr.s!=seed.s) break; //same sample
				if (exons[curr.e].chr!=exons[seed.e].chr) break; //same chromosome
				int copies = calculateCopies(data, curr.s, curr.e);
				if (is_del) //same CNV type (del)
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
                //outstream << "    EX LEFT " << data[curr.s].filename << " " << exons[curr.e].name << " " << curr.z << " " << copies << endl;
                ++detected;
                --i;
            }

            //extend to right
            i=r+1;
            while(i<results.count() && results[i].copies==2)
            {
                const ResultData& curr = results[i];
                if (curr.s!=seed.s) break; //same sample
				if (exons[curr.e].chr!=exons[seed.e].chr) break; //same chromosome
				int copies = calculateCopies(data, curr.s, curr.e);
				if (is_del) //same CNV type (del)
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
                //outstream << "    EX RIGH " << data[curr.s].filename << " " << exons[curr.e].name << " " << curr.z << " " << copies << endl;
                ++detected;
                ++i;
            }
        }
		outstream << "extended to " << detected << " additional regions from seeds" << endl << endl;

        //flag samples that have too many CNV events
        outstream << "flag samples that have too many CNV events as bad..." << endl;
        int c_bad_sample2 = 0;
        for (int i=0; i<results.count(); ++i)
        {
            //current region is CNV
            if (results[i].copies!=2)
            {
				if (!previousExists(results, exons, i) || results[i-1].copies==2)
                {
                    ++data[results[i].s].cnvs_merged;
                }
            }
        }
        for (int s=0; s<data.count(); ++s)
        {
            if (data[s].cnvs_merged>sam_max_cnvs)
            {
                data[s].qc += "sam_max_cnvs>" + QString::number(sam_max_cnvs) + " ";
                outstream << "  bad sample: " << data[s].name << " cnvs=" << data[s].cnvs_merged << endl;
                ++c_bad_sample2;
            }
        }
        outstream << "flagged " << c_bad_sample2 << " samples" << endl << endl;
		writeSampleDistributionCNVs(data, outstream);

        //store results
		QPair<int, int> tmp2 = storeResultAsTSV(results, data, exons, getOutfile("out"), comments, getFlag("anno"), getFlag("test"));
        int detected_merged = tmp2.first;
        detected = tmp2.second;
        outstream << "storing results TSV file..." << endl ;
        outstream << "detected " << detected_merged << " CNV events (" << detected << " overall regions)" << endl << endl ;

        //print a little statistics
        double corr_sum = 0;
        foreach(const SampleData& sample, data)
        {
            if (sample.qc=="")
            {
                corr_sum += sample.ref_correl;
            }
        }
        int c_valid = in.count() - c_bad_sample - c_bad_sample2;
        outstream << "statistics:" << endl;
        outstream << "  invalid regions: " << c_bad_region << " of " << (exons.count() + c_bad_region) << endl;
        outstream << "  invalid samples: " << (c_bad_sample + c_bad_sample2) << " of " << in.count() << endl;
        outstream << "  mean correlation of samples to reference: " << QString::number(corr_sum/c_valid, 'f', 4) << endl;
        outstream << "  mean CNV events per sample per 100 regions: " << QString::number(1.0*detected_merged/c_valid/(exons.count()/100.0), 'f', 4) << endl;
        outstream << "  mean regions per CNV event: " << QString::number(1.0*detected/detected_merged, 'f', 2) << endl;
        outstream << endl << endl;

        //store verbose files
        if (verbose)
        {
            storeSampleInfo(data);
            storeRegionInfo(exons);
            storeResultInfo(results, data, exons);
            storeNormalizedData(data, exons);
        }
    }
};

#include "main.moc"

int main(int argc, char *argv[])
{
    ConcreteTool tool(argc, argv);
    return tool.execute();
}

