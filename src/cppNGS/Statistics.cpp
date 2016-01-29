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
#include "NGSHelper.h"
#include "FastqFileStream.h"
#include "LinePlot.h"
#include "Helper.h"
#include "ChromosomeInfo.h"

#include "api/BamReader.h"
using namespace BamTools;

QCCollection Statistics::variantList(const VariantList& variants)
{
    QCCollection output;

    //var_total
	output.insert(QCValue("variant count", variants.count(), "Total number of variants in the target region.", "QC:2000013"));

    //var_perc_dbsnp
	int index = variants.annotationIndexByName("ID", true, false);
    if (variants.count()!=0 && index!=-1)
    {
        double dbsnp_count = 0;
        for(int i=0; i<variants.count(); ++i)
		{
			if (variants[i].annotations().at(index).startsWith("rs"))
            {
                ++dbsnp_count;
            }
        }
		output.insert(QCValue("known variants percentage", 100.0*dbsnp_count/variants.count(), "Percentage of variants that are known polymorphisms in the dbSNP database.", "QC:2000014"));
    }
    else
    {
		output.insert(QCValue("known variants percentage", "n/a (no variants)", "Percentage of variants that are known polymorphisms in the dbSNP database.", "QC:2000014"));
    }

	//high-impact variants
	index = variants.annotationIndexByName("ANN", true, false);
    if (variants.count()!=0 && index!=-1)
    {
		double high_impact_count = 0;
        for(int i=0; i<variants.count(); ++i)
		{
			if (variants[i].annotations().at(index).contains("|HIGH|"))
            {
				++high_impact_count;
            }
        }
		output.insert(QCValue("high-impact variants percentage", 100.0*high_impact_count/variants.count(), "Percentage of variants with high impact on the protein, i.e. stop-gain, stop-loss, frameshift, splice-acceptor or splice-donor variants.", "QC:2000015"));
    }
    else
    {
		output.insert(QCValue("high-impact variants percentage", "n/a (SnpEff ANN annotation not found, or no variants)", "Percentage of variants with high impact on the protein, i.e. stop-gain, stop-loss, frameshift, splice-acceptor or splice-donor variants.", "QC:2000015"));
    }

	//homozygous variants
	index = variants.annotationIndexByName("GT", true, false);
    if (variants.count()!=0 && index!=-1)
    {
        double hom_count = 0;
        for(int i=0; i<variants.count(); ++i)
        {
			QString geno = variants[i].annotations().at(index);
			if (geno=="1/1" || geno=="1|1")
            {
                ++hom_count;
            }
        }
		output.insert(QCValue("homozygous variants percentage", 100.0*hom_count/variants.count(), "Percentage of variants that are called as homozygous.", "QC:2000016"));
    }
    else
    {
		output.insert(QCValue("homozygous variants percentage", "n/a (GT annotation not found, or no variants)", "Percentage of variants that are called as homozygous.", "QC:2000016"));
    }

    //var_perc_indel / var_ti_tv_ratio
    double indel_count = 0;
    double ti_count = 0;
    double tv_count = 0;
    for(int i=0; i<variants.count(); ++i)
    {
        const Variant& var = variants[i];
		if (var.ref().length()>1 || var.obs().length()>1)
        {
            ++indel_count;
        }
        else if ((var.obs()=="A" && var.ref()=="G") || (var.obs()=="G" && var.ref()=="A") || (var.obs()=="T" && var.ref()=="C") || (var.obs()=="C" && var.ref()=="T"))
        {
            ++ti_count;
        }
        else
        {
            ++tv_count;
        }
    }

    if (variants.count()!=0)
    {
		output.insert(QCValue("indel variants percentage", 100.0*indel_count/variants.count(), "Percentage of variants that are insertions/deletions.", "QC:2000017"));
    }
    else
    {
		output.insert(QCValue("indel variants percentage", "n/a (no variants)", "Percentage of variants that are insertions/deletions.", "QC:2000017"));
    }

    if (tv_count!=0)
    {
		output.insert(QCValue("transition/transversion ratio", ti_count/tv_count , "Transition/transversion ratio of SNV variants.", "QC:2000018"));
    }
    else
    {
		output.insert(QCValue("transition/transversion ratio", "n/a (no variants or tansversions)", "Transition/transversion ratio of SNV variants.", "QC:2000018"));
    }

    return output;
}

