#include "StatisticsReads.h"
#include "LinePlot.h"
#include "Helper.h"
#include <BarPlot.h>


StatisticsReads::StatisticsReads(bool long_read)
	: c_forward_(0)
	, c_reverse_(0)
	, read_lengths_()
    , bases_sequenced_(0)
	, c_read_q20_(0.0)
	, c_base_q20_(0.0)
	, c_base_q30_(0.0)
	, pileups_()
	, qualities1_()
	, qualities2_()
	, qscore_dist_r1(0, 60, 1)
	, qscore_dist_r2(0, 60, 1)
	, long_read_(long_read)
	, base_qualities_(100)
	, read_qualities_(100)

{
}

void StatisticsReads::update(const FastqEntry& entry, ReadDirection direction)
{
	//update read counts
	if (direction==FORWARD)
	{
		++c_forward_;
	}
	else
	{
		++c_reverse_;
	}

	//check number of cycles
    int cycles = entry.bases.size();
    bases_sequenced_ += cycles;
	read_lengths_[cycles]++;
	if (cycles>pileups_.size())
	{
		pileups_.resize(cycles);
		qualities1_.resize(cycles);
		qualities2_.resize(cycles);
	}

	//create pileups
	for (int i=0; i<cycles; ++i) pileups_[i].inc(entry.bases[i]);

	//handle qualities
	double q_sum = 0.0;
	for (int i=0; i<cycles; ++i)
	{
		int q = entry.quality(i);
		q_sum += q;
		if (q>=20.0) ++c_base_q20_;
		if (q>=30.0) ++c_base_q30_;
		if (q >= base_qualities_.size()) THROW(ArgumentException, "Base quality > " + QByteArray::number(base_qualities_.size()) + " (" + QByteArray::number(q) + "). This should not happen!");
		base_qualities_[q]++;
		if (direction==FORWARD)
		{
			qualities1_[i] += q;
		}
		else
		{
			qualities2_[i] += q;
		}
	}
	double mean_qscore = q_sum/cycles;
	if (mean_qscore == mean_qscore) read_qualities_[std::round(mean_qscore)]++; //skip nan q score
	if (direction==FORWARD) qscore_dist_r1.inc(mean_qscore, true);
	else qscore_dist_r2.inc(mean_qscore, true);
	if (mean_qscore>=20.0) ++c_read_q20_;
}

void StatisticsReads::update(const BamAlignment& al)
{
	// ignore supplement and secondary reads
	if (al.isSupplementaryAlignment() || al.isSecondaryAlignment()) return;

	//update read counts
	bool is_forward;
	if(long_read_)
	{
		//LongReads are neuther R1 nor R2
		is_forward = true;
		++c_forward_;
	}
	else
	{
		is_forward = al.isRead1();
		if (is_forward)
		{
			++c_forward_;
		}
		else
		{
			++c_reverse_;
		}
	}


	//check number of cycles
	int cycles = al.length(); //no handling of length -1 necessary. It's not CRAM without bases/qualities.
	bases_sequenced_ += cycles;
	read_lengths_[cycles]++;
	if (cycles>pileups_.size())
	{
		pileups_.resize(cycles);
		qualities1_.resize(cycles);
		qualities2_.resize(cycles);
	}
	
	//create pileups
	QVector<int> base_ints = al.baseIntegers();
	for (int i=0; i<cycles; ++i)
	{
		int base = base_ints[i];
		
		if (base==1) pileups_[i].incA();
		else if (base==2) pileups_[i].incC();
		else if (base==4) pileups_[i].incG();
		else if (base==8) pileups_[i].incT();
		else if (base==15) pileups_[i].incN();
		else THROW(ProgrammingException, "Unknown base '" + QString::number(base_ints[i]) + "' in StatisticsReads::update!");
	}

	//handle qualities
	double q_sum = 0.0;
	for (int i=0; i<cycles; ++i)
	{
		int q = al.quality(i);
		q_sum += q;
		if (q>=20.0) ++c_base_q20_;
		if (q>=30.0) ++c_base_q30_;
		if (q >= base_qualities_.size()) THROW(ArgumentException, "Base quality > " + QByteArray::number(base_qualities_.size()) + " (" + QByteArray::number(q) + "). This should not happen!");
		base_qualities_[q]++;
		if (is_forward)
		{
			qualities1_[i] += q;
		}
		else
		{
			qualities2_[i] += q;
		}
	}
	double mean_qscore = q_sum/cycles;
	if (mean_qscore == mean_qscore) read_qualities_[std::round(mean_qscore)]++; //skip nan q score
	if (is_forward) qscore_dist_r1.inc(mean_qscore, true);
	else qscore_dist_r2.inc(mean_qscore, true);
	if (mean_qscore>=20.0) ++c_read_q20_;
}

