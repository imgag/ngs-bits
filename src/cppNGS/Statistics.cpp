#include "Statistics.h"
#include "Exceptions.h"
#include "ChromosomalIndex.h"
#include "Log.h"
#include "cmath"
#include "numeric"
#include "algorithm"
#include "BasicStatistics.h"
#include <QVector>
#include "Pileup.h"
#include "LinePlot.h"
#include "ScatterPlot.h"
#include "BarPlot.h"
#include "Helper.h"
#include "SampleSimilarity.h"
#include <QFileInfo>
#include <utility>
#include <QThreadPool>
#include "Histogram.h"
#include "FilterCascade.h"

class RegionDepth
{
public:
	RegionDepth(Chromosome chr, int start, int end):
	  chr_(chr)
	, start_(start)
	, end_(end)
	{
		depth_ = QVector<int>(end_-start_+1);
		depth_.fill(0);
	}

	//for QContainers
	RegionDepth()
	{
		chr_ = Chromosome();
		start_ = -1;
		end_ = -1;
		depth_ = QVector<int>();
	}

	void incrementRegion(int start, int end)
	{
		int idx_start = std::max(start, start_) - start_;
		int idx_end = std::min(end, end_) - start_;
		for (int i=idx_start; i<=idx_end; ++i)
		{
			depth_[i] += 1;
		}
	}

	//read access
	int operator[](int pos) const
	{
		if (pos < start_ || end_ < pos)
		{
			THROW(ArgumentException, "Access outside of valid region. Position " + QString::number(pos) + " not in region: " + QString::number(start_) + "-"  + QString::number(end_) + ".");
		}

		return depth_[pos-start_];
	}

	//Interface for ChromosomalIndex
	const Chromosome& chr() const
	{
		return chr_;
	}

	int start() const
	{
		return start_;
	}

	int end() const
	{
		return end_;
	}

	int count() const
	{
		return end_-start_+1;
	}

private:
	Chromosome chr_;
	int start_;
	int end_;
	QVector<int> depth_;
};



QCCollection Statistics::variantList(const VcfFile& variants, bool filter)
{
	//support only single sample vcf files
	if(variants.sampleIDs().count() > 1)
	{
		THROW(FileParseException, "Can not generate QCCollection for a vcf file with multiple samples.");
	}

	QCCollection output;

	//filter variants
	FilterResult filter_result(variants.count());
	if (filter)
	{
		FilterFilterColumnEmpty filter;
		filter.apply(variants, filter_result);
	}
	int vars_passing_filter = filter_result.countPassing();

	//var_total
	addQcValue(output, "QC:2000013", "variant count", vars_passing_filter);

	//var_perc_dbsnp and high-impact variants
	if (vars_passing_filter==0)
	{
		addQcValue(output, "QC:2000014", "known variants percentage", "n/a (no variants)");
		addQcValue(output, "QC:2000015", "high-impact variants percentage", "n/a (no variants)");
	}
	else
	{
		bool csq_entry_exists = variants.vcfHeader().infoIdDefined("CSQ");
		if (!csq_entry_exists)
		{
			addQcValue(output, "QC:2000014", "known variants percentage", "n/a (CSQ info field missing)");
			addQcValue(output, "QC:2000015", "high-impact variants percentage", "n/a (CSQ info field missing)");
		}
		else
		{
			bool csq2_entry_exists = variants.vcfHeader().infoIdDefined("CSQ2");
			double dbsnp_count = 0;
			double high_impact_count = 0;
			for(int i=0; i<variants.count(); ++i)
			{
				if (!filter_result.passing(i)) continue;

				if (variants[i].info("CSQ").contains("|rs") ) //works without splitting by transcript
				{
					++dbsnp_count;
				}
				if (variants[i].info("CSQ").contains("|HIGH|")) //works without splitting by transcript
				{
					++high_impact_count;
				}
				else if (csq2_entry_exists && variants[i].info("CSQ2").contains("|HIGH|")) //fallback to annotation with VcfAnnotateConsequence
				{
					++high_impact_count;
				}

			}
			addQcValue(output, "QC:2000014", "known variants percentage", 100.0*dbsnp_count/vars_passing_filter);
			addQcValue(output, "QC:2000015", "high-impact variants percentage", 100.0*high_impact_count/vars_passing_filter);
		}
	}

	//homozygous variants
	bool gt_entry_exists = variants.vcfHeader().formatIdDefined("GT");
	if (vars_passing_filter!=0 && gt_entry_exists)
	{
		double hom_count = 0;
		for(int i=0; i<variants.count(); ++i)
		{
			if (!filter_result.passing(i)) continue;

			QByteArray geno = variants[i].formatValueFromSample("GT");
			if (geno=="1/1" || geno=="1|1")
			{
				++hom_count;
			}
		}
		addQcValue(output, "QC:2000016", "homozygous variants percentage", 100.0*hom_count/vars_passing_filter);
	}
	else
	{
		addQcValue(output, "QC:2000016", "homozygous variants percentage", "n/a (GT annotation not found, or no variants)");
	}

	//var_perc_indel / var_ti_tv_ratio
	double indel_count = 0;
	double ti_count = 0;
	double tv_count = 0;
	for(int i=0; i<variants.count(); ++i)
	{
		if (!filter_result.passing(i)) continue;

		//only first variant is analyzed
		const  VcfLine& var = variants[i];
		if (var.isIns() || var.isDel())
		{
			++indel_count;
		}
		else if ((var.alt(0)=="A" && var.ref()=="G") || (var.alt(0)=="G" && var.ref()=="A") || (var.alt(0)=="T" && var.ref()=="C") || (var.alt(0)=="C" && var.ref()=="T"))
		{
			++ti_count;
		}
		else
		{
			++tv_count;
		}
	}

	if (vars_passing_filter!=0)
	{
		addQcValue(output, "QC:2000017", "indel variants percentage", 100.0*indel_count/vars_passing_filter);
	}
	else
	{
		addQcValue(output, "QC:2000017", "indel variants percentage", "n/a (no variants)");
	}

	if (tv_count!=0)
	{
		addQcValue(output, "QC:2000018", "transition/transversion ratio", ti_count/tv_count);
	}
	else
	{
		addQcValue(output, "QC:2000018", "transition/transversion ratio", "n/a (no variants or tansversions)");
	}

	//count mosaic variants
	int mosaic_var_count = 0;
	for(int i=0; i<variants.count(); ++i)
	{
		if (filter_result.passing(i)) continue;

		const  VcfLine& var = variants[i];
		if (var.filters().contains("mosaic")) ++mosaic_var_count;
	}
	addQcValue(output, "QC:2000142", "mosaic variant count", mosaic_var_count);

	return output;
}

QCCollection Statistics::phasing(const VcfFile& variants, bool filter, BedFile& phasing_blocks)
{
	//support only single sample vcf files
	if(variants.sampleIDs().count() > 1)
	{
		THROW(FileParseException, "Can not generate QCCollection for a vcf file with multiple samples.");
	}

	QCCollection output;

	//filter variants
	FilterResult filter_result(variants.count());
	if (filter)
	{
		FilterFilterColumnEmpty filter;
		filter.apply(variants, filter_result);
	}

	//iterate over all variants and extract phasing information
	BedLine current_phasing_block;
	int n_phased_variants = 0;
	int n_het_vars = 0;
	for(int i=0; i<variants.count(); ++i)
	{
		if (!filter_result.passing(i)) continue;

		//count hom variants (no phasing possible => has to be ignored)
		QByteArray genotype = variants[i].formatValueFromSample("GT");
		if (genotype=="0/1" || genotype=="1/0" || genotype=="0|1" || genotype=="1|0") n_het_vars++;

		//update phasing blocks
		QByteArray phasing_block_id = variants[i].formatValueFromSample("PS").trimmed();
		//skip unphased variants
		if(phasing_block_id == ".") continue;
		n_phased_variants++;
		if(current_phasing_block.isValid())
		{
			if(phasing_block_id == current_phasing_block.annotations().at(0))
			{
				//extend block
				current_phasing_block.setEnd(variants[i].end());
			}
			else
			{
				//store old block and start new block
				phasing_blocks.append(current_phasing_block);
				current_phasing_block = BedLine(variants[i].chr(), variants[i].start(), variants[i].end(), QByteArrayList() << phasing_block_id);
			}
		}
		else
		{
			//special case: first variant
			current_phasing_block = BedLine(variants[i].chr(), variants[i].start(), variants[i].end(), QByteArrayList() << phasing_block_id);
		}
	}
	//add last block
	phasing_blocks.append(current_phasing_block);
	//get lengths of phasing blocks
	QVector<double> block_sizes;
	for (int j = 0; j < phasing_blocks.count(); j++)
	{
		block_sizes.append(phasing_blocks[j].length());
	}
	//calculate statistics
	double mean_block_size = BasicStatistics::mean(block_sizes);
	double median_block_size = BasicStatistics::median(block_sizes);
	double max_block_size = *std::max_element(block_sizes.constBegin(), block_sizes.constEnd());
	addQcValue(output, "QC:2000133", "mean phasing block size", mean_block_size);
	addQcValue(output, "QC:2000134", "median phasing block size", median_block_size);
	addQcValue(output, "QC:2000135", "phasing block count", phasing_blocks.count());
	addQcValue(output, "QC:2000136", "phased variants percentage", 100.00 * ((float) n_phased_variants/n_het_vars));

	//create histogram
	Histogram phasing_block_distribution(0, max_block_size/1000.0, (max_block_size * 0.05)/1000.0);
	phasing_block_distribution.setXLabel("phasing block size (kb)");
	phasing_block_distribution.setYLabel("count");
	//add data
	foreach (double size, block_sizes) phasing_block_distribution.inc(size/1000.0, false);
	//write to file
	QString plotname = Helper::tempFileName(".png");
	phasing_block_distribution.store(plotname, false, true, 0.5);
	addQcPlot(output, "QC:2000137", "phasing block distribution plot", plotname);
	QFile::remove(plotname);

	return output;
}