QCCollection Statistics::mapping(const BedFile& bed_file, const QString& bam_file, int min_mapq)
{
    //check target region is merged/sorted and create index
    if (!bed_file.isMergedAndSorted())
    {
        THROW(ArgumentException, "Merged and sorted BED file required for coverage details statistics!");
    }
    ChromosomalIndex<BedFile> roi_index(bed_file);

    //open BAM file
    BamReader reader;
    NGSHelper::openBAM(reader, bam_file);
	ChromosomeInfo chr_info(reader);

    //create coverage statistics data structure
    long roi_bases = 0;
    QHash<int, QMap<int, int> > roi_cov;
    for (int i=0; i<bed_file.count(); ++i)
    {
        const BedLine& line = bed_file[i];

		if (!roi_cov.contains(line.chr().num()))
        {
			roi_cov.insert(line.chr().num(), QMap<int, int>());
        }

        for(int p=line.start(); p<=line.end(); ++p)
        {
			roi_cov[line.chr().num()].insert(p, 0);
        }
        roi_bases += line.length();
    }

    //init counts
    int al_total = 0;
    int al_mapped = 0;
    int al_ontarget = 0;
    int al_dup = 0;
    int al_proper_paired = 0;
    double bases_trimmed = 0;
	double insert_size_sum = 0;
	QVector<double> insert_dist;
    double bases_overlap_roi = 0;
    int max_length = 0;
    bool paired_end = false;

    //iterate through all alignments
    BamAlignment al;
    while (reader.GetNextAlignmentCore(al))
    {
        ++al_total;
        max_length = std::max(max_length, al.Length);

		if (al.IsPaired() && al.IsPrimaryAlignment())
        {
            paired_end = true;

            if (al.IsProperPair())
            {
                ++al_proper_paired;
				int insert_size = std::min(abs(al.InsertSize), 999); //cap insert size at 1000
				insert_size_sum += insert_size;
				int bin = insert_size/5;
				if (insert_dist.count()<=bin) insert_dist.resize(bin+1);
				insert_dist[bin] += 1;
            }
        }

        if (al.IsMapped())
        {
            ++al_mapped;

			const Chromosome& chr = chr_info.chromosome(al.RefID);
            int end_position = al.GetEndPosition();
            QVector<int> indices = roi_index.matchingIndices(chr, al.Position+1, end_position);
            if (indices.count()!=0)
            {
                ++al_ontarget;

				if (al.IsPrimaryAlignment() && !al.IsDuplicate() && al.MapQuality>=min_mapq)
                {
                    foreach(int index, indices)
                    {
                        int ol_start = std::max(bed_file[index].start(), al.Position+1);
                        int ol_end = std::min(bed_file[index].end(), end_position);
                        bases_overlap_roi += ol_end - ol_start + 1;
                        QMap<int, int>& roi_cov_chr_map = roi_cov[chr.num()];
                        for (int p=ol_start; p<=ol_end; ++p)
                        {
                            roi_cov_chr_map[p] += 1;
                        }
                    }
                }
            }
        }

        //trimmed bases (this is not entirely correct if the first alignments are all trimmed, but saves the second pass through the data)
        if (al.Length<max_length)
        {
            bases_trimmed += (max_length - al.Length);
        }

        if (al.IsDuplicate())
        {
            ++al_dup;
        }
    }
    reader.Close();

    //calculate coverage depth statistics
	QVector<double> depth_dist;
    QHashIterator<int, QMap<int, int> > it(roi_cov);
    while(it.hasNext())
    {
		it.next();
		QMapIterator<int, int> it2(it.value());
        while(it2.hasNext())
        {
			it2.next();
			int bin = std::min(it2.value()/5, 399); //upper bound of plot at 2000x
			if (depth_dist.count()<=bin) depth_dist.resize(bin+1);
			depth_dist[bin] += 1.0;
        }
	}

    //output
    QCCollection output;
	output.insert(QCValue("trimmed base percentage", 100.0 * bases_trimmed / al_total / max_length, "Percentage of bases that were trimmed during to adapter or quality trimming.", "QC:2000019"));
	output.insert(QCValue("mapped read percentage", 100.0 * al_mapped / al_total, "Percentage of reads that could be mapped to the reference genome.", "QC:2000020"));
	output.insert(QCValue("on-target read percentage", 100.0 * al_ontarget / al_total, "Percentage of reads that could be mapped to the target region.", "QC:2000021"));
    if (paired_end)
    {
		output.insert(QCValue("properly-paired read percentage", 100.0 * al_proper_paired / al_total, "Percentage of properly paired reads (for paired-end reads only).", "QC:2000022"));
		output.insert(QCValue("insert size", insert_size_sum / al_proper_paired, "Mean insert size (for paired-end reads only).", "QC:2000023"));
    }
    else
    {
		output.insert(QCValue("properly-paired read percentage", "n/a (single end)", "Percentage of properly paired reads (for paired-end reads only).", "QC:2000022"));
		output.insert(QCValue("insert size", "n/a (single end)", "Mean insert size (for paired-end reads only).", "QC:2000023"));
    }
    if (al_dup==0)
    {
		output.insert(QCValue("duplicate read percentage", "n/a (no duplicates marked or duplicates removed during data analysis)", "Percentage of reads removed because they were duplicates (PCR, optical, etc).", "QC:2000024"));
    }
    else
    {
		output.insert(QCValue("duplicate read percentage", 100.0 * al_dup / al_total, "Percentage of reads removed because they were duplicates (PCR, optical, etc)", "QC:2000024"));
    }
	output.insert(QCValue("target region read depth", bases_overlap_roi / roi_bases, "Average sequencing depth in target region.", "QC:2000025"));

    QVector<int> depths;
    depths << 10 << 20 << 30 << 50 << 100 << 200 << 500;
    QVector<QString> accessions;
	accessions << "QC:2000026" << "QC:2000027" << "QC:2000028" << "QC:2000029" << "QC:2000030" << "QC:2000031" << "QC:2000032";
    for (int i=0; i<depths.count(); ++i)
    {
		double cov_bases = 0.0;
		for (int j=depths[i]/5; j<depth_dist.count(); ++j) cov_bases += depth_dist[j];
		output.insert(QCValue("target region " + QString::number(depths[i]) + "x percentage", 100.0 * cov_bases / roi_bases, "Percentage of the target region that is covered at least " + QString::number(depths[i]) + "-fold.", accessions[i]));
    }

	//add depth distribtion plot
	LinePlot plot;
	plot.setXLabel("depth of coverage");
	plot.setYLabel("target region [%]");
	plot.setXValues(BasicStatistics::range(depth_dist.count(), 2.0, 5.0));
	for (int i=0; i<depth_dist.count(); ++i)
	{
		depth_dist[i] = 100.0 * depth_dist[i] / roi_bases;
	}
	plot.addLine(depth_dist, "");
	QString plotname = Helper::tempFileName(".png");
	plot.store(plotname);
	output.insert(QCValue::Image("depth distribution plot", plotname, "Depth of coverage distribution plot calculated one the target region.", "QC:2000037"));
	QFile::remove(plotname);

	//add insert size distribution plot
	if (paired_end)
	{
		LinePlot plot2;
		plot2.setXLabel("insert size");
		plot2.setYLabel("reads [%]");
		plot2.setXValues(BasicStatistics::range(insert_dist.count(), 2.0, 5.0));
		double insert_count = std::accumulate(insert_dist.begin(), insert_dist.end(), 0.0);
		for (int i=0; i<insert_dist.count(); ++i)
		{
			insert_dist[i] = 100.0 * insert_dist[i] / insert_count;
		}
		plot2.addLine(insert_dist, "");

		plotname = Helper::tempFileName(".png");
		plot2.store(plotname);
		output.insert(QCValue::Image("insert size distribution plot", plotname, "Insert size distribution plot.", "QC:2000038"));
		QFile::remove(plotname);
	}

    return output;
}

