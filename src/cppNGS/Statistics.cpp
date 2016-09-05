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
#include "ScatterPlot.h"
#include "BarPlot.h"
#include "Helper.h"
#include "ChromosomeInfo.h"
#include "SampleCorrelation.h"
#include <QFileInfo>
#include "Settings.h"

#include "api/BamReader.h"
using namespace BamTools;

QCCollection Statistics::variantList(const VariantList& variants)
{
    QCCollection output;

    //var_total
	output.insert(QCValue("variant count", variants.count(), "Total number of variants in the target region.", "QC:2000013"));

    //var_perc_dbsnp
    const int i_id = variants.annotationIndexByName("ID", true, false);
    if (variants.count()!=0 && i_id!=-1)
    {
        double dbsnp_count = 0;
        for(int i=0; i<variants.count(); ++i)
		{
            if (variants[i].annotations().at(i_id).startsWith("rs"))
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
    const int i_ann = variants.annotationIndexByName("ANN", true, false);
    if (variants.count()!=0 && i_ann!=-1)
    {
		double high_impact_count = 0;
        for(int i=0; i<variants.count(); ++i)
		{
            if (variants[i].annotations().at(i_ann).contains("|HIGH|"))
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
    const int i_gt = variants.annotationIndexByName("GT", true, false);
    if (variants.count()!=0 && i_gt!=-1)
    {
        double hom_count = 0;
        for(int i=0; i<variants.count(); ++i)
        {
            QString geno = variants[i].annotations().at(i_gt);
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

    //deviation from expected allel frequency
    int i_ao = -1;
    int i_dp = -1;
    for(int i=0; i<variants.annotationDescriptions().count(); ++i)
    {
        if (variants.annotationDescriptions()[i].name()=="AO" && !variants.annotationDescriptions()[i].sampleSpecific())
        {
           i_ao = i;
        }
        if (variants.annotationDescriptions()[i].name()=="DP" && !variants.annotationDescriptions()[i].sampleSpecific())
        {
           i_dp = i;
        }
    }
    int diff_count = 0;
    double diff_sum = 0.0;
    if (i_ao!=-1 && i_dp!=-1)
    {
        for(int i=0; i<variants.count(); ++i)
        {
            if (!variants[i].isSNV()) continue;

            bool ok = true;
            int dp = variants[i].annotations().at(i_dp).toInt(&ok);
            if (!ok || dp<30) continue;
            int ao = variants[i].annotations().at(i_ao).toInt(&ok);
            if (!ok) continue;
            double af = (double)ao/dp;
            double diff = std::min(std::min(af, std::fabs(0.5-af)), std::fabs(1.0-af));
            diff_sum += diff;
            ++diff_count;
        }
    }
    if (diff_count>30)
    {
        output.insert(QCValue("SNV allele frequency deviation", QString::number(diff_sum/diff_count, 'f', 4), "Mean deviation from expected allele frequency (e.g. 0.0, 0.5 or 1.0 for diploid organisms) for single nucleotide variants.", "QC:2000051"));
    }
    else
    {
        output.insert(QCValue("SNV allele frequency deviation", "n/a (AO/DP annotation not found, or not enough variants)", "Mean deviation from expected allele frequency (e.g. 0.0, 0.5 or 1.0 for diploid organisms) for single nucleotide variants.", "QC:2000051"));
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
    long long roi_bases = 0;
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
    double bases_mapped = 0;
    double bases_clipped = 0;
    double insert_size_sum = 0;
	QVector<double> insert_dist;
    long long bases_usable = 0;
    int max_length = 0;
    bool paired_end = false;

    //iterate through all alignments
    BamAlignment al;
    while (reader.GetNextAlignmentCore(al))
    {
		//skip secondary alignments
		if (!al.IsPrimaryAlignment()) continue;

        ++al_total;
        max_length = std::max(max_length, al.Length);

        //insert size
		if (al.IsPaired())
        {
            paired_end = true;

            if (al.IsProperPair())
            {
                ++al_proper_paired;
				int insert_size = std::min(abs(al.InsertSize), 999); //cap insert size at 1000
				insert_size_sum += insert_size;
				int bin = insert_size/5;
				if (insert_dist.count()<=bin) insert_dist.resize(bin+1);
                insert_dist[bin] += 1; //TODO use Histogram class!?
            }
        }

        if (al.IsMapped())
        {
            ++al_mapped;

            //calculate soft/hard-clipped bases
            const int start_pos = al.Position+1;
            const int end_pos = al.GetEndPosition();
            bases_mapped += al.Length;
            for (auto it=al.CigarData.cbegin(); it!=al.CigarData.cend(); ++it)
            {
                if (it->Type=='S' || it->Type=='H')
                {
                    bases_clipped += it->Length;
                }
            }

            //calculate usable bases and base-resolution coverage
			const Chromosome& chr = chr_info.chromosome(al.RefID);
            QVector<int> indices = roi_index.matchingIndices(chr, start_pos, end_pos);
            if (indices.count()!=0)
            {
                ++al_ontarget;

				if (!al.IsDuplicate() && al.MapQuality>=min_mapq)
                {
                    foreach(int index, indices)
                    {
                        const int ol_start = std::max(bed_file[index].start(), start_pos);
                        const int ol_end = std::min(bed_file[index].end(), end_pos);
                        bases_usable += ol_end - ol_start + 1;
						QMap<int, int>& roi_cov_chr_map = roi_cov[chr.num()];
                        for (int p=ol_start; p<=ol_end; ++p) //TODO use iterator, also for low-depth algo!
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
    output.insert(QCValue("clipped base percentage", 100.0 * bases_clipped / bases_mapped, "Percentage of the bases that are soft-clipped or hand-clipped during mapping.", "QC:2000052"));
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
    output.insert(QCValue("bases usable (MB)", (double)bases_usable / 1000000.0, "Bases sequenced that are usable for variant calling (in megabases).", "QC:2000050"));
    output.insert(QCValue("target region read depth", (double)bases_usable / roi_bases, "Average sequencing depth in target region.", "QC:2000025"));

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
    double bases_mapped = 0;
    double bases_clipped = 0;
    double insert_size_sum = 0;
	QVector<double> insert_dist;
    long long bases_usable = 0;
    int max_length = 0;
    bool paired_end = false;

    //iterate through all alignments
    BamAlignment al;
    while (reader.GetNextAlignmentCore(al))
	{
		//skip secondary alignments
		if (!al.IsPrimaryAlignment()) continue;

        ++al_total;
        max_length = std::max(max_length, al.Length);

        //insert size
		if (al.IsPaired())
        {
            paired_end = true;

            if (al.IsProperPair())
            {
				++al_proper_paired;
                const int insert_size = std::min(abs(al.InsertSize),  999); //cap insert size at 1000
				insert_size_sum += insert_size;
                int bin = insert_size/5;  //TODO use Histogram class!?
				if (insert_dist.count()<=bin) insert_dist.resize(bin+1);
				insert_dist[bin] += 1;
            }
        }

        if (al.IsMapped())
        {
            ++al_mapped;

            //calculate soft/hard-clipped bases
            bases_mapped += al.Length;
            for (auto it=al.CigarData.cbegin(); it!=al.CigarData.cend(); ++it)
            {
                if (it->Type=='S' || it->Type=='H')
                {
                    qDebug() << it->Type << it->Length;
                    bases_clipped += it->Length;
                }
            }

            //usable
			if (chr_info.chromosome(al.RefID).isNonSpecial())
            {
                ++al_ontarget;

				if (!al.IsDuplicate() && al.MapQuality>=min_mapq)
				{
                    bases_usable += al.Length;
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
    output.insert(QCValue("clipped base percentage", 100.0 * bases_clipped / bases_mapped, "Percentage of the bases that are soft-clipped or hand-clipped during mapping.", "QC:2000052"));
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
    output.insert(QCValue("bases usable (MB)", (double)bases_usable / 1000000.0, "Bases sequenced that are usable for variant calling (in megabases).", "QC:2000050"));
    output.insert(QCValue("target region read depth", (double) bases_usable / chr_info.genomeSize(true), "Average sequencing depth in target region.", "QC:2000025"));

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

QCCollection Statistics::somatic(QString& tumor_bam, QString& normal_bam, QString& somatic_vcf, QString& build, QString target_file)
{
	QCCollection output;

	//sample correlation
	SampleCorrelation sc;
	sc.calculateFromBam(tumor_bam,normal_bam,30,500);
	output.insert(QCValue("sample correlation", sc.sampleCorrelation(), ".", "QC:2000040"));

	//variants
	VariantList variants;
	variants.load(somatic_vcf);
	variants.sort();

	//total variants
	output.insert(QCValue("variant count", variants.count(), "Total number of variants in the target region.", "QC:2000013"));

	//total variants filtered
	int somatic_count = 0;
	for(int i=0; i<variants.count(); ++i)
	{
		if (!variants[i].filters().empty())	continue;
		++somatic_count;
	}
	output.insert(QCValue("somatic variant count", somatic_count, "Total number of somatic variants in the target region.", "QC:2000041"));

	//var_perc_dbsnp
	int index = variants.annotationIndexByName("ID", true, false);
	if (variants.count()!=0 && index!=-1)
	{
		double dbsnp_count = 0;
		for(int i=0; i<variants.count(); ++i)
		{
			if (!variants[i].filters().empty())	continue;

			if (variants[i].annotations().at(index).startsWith("rs"))
			{
				++dbsnp_count;
			}
		}
		output.insert(QCValue("known somatic variants percentage", 100.0*dbsnp_count/somatic_count, "Percentage of somatic variants that are known polymorphisms in the dbSNP database.", "QC:2000045"));
	}
	else
	{
		output.insert(QCValue("known somatic variants percentage", "n/a (no somatic variants)", "Percentage of somatic variants that are known polymorphisms in the dbSNP database.", "QC:2000045"));
	}

	//var_perc_indel / var_ti_tv_ratio
	double indel_count = 0;
	double ti_count = 0;
	double tv_count = 0;
	for(int i=0; i<variants.count(); ++i)
	{
		if (!variants[i].filters().empty())	continue;

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
	if (somatic_count!=0)
	{
		output.insert(QCValue("somatic indel percentage", 100.0*indel_count/somatic_count, "Percentage of somatic variants that are insertions/deletions.", "QC:2000042"));
	}
	else
	{
		output.insert(QCValue("somatic indel variants percentage", "n/a (no variants)", "Percentage of variants that are insertions/deletions.", "QC:2000042"));
	}
	if (tv_count!=0)
	{
		output.insert(QCValue("somatic transition/transversion ratio", ti_count/tv_count , "Somatic Transition/transversion ratio of SNV variants.", "QC:2000043"));
	}
	else
	{
		output.insert(QCValue("somatic transition/transversion ratio", "n/a (no variants or transversions)", "Somatic transition/transversion ratio of SNV variants.", "QC:2000043"));
	}

	QString tumor_id = QFileInfo(tumor_bam).baseName();
	QString normal_id = QFileInfo(normal_bam).baseName();
	QStringList nuc = QStringList{"A","C","G","T"};

	if(!variants.sampleNames().contains(tumor_id))	Log::error("Tumor sample " + tumor_id + " was not found in variant file " + somatic_vcf);
	if(!variants.sampleNames().contains(normal_id))	Log::error("Normal sample " + normal_id + " was not found in variant file " + somatic_vcf);

	//plot1: allele frequencies
	ScatterPlot plot1;
	plot1.setXLabel("allele frequency tumor");
	plot1.setYLabel("allele frequency normal");
	plot1.setXRange(-0.015,1.015);
	plot1.setYRange(-0.015,1.015);
	QList< QPair<double,double> > points_black;
	QList< QPair<double,double> > points_green;
	for(int i=0; i<variants.count(); ++i)
	{
		double af_tumor = -1;
		double af_normal = -1;
		int count_mut = 0;
		int count_all = 0;

		//strelka SNV tumor and normal
		int idx_strelka_snv = variants.annotationIndexByName("AU", tumor_id, true, false);
		int idx_strelka_indel = variants.annotationIndexByName("TIR", tumor_id, true, false);
		if( idx_strelka_snv!=-1 && !variants[i].annotations()[idx_strelka_snv].isEmpty() )
		{			
			count_mut = 0;
			count_all = 0;
			foreach(QString n, nuc)
			{
				int index_n = variants.annotationIndexByName((n+"U"), tumor_id);
				int tmp = variants[i].annotations()[index_n].split(',')[0].toInt();
				if(n==variants[i].obs())	count_mut += tmp;
				count_all += tmp;
			}
			if(count_all>0)	af_tumor = (double)count_mut/count_all;

			count_mut = 0;
			count_all = 0;
			foreach(QString n, nuc)
			{
				int index_n = variants.annotationIndexByName((n+"U"), normal_id);
				int tmp = variants[i].annotations()[index_n].split(',')[0].toInt();
				if(n==variants[i].obs())	count_mut += tmp;
				count_all += tmp;
			}
			if(count_all>0)	af_normal = (double)count_mut/count_all;
		}
		else if( idx_strelka_indel!=-1 && !variants[i].annotations()[idx_strelka_indel].isEmpty() )	//indels strelka
		{
			//TIR + TAR tumor
			count_mut = 0;
			count_all = 0;
			int idx = variants.annotationIndexByName("TIR", tumor_id);
			count_mut = variants[i].annotations()[idx].split(',')[0].toInt();
			idx = variants.annotationIndexByName("TAR", tumor_id);
			count_all = variants[i].annotations()[idx].split(',')[0].toInt() + count_mut;
			if(count_all>0)	af_tumor = (double)count_mut/count_all;

			//TIR + TAR normal
			count_mut = 0;
			count_all = 0;
			idx = variants.annotationIndexByName("TIR", normal_id);
			count_mut = variants[i].annotations()[idx].split(',')[0].toInt();
			idx = variants.annotationIndexByName("TAR", normal_id);
			count_all = variants[i].annotations()[idx].split(',')[0].toInt() + count_mut;
			if(count_all>0)	af_normal = (double)count_mut/count_all;
		}
		//freebayes tumor and normal
		//##FORMAT=<ID=RO,Number=1,Type=Integer,Description="Reference allele observation count">
		//##FORMAT=<ID=AO,RONumber=A,Type=Integer,Description="Alternate allele observation count">
		else if(variants.annotationIndexByName("AO", tumor_id, true, false) != -1)
		{
			int index_ro = variants.annotationIndexByName("RO", tumor_id);
			int index_ao = variants.annotationIndexByName("AO", tumor_id);
			count_mut = variants[i].annotations()[index_ao].toInt();
			count_all = count_mut + variants[i].annotations()[index_ro].toInt();
			if(count_all>0)	af_tumor = (double)count_mut/count_all;

			index_ro = variants.annotationIndexByName("RO", normal_id);
			index_ao = variants.annotationIndexByName("AO", normal_id);
			count_mut = variants[i].annotations()[index_ao].toInt();
			count_all = count_mut + variants[i].annotations()[index_ro].toInt();
			if(count_all>0)	af_normal = (double)count_mut/count_all;
		}
		else
		{
			Log::error("Could not identify vcf format in line " + QString::number(i) + ". Sample-ID: " + tumor_id + ". Only strelka and freebayes are currently supported.");
		}

		//find AF and set x and y points, implement freebayes and strelka fields
		QPair<double,double> point;
		point.first = af_tumor;
		point.second = af_normal;
		if (!variants[i].filters().empty())	points_black.append(point);
		else points_green.append(point);
	}

	QList< QPair<double,double> > points;
	points  << points_black << points_green;

	QString g = "k";
	QString b = "g";
	QList< QString > colors;
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
	plot1.addColorLegend(b,"filtered variants");

	QString plot1name = Helper::tempFileName(".png");
	plot1.store(plot1name);
	output.insert(QCValue::Image("somatic variants allele frequencies plot", plot1name, ".", "QC:2000048"));
	QFile::remove(plot1name);

	//plot2: somatic variant signature, only pyrimidines are of interest
	BarPlot plot2;
	plot2.setXLabel("triplett");
	plot2.setYLabel("count");
	QString c,co,cod;
	QMap<QString,QString> color_map = QMap<QString,QString>{{"C>A","b"},{"C>G","k"},{"C>T","r"},{"T>A","g"},{"T>G","c"},{"T>C","y"}};
	foreach(QString color, color_map)
	{
		plot2.addColorLegend(color,color_map.key(color));
	}
	QList<QString> codons;
	QList<int> counts;
	QList<int> counts_target;
	QList<int> counts_genome;
	QList<double> counts_normalized;
	QList<double> frequencies;
	QList<QString> labels;
	colors = QList<QString>();
	QStringList sig = QStringList{"C","T"};
	foreach(QString r, sig)
	{
		c = r;
		foreach(QString o, nuc)
		{
			if(c == o)	continue;
			foreach(QString rr, nuc)
			{
				co = rr + c;
				foreach(QString rrr, nuc)
				{
					cod = co + rrr + " - " + o;
					codons.append(cod);
					counts.append(0);
					counts_target.append(0);
					counts_genome.append(0);
					counts_normalized.append(0);
					frequencies.append(0);
					colors.append(color_map[r+">"+o]);
					labels.append(co + rrr);
				}
			}
		}
	}
	//codons: count codons from variant list
	FastaFileIndex reference(Settings::string("reference_genome"));
	for(int i=0; i<variants.count(); ++i)
	{
		if(!variants[i].filters().empty())	continue;	//skip non-somatic variants
		if(!variants[i].isSNV())	continue;	//skip indels

		Variant v = variants[i];
		QString c = reference.seq(v.chr(),v.start()-1,1,true) + v.ref().toUpper() + reference.seq(v.chr(),v.start()+1,1,true) + " - " + v.obs().toUpper();

		if(!codons.contains(c))	continue;
		int index = codons.indexOf(c);
		++counts[index];
	}

	plot2.setYLabel("variant type percentage");
	QHash < QString, int > count_codons_target({
	   {"ACA",0},{"ACC",0},{"ACG",0},{"ACT",0},{"CCA",0},{"CCC",0},{"CCG",0},{"CCT",0},
	   {"GCA",0},{"GCC",0},{"GCG",0},{"GCT",0},{"TCA",0},{"TCC",0},{"TCG",0},{"TCT",0},
	   {"ATA",0},{"ATC",0},{"ATG",0},{"ATT",0},{"CTA",0},{"CTC",0},{"CTG",0},{"CTT",0},
	   {"GTA",0},{"GTC",0},{"GTG",0},{"GTT",0},{"TTA",0},{"TTC",0},{"TTG",0},{"TTT",0}
   });
	if(target_file.isEmpty() && build=="hg19")
	{
		count_codons_target = {
			{"ACA",57602816},{"ACC",33256008},{"ACG",7181818},{"ACT",45999566},{"CCA",52722217},{"CCC",37602581},{"CCG",7900670},{"CCT",50842624},
			{"GCA",41189353},{"GCC",34053539},{"GCG",6813566},{"GCT",40009228},{"TCA",56034760},{"TCC",44155965},{"TCG",6321389},{"TCT",63331604},
			{"ATA",58959287},{"ATC",38179637},{"ATG",52548181},{"ATT",71375695},{"CTA",36871406},{"CTC",48167888},{"CTG",57996845},{"CTT",57146885},
			{"GTA",32466396},{"GTC",27046634},{"GTG",43059504},{"GTT",41794778},{"TTA",59555859},{"TTC",56449722},{"TTG",54311749},{"TTT",110166711}
		};
	}
	else if(target_file.isEmpty() && build=="hg38")
	{
		count_codons_target = {
			{"ACA",59305516},{"ACC",33784390},{"ACG",7584302},{"ACT",47476086},{"CCA",53293160},{"CCC",38036593},{"CCG",8026845},{"CCT",51880303},
			{"GCA",42481943},{"GCC",34497599},{"GCG",7078395},{"GCT",40674873},{"TCA",57800075},{"TCC",44918305},{"TCG",6712244},{"TCT",65492835},
			{"ATA",60308591},{"ATC",39076747},{"ATG",53548035},{"ATT",73292370},{"CTA",37666053},{"CTC",49481013},{"CTG",59039769},{"CTT",59337262},
			{"GTA",33265786},{"GTC",27466578},{"GTG",44578403},{"GTT",43191653},{"TTA",60159779},{"TTC",58899235},{"TTG",56762262},{"TTT",113868707}
		};
	}
	else if(target_file.isEmpty() && build=="count")
	{
		FastaFileIndex reference(Settings::string("reference_genome"));
		int bin = 50000000;
		for(int i=0; i<reference.names().count(); ++i)
		{
			Chromosome chr = reference.names().at(i);

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
				foreach(QString codon, count_codons_target.keys())
				{
					count_codons_target[codon] += seq.count(codon.toUpper().toLatin1());
				}
			}
		}
	}
	else if(!target_file.isEmpty())
	{
		//codons: count in target file
		BedFile target;
		target.load(target_file);
		if(target.baseCount()<100000)	Log::warn("Target size is less than 100 kb. Mutation signature may be imprecise.");
		for(int i=0; i<target.count(); ++i)
		{
			Sequence seq = reference.seq(target[i].chr(),target[i].start(),target[i].length(),true);
			foreach(QString codon, count_codons_target.keys())
			{
				count_codons_target[codon] += seq.count(codon.toLatin1());
			}
		}
	}
	else	Log::error("Unknown genome build " + build + " or no target file given.");

	//codons: normalize current codons and calculate percentages for each codon
	double y_max = 5;
	double sum = 0;
	for(int i=0; i<codons.count(); ++i)
	{
		QString cod = codons[i].mid(0,3);
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
	output.insert(QCValue::Image("somatic variant signature plot", plot2name, ".", "QC:2000047"));
	QFile::remove(plot2name);

	//plot3: somatic variant distances, only for whole genome sequencing
	if(target_file.isEmpty())
	{
		ScatterPlot plot3;
		plot3.setXLabel("chromosomes");
		plot3.setYLabel("somatic variant distance [bp]");
		plot3.setYLogScale(true);
		//(0) generate chromosomal map
		long long genome_size = 0;
		QMap<Chromosome,long long> chrom_starts;
		QStringList fai = Helper::loadTextFile(Settings::string("reference_genome") + ".fai", true, '~', true);
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
		QList< QPair<double,double> > points3;
		QString tmp_chr = "";
		int tmp_pos = 0;
		double tmp_offset = 0;
		double max = 0;
		for(int i=0; i<variants.count(); ++i)	//list has to be sorted by chrom. position
		{
			if(!variants[i].chr().isNonSpecial())	continue;
			if(!variants[i].filters().empty())	continue;	//skip non-somatic variants

			if(tmp_chr == variants[i].chr().str())	//same chromosome
			{
				//convert distance to x-Axis position
				QPair<double,double> point;
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
		output.insert(QCValue::Image("somatic variant distance plot", plot3name, ".", "QC:2000046"));
		QFile::remove(plot3name);
	}

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

	//iterate trough all regions (i.e. exons in most cases)
	for (int i=0; i<bed_file.count(); ++i)
	{
		const BedLine& bed_line = bed_file[i];
		const int start = bed_line.start();
		//qDebug() << bed_line.chr().str().constData() << ":" << bed_line.start() << "-" << bed_line.end();

		//init coverage statistics
        QVector<int> roi_cov(bed_line.length(), 0);

		//jump to region
		int ref_id = chr_info.refID(bed_line.chr());
		bool jump_ok = reader.SetRegion(ref_id, bed_line.start()-1, ref_id, bed_line.end());
		if (!jump_ok) THROW(FileAccessException, QString::fromStdString(reader.GetErrorString()));

		//iterate through all alignments
        BamAlignment al;
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
        bool reg_open = false;
		int reg_start = -1;
        for (int p=0; p<roi_cov.count(); ++p)
        {
            bool low_cov = roi_cov[p]<cutoff;
            if (reg_open && !low_cov)
            {
				output.append(BedLine(bed_line.chr(), reg_start+start, p+start-1, bed_line.annotations()));
                reg_open = false;
                reg_start = -1;
            }
            if (!reg_open && low_cov)
            {
                reg_open = true;
                reg_start = p;
            }
        }
        if (reg_open)
        {
			output.append(BedLine(bed_line.chr(), reg_start+start, bed_line.length()+start-1, bed_line.annotations()));
        }
    }

	return output;
}

BedFile Statistics::lowCoverage(const QString& bam_file, int cutoff, int min_mapq)
{
    BedFile output;

    //open BAM file
    BamReader reader;
    NGSHelper::openBAM(reader, bam_file);
    ChromosomeInfo chr_info(reader);

    //iteratore through chromosomes
    QList<Chromosome> chrs = chr_info.chromosomes();
    foreach(const Chromosome& chr, chrs)
    {
        if (!chr.isNonSpecial()) continue;

        int chr_size = chr_info.size(chr);
        //qDebug() << chr.str() << chr_size;
        QVector<int> cov(chr_size, 0);

        //jump to chromosome
        int ref_id = chr_info.refID(chr);
        bool jump_ok = reader.SetRegion(ref_id, 0, ref_id, chr_size);
        if (!jump_ok) THROW(FileAccessException, QString::fromStdString(reader.GetErrorString()));

        //iterate through all alignments
        BamAlignment al;
        while (reader.GetNextAlignmentCore(al))
        {
            if (al.IsDuplicate()) continue;
            if (!al.IsPrimaryAlignment()) continue;
            if (!al.IsMapped() || al.MapQuality<min_mapq) continue;

            const int end = al.GetEndPosition();
            for (int p=al.Position; p<end; ++p)
            {
                ++cov[p];
            }
        }

        //create low-coverage regions file
        bool reg_open = false;
        int reg_start = -1;
        for (int p=0; p<cov.count(); ++p)
        {
            bool low_cov = cov[p]<cutoff;
            if (reg_open && !low_cov)
            {
                //qDebug() << chr.str() << reg_start+1 << p;
                output.append(BedLine(chr, reg_start+1, p));
                reg_open = false;
                reg_start = -1;
            }
            if (!reg_open && low_cov)
            {
                reg_open = true;
                reg_start = p;
            }
        }
        if (reg_open)
        {
            //qDebug() << chr.str() << reg_start+1 << chr_size;
            output.append(BedLine(chr, reg_start+1, chr_size));
        }
    }

    output.merge();
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