QCCollection Statistics::mapping(const BedFile& bed_file, const QString& bam_file, const QString& ref_file, int min_mapq, bool is_cfdna)
{
	//check target region is merged/sorted and create index
	if (!bed_file.isMergedAndSorted())
	{
		THROW(ArgumentException, "Merged and sorted BED file required for coverage details statistics!");
	}
	ChromosomalIndex<BedFile> roi_index(bed_file);

	//create coverage statistics data structure
	long long roi_bases = 0;

	QVector<RegionDepth> roi_cov(bed_file.count());
	for (int i=0; i<bed_file.count(); ++i)
	{
		const BedLine& line = bed_file[i];
		roi_cov[i] = RegionDepth(line.chr(), line.start(), line.end());
		roi_bases += line.length();
	}

	//create AT/GC dropout datastructure
	FastaFileIndex ref_idx(ref_file);
	BedFile dropout;
	dropout.add(bed_file);
	dropout.chunk(100);
	QHash<int, double> gc_roi;
	QHash<int, double> gc_reads;
	QHash<int, int> gc_index_to_bin_map;
	for (int i=0; i<dropout.count(); ++i)
	{
		BedLine& line = dropout[i];
		Sequence seq = ref_idx.seq(line.chr(), line.start(), line.length());
		double gc_content = seq.gcContent();
		if (!BasicStatistics::isValidFloat(gc_content))
		{
			gc_index_to_bin_map[i] = -1;
		}
		else
		{
			int bin = (int)std::floor(100.0*gc_content);
			gc_index_to_bin_map[i] = bin;
			gc_roi[bin] += 1.0;
		}
	}
	ChromosomalIndex<BedFile> dropout_index(dropout);

	//init counts
	long long al_total = 0;
	long long al_mapped = 0;
	long long al_ontarget = 0;
    long long al_neartarget = 0;
	long long al_dup = 0;
	long long al_proper_paired = 0;
	long long insert_size_read_count = 0;
	double bases_trimmed = 0;
	double bases_mapped = 0;
	double bases_clipped = 0;
	double insert_size_sum = 0;
	Histogram insert_dist(0, 999, 5);
	long long bases_usable = 0;
	QVector<long long> bases_usable_dp(5); //usable bases by duplication level
	long long bases_usable_raw = 0; //usable bases in BAM before deduplication
	bases_usable_dp.fill(0);
	Histogram dp_dist(0.5, 4.5, 1);
	int max_length = 0;
	bool paired_end = false;

	//iterate through all alignments
	BamReader reader(bam_file, ref_file);
	reader.skipBases();
	reader.skipQualities();
	BamAlignment al;
	while (reader.getNextAlignment(al))
	{
		//skip secondary alignments
		if (al.isSecondaryAlignment() || al.isSupplementaryAlignment()) continue;

		++al_total;

		const int length = al.length();
		max_length = std::max(max_length, length);

		//track if spliced alignment
		bool spliced_alignment = false;

		if (!al.isUnmapped())
		{
			++al_mapped;

			//calculate soft/hard-clipped bases
			const int start_pos = al.start();
			const int end_pos = al.end();
			bases_mapped += length;
			const QList<CigarOp> cigar_data = al.cigarData();
			foreach(const CigarOp& op, cigar_data)
			{
				if (op.Type==BAM_CSOFT_CLIP || op.Type==BAM_CHARD_CLIP)
				{
					bases_clipped += op.Length;
				}
				else if (op.Type==BAM_CREF_SKIP)
				{
					spliced_alignment = true;
				}
			}

			//calculate usable bases, base-resolution coverage and GC statistics
			const Chromosome& chr = reader.chromosome(al.chromosomeID());
			QVector<int> indices = roi_index.matchingIndices(chr, start_pos-250, end_pos+250);
			if (indices.count()!=0)
			{
				++al_neartarget;

				//check if on target
				indices = roi_index.matchingIndices(chr, start_pos, end_pos);
				if (indices.count()!=0)
				{
					++al_ontarget;

					int dp = al.tagi("DP");
					if (dp != 0)
					{
                        dp_dist.inc(std::min(dp, 4), true);
					}

					//calculate usable bases and base-resolution coverage on target region
					if (!al.isDuplicate() && al.mappingQuality()>=min_mapq)
					{
						foreach(int index, indices)
						{
							const int ol_start = std::max(bed_file[index].start(), start_pos);
							const int ol_end = std::min(bed_file[index].end(), end_pos);
							bases_usable += ol_end - ol_start + 1;
							bases_usable_dp[std::min(dp, 4)] += ol_end - ol_start + 1;
							bases_usable_raw += (ol_end - ol_start + 1)  * (dp + 1);
							roi_cov[index].incrementRegion(ol_start, ol_end);
						}
					}

					//calculate GC statistics
					indices = dropout_index.matchingIndices(chr, start_pos, end_pos);
					foreach(int index, indices)
					{
						int bin = gc_index_to_bin_map[index];
						if (bin>=0)
						{
							gc_reads[bin] += 1.0/indices.count();
						}
					}
				}
			}
		}

		//insert size
		if (al.isPaired())
		{
			paired_end = true;

			if (al.isProperPair())
			{
				++al_proper_paired;

				//if alignment is spliced, exclude it from insert size calculation
				if (!spliced_alignment)
				{
					const int insert_size = abs(al.insertSize());
					if (insert_size < 1000)
					{
						++insert_size_read_count;
						insert_size_sum += insert_size;
						insert_dist.inc(insert_size, true);
					}
				}
			}
		}

		//trimmed bases (this is not entirely correct if the first alignments are all trimmed, but saves the second pass through the data)
		if (length<max_length)
		{
			bases_trimmed += (max_length - length);
		}

		if (al.isDuplicate())
		{
			++al_dup;
		}
	}

	//calculate AT/GC dropout
	QList<double> values = gc_roi.values();
	double gc_sum = std::accumulate(values.begin(),values.end(), 0.0);
	values = gc_reads.values();
	double roi_sum = std::accumulate(values.begin(),values.end(), 0.0);
	double at_dropout = 0;
	double gc_dropout = 0;
	QVector<double> gc_read_percentages;
	QVector<double> gc_roi_percentages;
	for (int i=0; i<100; ++i)
	{
		double roi_perc = 100.0*gc_roi[i]/gc_sum;
		gc_roi_percentages << roi_perc;
		double read_perc = 100.0*gc_reads[i]/roi_sum;
		gc_read_percentages << read_perc;

		double diff = roi_perc-read_perc;
		if (diff>0)
		{
			if (i<=50)
			{
				at_dropout += diff;
			}
			if (i>=50)
			{
				gc_dropout += diff;
			}
		}
	}

	//calculate coverage depth statistics
	double avg_depth = (double) bases_usable / roi_bases;
	int half_depth = std::round(0.5*avg_depth);
	long long bases_covered_at_least_half_depth = 0;
	int hist_max = 599;
	int hist_step = 5;
	if (avg_depth>200)
	{
		hist_max += 400;
		hist_step += 5;
	}
	if (avg_depth>500)
	{
		hist_max += 500;
	}
	if (avg_depth>1000)
	{
		hist_max += 1000;
	}
	// special parameter for cfDNA
	if(is_cfdna)
	{
		hist_max = 20000;
		hist_step = 500;
	}
	Histogram depth_dist(0, hist_max, hist_step);
	for(int i=0; i< roi_cov.count(); ++i)
	{
		RegionDepth& region = roi_cov[i];
		for(int j=roi_cov[i].start(); j<=roi_cov[i].end(); ++j)
		{
			int depth = region[j];
			depth_dist.inc(depth, true);

			if(depth>=half_depth)
			{
				++bases_covered_at_least_half_depth;
			}
		}
	}

	//output
	QCCollection output;
	addQcValue(output, "QC:2000019", "trimmed base percentage", 100.0 * bases_trimmed / al_total / max_length);
	addQcValue(output, "QC:2000052", "clipped base percentage", 100.0 * bases_clipped / bases_mapped);
	addQcValue(output, "QC:2000020", "mapped read percentage", 100.0 * al_mapped / al_total);
	addQcValue(output, "QC:2000021", "on-target read percentage", 100.0 * al_ontarget / al_total);
	addQcValue(output, "QC:2000057", "near-target read percentage", 100.0 * al_neartarget / al_total);
	if (paired_end)
	{
		addQcValue(output, "QC:2000022", "properly-paired read percentage", 100.0 * al_proper_paired / al_total);
		addQcValue(output, "QC:2000023", "insert size", insert_size_sum / insert_size_read_count);
	}
	else
	{
		addQcValue(output, "QC:2000022", "properly-paired read percentage", "n/a (single end)");
		addQcValue(output, "QC:2000023", "insert size", "n/a (single end)");
	}
	if (al_dup==0)
	{
		addQcValue(output, "QC:2000024", "duplicate read percentage", "n/a (no duplicates marked or duplicates removed during data analysis)");
	}
	else
	{
		addQcValue(output, "QC:2000024", "duplicate read percentage", 100.0 * al_dup / al_total);
	}
	addQcValue(output, "QC:2000050", "bases usable (MB)", (double)bases_usable / 1000000.0);
	addQcValue(output, "QC:2000025", "target region read depth", avg_depth);

	//cfDNA specific
	QVector<double> cumsum_depth(5);
	cumsum_depth.fill(0);
	double cumsum_depth_running = 0;
	if (is_cfdna)
	{
		for (int i=4; i>=0; --i)
		{
			cumsum_depth_running += (double)bases_usable_dp[i] / roi_bases;
			cumsum_depth[i] = cumsum_depth_running;
		}

		for (int i=2; i<=4; ++i)
		{
			addQcValue(output, "QC:200007" + QByteArray::number(i-1), "target region read depth " + QByteArray::number(i) + "-fold duplication", cumsum_depth[i]);
		}
		addQcValue(output, "QC:2000074", "raw target region read depth", (double)bases_usable_raw / roi_bases);
	}

	QVector<int> depths;
	depths << 10 << 20 << 30 << 50 << 60 << 100 << 200 << 500;
	QVector<QByteArray> accessions;
	accessions << "QC:2000026" << "QC:2000027" << "QC:2000028" << "QC:2000029" << "QC:2000099" << "QC:2000030" << "QC:2000031" << "QC:2000032";

	if (is_cfdna)
	{
		//extend depth range for cfDNA samples
		depths << 1000 << 2500 << 5000 << 7500 << 10000 << 15000;
		accessions << "QC:2000065" << "QC:2000066" << "QC:2000067" << "QC:2000068" << "QC:2000069" << "QC:2000070";
	}

	for (int i=0; i<depths.count(); ++i)
	{
		double cov_bases = 0.0;
		for (int bin=depth_dist.binIndex(depths[i]); bin<depth_dist.binCount(); ++bin) cov_bases += depth_dist.binValue(bin);
		addQcValue(output, accessions[i], "target region " + QByteArray::number(depths[i]) + "x percentage", 100.0 * cov_bases / roi_bases);
	}
	addQcValue(output, "QC:2000058", "target region half depth percentage", 100.0 * bases_covered_at_least_half_depth / roi_bases);
	addQcValue(output, "QC:2000059", "AT dropout", at_dropout);
	addQcValue(output, "QC:2000060", "GC dropout", gc_dropout);

	//add depth distribtion plot
	LinePlot plot;
	plot.setXLabel("depth of coverage");
	plot.setYLabel("target region [%]");
	plot.setXValues(depth_dist.xCoords());
	plot.addLine(depth_dist.yCoords(true));
	QString plotname = Helper::tempFileName(".png");
	plot.store(plotname);
	addQcPlot(output, "QC:2000037", "depth distribution plot", plotname);
	QFile::remove(plotname);

	//add insert size distribution plot
	if (paired_end)
	{
		LinePlot plot2;
		plot2.setXLabel("insert size");
		plot2.setYLabel("reads [%]");
		plot2.setXValues(insert_dist.xCoords());
		plot2.addLine(insert_dist.yCoords(true));

		plotname = Helper::tempFileName(".png");
		plot2.store(plotname);
		addQcPlot(output, "QC:2000038", "insert size distribution plot", plotname);
		QFile::remove(plotname);
	}

	//add fragment duplication distribution plot
	if (is_cfdna && dp_dist.binSum() != 0)
	{
		LinePlot plot3;
		plot3.setXLabel("duplicates");
		plot3.setYLabel("fragments [%]");
		plot3.setYRange(0, 100);
		plot3.setXValues(dp_dist.xCoords());
		plot3.addLine(dp_dist.yCoords(true));

		plotname = Helper::tempFileName(".png");
		plot3.store(plotname);
		addQcPlot(output, "QC:2000075", "fragment duplication distribution plot", plotname);

		QFile::remove(plotname);
	}

	//add duplication depth distribution plot
	if (is_cfdna && dp_dist.binSum() != 0)
	{
		LinePlot plot4;
		plot4.setXLabel("minimum number of duplicates");
		plot4.setYLabel("depth of coverage");
		QVector<double> xvalues;
		for (int i=1; i<=cumsum_depth.size()-1; ++i)
		{
			xvalues << i;
		}

		plot4.setXValues(xvalues);
		cumsum_depth.pop_front();
		plot4.addLine(cumsum_depth);

		plotname = Helper::tempFileName(".png");
		plot4.store(plotname);
		addQcPlot(output, "QC:2000076", "duplication-coverage plot", plotname);
		QFile::remove(plotname);
	}

	//add GC bias plot
	LinePlot plot3;
	plot3.setXLabel("GC bin");
	plot3.setYLabel("count [%]");
	plot3.setXValues(BasicStatistics::range(0.0, 100.0, 1.0));
	plot3.addLine(gc_roi_percentages, "target region");
	plot3.addLine(gc_read_percentages, "reads");
	plotname = Helper::tempFileName(".png");
	plot3.store(plotname);
	addQcPlot(output, "QC:2000061","GC bias plot", plotname);
	QFile::remove(plotname);

	//add YX read ratio
	double yx_ratio = yxRatio(reader);
	if (!std::isnan(yx_ratio))
	{
		addQcValue(output, "QC:2000139", "chrY/chrX read ratio", QString::number(yx_ratio, 'f', 4));
	}

	return output;
}