QCCollection StatisticsReads::getResult()
{
	//create output values
	QCCollection output;

	long long total_reads = c_forward_ + c_reverse_;
	long long c_base_n = 0;
	long long c_base_gc = 0;
	long long bases_total = 0;
	foreach(const Pileup& pileup, pileups_)
	{
		c_base_n += pileup.n();
		c_base_gc += pileup.g() + pileup.c();
		bases_total += pileup.depth(false, true);
	}

	output.insert(QCValue("read count", total_reads, "Total number of reads (forward and reverse reads of paired-end sequencing count as two reads).", "QC:2000005"));
	QString lengths = "";
	QList<int> tmp = read_lengths_.keys();
	std::sort(tmp.begin(), tmp.end());
	if (tmp.size()<4)
	{
		lengths = QString::number(tmp[0]);
		for (int i=1; i<tmp.size(); ++i)
		{
			lengths += ", " + QString::number(tmp[i]);
		}
	}
    else
	{
		lengths = QString::number(tmp[0]) + "-" + QString::number(tmp[tmp.size()-1]);
	}
	output.insert(QCValue("read length", lengths, "Raw read length of a single read before trimming. Comma-separated list of lenghs or length range, if reads have different lengths.", "QC:2000006"));
	output.insert(QCValue("bases sequenced (MB)", (double)bases_sequenced_/1000000.0, "Bases sequenced in total (in megabases).", "QC:2000049"));
    output.insert(QCValue("Q20 read percentage", 100.0*c_read_q20_/total_reads, "The percentage of reads with a mean base quality score greater than Q20.", "QC:2000007"));
	output.insert(QCValue("Q20 base percentage", 100.0*c_base_q20_/bases_total, "The percentage of bases with a minimum quality score of Q20.", "QC:2000148"));
	output.insert(QCValue("Q30 base percentage", 100.0*c_base_q30_/bases_total, "The percentage of bases with a minimum quality score of Q30.", "QC:2000008"));
	output.insert(QCValue("no base call percentage", 100.0*c_base_n/bases_total, "The percentage of bases without base call (N).", "QC:2000009"));
	output.insert(QCValue("gc content percentage", 100.0*c_base_gc/(bases_total-c_base_n), "The percentage of bases that are called to be G or C.", "QC:2000010"));

	int n95 = 0;
	if (long_read_)
	{
		//calculate N50 value
		long long bases = 0;
		int n50 = 0;

		QMapIterator<int,long long> it(read_lengths_);
		it.toBack();
		while(it.hasPrevious())
		{
			it.previous();
			bases += it.key() * it.value();

			// break if 50% of bases_sequenced is reached
			if(bases > (bases_sequenced_/2))
			{
				n50 = it.key();
				break;
			}
		}
		output.insert(QCValue("N50 read length (bp)", n50, "Minimum read length to reach 50% of sequenced bases.", "QC:2000131"));

		bases = 0;
		it.toFront();
		while (it.hasNext())
		{
			it.next();
			bases += it.key() * it.value();

			// break if 50% of bases_sequenced is reached
			if(bases > (double) 0.95*bases_sequenced_)
			{
				n95 = it.key();
				break;
			}
		}


		//ceil n95 to next 10k
		n95 = std::ceil(n95/10000.0) * 10000;
	}

	//create output base distribution plot
	int cycles = pileups_.count();
	if (long_read_) cycles = std::min(n95, cycles);
	QVector<double> line_a(cycles), line_c(cycles), line_g(cycles), line_t(cycles), line_n(cycles), line_gc(cycles), line_x(cycles);
	int i=0;
	foreach(const Pileup& pileup, pileups_)
	{
		double depth_no_n = pileup.depth(false);
		line_a[i] = 100.0 * pileup.a() / depth_no_n;
		line_c[i] = 100.0 * pileup.c() / depth_no_n;
		line_g[i] = 100.0 * pileup.g() / depth_no_n;
		line_t[i] = 100.0 * pileup.t() / depth_no_n;
		line_n[i] = 100.0 * pileup.n() / (depth_no_n + pileup.n());
		line_gc[i] = line_g[i] + line_c[i];
		line_x[i] = i+1;
		++i;
		if(long_read_ && i >= cycles) break;
	}
	LinePlot plot;
	plot.setXLabel("cycle");
	plot.setYLabel("base [%]");
	plot.setYRange(0.0, 100.0);
	plot.setXValues(line_x);
	plot.addLine(line_a, "A");
	plot.addLine(line_c, "C");
	plot.addLine(line_g, "G");
	plot.addLine(line_t, "T");
	plot.addLine(line_n, "N");
	plot.addLine(line_gc, "GC");
	QString plotname = Helper::tempFileName(".png");
	plot.store(plotname);
	output.insert(QCValue::ImageFromFile("base distribution plot", plotname, "Base distribution plot per cycle.", "QC:2000011"));
	QFile::remove(plotname);


	//create output quality distribution plot
	for(int j=0; j<qualities1_.count(); ++j)
	{
		int depth = pileups_[j].depth(false, true);
		//divide by 2 if paired-end reads
		if(c_reverse_ > 0) depth /= 2;
		qualities1_[j] /= depth;
		qualities2_[j] /= depth;
	}
	if (long_read_)
	{
		//short quality lines
		qualities1_ = qualities1_.mid(0, cycles);
		qualities2_ = qualities2_.mid(0, cycles);
	}

	LinePlot plot2;
	plot2.setXLabel("cycle");
	plot2.setYLabel("mean Q score");
	plot2.setYRange(0.0, 41.5);
	plot2.setXValues(line_x);
	plot2.addLine(qualities1_, "forward reads");
	if (c_reverse_>0)
	{
		plot2.addLine(qualities2_, "reverse reads");
	}
	QString plotname2 = Helper::tempFileName(".png");
	plot2.store(plotname2);
	output.insert(QCValue::ImageFromFile("Q score plot", plotname2, "Mean Q score per cycle for forward/reverse reads.", "QC:2000012"));
	QFile::remove(plotname2);

	// plot Q score distribution
	LinePlot plot2b;
	plot2b.setXLabel("read Q score");
	plot2b.setYLabel("read density");
	plot2b.setYRange(1, 1.1*std::max(qscore_dist_r1.maxValue(), qscore_dist_r2.maxValue()));
	plot2b.setXValues(qscore_dist_r1.xCoords());
	plot2b.addLine(qscore_dist_r1.yCoords(), "forward reads");
	if (c_reverse_>0)
	{
		plot2b.addLine(qscore_dist_r2.yCoords(), "reverse reads");
	}
	QString plotname2b = Helper::tempFileName(".png");
	plot2b.store(plotname2b);
	output.insert(QCValue::ImageFromFile("read Q score distribution", plotname2b, "Distrubition of the mean forward/reverse Q score for each read.", "QC:2000138"));
	QFile::remove(plotname2b);

	//calculate long read QC values:
	if(long_read_)
	{

		//create read length histogram
		QMapIterator<int,long long> it(read_lengths_);
		int hist_min = std::max(0, read_lengths_.firstKey() - 20);
		int hist_max = n95 + 20; //ignore super-long reads
		Histogram read_length_hist = Histogram(hist_min, hist_max, (hist_max-hist_min)/60);
		it.toFront();
		while(it.hasNext())
		{
			it.next();
			for (int i = 0; i < it.value(); ++i) read_length_hist.inc(it.key(), true);
		}

		//add read length histogram
		BarPlot plot3;
		plot3.setXLabel("read length (bp)");
		plot3.setYLabel("read counts");
		plot3.setYRange(0, (int) ((read_length_hist.maxValue() + 1) * 1.2));
		plot3.setXRange(-2, read_length_hist.binCount() + 2);
		QList<QString> bins;
		foreach (double x, read_length_hist.xCoords())
		{
			if(((int) x) % 10 == 0)
			{
				bins << QString::number((int) x);
			}
			else
			{
				bins << "";
			}
		}
        plot3.setValues(read_length_hist.yCoords().toList(), bins);
		QString plotname3 = Helper::tempFileName(".png");
		plot3.store(plotname3);
		output.insert(QCValue::ImageFromFile("Read length histogram", plotname3, "Histogram of read lengths", "QC:2000132"));
		QFile::remove(plotname3);

		//create QScore histogram
		//prepare data
		QList<QString> labels;
		QList<double> values;
		double max_count = 0;
		int mode_base_q_score = 0;
		int median_base_q_score = 0;
		bool median_found = false;
		long long bases_checked = 0;
		for (int i = 0; i <= 60; ++i) //limit plot to 60
		{
			labels << QString::number(i);
			values << base_qualities_[i];

			if (base_qualities_[i] >= max_count)
			{
				max_count = base_qualities_[i];
				if (i < 50) mode_base_q_score = i; //ignore peak at 50
			}

			// get median q score
			bases_checked += base_qualities_[i];
			if (!median_found && (bases_checked >= (bases_sequenced_ / 2)))
			{
				median_base_q_score = i;
				median_found = true;
			}
		}
		BarPlot plot4;
		plot4.setXLabel("Q score");
		plot4.setYLabel("base counts");
		plot4.setYRange(0, ((max_count + 1) * 1.1));
		plot4.setXRange(0, 60);
		plot4.setValues(values, labels);
		QString plotname4 = Helper::tempFileName(".png");
		plot4.store(plotname4);
		output.insert(QCValue::ImageFromFile("base Q score histogram", plotname4, "Histogram of base Q scores.", "QC:2000143"));
		QFile::remove(plotname4);
		output.insert(QCValue("median base Q score", median_base_q_score, "Median Q score of all bases of the sample.", "QC:2000144"));
		output.insert(QCValue("mode base Q score", mode_base_q_score, "Most frequent Q score of all bases of the sample.", "QC:2000145"));

		//get median/mode read q score
		max_count = 0;
		int mode_read_q_score = 0;
		int median_read_q_score = 0;
		median_found = false;
		long long reads_checked = 0;
		for (int i = 0; i < read_qualities_.size(); ++i)
		{

			if (read_qualities_[i] >= max_count)
			{
				max_count = read_qualities_[i];
				mode_read_q_score = i;
			}

			// get median q score
			reads_checked += read_qualities_[i];
			if (!median_found && (reads_checked >= (c_forward_ / 2)))
			{
				median_read_q_score = i;
				median_found = true;
			}
		}

		output.insert(QCValue("median read Q score", median_read_q_score, "Median Q score of all reads of the sample.", "QC:2000146"));
		output.insert(QCValue("mode read Q score", mode_read_q_score, "Most frequent Q score of all reads of the sample.", "QC:2000147"));
	}





	return output;
}