QCCollection Statistics::mapping(const QString &bam_file, int min_mapq)
{
    //open BAM file
    BamReader reader;
    NGSHelper::openBAM(reader, bam_file);
	ChromosomeInfo chr_info(reader);

    //init counts
    int al_total = 0;
    int al_mapped = 0;
    int al_ontarget = 0;
    int al_dup = 0;
    int al_proper_paired = 0;
    double bases_trimmed = 0;
	double insert_size_sum = 0;
	QVector<double> insert_dist;
    double bases_overlap_roi = 0;
    int max_length = 0;
    bool paired_end = false;

    //iterate through all alignments
    BamAlignment al;
    while (reader.GetNextAlignmentCore(al))
    {
        ++al_total;
        max_length = std::max(max_length, al.Length);

		if (al.IsPaired() && al.IsPrimaryAlignment())
        {
            paired_end = true;

            if (al.IsProperPair())
            {
				++al_proper_paired;
				int insert_size = std::min(abs(al.InsertSize),  999); //cap insert size at 1000
				insert_size_sum += insert_size;
				int bin = insert_size/5;
				if (insert_dist.count()<=bin) insert_dist.resize(bin+1);
				insert_dist[bin] += 1;
            }
        }

        if (al.IsMapped())
        {
            ++al_mapped;

			if (chr_info.chromosome(al.RefID).isNonSpecial())
            {
                ++al_ontarget;

				if (al.IsPrimaryAlignment() && !al.IsDuplicate() && al.MapQuality>=min_mapq)
				{
                    bases_overlap_roi += al.Length;
                }
            }
        }

        //trimmed bases (this is not entirely correct if the first alignments are all trimmed, but saves the second pass through the data)
        if (al.Length<max_length)
        {
            bases_trimmed += (max_length - al.Length);
        }

        if (al.IsDuplicate())
        {
            ++al_dup;
        }
    }
    reader.Close();

    //output
    QCCollection output;
	output.insert(QCValue("trimmed base percentage", 100.0 * bases_trimmed / al_total / max_length, "Percentage of bases that were trimmed during to adapter or quality trimming.", "QC:2000019"));
	output.insert(QCValue("mapped read percentage", 100.0 * al_mapped / al_total, "Percentage of reads that could be mapped to the reference genome.", "QC:2000020"));
	output.insert(QCValue("on-target read percentage", 100.0 * al_ontarget / al_total, "Percentage of reads that could be mapped to the target region.", "QC:2000021"));
    if (paired_end)
    {
		output.insert(QCValue("properly-paired read percentage", 100.0 * al_proper_paired / al_total, "Percentage of properly paired reads (for paired-end reads only).", "QC:2000022"));
		output.insert(QCValue("insert size", insert_size_sum / al_proper_paired, "Mean insert size (for paired-end reads only).", "QC:2000023"));
    }
    else
    {
		output.insert(QCValue("properly-paired read percentage", "n/a (single end)", "Percentage of properly paired reads (for paired-end reads only).", "QC:2000022"));
		output.insert(QCValue("insert size", "n/a (single end)", "Mean insert size (for paired-end reads only).", "QC:2000023"));
    }
    if (al_dup==0)
    {
		output.insert(QCValue("duplicate read percentage", "n/a (duplicates not marked or removed during data analysis)", "Percentage of reads removed because they were duplicates (PCR, optical, etc).", "QC:2000024"));
    }
    else
    {
		output.insert(QCValue("duplicate read percentage", 100.0 * al_dup / al_total, "Percentage of reads removed because they were duplicates (PCR, optical, etc).", "QC:2000024"));
    }
	output.insert(QCValue("target region read depth", bases_overlap_roi / chr_info.genomeSize(true), "Average sequencing depth in target region.", "QC:2000025"));

	//add insert size distribution plot
	if (paired_end)
	{
		LinePlot plot2;
		plot2.setXLabel("insert size");
		plot2.setYLabel("reads [%]");
		plot2.setXValues(BasicStatistics::range(insert_dist.count(), 2.0, 5.0));
		double insert_count = std::accumulate(insert_dist.begin(), insert_dist.end(), 0.0);
		for (int i=0; i<insert_dist.count(); ++i)
		{
			insert_dist[i] = 100.0 * insert_dist[i] / insert_count;
		}
		plot2.addLine(insert_dist, "");

		QString plotname = Helper::tempFileName(".png");
		plot2.store(plotname);
		output.insert(QCValue::Image("insert size distribution plot", plotname, "Insert size distribution plot.", "QC:2000038"));
		QFile::remove(plotname);
	}

    return output;
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
    QList<Chromosome> chr_list = chromosomes.toList();
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

QCCollection Statistics::mapping3Exons(const QString& bam_file)
{
    //roi
    BedFile roi_qc;
    roi_qc.append(BedLine("chr1", 152057442, 152060019));
    roi_qc.append(BedLine("chr9", 5919683, 5923309));
    roi_qc.append(BedLine("chr18", 19995536, 19997774));

    //open BAM file
    BamReader reader;
    NGSHelper::openBAM(reader, bam_file);

    //iterate through target regions
    long depth_all = 0;
    long depth_highcov = 0;
    long c_n = 0;
    long c_indel = 0;
    long c_snv = 0;
    for (int i=0; i<roi_qc.count(); ++i)
    {
        const BedLine& line = roi_qc[i];
        QList<Pileup> pileups;
		NGSHelper::getPileups(pileups, reader, line.chr(), line.start(), line.end());
        foreach(const Pileup& pileup, pileups)
        {
            int depth = pileup.depth(false);
            depth_all += depth;
            c_n += pileup.n();

            if (depth>=20)
            {
                depth_highcov += depth;
                int indel_count = pileup.indels().count();
                if (indel_count<0.25*depth)
                {
                    c_indel += indel_count;
                }
                int snv_count = depth - pileup.max();
                if (snv_count<0.25*depth)
                {
                    c_snv += snv_count;
                }
            }
        }
    }

    //generate output
    QCCollection output;
    double avg_depth = (double)depth_all/roi_qc.baseCount();
	output.insert(QCValue("error estimation read depth", avg_depth, "Average read depth on the special target region used for error estimation after mapping: chr1:152057442-152060019, chr9:5919683-5923309, chr18:19995536-19997774.", "QC:2000033"));
    if (avg_depth<10)
    {
		output.insert(QCValue("error estimation N percentage", "n/a (average depth too low)", "No base call (N) percentage determined on special target region after mapping.", "QC:2000034"));
		output.insert(QCValue("error estimation SNV percentage", "n/a (average depth too low)", "SNV error percentage determined on special target region after mapping.", "QC:2000035"));
		output.insert(QCValue("error estimation indel percentage", "n/a (average depth too low)", "INDEL error percentage determined on special target region after mapping.", "QC:2000036"));
    }
    else
    {
		output.insert(QCValue("error estimation N percentage", 100.0*c_n/depth_all, "No base call (N) percentage determined on special target region after mapping.", "QC:2000034"));
		output.insert(QCValue("error estimation SNV percentage", 100.0*c_snv/depth_highcov, "SNV error percentage determined on special target region after mapping.", "QC:2000035"));
		output.insert(QCValue("error estimation indel percentage", 100.0*c_indel/depth_highcov, "INDEL error percentage determined on special target region after mapping.", "QC:2000036"));
    }

    return output;
}

BedFile Statistics::lowCoverage(const BedFile& bed_file, const QString& bam_file, int cutoff, int min_mapq)
{
	BedFile output;

    //check target region is merged/sorted and create index
    if (!bed_file.isMergedAndSorted())
    {
		THROW(ArgumentException, "Merged and sorted BED file required for low-coverage statistics!");
    }

    //open BAM file
    BamReader reader;
    NGSHelper::openBAM(reader, bam_file);
	ChromosomeInfo chr_info(reader);

	//init datastructures
	QVector<int> roi_cov;
	BamAlignment al;

	//iterate trough all regions (i.e. exons in most cases)
	for (int i=0; i<bed_file.count(); ++i)
	{
		const BedLine& bed_line = bed_file[i];
		const int start = bed_line.start();
		//qDebug() << bed_line.chr().str().constData() << ":" << bed_line.start() << "-" << bed_line.end();

		//init coverage statistics
		roi_cov.fill(0, bed_line.length());

		//jump to region
		int ref_id = chr_info.refID(bed_line.chr());
		bool jump_ok = reader.SetRegion(ref_id, bed_line.start()-1, ref_id, bed_line.end());
		if (!jump_ok) THROW(FileAccessException, QString::fromStdString(reader.GetErrorString()));

		//iterate through all alignments
		while (reader.GetNextAlignmentCore(al))
		{
			if (al.IsDuplicate()) continue;
			if (!al.IsPrimaryAlignment()) continue;
			if (!al.IsMapped() || al.MapQuality<min_mapq) continue;

			const int ol_start = std::max(start, al.Position+1) - start;
			const int ol_end = std::min(bed_line.end(), al.GetEndPosition()) - start;
			for (int p=ol_start; p<=ol_end; ++p)
			{
				++roi_cov[p];
			}
		}

		//create low-coverage regions file
		BedFile tmp;
		for (int p=0; p<roi_cov.count(); ++p)
		{
			if (roi_cov[p]<cutoff)
			{
				tmp.append(BedLine(bed_line.chr(), p+start, p+start));
			}
		}

		//merge and add to output
		tmp.merge();
		output.add(tmp);
	}

	output.sort();
	return output;
}

void Statistics::avgCoverage(BedFile& bed_file, const QString& bam_file, int min_mapq)
{
    //check target region is merged/sorted and create index
    if (!bed_file.isMergedAndSorted())
    {
        THROW(ArgumentException, "Merged and sorted BED file required for coverage calculation!");
    }

    //open BAM file
    BamReader reader;
    NGSHelper::openBAM(reader, bam_file);
	ChromosomeInfo chr_info(reader);

    //init coverage statistics data structure
    QVector<long> cov;
    cov.fill(0, bed_file.count());

    //iterate through all alignments
    ChromosomalIndex<BedFile> bed_idx(bed_file);
    BamAlignment al;
    while (reader.GetNextAlignmentCore(al))
    {
		if (al.IsDuplicate()) continue;
		if (!al.IsPrimaryAlignment()) continue;
        if (!al.IsMapped() || al.MapQuality<min_mapq) continue;

		const Chromosome& chr = chr_info.chromosome(al.RefID);
        int end_position = al.GetEndPosition();
        QVector<int> indices = bed_idx.matchingIndices(chr, al.Position+1, end_position);
        foreach(int index, indices)
        {
            cov[index] += std::min(bed_file[index].end(), end_position) - std::max(bed_file[index].start(), al.Position+1);
        }
    }
    reader.Close();

    //calculate output
    for (int i=0; i<bed_file.count(); ++i)
    {
        bed_file[i].annotations().append(QString::number((double)(cov[i]) / bed_file[i].length(), 'f', 2));
    }
}

QString Statistics::genderXY(const QString& bam_file, QStringList& debug_output, double max_female, double min_male)
{
    //open BAM file
    BamReader reader;
    NGSHelper::openBAM(reader, bam_file);

    //get RefID of X and Y chromosome
	int chrx = ChromosomeInfo::refID(reader, "chrX");
	int chry = ChromosomeInfo::refID(reader, "chrY");

    //restrict to X and Y chromosome
	bool jump_ok = reader.SetRegion(chrx, 0, chry, reader.GetReferenceData()[chry].RefLength);
	if (!jump_ok) THROW(FileAccessException, QString::fromStdString(reader.GetErrorString()));

    //iterate through all alignments and count
    int count_x = 0;
    int count_y = 0;
    BamAlignment al;
    while (reader.GetNextAlignmentCore(al))
    {
        if (al.RefID==chrx)
        {
            ++count_x;
        }
        else if (al.RefID==chry)
        {
            ++count_y;
        }
    }
    reader.Close();

    //debug output
    double ratio_yx = (double) count_y / count_x;
    debug_output << "read count chrX: " + QString::number(count_x);
    debug_output << "read count chrY: " + QString::number(count_y);
    debug_output << "ratio chrY/chrX: " + QString::number(ratio_yx, 'f', 4);

    //output
    if (ratio_yx<=max_female) return "female";
    if (ratio_yx>=min_male) return "male";
    return "unknown (ratio in gray area)";
}

QString Statistics::genderHetX(const QString& bam_file, QStringList& debug_output, double max_male, double min_female)
{
    //open BAM file
    BamReader reader;
    NGSHelper::openBAM(reader, bam_file);

    //restrict to X chromosome
	int chrx = ChromosomeInfo::refID(reader, "chrX");
	bool jump_ok = reader.SetRegion(chrx, 0, chrx, reader.GetReferenceData()[chrx].RefLength);
	if (!jump_ok) THROW(FileAccessException, QString::fromStdString(reader.GetErrorString()));

    //get SNP positions on X chromsome
    QVector<int> positions;
    VariantList snps = NGSHelper::getSNPs();
    for(int i=0; i<snps.count(); ++i)
    {
        if (snps[i].chr()=="chrX") positions.append(snps[i].start());
    }
    QVector<Pileup> counts;
    counts.fill(Pileup(), positions.count());

    //iterate through all alignments and create counts
    BamAlignment al;
    while (reader.GetNextAlignmentCore(al))
    {
        if (al.MapQuality<20) continue;

        int start = al.Position + 1;
        int end = al.GetEndPosition();
        bool char_data_built = false;

        for (int i=0; i<positions.count(); ++i)
        {
            int pos = positions[i];
            if (start <= pos && end >= pos)
            {
                if (!char_data_built)
                {
                    al.BuildCharData();
                    char_data_built = true;
                }

                QPair<char, int> base = NGSHelper::extractBaseByCIGAR(al, pos);
                counts[i].inc(base.first);
            }
        }
    }
    reader.Close();

    //count
    int hom_count = 0;
    int het_count = 0;
    for (int i=0; i<positions.count(); ++i)
    {
        int depth = counts[i].depth(false);
        if (depth>=30)
        {
            int max = counts[i].max();
            if (max<0.8 * depth)
            {
                ++het_count;
            }
            else
            {
                ++hom_count;
            }
        }
    }

    //debug output
    double het_frac = (double) het_count / (het_count + hom_count);
    debug_output << "SNPs with coverage 30 or more: " +  QString::number(hom_count + het_count) + " of " + QString::number(positions.count());
    debug_output << "hom count: " + QString::number(hom_count);
    debug_output << "het count: " + QString::number(het_count);
    debug_output << "het fraction: " + QString::number(het_frac, 'f', 4);

    //output
    if (hom_count + het_count < 20) return "unknown (too few SNPs)";
    if (het_frac<=max_male) return "male";
    if (het_frac>=min_female) return "female";
	return "unknown (fraction in gray area)";
}

QString Statistics::genderSRY(const QString& bam_file, QStringList& debug_output, double min_cov)
{
	//open BAM file
	BamReader reader;
	NGSHelper::openBAM(reader, bam_file);

	//restrict to SRY gene
	int chry = ChromosomeInfo::refID(reader, "chrY");
	int start = 2655031;
	int end = 2655641;
	bool jump_ok = reader.SetRegion(chry, start, chry, end);
	if (!jump_ok) THROW(FileAccessException, QString::fromStdString(reader.GetErrorString()));

	//calcualte average coverage
	double cov = 0.0;
	BamAlignment al;
	while (reader.GetNextAlignmentCore(al))
	{
		cov += al.GetEndPosition() - al.Position;
	}
	reader.Close();
	cov /= (end-start);

	//output
	debug_output << "coverge SRY: " + QString::number(cov, 'f', 2);
	if (cov>=min_cov) return "male";
	return "female";
}