QCCollection Statistics::mapping(const QString &bam_file, const QString& ref_file, int min_mapq)
{
	//open BAM file
    BamReader reader(bam_file, ref_file);
    reader.skipBases();
    reader.skipQualities();
	reader.skipTags();
	FastaFileIndex ref_idx(ref_file);

	//init counts
	long long al_total = 0;
	long long al_mapped = 0;
	long long al_ontarget = 0;
	long long al_dup = 0;
	long long al_proper_paired = 0;
	long long insert_size_read_count = 0;
	double bases_trimmed = 0;
	double bases_mapped = 0;
	double bases_clipped = 0;
	double insert_size_sum = 0;
	Histogram insert_dist(0, 999, 5);
	long long bases_usable = 0;
	int max_length = 0;
	bool paired_end = false;

	//iterate through all alignments
	BamAlignment al;
	while (reader.getNextAlignment(al))
	{
		//skip secondary alignments
		if (al.isSecondaryAlignment() || al.isSupplementaryAlignment()) continue;

		++al_total;

		const int length = al.length();
		max_length = std::max(max_length, length);

		//track if spliced alignment
		bool spliced_alignment = false;

		if (!al.isUnmapped())
		{
			++al_mapped;

			//calculate soft/hard-clipped bases
			bases_mapped += length;
			const QList<CigarOp> cigar_data = al.cigarData();
			foreach(const CigarOp& op, cigar_data)
			{
				if (op.Type==BAM_CSOFT_CLIP || op.Type==BAM_CHARD_CLIP)
				{
					bases_clipped += op.Length;
				}
				else if (op.Type==BAM_CREF_SKIP)
				{
					spliced_alignment = true;
				}
			}

			//usable
			if (reader.chromosome(al.chromosomeID()).isNonSpecial())
			{
				++al_ontarget;

				if (!al.isDuplicate() && al.mappingQuality()>=min_mapq)
				{
					bases_usable += length;
				}
			}
		}

		//insert size
		if (al.isPaired())
		{
			paired_end = true;

			if (al.isProperPair())
			{
				++al_proper_paired;
				//if alignment is spliced, exclude it from insert size calculation
				if (!spliced_alignment)
				{
					const int insert_size = abs(al.insertSize());
					if (insert_size < 1000)
					{
						++insert_size_read_count;
						insert_size_sum += insert_size;
						insert_dist.inc(insert_size, true);
					}
				}
			}
		}

		//trimmed bases (this is not entirely correct if the first alignments are all trimmed, but saves the second pass through the data)
		if (length<max_length)
		{
			bases_trimmed += (max_length - length);
		}

		if (al.isDuplicate())
		{
			++al_dup;
		}
	}
	bases_usable -= bases_clipped;

	//calcualte number of 'N' bases in genome (takes about 10s, but that's ok for WGS QC)
	double no_base = 0.0;
	foreach(const Chromosome& c, reader.chromosomes())
	{
		if (c.isNonSpecial())
		{
			int n = ref_idx.n(c);
			no_base +=n;
		}
	}

	//output
	QCCollection output;
	addQcValue(output, "QC:2000019", "trimmed base percentage", 100.0 * bases_trimmed / al_total / max_length);
	addQcValue(output, "QC:2000052", "clipped base percentage", 100.0 * bases_clipped / bases_mapped);
	addQcValue(output, "QC:2000020", "mapped read percentage", 100.0 * al_mapped / al_total);
	addQcValue(output, "QC:2000021", "on-target read percentage", 100.0 * al_ontarget / al_total);
	if (paired_end)
	{
		addQcValue(output, "QC:2000022", "properly-paired read percentage", 100.0 * al_proper_paired / al_total);
		addQcValue(output, "QC:2000023", "insert size", insert_size_sum / insert_size_read_count);
	}
	else
	{
		addQcValue(output, "QC:2000022", "properly-paired read percentage", "n/a (single end)");
		addQcValue(output, "QC:2000023", "insert size", "n/a (single end)");
	}
	if (al_dup==0)
	{
		addQcValue(output, "QC:2000024", "duplicate read percentage", "n/a (duplicates not marked or removed during data analysis)");
	}
	else
	{
		addQcValue(output, "QC:2000024", "duplicate read percentage", 100.0 * al_dup / al_total);
	}
	addQcValue(output, "QC:2000050", "bases usable (MB)", (double) bases_usable / 1000000.0);
	addQcValue(output, "QC:2000025", "target region read depth", (double) bases_usable / (reader.genomeSize(false) - no_base));

	//add insert size distribution plot
	if (paired_end)
	{
		if (insert_dist.binSum()>0)
		{
			LinePlot plot2;
			plot2.setXLabel("insert size");
			plot2.setYLabel("reads [%]");
			plot2.setXValues(insert_dist.xCoords());
			plot2.addLine(insert_dist.yCoords(true));

			QString plotname = Helper::tempFileName(".png");
			plot2.store(plotname);
			addQcPlot(output, "QC:2000038", "insert size distribution plot", plotname);
			QFile::remove(plotname);
		}
		else
		{
			Log::warn("Skipping insert size histogram - no read pairs found!");
		}
	}

	//add YX read ratio
	double yx_ratio = yxRatio(reader);
	if (!std::isnan(yx_ratio))
	{
		addQcValue(output, "QC:2000139", "chrY/chrX read ratio", QString::number(yx_ratio, 'f', 4));
	}

	return output;
}

QCCollection Statistics::mapping_wgs(const QString &bam_file, const QString& bedpath, int min_mapq, const QString& ref_file)
{
	//open BAM file
    BamReader reader(bam_file, ref_file);
    reader.skipBases();
    reader.skipQualities();
	reader.skipTags();
	FastaFileIndex ref_idx(ref_file);
	bool roi_available = false;
	BedFile roi;
	if (bedpath != "")
	{
		roi_available = true;
		roi.load(bedpath, false);

		//check target region is merged/sorted
		if (!roi.isMergedAndSorted())
		{
			roi.sort();
			roi.merge();
		}
	}

    //create coverage statistics data structure
	QVector<RegionDepth> roi_cov(roi.count());

	for (int i=0; i<roi.count(); ++i)
	{
		const BedLine& line = roi[i];
		roi_cov[i] = RegionDepth(line.chr(), line.start(), line.end());
	}

	//prepare AT/GC dropout data structure
	BedFile dropout;
	dropout.add(roi);
	dropout.chunk(100);
	QHash<int, double> gc_roi;
	QHash<int, double> gc_reads;
	QHash<int, int> gc_index_to_bin_map;
	for (int i=0; i<dropout.count(); ++i)
	{
		BedLine& line = dropout[i];
		Sequence seq = ref_idx.seq(line.chr(), line.start(), line.length());
		double gc_content = seq.gcContent();
		if (!BasicStatistics::isValidFloat(gc_content))
		{
			gc_index_to_bin_map[i] = -1;
		}
		else
		{
			int bin = (int)std::floor(100.0*gc_content);
			gc_index_to_bin_map[i] = bin;
			gc_roi[bin] += 1.0;
		}
	}
	ChromosomalIndex<BedFile> dropout_index(dropout);

	//init counts
	long long al_total = 0;
	long long al_mapped = 0;
	long long al_ontarget = 0;
	long long al_dup = 0;
	long long al_proper_paired = 0;
	long long insert_size_read_count = 0;
	double bases_trimmed = 0;
	double bases_mapped = 0;
	double bases_clipped = 0;
	double insert_size_sum = 0;
	Histogram insert_dist(0, 999, 5);
	long long bases_usable = 0;
	long long bases_usable_roi = 0;

	int max_length = 0;
	bool paired_end = false;

	//iterate through all alignments
	BamAlignment al;
	while (reader.getNextAlignment(al))
	{
		//skip secondary alignments
		if (al.isSecondaryAlignment() || al.isSupplementaryAlignment()) continue;

		++al_total;

		const int length = al.length();
		max_length = std::max(max_length, length);

		//track if spliced alignment
		bool spliced_alignment = false;

		if (!al.isUnmapped())
		{
			++al_mapped;

			//calculate soft/hard-clipped bases
			bases_mapped += length;
			const QList<CigarOp> cigar_data = al.cigarData();
			foreach(const CigarOp& op, cigar_data)
			{
				if (op.Type==BAM_CSOFT_CLIP || op.Type==BAM_CHARD_CLIP)
				{
					bases_clipped += op.Length;
				}
				else if (op.Type==BAM_CREF_SKIP)
				{
					spliced_alignment = true;
				}
			}

			//usable
			Chromosome chr = reader.chromosome(al.chromosomeID());
			if (chr.isNonSpecial())
			{
				++al_ontarget;
				if (!al.isDuplicate() && al.mappingQuality()>=min_mapq)
				{
					bases_usable += length;
				}
			}
		}

		//insert size
		if (al.isPaired())
		{
			paired_end = true;

			if (al.isProperPair())
			{
				++al_proper_paired;
				//if alignment is spliced, exclude it from insert size calculation
				if (!spliced_alignment)
				{
					const int insert_size = abs(al.insertSize());
					if (insert_size < 1000)
					{
						++insert_size_read_count;
						insert_size_sum += insert_size;
						insert_dist.inc(insert_size, true);
					}
				}
			}
		}

		//trimmed bases (this is not entirely correct if the first alignments are all trimmed, but saves the second pass through the data)
		if (length<max_length)
		{
			bases_trimmed += (max_length - length);
		}

		if (al.isDuplicate())
		{
			++al_dup;
		}
	}

	for (int i=0; i<roi.count(); ++i)
	{
		reader.setRegion(roi[i].chr(), roi[i].start(), roi[i].end());

		BamAlignment al;
		while (reader.getNextAlignment(al))
		{
			//skip secondary alignments
			if (al.isSecondaryAlignment() || al.isSupplementaryAlignment() || al.isUnmapped()) continue;

			//calculate GC statistics
			QVector<int> indices = dropout_index.matchingIndices(reader.chromosome(al.chromosomeID()), al.start(), al.end());
			foreach(int index, indices)
			{
				int bin = gc_index_to_bin_map[index];
				if (bin>=0)
				{
					gc_reads[bin] += 1.0/indices.count();
				}
			}

			if (!al.isDuplicate() && al.mappingQuality()>=min_mapq)
			{
				//calculate usable bases and base-resolution coverage on target region
				bases_usable_roi += al.length();
				roi_cov[i].incrementRegion(al.start(), al.end());
			}
		}
	}
	bases_usable -= bases_clipped;

	//calculate coverage depth statistics
	double avg_depth = (double) bases_usable_roi / roi.baseCount();
	int half_depth = std::round(0.5*avg_depth);
	long long bases_covered_at_least_half_depth = 0;
	int hist_max = 599;
	int hist_step = 5;

	Histogram depth_dist(0, hist_max, hist_step);
	for(int i=0; i<roi_cov.count(); ++i)
	{
		for(int j=roi_cov[i].start(); j<=roi_cov[i].end(); ++j)
		{
			int depth = roi_cov[i][j];
			depth_dist.inc(depth, true);
			if(depth>=half_depth)
			{
				++bases_covered_at_least_half_depth;
			}
		}
	}

	//calculate AT/GC dropout
	QList<double> values = gc_roi.values();
	double gc_sum = std::accumulate(values.begin(),values.end(), 0.0);
	values = gc_reads.values();
	double roi_sum = std::accumulate(values.begin(),values.end(), 0.0);
	double at_dropout = 0;
	double gc_dropout = 0;
	QVector<double> gc_read_percentages;
	QVector<double> gc_roi_percentages;
	for (int i=0; i<100; ++i)
	{
		double roi_perc = 100.0*gc_roi[i]/gc_sum;
		gc_roi_percentages << roi_perc;
		double read_perc = 100.0*gc_reads[i]/roi_sum;
		gc_read_percentages << read_perc;

		double diff = roi_perc-read_perc;
		if (diff>0)
		{
			if (i<=50)
			{
				at_dropout += diff;
			}
			if (i>=50)
			{
				gc_dropout += diff;
			}
		}
	}

	//calcualte number of 'N' bases in genome (takes about 10s, but that's ok for WGS QC)
	double no_base = 0.0;
	foreach(const Chromosome& c, reader.chromosomes())
	{
		if (c.isNonSpecial())
		{
			int n = ref_idx.n(c);
			no_base +=n;
		}
	}

	//output
	QCCollection output;
	addQcValue(output, "QC:2000019", "trimmed base percentage", 100.0 * bases_trimmed / al_total / max_length);
	addQcValue(output, "QC:2000052", "clipped base percentage", 100.0 * bases_clipped / bases_mapped);
	addQcValue(output, "QC:2000020", "mapped read percentage", 100.0 * al_mapped / al_total);
	addQcValue(output, "QC:2000021", "on-target read percentage", 100.0 * al_ontarget / al_total);
	if (paired_end)
	{
		addQcValue(output, "QC:2000022", "properly-paired read percentage", 100.0 * al_proper_paired / al_total);
		addQcValue(output, "QC:2000023", "insert size", insert_size_sum / insert_size_read_count);
	}
	else
	{
		addQcValue(output, "QC:2000022", "properly-paired read percentage", "n/a (single end)");
		addQcValue(output, "QC:2000023", "insert size", "n/a (single end)");
	}
	if (al_dup==0)
	{
		addQcValue(output, "QC:2000024", "duplicate read percentage", "n/a (duplicates not marked or removed during data analysis)");
	}
	else
	{
		addQcValue(output, "QC:2000024", "duplicate read percentage", 100.0 * al_dup / al_total);
	}
	addQcValue(output, "QC:2000050", "bases usable (MB)", (double) bases_usable / 1000000.0);
	addQcValue(output, "QC:2000025", "target region read depth", (double) bases_usable / (reader.genomeSize(false) - no_base));

	if (roi_available)
	{
		QVector<int> depth_values;
		depth_values << 10 << 20 << 30 << 50 << 60 << 100 << 200 << 500;
		QVector<QByteArray> accessions;
		accessions << "QC:2000026" << "QC:2000027" << "QC:2000028" << "QC:2000029" << "QC:2000099" << "QC:2000030" << "QC:2000031" << "QC:2000032";

		for (int i=0; i<depth_values.count(); ++i)
		{
			double cov_bases = 0.0;
			for (int bin=depth_dist.binIndex(depth_values[i]); bin<depth_dist.binCount(); ++bin) cov_bases += depth_dist.binValue(bin);
			addQcValue(output, accessions[i], "target region " + QByteArray::number(depth_values[i]) + "x percentage", 100.0 * cov_bases / roi.baseCount());
		}
		addQcValue(output, "QC:2000058", "target region half depth percentage", 100.0 * bases_covered_at_least_half_depth / roi.baseCount());
		addQcValue(output, "QC:2000059", "AT dropout", at_dropout);
		addQcValue(output, "QC:2000060", "GC dropout", gc_dropout);
	}

	if (roi_available)
	{
		//add depth distribtion plot
		LinePlot plot;
		plot.setXLabel("depth of coverage");
		plot.setYLabel("target region [%]");
		plot.setXValues(depth_dist.xCoords());
		plot.addLine(depth_dist.yCoords(true));
		QString plotname = Helper::tempFileName(".png");
		plot.store(plotname);
		addQcPlot(output, "QC:2000037", "depth distribution plot", plotname);
		QFile::remove(plotname);
	}

	//add insert size distribution plot
	if (paired_end)
	{
		if (insert_dist.binSum()>0)
		{
			LinePlot plot2;
			plot2.setXLabel("insert size");
			plot2.setYLabel("reads [%]");
			plot2.setXValues(insert_dist.xCoords());
			plot2.addLine(insert_dist.yCoords(true));

			QString plotname = Helper::tempFileName(".png");
			plot2.store(plotname);
			addQcPlot(output, "QC:2000038", "insert size distribution plot", plotname);
			QFile::remove(plotname);
		}
		else
		{
			Log::warn("Skipping insert size histogram - no read pairs found!");
		}
	}

	//add GC bias plot
	if (roi_available)
	{
		LinePlot plot3;
		plot3.setXLabel("GC bin");
		plot3.setYLabel("count [%]");
		plot3.setXValues(BasicStatistics::range(0.0, 100.0, 1.0));
		plot3.addLine(gc_roi_percentages, "target region");
		plot3.addLine(gc_read_percentages, "reads");
		QString plotname = Helper::tempFileName(".png");
		plot3.store(plotname);
		addQcPlot(output, "QC:2000061","GC bias plot", plotname);
		QFile::remove(plotname);
	}

	//add YX read ratio
	double yx_ratio = yxRatio(reader);
	if (!std::isnan(yx_ratio))
	{
		addQcValue(output, "QC:2000139", "chrY/chrX read ratio", QString::number(yx_ratio, 'f', 4));
	}

	return output;
}

QCCollection Statistics::mapping_housekeeping(const BedFile& bed_file, const QString& bam_file, const QString& ref_file, int min_mapq)
{
	QCCollection metrics;

	// get QC of housekeeping genes
	QCCollection housekeeping_qc = Statistics::mapping(bed_file, bam_file, ref_file, min_mapq);
	//extract parameter
	Statistics::addQcValue(metrics, "QC:2000100", "housekeeping genes read percentage", housekeeping_qc.value("QC:2000021", true).asDouble());
	Statistics::addQcValue(metrics, "QC:2000101", "housekeeping genes read depth", housekeeping_qc.value("QC:2000025", true).asDouble());
	QVector<int> coverage_steps = QVector<int>() << 10 << 20 << 30 << 50 << 100 << 200 << 500;
	for (int i = 0; i < coverage_steps.length(); ++i)
	{
		Statistics::addQcValue(metrics,
							   QByteArray("QC:200010") + QByteArray::number((2 + i)),
							   QByteArray("housekeeping genes ") + QByteArray::number(coverage_steps.at(i)) + "x percentage",
							   housekeeping_qc.value(QByteArray("QC:20000") + QByteArray::number((26 + i)), true).asDouble());
	}
	return metrics;
}

QCCollection Statistics::region(const BedFile& bed_file, bool merge)
{
	//sort if necessary
	BedFile regions = bed_file;
	bool is_sorted = regions.isSorted();

	//merge if necessary
	bool is_merged = regions.isMerged();
	if (!is_merged && merge)
	{
		regions.merge();
		is_merged = true;
		is_sorted = true;
	}

	//traverse lines
	QSet<Chromosome> chromosomes;
	QVector<double> lengths;
	int length_min = std::numeric_limits<int>::max();
	int length_max = std::numeric_limits<int>::min();
	lengths.reserve(regions.count());
	for (int i=0; i<regions.count(); ++i)
	{
		const BedLine& line = regions[i];
		chromosomes.insert(line.chr());
		int length = line.length();
		length_min = std::min(length, length_min);
		length_max = std::max(length, length_max);
		lengths.append(length);
	}

	//length statistics
	double length_sum = std::accumulate(lengths.begin(), lengths.end(), 0.0);
	double length_mean = length_sum / lengths.size();
	double sq_sum = std::inner_product(lengths.begin(), lengths.end(), lengths.begin(), 0.0);
	double length_stdev = std::sqrt(sq_sum / lengths.size() - length_mean * length_mean);

	//chromosome list string
    QList<Chromosome> chr_list = chromosomes.values();
	std::sort(chr_list.begin(), chr_list.end());
	QString chr_list_str = "";
	foreach(Chromosome chr, chr_list)
	{
		if (chr_list_str!="") chr_list_str += ", ";
		chr_list_str += chr.strNormalized(false);
	}

	//output
	QCCollection output;
	output.insert(QCValue("roi_bases", length_sum, "Number of bases in the (merged) target region."));
	output.insert(QCValue("roi_fragments", regions.count(), "Number of (merged) target regions."));
	output.insert(QCValue("roi_chromosomes", QString::number(chromosomes.count()) + " (" + chr_list_str + ")", "Chromosomes in the target region."));
	output.insert(QCValue("roi_is_sorted", is_sorted ? "yes" : "no", "If the target region is sorted according to chromosome and start position."));
	output.insert(QCValue("roi_is_merged", is_merged ? "yes" : "no", "If the target region is merged, i.e. it has no overlapping fragments."));
	output.insert(QCValue("roi_fragment_min", length_min, "Minimum fragment size of (merged) target region."));
	output.insert(QCValue("roi_fragment_max", length_max, "Maximum fragment size of (merged) target region."));
	output.insert(QCValue("roi_fragment_mean", length_mean, "Mean fragment size of (merged) target region."));
	output.insert(QCValue("roi_fragment_stdev", length_stdev, "Fragment size standard deviation of (merged) target region."));
	return output;
}

QCValue Statistics::mutationBurden(QString somatic_vcf, QString target, QString blacklist)
{
	QString qcml_name = "raw somatic variant rate";
	QString qcml_desc = "Somatic variant rate in variants per Megabase without normalization to TSG/Oncogenes or exome size. SNVs in blacklisted genes were discarded for the calculation.";
	QString qcml_id = "QC:2000089";
	QCValue undefined = QCValue(qcml_name, "n/a", qcml_desc, qcml_id);

	if(target.isEmpty() || blacklist.isEmpty()) return undefined;

	BedFile target_file;
	target_file.load(target);

	BedFile blacklist_file;
	blacklist_file.load(blacklist);

	if(target_file.count() == 0 || blacklist_file.count() == 0) return undefined;

	//Remove blacklisted region from target region
	blacklist_file.merge();
	target_file.subtract(blacklist_file);

	//Process variants
	VcfFile vcf_file;
	vcf_file.load(somatic_vcf);
	int somatic_var_count = 0;
	for(int i=0;i<vcf_file.count();++i)
	{
		//Skip low quality filtered variants
		if(vcf_file[i].filters().contains("freq-nor")) continue;
		if(vcf_file[i].filters().contains("freq-tum")) continue;
		if(vcf_file[i].filters().contains("depth-nor")) continue;
		if(vcf_file[i].filters().contains("depth-tum")) continue;
		if(vcf_file[i].filters().contains("lt-3-reads")) continue;
		if(vcf_file[i].filters().contains("LowEVS")) continue; //Skip strelka2 low quality variants
		if(vcf_file[i].filters().contains("LowDepth")) continue; //Skip strelka2 low depth variants
		if(vcf_file[i].filters().contains("weak-evidence")) continue; //Skip dragen low quality variants

		const Chromosome chr = vcf_file[i].chr();
		int start = vcf_file[i].start();
		int end = vcf_file[i].end();
		if(target_file.overlapsWith(chr, start, end))
		{
			++somatic_var_count;
		}
	}

	//Calculate mutation burden
	double target_size = target_file.baseCount() / 1000000.0;
	double mutation_burden = somatic_var_count / target_size;
	return QCValue(qcml_name, QString::number(mutation_burden, 'f', 2), qcml_desc, qcml_id);
}

QCValue Statistics::mutationBurdenNormalized(QString somatic_vcf, QString exons, QString target, QString tsg, QString blacklist)
{
	QString qcml_name = "somatic variant rate";
	QString qcml_desc = "Categorized somatic variant rate followed by the somatic variant rate [variants/Mbp] normalized for the target region and exome size and corrected for tumor suppressors.";
	QString qcml_id = "QC:2000053";
	QCValue undefined = QCValue(qcml_name, "n/a", qcml_desc, qcml_id);

	//check input files given
	if(exons.isEmpty() || target.isEmpty() || tsg.isEmpty() || blacklist.isEmpty())
	{
		return undefined;
	}

	//check input files not empty
	BedFile target_file;
	target_file.load(target);

	BedFile target_exon_file;
	target_exon_file.load(exons);
	double exome_size = target_exon_file.baseCount() / 1000000.0;

	BedFile blacklist_file;
	blacklist_file.load(blacklist);

	BedFile tsg_bed_file;
	tsg_bed_file.load(tsg);

	if(target_file.count()==0 || target_exon_file.count()==0 || blacklist_file.count()==0 || tsg_bed_file.count()==0)
	{
		return undefined;
	}

	//Reduce target region to exons only, we will consider only exonic variants
	target_exon_file.merge();
	target_file.intersect(target_exon_file);

	//Remove blacklisted region from target region
	blacklist_file.merge();
	target_file.subtract(blacklist_file);

	//Check target is still non-empty
	if (target_file.count()==0) return undefined;

	//Process variants
	VcfFile vcf_file;
	vcf_file.load(somatic_vcf);
	int somatic_var_count = 0;
	int somatic_count_in_tsg = 0;
	for(int i=0;i<vcf_file.count();++i)
	{
		if(vcf_file[i].filters().contains("freq-nor")) continue;
		if(vcf_file[i].filters().contains("freq-tum")) continue;
		if(vcf_file[i].filters().contains("depth-nor")) continue;
		if(vcf_file[i].filters().contains("depth-tum")) continue;
		if(vcf_file[i].filters().contains("lt-3-reads")) continue;
		if(vcf_file[i].filters().contains("LowEVS")) continue; //Skip strelka2 low quality variants
		if(vcf_file[i].filters().contains("LowDepth")) continue; //Skip strelka2 low depth variants
		if(vcf_file[i].filters().contains("weak-evidence")) continue; //Skip dragen low quality variants

		const Chromosome chr = vcf_file[i].chr();
		int start = vcf_file[i].start();
		int end = vcf_file[i].end();

		if(target_file.overlapsWith(chr, start, end))
		{
			++somatic_var_count;

			if (tsg_bed_file.overlapsWith(chr, start, end))
			{
				++somatic_count_in_tsg;
			}
		}
	}

	//Calculate mutation burden
	double target_size = target_file.baseCount() / 1000000.0;
	double mutation_burden = ( (somatic_var_count - somatic_count_in_tsg) * exome_size / target_size + somatic_count_in_tsg ) / exome_size;
	return QCValue(qcml_name, QString::number(mutation_burden, 'f', 2), qcml_desc, qcml_id);
}

QCCollection Statistics::somaticCustomDepth(const BedFile& bed_file, QString bam_file, QString ref_file, int min_mapq)
{
	//check target region is merged/sorted and create index
	if (!bed_file.isMergedAndSorted())
	{
		THROW(ArgumentException, "Merged and sorted BED file required for depth details statistics!");
	}
	ChromosomalIndex<BedFile> roi_index(bed_file);

	//create coverage statistics data structure
	long long roi_bases = 0;
	QHash<int, QMap<int, int> > roi_cov;
	for (int i=0; i<bed_file.count(); ++i)
	{
		const BedLine& line = bed_file[i];
		int chr_num = line.chr().num();
		if (!roi_cov.contains(chr_num))
		{
			roi_cov.insert(chr_num, QMap<int, int>());
		}

		for(int p=line.start(); p<=line.end(); ++p)
		{
			roi_cov[chr_num].insert(p, 0);
		}
		roi_bases += line.length();
	}

	long long bases_usable = 0;

	//iterate through all alignments
	BamReader reader(bam_file, ref_file);
	reader.skipBases();
	reader.skipQualities();
	reader.skipTags();
	BamAlignment al;
	while (reader.getNextAlignment(al))
	{
		//skip secondary alignments
		if (al.isSecondaryAlignment() || al.isSupplementaryAlignment()) continue;

		if (!al.isUnmapped())
		{

			//calculate soft/hard-clipped bases
			const int start_pos = al.start();
			const int end_pos = al.end();

			//calculate usable bases, base-resolution coverage and GC statistics
			const Chromosome& chr = reader.chromosome(al.chromosomeID());
			QVector<int> indices = roi_index.matchingIndices(chr, start_pos-250, end_pos+250);
			if (indices.count()!=0)
			{
				//check if on target
				indices = roi_index.matchingIndices(chr, start_pos, end_pos);
				if (indices.count()!=0)
				{
					//calculate usable bases and base-resolution coverage on target region
					if (!al.isDuplicate() && al.mappingQuality()>=min_mapq)
					{
						foreach(int index, indices)
						{
							const int ol_start = std::max(bed_file[index].start(), start_pos);
							const int ol_end = std::min(bed_file[index].end(), end_pos);
							bases_usable += ol_end - ol_start + 1;

							auto it = roi_cov[chr.num()].lowerBound(ol_start);
							auto end = roi_cov[chr.num()].upperBound(ol_end);
							while (it!=end)
							{
								(*it)++;
								++it;
							}
						}
					}

				}
			}
		}

	}

	//calculate coverage depth statistics
	double avg_depth = (double) bases_usable / roi_bases;

	int hist_max = 599;
	int hist_step = 5;
	if (avg_depth>200)
	{
		hist_max += 400;
		hist_step += 5;
	}
	if (avg_depth>500)
	{
		hist_max += 500;
	}
	if (avg_depth>1000)
	{
		hist_max += 1000;
	}


	Histogram depth_dist(0, hist_max, hist_step);
	QHashIterator<int, QMap<int, int> > it(roi_cov);
	while(it.hasNext())
	{
		it.next();
		QMapIterator<int, int> it2(it.value());
		while(it2.hasNext())
		{
			it2.next();

			int depth = it2.value();
			depth_dist.inc(depth, true);
		}
	}

	//output
	QCCollection output;
	addQcValue(output, "QC:2000097", "somatic custom target region read depth", avg_depth);


	QVector<int> depths;
	depths << 10 << 20 << 30 << 50 << 60 << 100 << 200 << 500;
	QVector<QByteArray> accessions;
	accessions << "QC:2000090" << "QC:2000091" << "QC:2000092" << "QC:2000093" << "QC:2000098" << "QC:2000094" << "QC:2000095" << "QC:2000096";


	for (int i=0; i<depths.count(); ++i)
	{
		double cov_bases = 0.0;
		for (int bin=depth_dist.binIndex(depths[i]); bin<depth_dist.binCount(); ++bin) cov_bases += depth_dist.binValue(bin);
		addQcValue(output, accessions[i], "somatic custom target " + QByteArray::number(depths[i]) + "x percentage", 100.0 * cov_bases / roi_bases);
	}

	return output;
}

QCCollection Statistics::somatic(GenomeBuild build, QString& tumor_bam, QString& normal_bam, QString& somatic_vcf, QString ref_fasta, const BedFile& target_file, bool skip_plots)
{
	QCCollection output;

	//sample correlation
	auto tumor_genotypes = SampleSimilarity::genotypesFromBam(build, tumor_bam, 30, 500, true, target_file, ref_fasta);

	auto normal_genotypes = SampleSimilarity::genotypesFromBam(build, normal_bam, 30, 500, true, target_file, ref_fasta);
	SampleSimilarity sc;

	sc.calculateSimilarity(tumor_genotypes, normal_genotypes);

	addQcValue(output, "QC:2000040", "sample correlation", sc.olCount()<100 ? "n/a (too few variants)" : QString::number(sc.sampleCorrelation(),'f',2));

	//variants
	VcfFile variants;

	variants.load(somatic_vcf);

	variants.sort();

	//total variants
	addQcValue(output, "QC:2000013", "variant count", variants.count());

	//total variants filtered
	int somatic_count = 0;
	for(int i=0; i<variants.count(); ++i)
	{
		if (!variants[i].filtersPassed())	continue;
		++somatic_count;
	}
	addQcValue(output, "QC:2000041", "somatic variant count", somatic_count);

	//percentage known variants
	double known_count = 0;
	if (variants.vcfHeader().infoIdDefined("gnomADg_AF"))
	{
		if (variants.count()!=0)
		{
			for(int i=0; i<variants.count(); ++i)
			{
				if (!variants[i].filtersPassed())	continue;

				QByteArray anno = variants[i].info("gnomADg_AF");

				bool ok = false;
				double tmp = anno.toDouble(&ok);
				if (ok && tmp>0.01)
				{
					++known_count;
				}

			}

			addQcValue(output, "QC:2000045", "known somatic variants percentage", 100.0*known_count/somatic_count);

		}
		else
		{
			addQcValue(output, "QC:2000045", "known somatic variants percentage", "n/a (no somatic variants)");
		}
	}
	else
	{
		addQcValue(output, "QC:2000045", "known somatic variants percentage", "n/a (no gnomADg_AF annotation info field)");
	}

	//var_perc_indel / var_ti_tv_ratio
	double indel_count = 0;
	double ti_count = 0;
	double tv_count = 0;
	for(int i=0; i<variants.count(); ++i)
	{
		if (!variants[i].filtersPassed()) continue;

		const  VcfLine& var = variants[i];
		if (var.isIns() || var.isDel())
		{
			++indel_count;
		}
		else if ((var.alt(0)=="A" && var.ref()=="G") || (var.alt(0)=="G" && var.ref()=="A") || (var.alt(0)=="T" && var.ref()=="C") || (var.alt(0)=="C" && var.ref()=="T"))
		{
			++ti_count;
		}
		else
		{
			++tv_count;
		}
	}
	if (somatic_count!=0)
	{
		addQcValue(output, "QC:2000042", "somatic indel variants percentage", 100.0*indel_count/somatic_count);
	}
	else
	{
		addQcValue(output, "QC:2000042", "somatic indel variants percentage", "n/a (no variants)");
	}
	if (tv_count!=0)
	{
		addQcValue(output, "QC:2000043", "somatic transition/transversion ratio", ti_count/tv_count);
	}
	else
	{
		addQcValue(output, "QC:2000043", "somatic transition/transversion ratio", "n/a (no variants or transversions)");
	}


	//estimate tumor content
	int min_depth = 30;
	//process variants
	QVector<double> freqs;
	BamReader reader_tumor(tumor_bam, ref_fasta);
	BamReader reader_normal(normal_bam, ref_fasta);
	for (int i=0; i<variants.count(); ++i)
	{
		const  VcfLine& v = variants[i];

		if (!v.isSNV()) continue;
		if (!v.chr().isAutosome()) continue;
		if(!variants[i].filtersPassed()) continue;	//skip non-somatic variants

		Pileup pileup_tu = reader_tumor.getPileup(v.chr(), v.start());
		if (pileup_tu.depth(true) < min_depth) continue;
		Pileup pileup_no = reader_normal.getPileup(v.chr(), v.start());
		if (pileup_no.depth(true) < min_depth) continue;

		double no_freq = pileup_no.frequency(v.ref()[0], v.alt(0)[0]);
		if (!BasicStatistics::isValidFloat(no_freq) || no_freq >= 0.01) continue;

		double tu_freq = pileup_tu.frequency(v.ref()[0], v.alt(0)[0]);
		if (!BasicStatistics::isValidFloat(tu_freq) || tu_freq > 0.6) continue;

		freqs.append(tu_freq);
	}

	//sort data
	std::sort(freqs.begin(), freqs.end());

	//print tumor content estimate
	int n = 10;
	QString value = "";
	if (freqs.count()>=n)
	{
		freqs = freqs.mid(freqs.count()-n);
		double tmp = BasicStatistics::median(freqs, false)*200;
		tmp = std::min(tmp, 100.0);
		value = QString::number(tmp, 'f', 2);
	}
	else
	{
		value = "n/a (too few variants)";
	}
	addQcValue(output, "QC:2000054", "tumor content estimate", value);

	if(skip_plots)	return output;

	//plotting
	QString tumor_id = QFileInfo(tumor_bam).baseName();
	QString normal_id = QFileInfo(normal_bam).baseName();
	QList<Sequence> nucleotides = QList<Sequence>{"A","C","G","T"};

	if(!variants.sampleIDs().contains(tumor_id.toUtf8()))	Log::error("Tumor sample " + tumor_id + " was not found in variant file " + somatic_vcf);
	if(!variants.sampleIDs().contains(normal_id.toUtf8()))	Log::error("Normal sample " + normal_id + " was not found in variant file " + somatic_vcf);

	//plot0: histogram allele frequencies somatic mutations
	Histogram hist_filtered(0,1,0.0125);
	Histogram hist_all(0,1,0.0125);
	for(int i=0; i<variants.count(); ++i)
	{
		if(!variants[i].isSNV())	continue;	//skip indels

		//strelka SNV tumor and normal
		if( variants.vcfHeader().formatIdDefined("AU") && !variants[i].formatValueFromSample("AU", tumor_id.toUtf8()).isEmpty() )
		{
			int count_mut = 0;
			int count_all = 0;
			foreach(const QByteArray& n, nucleotides)
			{
				int tmp = variants[i].formatValueFromSample(n+"U", tumor_id.toUtf8()).split(',')[0].toInt();
				if(n==variants[i].alt(0)) count_mut += tmp;
				count_all += tmp;
			}
			if(count_all>0)
			{
				hist_all.inc((double)count_mut/count_all);
				if(variants[i].filtersPassed()) hist_filtered.inc((double)count_mut/count_all);
			}
		}
		//freebayes tumor and normal
		//##FORMAT=<ID=RO,Number=1,Type=Integer,Description="Reference allele observation count">
		//##FORMAT=<ID=AO,RONumber=A,Type=Integer,Description="Alternate allele observation count">
		else if(variants.vcfHeader().formatIdDefined("AO"))
		{
			int count_mut = variants[i].formatValueFromSample("AO", tumor_id.toUtf8()).toInt();
			int count_all = count_mut + variants[i].formatValueFromSample( "RO", tumor_id.toUtf8()).toInt();
			if(count_all>0)
			{
				hist_all.inc((double)count_mut/count_all);
				if(variants[i].filtersPassed())	hist_filtered.inc((double)count_mut/count_all);
			}
		}
		//mutect
		//##FORMAT=<ID=FA,Number=A,Type=Float,Description="Allele fraction of the alternate allele with regard to reference">
		else if(variants.vcfHeader().formatIdDefined("FA"))
		{
			hist_all.inc(variants[i].formatValueFromSample("FA", tumor_id.toUtf8()).toDouble());;
			if(variants[i].filtersPassed()) hist_filtered.inc(variants[i].formatValueFromSample("FA", tumor_id.toUtf8()).toDouble());
		}
		//MuTect2
		//##FORMAT=<ID=AF,Number=A,Type=Float,Description="Allele fractions of alternate alleles in the tumor">
		else if(variants.vcfHeader().formatIdDefined("AF"))
		{
			hist_all.inc(variants[i].formatValueFromSample("AF", tumor_id.toUtf8()).toDouble());;
			if(variants[i].filtersPassed()) hist_filtered.inc(variants[i].formatValueFromSample("AF", tumor_id.toUtf8()).toDouble());
		}
		// else: strelka indel
	}

	QString plot0name = Helper::tempFileName(".png");
	hist_all.setLabel("all variants");
	hist_filtered.setLabel("variants with filter PASS");
	Histogram::storeCombinedHistogram(plot0name, QList<Histogram>({hist_all,hist_filtered}),"tumor allele frequency","count");
	addQcPlot(output, "QC:2000055","somatic SNVs allele frequency histogram", plot0name);
	QFile::remove(plot0name);

	//plot0b: absolute count mutation distribution
	BarPlot plot0b;
	plot0b.setXLabel("base change");
	plot0b.setYLabel("count");
	QMap<QString,QString> color_map = QMap<QString,QString>{{"C>A","b"},{"C>G","k"},{"C>T","r"},{"T>A","g"},{"T>G","c"},{"T>C","y"}};
	foreach(QString color, color_map)
	{
		plot0b.addColorLegend(color,color_map.key(color));
	}

	QList<int> counts({0,0,0,0,0,0});
	QList<QString> nuc_changes({"C>A","C>G","C>T","T>A","T>G","T>C"});
	QList<QString> colors({"b","k","r","g","c","y"});
	for(int i=0; i<variants.count(); ++i)
	{
		if(!variants[i].filtersPassed()) continue;	//skip non-somatic variants
		if(!variants[i].isSNV()) continue;	//skip indels

		VcfLine v = variants[i];
		QString n = v.ref()+">"+v.alt(0);
		bool contained = false;
		if(nuc_changes.contains(n))	contained = true;
		else
		{
			n = v.ref().toReverseComplement() + ">" + v.alt(0).toReverseComplement();
			if(nuc_changes.contains(n))	contained = true;
		}

		if(!contained)
		{
			Log::warn("Unidentified nucleotide change " + n);
			continue;
		}
		++counts[nuc_changes.indexOf(n)];
	}

	int ymax = 0;
	foreach(int c, counts)
	{
		if(c>ymax)	ymax = c;
	}

	plot0b.setYRange(-ymax*0.02,ymax*1.2);
	plot0b.setXRange(-1.5,nuc_changes.count()+0.5);
	plot0b.setValues(counts, nuc_changes, colors);
	QString plot0bname = Helper::tempFileName(".png");
	plot0b.store(plot0bname);
	addQcPlot(output, "QC:2000056","somatic SNV mutation types", plot0bname);
	QFile::remove(plot0bname);

	//plot1: allele frequencies
	ScatterPlot plot1;
	plot1.setXLabel("tumor allele frequency");
	plot1.setYLabel("normal allele frequency");
	plot1.setXRange(-0.015,1.015);
	plot1.setYRange(-0.015,1.015);
    QList< std::pair<double,double> > points_black;
    QList< std::pair<double,double> > points_green;
	for(int i=0; i<variants.count(); ++i)
	{
		double af_tumor = -1;
		double af_normal = -1;
		int count_mut = 0;
		int count_all = 0;

		//strelka SNV tumor and normal
		if( variants.vcfHeader().formatIdDefined("TIR") && !variants[i].formatValueFromSample("AU", tumor_id.toUtf8()).isEmpty() )
		{
			count_mut = 0;
			count_all = 0;
			foreach(const QByteArray& n, nucleotides)
			{
				int tmp = variants[i].formatValueFromSample(n+"U", tumor_id.toUtf8()).split(',')[0].toInt();
				if(n==variants[i].alt(0))	count_mut += tmp;
				count_all += tmp;
			}
			if(count_all>0)	af_tumor = (double)count_mut/count_all;

			count_mut = 0;
			count_all = 0;
			foreach(const QByteArray& n, nucleotides)
			{
				int tmp = variants[i].formatValueFromSample(n+"U", normal_id.toUtf8()).split(',')[0].toInt();
				if(n==variants[i].alt(0))	count_mut += tmp;
				count_all += tmp;
			}
			if(count_all>0)	af_normal = (double)count_mut/count_all;
		}
		else if( variants.vcfHeader().formatIdDefined("TIR") && !variants[i].formatValueFromSample("TIR", tumor_id.toUtf8()).isEmpty() )	//indels strelka
		{
			//TIR + TAR tumor
			count_mut = 0;
			count_all = 0;
			count_mut = variants[i].formatValueFromSample("TIR", tumor_id.toUtf8()).split(',')[0].toInt();
			count_all = variants[i].formatValueFromSample("TAR", tumor_id.toUtf8()).split(',')[0].toInt() + count_mut;
			if(count_all>0)	af_tumor = (double)count_mut/count_all;

			//TIR + TAR normal
			count_mut = 0;
			count_all = 0;
			count_mut = variants[i].formatValueFromSample("TIR", tumor_id.toUtf8()).split(',')[0].toInt();
			count_all = variants[i].formatValueFromSample("TAR", tumor_id.toUtf8()).split(',')[0].toInt() + count_mut;
			if(count_all>0)	af_normal = (double)count_mut/count_all;
		}
		//freebayes tumor and normal
		//##FORMAT=<ID=RO,Number=1,Type=Integer,Description="Reference allele observation count">
		//##FORMAT=<ID=AO,RONumber=A,Type=Integer,Description="Alternate allele observation count">
		else if(variants.vcfHeader().formatIdDefined("AO"))
		{
			count_mut = variants[i].formatValueFromSample("AO", tumor_id.toUtf8()).toInt();
			count_all = count_mut + variants[i].formatValueFromSample("RO", tumor_id.toUtf8()).toInt();
			if(count_all>0)	af_tumor = (double)count_mut/count_all;

			count_mut = variants[i].formatValueFromSample("AO", normal_id.toUtf8()).toInt();
			count_all = count_mut + variants[i].formatValueFromSample("RO", normal_id.toUtf8()).toInt();
			if(count_all>0)	af_normal = (double)count_mut/count_all;
		}
		//mutect
		//##FORMAT=<ID=FA,Number=A,Type=Float,Description="Allele fraction of the alternate allele with regard to reference">
		else if(variants.vcfHeader().formatIdDefined("FA"))
		{
			af_tumor = variants[i].formatValueFromSample("FA", tumor_id.toUtf8()).toDouble();
			af_normal = variants[i].formatValueFromSample("FA", normal_id.toUtf8()).toDouble();
		}
		//MuTect2
		//##FORMAT=<ID=AF,Number=A,Type=Float,Description="Allele fractions of alternate alleles in the tumor">
		else if(variants.vcfHeader().formatIdDefined("AF"))
		{
			af_tumor = variants[i].formatValueFromSample("AF", tumor_id.toUtf8()).toDouble();
			af_normal = variants[i].formatValueFromSample("AF", normal_id.toUtf8()).toDouble();
		}
		else
		{
			Log::error("Could not identify vcf format in line " + QString::number(i+1) + ". Sample-ID: " + tumor_id + ". Position " + variants[i].chr().str() + ":" + QString::number(variants[i].start()) + ". Only strelka, freebayes and mutect2 are currently supported.");
		}

		//find AF and set x and y points, implement freebayes and strelka fields
        std::pair<double,double> point;
		point.first = af_tumor;
		point.second = af_normal;
		if (!variants[i].filtersPassed())	points_black.append(point);
		else points_green.append(point);
	}

    QList< std::pair<double,double> > points;
	points  << points_black << points_green;

	QString g = "k";
	QString b = "g";
	colors.clear();
	for(int i=0;i<points_black.count();++i)
	{
		colors.append(g);
	}
	for(int i=0;i<points_green.count();++i)
	{
		colors.append(b);
	}

	plot1.setValues(points, colors);
	plot1.addColorLegend(g,"all variants");
	plot1.addColorLegend(b,"variants with filter PASS");

	QString plot1name = Helper::tempFileName(".png");
	plot1.store(plot1name);
	addQcPlot(output, "QC:2000048", "somatic variants allele frequencies plot", plot1name);
	QFile::remove(plot1name);

	//plot2: somatic variant signature
	BarPlot plot2;
	plot2.setXLabel("triplett");
	plot2.setYLabel("count");
	color_map = QMap<QString,QString>{{"C>A","b"},{"C>G","k"},{"C>T","r"},{"T>A","g"},{"T>G","c"},{"T>C","y"}};
	foreach(QString color, color_map)
	{
		plot2.addColorLegend(color,color_map.key(color));
	}
	QList<Sequence> codons;
	counts.clear();
	QList<double> counts_normalized;
	QList<double> frequencies;
	QList<QString> labels;
	colors = QList<QString>();
	QList<Sequence> sig = QList<Sequence>{"C","T"};
	foreach(const QByteArray& r, sig)
	{
		foreach(const QByteArray& o, nucleotides)
		{
			if(r == o)	continue;
			foreach(const QByteArray& rr, nucleotides)
			{
				foreach(const QByteArray& rrr, nucleotides)
				{
					codons.append(rr + r + rrr + " - " + o);
					counts.append(0);
					counts_normalized.append(0);
					frequencies.append(0);
					colors.append(color_map[r+">"+o]);
					labels.append(rr + r + rrr);
				}
			}
		}
	}
	//codons: count codons from variant list
	FastaFileIndex reference(ref_fasta);
	for(int i=0; i<variants.count(); ++i)
	{
		if(!variants[i].filtersPassed()) continue;	//skip non-somatic variants
		if(!variants[i].isSNV()) continue;	//skip indels

		const  VcfLine& v = variants[i];

		Sequence c = reference.seq(v.chr(),v.start()-1,1,true) + v.ref().toUpper() + reference.seq(v.chr(),v.start()+1,1,true) + " - " + v.altString().toUpper();
		bool contained = codons.contains(c);
		if(!contained)
		{
			c = Sequence(reference.seq(v.chr(),v.start()-1,1,true).toUpper() + v.ref().toUpper() + reference.seq(v.chr(),v.start()+1,1,true).toUpper()).toReverseComplement() + " - " + v.altString().toReverseComplement().toUpper();
			contained = codons.contains(c);
		}

		if(contained)
		{
			++counts[codons.indexOf(c)];
		}
	}

	//	for(int i=0; i<codons.length();++i)
	//	{
	//		qDebug() << codons[i] << QString::number(counts[i]);
	//	}

	plot2.setYLabel("variant type percentage");
	if(target_file.count() != 0)	plot2.setYLabel("normalized variant type percentage");
	QHash<Sequence, int> count_codons_target({
												 {"ACA",0},{"ACC",0},{"ACG",0},{"ACT",0},{"CCA",0},{"CCC",0},{"CCG",0},{"CCT",0},
												 {"GCA",0},{"GCC",0},{"GCG",0},{"GCT",0},{"TCA",0},{"TCC",0},{"TCG",0},{"TCT",0},
												 {"ATA",0},{"ATC",0},{"ATG",0},{"ATT",0},{"CTA",0},{"CTC",0},{"CTG",0},{"CTT",0},
												 {"GTA",0},{"GTC",0},{"GTG",0},{"GTT",0},{"TTA",0},{"TTC",0},{"TTG",0},{"TTT",0}
											 });
	if(target_file.count() == 0)
	{
		FastaFileIndex reference(ref_fasta);
		int bin = 50000000;
		foreach(const Chromosome& chr, reference.chromosomes())
		{
			if(!chr.isNonSpecial()) continue;

			int chrom_length = reference.lengthOf(chr);
			for(int j=1; j<=chrom_length; j+=bin)
			{
				int start = j;
				int length = bin;
				if(start>1)	//make bins overlap
				{
					start -= 2;
					length += 2;
				}
				if((start+length-1)>chrom_length)	length = (chrom_length - start + 1);
				Sequence seq = reference.seq(chr,start,length,true);
				foreach(const Sequence& codon, count_codons_target.keys())
				{
					count_codons_target[codon] += seq.count(codon);
					count_codons_target[codon] += seq.count(codon.toReverseComplement());
				}
			}
		}
	}
	else
	{
		//codons: count in target file
		if(target_file.baseCount()<100000)	Log::warn("Target size is less than 100 kb. Mutation signature may be imprecise.");
		for(int i=0; i<target_file.count(); ++i)
		{
			Sequence seq = reference.seq(target_file[i].chr(),target_file[i].start(),target_file[i].length(),true);
			foreach(const Sequence& codon, count_codons_target.keys())
			{
				count_codons_target[codon] += seq.count(codon);
				count_codons_target[codon] += seq.count(codon.toReverseComplement());
			}
		}
	}

	//	foreach(QString codon, count_codons_target.keys())
	//	{
	//		qDebug() << codon << QString::number(count_codons_target[codon]);
	//	}

	//codons: normalize current codons and calculate percentages for each codon
	double y_max = 5;
	double sum = 0;
	for(int i=0; i<codons.count(); ++i)
	{
		Sequence cod = codons[i].mid(0,3);
		counts_normalized[i] = counts[i]/double(count_codons_target[cod]);
		sum += counts_normalized[i];
	}
	if(sum!=0)
	{
		for(int i=0; i<counts_normalized.count(); ++i)
		{
			frequencies[i] = counts_normalized[i]/sum * 100;
			if(frequencies[i]>y_max)	y_max = frequencies[i];
		}
	}

	plot2.setXRange(-1.5,frequencies.count()+0.5);
	plot2.setYRange(-y_max*0.02,y_max*1.2);
	plot2.setValues(frequencies, labels, colors);
	QString plot2name = Helper::tempFileName(".png");
	plot2.store(plot2name);
	addQcPlot(output, "QC:2000047", "somatic variant signature plot", plot2name);
	QFile::remove(plot2name);

	//plot3: somatic variant distances, only for whole genome sequencing
	if(target_file.count() == 0)
	{
		ScatterPlot plot3;
		plot3.setXLabel("chromosomes");
		plot3.setYLabel("somatic variant distance [bp]");
		plot3.setYLogScale(true);
		//(0) generate chromosomal map
		long long genome_size = 0;
		QMap<Chromosome,long long> chrom_starts;
		QStringList fai = Helper::loadTextFile(ref_fasta + ".fai", true, '~', true);
		foreach(QString line, fai)
		{
			QStringList parts = line.split("\t");
			if (parts.count()<2) continue;
			bool ok = false;
			int value = parts[1].toInt(&ok);
			if (!ok) continue;
			Chromosome c = Chromosome(parts[0]);
			if(!c.isNonSpecial())	continue;
			chrom_starts[c] = genome_size;
			genome_size += value;
		}
		QMap<Chromosome,double> chrom_starts_norm;
		foreach(Chromosome c, chrom_starts.keys())
		{
			double offset = double(chrom_starts[c])/double(genome_size);
			chrom_starts_norm[c] = offset;
		}
		//(1) add chromosome lines
		foreach(Chromosome c, chrom_starts.keys())
		{
			if(chrom_starts_norm[c]==0)	continue;
			plot3.addVLine(chrom_starts_norm[c]);
		}
		//(2) calculate distance for each chromosome and convert it to x-coordinates
        QList< std::pair<double,double> > points3;
		QString tmp_chr = "";
		int tmp_pos = 0;
		double tmp_offset = 0;
		double max = 0;
		for(int i=0; i<variants.count(); ++i)	//list has to be sorted by chrom. position
		{
			if(!variants[i].chr().isNonSpecial()) continue;
			if(!variants[i].filtersPassed()) continue;	//skip non-somatic variants

			if(tmp_chr == variants[i].chr().str())	//same chromosome
			{
				//convert distance to x-Axis position
                std::pair<double,double> point;
				point.first = tmp_offset + double(variants[i].start())/double(genome_size);
				point.second = variants[i].start() - tmp_pos;
				if(max < point.second)	max = point.second;
				points3.append(point);
			}

			if((tmp_chr != variants[i].chr().str()) && i>0)	//if a different chromosome is found => udpate offset
			{
				if(!chrom_starts_norm.contains(variants[i].chr()))	continue; //skip invalid chromosomes
				tmp_pos = 0;
				tmp_offset = chrom_starts_norm[tmp_chr];
			}

			tmp_chr = variants[i].chr().str();
			tmp_pos = variants[i].start();
		}

		plot3.setYRange(0.975,max*100);
		plot3.setXRange(0,1);
		plot3.noXTicks();
		plot3.setValues(points3);
		QString plot3name = Helper::tempFileName(".png");
		plot3.store(plot3name);
		addQcPlot(output, "QC:2000046","somatic variant distance plot", plot3name);
		QFile::remove(plot3name);
	}

	return output;
}

QCCollection Statistics::contamination(GenomeBuild build, QString bam, const QString& ref_file, bool debug, int min_cov, int min_snps, bool include_not_properly_paired)
{
	//open BAM
	BamReader reader(bam, ref_file);

	//calcualate frequency histogram
	Histogram hist(0, 1, 0.05);
	int passed = 0;
	double passed_depth_sum = 0.0;
	VcfFile snps = NGSHelper::getKnownVariants(build, true, 0.2, 0.8);
	for(int i=0; i<snps.count(); ++i)
	{
		Pileup pileup = reader.getPileup(snps[i].chr(), snps[i].start(), -1, 1, include_not_properly_paired);
		int depth = pileup.depth(false);
		if (depth<min_cov) continue;

		double freq = pileup.frequency(snps[i].ref()[0], snps[i].alt(0)[0]);

		//skip non-informative snps
		if (!BasicStatistics::isValidFloat(freq)) continue;

		++passed;
		passed_depth_sum += depth;

		hist.inc(freq);
	}

	//debug output
	if (debug)
	{
		QTextStream stream(stdout);
		stream << "Contamination debug output:\n";
		stream << passed << " of " << snps.count() << " SNPs passed quality filters\n";
		stream << "Average depth of passed SNPs: " << QString::number(passed_depth_sum/passed,'f', 2) << "\n";
		stream << "\nAF histogram:\n";
		hist.print(stream, "", 2, 0);
	}

	//output
	double off = 0.0;
	for (int i=1; i<=5; ++i) off += hist.binValue(i, true);
	for (int i=14; i<=18; ++i) off += hist.binValue(i, true);
	QCCollection output;
	QString value = (passed < min_snps) ? "n/a" : QString::number(off, 'f', 2);
	addQcValue(output, "QC:2000051", "SNV allele frequency deviation", value);
	return output;
}

AncestryEstimates Statistics::ancestry(GenomeBuild build, QString filename, int min_snp, double abs_score_cutoff, double max_mad_dist)
{
	//init score statistics
	struct PopScore
	{
		double median;
		double mad;
	};
	static QMap<QString, QMap<QString, PopScore>> scores;
	if (scores.isEmpty())
	{
		scores["AFR"]["AFR"] = {0.5002, 0.0291};
		scores["AFR"]["EUR"] = {0.0553, 0.0280};
		scores["AFR"]["SAS"] = {0.1061, 0.0267};
		scores["AFR"]["EAS"] = {0.0895, 0.0274};
		scores["EUR"]["AFR"] = {0.0727, 0.0271};
		scores["EUR"]["EUR"] = {0.3251, 0.0252};
		scores["EUR"]["SAS"] = {0.1922, 0.0249};
		scores["EUR"]["EAS"] = {0.0603, 0.0264};
		scores["SAS"]["AFR"] = {0.0698, 0.0264};
		scores["SAS"]["EUR"] = {0.1574, 0.0295};
		scores["SAS"]["SAS"] = {0.3395, 0.0291};
		scores["SAS"]["EAS"] = {0.1693, 0.0288};
		scores["EAS"]["AFR"] = {0.08415, 0.0275};
		scores["EAS"]["EUR"] = {0.06725, 0.0269};
		scores["EAS"]["SAS"] = {0.21495, 0.0228};
		scores["EAS"]["EAS"] = {0.47035, 0.0242};
	}

	//copy ancestry SNP file from resources (gzopen cannot access Qt resources)
	QString snp_file = ":/Resources/" + buildToString(build) + "_ancestry.vcf";
	if (!QFile::exists(snp_file)) THROW(ProgrammingException, "Unsupported genome build '" + buildToString(build) + "' for ancestry estimation!");
	QString tmp = Helper::tempFileNameNonRandom(buildToString(build) + "_ancestry.vcf");
	QFile::copy(snp_file, tmp);

	//load ancestry SNP file
	VcfFile vars_ancestry;
	vars_ancestry.load(tmp);
	ChromosomalIndex<VcfFile> vars_ancestry_idx(vars_ancestry);

	//remove temporary file
	QFile::remove(tmp);

	//create ROI to speed up loading the sample file, e.g. for genomes.
	BedFile roi;
	for(int i=0; i<vars_ancestry.count(); ++i)
	{
		const VcfLine& var = vars_ancestry[i];
		roi.append(BedLine(var.chr(), var.start(), var.end()));
	}
	roi.merge(true);

	//load relevant variants from VCF
	VcfFile vl;
	vl.setRegion(roi);
	vl.load(filename);

	//multi-sample VCF is not supported
	if(vl.sampleIDs().count()!=1)
	{
		THROW(ArgumentException, "Only single-sample VCFs are supported for ancestry estimation!");
	}

	//determine required annotation indices
	if(!vl.vcfHeader().formatIdDefined("GT"))
	{
		THROW(ArgumentException, "VCF file does not contain FORMAT entry 'GT', which is required for ancestry estimation!")
	}

	//process variants
	QVector<double> geno_sample;
	QVector<double> af_afr;
	QVector<double> af_eur;
	QVector<double> af_sas;
	QVector<double> af_eas;
	for(int i=0; i<vl.count(); ++i)
	{
		const VcfLine& v = vl[i];

		//skip non-informative SNPs
		int index = vars_ancestry_idx.matchingIndex(v.chr(), v.start(), v.end());
		if (index==-1) continue;

		const VcfLine& v2 = vars_ancestry[index];
		if (v.ref()!=v2.ref() || v.alt()!=v2.alt()) continue;

		//genotype sample
		geno_sample << vl[i].formatValueFromSample("GT").count('1');

		//population AFs
		af_afr << v2.info("AF_AFR").toDouble();
		af_eur << v2.info("AF_EUR").toDouble();
		af_sas << v2.info("AF_SAS").toDouble();
		af_eas << v2.info("AF_EAS").toDouble();
	}

	//not enough informative SNPs
	AncestryEstimates output;
	output.snps = geno_sample.count();
	if (geno_sample.count()<min_snp)
	{
		output.afr = std::numeric_limits<double>::quiet_NaN();
		output.eur = std::numeric_limits<double>::quiet_NaN();
		output.sas = std::numeric_limits<double>::quiet_NaN();
		output.eas = std::numeric_limits<double>::quiet_NaN();
		output.population = "NOT_ENOUGH_SNPS";
		return output;
	}

	//compose output
	output.afr = BasicStatistics::correlation(geno_sample, af_afr);
	if (output.afr<0) output.afr = 0.0;
	output.eur = BasicStatistics::correlation(geno_sample, af_eur);
	if (output.eur<0) output.eur = 0.0;
	output.sas = BasicStatistics::correlation(geno_sample, af_sas);
	if (output.sas<0) output.sas = 0.0;
	output.eas = BasicStatistics::correlation(geno_sample, af_eas);
	if (output.eas<0) output.eas = 0.0;

	//determine population
	QSet<QString> pop_matches;
	if (output.afr>=abs_score_cutoff) pop_matches << "AFR";
	if (output.eur>=abs_score_cutoff) pop_matches << "EUR";
	if (output.sas>=abs_score_cutoff) pop_matches << "SAS";
	if (output.eas>=abs_score_cutoff) pop_matches << "EAS";
	foreach(const QString& pop, scores.keys())
	{
		bool in_dist = true;
		if (fabs((output.afr-scores[pop]["AFR"].median)/scores[pop]["AFR"].mad)>max_mad_dist) in_dist = false;
		if (fabs((output.eur-scores[pop]["EUR"].median)/scores[pop]["EUR"].mad)>max_mad_dist) in_dist = false;
		if (fabs((output.sas-scores[pop]["SAS"].median)/scores[pop]["SAS"].mad)>max_mad_dist) in_dist = false;
		if (fabs((output.eas-scores[pop]["EAS"].median)/scores[pop]["EAS"].mad)>max_mad_dist) in_dist = false;
		if (in_dist) pop_matches << pop;
	}
	if (pop_matches.count()!=1)
	{
		output.population = "ADMIXED/UNKNOWN";
	}
	else
	{
		output.population = *(pop_matches.begin());
	}

	return output;
}

BedFile Statistics::lowOrHighCoverage(const BedFile& bed_file, const QString& bam_file, int cutoff, int min_mapq, int min_baseq, int threads, const QString& ref_file, bool is_high, bool random_access, bool debug)
{
	//check BED is sorted for WGS mode
	if (!random_access && !bed_file.isSorted()) THROW(ArgumentException, "Input BED file has to be sorted for sweep algorithm!");

	//create analysis chunks (200 lines)
    QElapsedTimer timer;
	timer.start();
	QList<WorkerLowOrHighCoverage::Chunk> bed_chunks;
	if (!random_access)
	{
		//determine chr chunks
        QList<std::pair<long, WorkerLowOrHighCoverage::Chunk>> chunks_with_size;
		foreach(const Chromosome& chr, bed_file.chromosomes())
		{
			//determine start index
			int start = -1;
			for (int i=0; i<bed_file.count(); ++i)
			{
				if (bed_file[i].chr()==chr)
				{
					start = i;
					break;
				}
			}

			//determine end index
			int end = -1;
			for (int i=bed_file.count()-1; i>=0; --i)
			{
				if (bed_file[i].chr()==chr)
				{
					end = i;
					break;
				}
			}

			//deterine base count of chunks
			long bases = 0;
			for (int i=start; i<=end; ++i)
			{
				bases += bed_file[i].length();
			}
            chunks_with_size << std::make_pair(bases,  WorkerLowOrHighCoverage::Chunk{bed_file, start, end, "", BedFile()});
		}

		//sort chunks by size
		std::sort(chunks_with_size.begin(), chunks_with_size.end(),
            [](const std::pair<long, WorkerLowOrHighCoverage::Chunk>& a, const std::pair<long, WorkerLowOrHighCoverage::Chunk>& b)
			{
				return a.first > b.first;
			}
		);

		//add chunks ordered by size
		foreach(const auto& entry, chunks_with_size)
		{
            bed_chunks.append(entry.second);
		}
	}
	else
	{
		for (int start=0; start<bed_file.count(); start += 200)
		{
			int end = start+199;
			if (end>=bed_file.count()) end = bed_file.count() -1;
			bed_chunks << WorkerLowOrHighCoverage::Chunk{bed_file, start, end, QString(), BedFile{}};
		}
	}

	//debug output
	if (debug)
	{
		QTextStream out(stdout);
        out << "Using '" << (random_access ? "random access" : "sweep") << "' algorithm!" << QT_ENDL;
        out << "Creating " << bed_chunks.count() << " chunks took " << Helper::elapsedTime(timer) << QT_ENDL;
        out << "Starting processing chunks with " << threads << " threads" << QT_ENDL;
	}

	//create thread pool
	QThreadPool thread_pool;
	thread_pool.setMaxThreadCount(threads);

	//start analysis chunks
	for (int i=0; i<bed_chunks.count(); ++i)
	{
		if (!random_access)
		{
            if (debug) QTextStream(stdout) << "Creating BED index" << QT_ENDL;
			ChromosomalIndex<BedFile> bed_index(bed_file);

            if (debug) QTextStream(stdout) << "Starting worker " << i << QT_ENDL;
			WorkerLowOrHighCoverageChr* worker = new WorkerLowOrHighCoverageChr(bed_chunks[i], bed_index, bam_file, cutoff, min_mapq, min_baseq, ref_file, is_high, debug);
			thread_pool.start(worker);

			//wait until finished
            if (debug) QTextStream(stdout) << "Waiting for workers to finish..." << QT_ENDL;
			thread_pool.waitForDone();
		}
		else
		{
			WorkerLowOrHighCoverage* worker = new WorkerLowOrHighCoverage(bed_chunks[i], bam_file, cutoff, min_mapq, min_baseq, ref_file, is_high, debug);
			thread_pool.start(worker);

			//wait until finished
            if (debug) QTextStream(stdout) << "Waiting for workers to finish..." << QT_ENDL;
			thread_pool.waitForDone();
		}
	}

	//debug output
    if (debug) QTextStream(stdout) << "Writing output" << QT_ENDL;

	//check for errors and merge results
	BedFile output;
	foreach(const WorkerLowOrHighCoverage::Chunk& bed_chunk, bed_chunks)
	{
		if (!bed_chunk.error.isEmpty()) THROW(Exception, bed_chunk.error);

		for (int l=0; l<bed_chunk.output.count(); ++l)
		{
			output.append(bed_chunk.output[l]);
		}
	}

	output.merge(true, true, true);
	return output;
}

double Statistics::yxRatio(BamReader& reader, double* count_x, double* count_y)
{
	// return NaN if chrX or chrY does not exist
	if (!reader.chromosomes().contains(Chromosome("chrX")) ||
		!reader.chromosomes().contains(Chromosome("chrY")))
	{
		return std::numeric_limits<double>::quiet_NaN();
	}
	//reads chrY
	double reads_y = 0;
	BamAlignment al;
	reader.setRegion("chrY", 1, reader.chromosomeSize("chrY"));
	while(reader.getNextAlignment(al))
	{
		if (al.isSecondaryAlignment() || al.isSupplementaryAlignment()) continue;
		if (al.mappingQuality()<30) continue;
		reads_y += 1.0;
	}
	if (count_y!=nullptr) *count_y = reads_y;

	//reads chrX
	double reads_x = 0;
	reader.setRegion("chrX", 1, reader.chromosomeSize("chrX"));
	while(reader.getNextAlignment(al))
	{
		if (al.isSecondaryAlignment() || al.isSupplementaryAlignment()) continue;
		if (al.mappingQuality()<30) continue;
		reads_x += 1.0;
	}
	if (count_x!=nullptr) *count_x = reads_x;

	if (reads_x==0) return std::numeric_limits<double>::quiet_NaN();

	return reads_y / reads_x;
}

BedFile Statistics::lowCoverage(const BedFile& bed_file, const QString& bam_file, int cutoff, int min_mapq, int min_baseq, int threads, const QString& ref_file, bool random_access, bool debug)
{
	return lowOrHighCoverage(bed_file, bam_file, cutoff, min_mapq, min_baseq, threads, ref_file, false, random_access, debug);
}

void Statistics::avgCoverage(BedFile& bed_file, const QString& bam_file, int min_mapq, int threads, int decimals, const QString& ref_file, bool random_access, bool skip_mismapped, bool debug)
{
	//check BED is sorted for chromosomal sweep algorithm
	if (!random_access && !bed_file.isSorted()) THROW(ArgumentException, "Input BED file has to be sorted for sweep algorithm!");

	//create analysis chunks
    QElapsedTimer timer;
	timer.start();
	QList<WorkerAverageCoverage::Chunk> chunks;
	if (!random_access)
	{
		//determine chr chunks
        QList<std::pair<long, WorkerAverageCoverage::Chunk>> chunks_with_size;
		foreach(const Chromosome& chr, bed_file.chromosomes())
		{
			//determine start index
			int start = -1;
			for (int i=0; i<bed_file.count(); ++i)
			{
				if (bed_file[i].chr()==chr)
				{
					start = i;
					break;
				}
			}

			//determine end index
			int end = -1;
			for (int i=bed_file.count()-1; i>=0; --i)
			{
				if (bed_file[i].chr()==chr)
				{
					end = i;
					break;
				}
			}

			//deterine base count of chunks
			long bases = 0;
			for (int i=start; i<=end; ++i)
			{
				bases += bed_file[i].length();
			}
            chunks_with_size << std::make_pair(bases,  WorkerAverageCoverage::Chunk{bed_file, start, end, ""});
		}

		//sort chunks by size
		std::sort(chunks_with_size.begin(), chunks_with_size.end(),
            [](const std::pair<long, WorkerAverageCoverage::Chunk>& a, const std::pair<long, WorkerAverageCoverage::Chunk>& b)
			{
				return a.first > b.first;
			}
		);

		//add chunks ordered by size
		foreach(const auto& entry, chunks_with_size)
		{
            chunks.append(entry.second);
		}
	}
	else
	{
		const int chunk_size = 200;
		for (int start=0; start<bed_file.count(); start += chunk_size)
		{
			int end = start+chunk_size-1;
			if (end>=bed_file.count()) end = bed_file.count() -1;
			chunks << WorkerAverageCoverage::Chunk{bed_file, start, end, ""};
		}
	}

	//debug output
	if (debug)
	{
		QTextStream out(stdout);
        out << "Using '" << (random_access ? "random access" : "sweep") << "' algorithm!" << QT_ENDL;
        out << "Creating " << chunks.count() << " chunks took " << Helper::elapsedTime(timer) << QT_ENDL;
	}

	//create thread pool
	QThreadPool thread_pool;
	thread_pool.setMaxThreadCount(threads);

	//start analysis chunks
	for (int i=0; i<chunks.count(); ++i)
	{
		if (!random_access)
		{
			WorkerAverageCoverageChr* worker = new WorkerAverageCoverageChr(chunks[i], bam_file, min_mapq, decimals, ref_file, skip_mismapped, debug);
			thread_pool.start(worker);
		}
		else
		{
			WorkerAverageCoverage* worker = new WorkerAverageCoverage(chunks[i], bam_file, min_mapq, decimals, ref_file, skip_mismapped, debug);
			thread_pool.start(worker);
		}
	}

	//wait until finished
	thread_pool.waitForDone();

	//check if error occured
	foreach(const WorkerAverageCoverage::Chunk& chunk, chunks)
	{
		if (!chunk.error.isEmpty()) THROW(Exception, chunk.error);
	}
}

BedFile Statistics::highCoverage(const BedFile& bed_file, const QString& bam_file, int cutoff, int min_mapq, int min_baseq, int threads, const QString& ref_file, bool random_access, bool debug)
{
	return lowOrHighCoverage(bed_file, bam_file, cutoff, min_mapq, min_baseq, threads, ref_file, true, random_access, debug);
}

GenderEstimate Statistics::genderXY(QString bam_file, double max_female, double min_male, const QString& ref_file)
{
	//open BAM file
	BamReader reader(bam_file, ref_file);
	reader.skipBases();
	reader.skipQualities();
	reader.skipTags();
	double count_x = 0.0;
	double count_y = 0.0;
	double ratio_yx = Statistics::yxRatio(reader, &count_x, &count_y);

	//output
	GenderEstimate output;
	output.add_info << KeyValuePair("reads_chry", QString::number(count_y, 'f', 0));
	output.add_info << KeyValuePair("reads_chrx", QString::number(count_x, 'f', 0));
	output.add_info << KeyValuePair("ratio_chry_chrx", QString::number(ratio_yx, 'f', 4));

	//output
	if (ratio_yx<=max_female) output.gender = "female";
	else if (ratio_yx>=min_male) output.gender = "male";
	else output.gender = "unknown (ratio in gray area)";

	return output;
}

GenderEstimate Statistics::genderHetX(GenomeBuild build, QString bam_file, double max_male, double min_female, const QString& ref_file, bool include_not_properly_paired)
{
	//open BAM file
	BamReader reader(bam_file, ref_file);

	//load common SNPs on chrX that are outside the PAR
	Chromosome chrx("chrX");
	int chrx_end_pos = reader.chromosomeSize(chrx);
	BedFile roi_chrx(chrx, 1, chrx_end_pos);
	roi_chrx.subtract(NGSHelper::pseudoAutosomalRegion(build));
	VcfFile snps = NGSHelper::getKnownVariants(build, true, roi_chrx, 0.2, 0.8);

	//count het SNPs
	int c_all = 0;
	int c_het = 0;
	QVector<Pileup> counts;
	counts.reserve(snps.count());
	for (int i=0; i<snps.count(); ++i)
	{
		const VcfLine& snp = snps[i];
		Pileup pileup = reader.getPileup(snp.chr(), snp.start(), -1, 20, include_not_properly_paired, 20);

		int depth = pileup.depth(false);
		if (depth<20) continue;

		double af = pileup.frequency(snp.ref()[0], snp.alt()[0][0]);
		if (!BasicStatistics::isValidFloat(af)) continue;

		++c_all;
		if (af>0.1 && af<0.9) ++c_het;
	}

	double het_frac = (double) c_het / c_all;

	//output
	GenderEstimate output;
	output.add_info << KeyValuePair("snps_usable", QString::number(c_all) + " of " + QString::number(snps.count()));
	output.add_info << KeyValuePair("hom_count", QString::number(c_all - c_het));
	output.add_info << KeyValuePair("het_count", QString::number(c_het));
	output.add_info << KeyValuePair("het_fraction", QString::number(het_frac, 'f', 4));

	if (c_all<20) output.gender = "unknown (too few SNPs)";
	else if (het_frac<=max_male) output.gender = "male";
	else if (het_frac>=min_female) output.gender = "female";
	else output.gender = "unknown (fraction in gray area)";

	return output;
}

GenderEstimate Statistics::genderSRY(GenomeBuild build, QString bam_file, double min_cov, const QString& ref_file)
{
	//construct ROI
	int start = build==GenomeBuild::HG38 ? 2786989 : 2655031;
	int end = build==GenomeBuild::HG38 ? 2787603 : 2655641;
	BedFile roi;
	roi.append(BedLine("chrY", start, end));

	//calculate coverage
	Statistics::avgCoverage(roi, bam_file, 1, 1, 2 , ref_file);
	double cov =  roi[0].annotations()[0].toDouble();

	//output
	GenderEstimate output;
	output.add_info << KeyValuePair("coverage_sry", QString::number(cov, 'f', 2));
	output.gender = cov>=min_cov ? "male" : "female";
	return output;
}

template<typename T>
void Statistics::addQcValue(QCCollection& output, QByteArray accession, QByteArray name, const T& value)
{
	//load qcML terms
	static OntologyTermCollection terms("://Resources/qcML.obo", false);

	//check
	if (!terms.containsByID(accession))
	{
		THROW(ProgrammingException, "qcML does not contain term with accession '" + accession + "'!");
	}
	const OntologyTerm& term = terms.getByID(accession);
	if (term.name()!=name)
	{
		THROW(ProgrammingException, "qcML term with accession '" + accession + "' does not have name '" + name + "'!");
	}

	output.insert(QCValue(name, value, term.definition(), accession));
}

void Statistics::addQcPlot(QCCollection& output, QByteArray accession, QByteArray name, QString filename)
{
	//load qcML terms
	static OntologyTermCollection terms("://Resources/qcML.obo", false);

	//check
	if (!terms.containsByID(accession))
	{
		THROW(ProgrammingException, "qcML does not contain term with accession '" + accession + "'!");
	}
	const OntologyTerm& term = terms.getByID(accession);
	if (term.name()!=name)
	{
		THROW(ProgrammingException, "qcML term with accession '" + accession + "' does not have name '" + name + "'!");
	}

	output.insert(QCValue::ImageFromFile(name, filename, term.definition(), accession));
}
