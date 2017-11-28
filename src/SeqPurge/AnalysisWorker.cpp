#include "AnalysisWorker.h"
#include "cmath"
#include "NGSHelper.h"
#include "BasicStatistics.h"

QVector<double> AnalysisWorker::fak_cache = QVector<double>();

AnalysisWorker::AnalysisWorker(QSharedPointer<FastqEntry> e1, QSharedPointer<FastqEntry> e2, TrimmingParameters& params, TrimmingStatistics& stats, ErrorCorrectionStatistics& ecstats, TrimmingData& data)
    : QRunnable()
	, e1_(e1)
	, e2_(e2)
	, params_(params)
	, stats_(stats)
	, ecstats_(ecstats)
	, data_(data)
{
}

AnalysisWorker::~AnalysisWorker()
{
}

void AnalysisWorker::precalculateFactorials()
{
	if (!fak_cache.isEmpty()) return;

	//calculate faktorials until double overflow happens
	int i = 0;
	double value = 1.0;
	while(BasicStatistics::isValidFloat(value))
	{
		fak_cache.append(value);
		++i;
		value *= i;
	}
}

double AnalysisWorker::fak(int n)
{
	//not in cache (i.e. double overflow) => NAN
	if (fak_cache.count()<n+1)
	{
		return std::numeric_limits<double>::quiet_NaN();
	}

	return fak_cache[n];
}

double AnalysisWorker::matchProbability(int matches, int mismatches)
{
	//handle double overflow of factorial (approximately at 160)
	int count = matches + mismatches;
	while(!BasicStatistics::isValidFloat(fak(count)))
	{
		matches /= 2;
		mismatches /=2;
		count = matches + mismatches;
	}

	//calculate probability
	double p = 0.0;
	for (int i=matches; i<=count; ++i)
	{
		double q = std::pow(0.75, count-i) * std::pow(0.25, i) * fak(count) / fak(i) / fak(count-i);
		p += q;
	}

	//check that result is valid
	if (!BasicStatistics::isValidFloat(p))
	{
		THROW(ProgrammingException, "Calculated probabilty for " + QString::number(matches) + " matches and " + QString::number(mismatches) + " mismatches is not a valid float!");
	}

	return p;
}

void AnalysisWorker::checkHeaders(const QByteArray& h1, const QByteArray& h2)
{
	QByteArray tmp1 = h1.split(' ').at(0);
	QByteArray tmp2 = h2.split(' ').at(0);
	if (tmp1.endsWith("/1") && tmp2.endsWith("/2"))
	{
		tmp1.chop(2);
		tmp2.chop(2);
	}
	if (tmp1!=tmp2)
	{
		THROW(Exception, "Headers of reads do not match:\n" + tmp1 + "\n" + tmp2);
	}
}

void AnalysisWorker::correctErrors(QTextStream& debug_out)
{
	int mm_count = 0;
	const int count = std::min(e1_->bases.count(), e2_->bases.count());
	for (int i=0; i<count; ++i)
	{
		const int i2 = count-i-1;

		//error detected
		if (e1_->bases[i]!=NGSHelper::complement(e2_->bases[i2]))
		{
			++mm_count;
			int q1 = e1_->quality(i, params_.qoff);
			int q2 = e2_->quality(i2, params_.qoff);

			//debug output
			if (params_.debug)
			{
				if (mm_count!=0)
				{
					debug_out << "R1: " << e1_->bases << endl;
					debug_out << "Q1: "<< e1_->qualities << endl;
					debug_out << "R2: "<< e2_->bases << endl;
					debug_out << "Q2: "<< e2_->qualities << endl;
				}
				debug_out << "  MISMATCH index=" << i << " R1=" << e1_->bases[i] << "/" << q1 << " R2=" << e2_->bases[i2]<< "/" << q2 << endl;
			}

			//correct error
			if (q1>q2)
			{
				char replacement = NGSHelper::complement(e1_->bases[i]);
				if (params_.debug)
				{
					debug_out << "    CORRECTED R2: " << e2_->bases[i2] << " => " << replacement << endl;
				}
				e2_->bases[i2] = replacement;
				e2_->qualities[i2] = e1_->qualities[i];
				++ecstats_.mismatch_r2[i2];
			}
			else if(q1<q2)
			{
				char replacement = NGSHelper::complement(e2_->bases[i2]);
				if (params_.debug)
				{
					debug_out << "    CORRECTED R1: " << e1_->bases[i] << " => " << replacement << endl;
				}
				e1_->bases[i] = replacement;
				e1_->qualities[i] = e2_->qualities[i2];
				++ecstats_.mismatch_r1[i];
			}
		}
	}

	if (mm_count>0)
	{
		++ecstats_.errors_per_read[mm_count];
	}
}

void AnalysisWorker::run()
{	
	QTextStream debug_out(stdout);

	//check that headers match
	checkHeaders(e1_->header, e2_->header);

	if (params_.debug)
	{
		debug_out << "#############################################################################" << endl;
		debug_out << "Header:     " << e1_->header << endl;
		debug_out << "Read 1 in:  " << e1_->bases << endl;
		debug_out << "Read 2 in:  " << e2_->bases << endl;
		debug_out << "Quality 1:  " << e1_->qualities << endl;
		debug_out << "Quality 2:  " << e2_->qualities << endl;
	}

	//make sure the sequences have the same length
	QByteArray seq1 = e1_->bases;
	QByteArray seq2 = NGSHelper::changeSeq(e2_->bases, true, true);
	int length_s1_orig = seq1.count();
	int length_s2_orig = seq2.count();
	int min_length = std::min(length_s1_orig, length_s2_orig);
	int max_length = std::max(length_s1_orig, length_s2_orig);
	
	//check length
	if (max_length>=MAXLEN)
	{
		THROW(ProgrammingException, "Read length unsupported! A maximum read length of " + QString::number(MAXLEN) + " is supported!");
	}

	//update QC statistics (has to be done before trimming)
	if (!params_.qc.isEmpty())
	{
		stats_.qc.update(*e1_, StatisticsReads::FORWARD);
		stats_.qc.update(*e2_, StatisticsReads::REVERSE);
	}

	//init statistics
	int reads_trimmed_insert = 0;
	int reads_trimmed_adapter = 0;
	int reads_trimmed_q = 0;
	int reads_trimmed_n = 0;
	int reads_removed = 0;

	//step 1: trim by insert match
	int best_offset = -1;
	double best_p = 1.0;
	const char* seq1_data = seq1.constData();
	const char* seq2_data = seq2.constData();
	for (int offset=1; offset<min_length; ++offset)
	{
		//optimization: we can abort when we have reached the maximum possible number of mismatches
		//              for the current offset and read length. Like that we can avoid about 75% of
		//              the base comparisons we would actually have to make.
		int max_mismatches = (int)(std::ceil((1.0-params_.match_perc/100.0) * (min_length-offset)));

		int matches = 0;
		int mismatches = 0;
		int invalid = 0;
		for (int j=offset; j<min_length; ++j)
		{
			char b1 = seq1_data[j-offset];
			char b2 = seq2_data[j];
			if (b1=='N' || b2=='N')
			{
				++invalid;
			}
			else if (b1==b2)
			{
				++matches;
			}
			else
			{
				++mismatches;
				if (mismatches>max_mismatches) break;
			}
		}
		//debug_out << offset << matches << mismatches << (100.0*matches/(matches + mismatches)) << endl;

		if ((matches + mismatches)==0 || 100.0*matches/(matches + mismatches) < params_.match_perc) continue;
		if (params_.debug)
		{
			debug_out << "  offset: " << offset << endl;
			debug_out << "  match_perc: " << (100.0*matches/(matches + mismatches)) << "%" << endl;
		}

		//calculate the probability of seeing n or more matches at random
		double p = matchProbability(matches, mismatches);
		if (p>params_.mep) continue;
		if (params_.debug) debug_out << "  mep: " << p << endl;

		//check that at least on one side the adapter is present - if not continue
		QByteArray adapter1 = seq1.mid(length_s2_orig-offset, params_.adapter_overlap);
		int a1_matches = 0;
		int a1_mismatches = 0;
		int a1_invalid = 0;
		for (int i=0; i<adapter1.count(); ++i)
		{
			//forward
			char b1 = adapter1.constData()[i];
			char b2 = params_.a1.constData()[i];

			if (b1=='N' || b2=='N')
			{
				++a1_invalid;
			}
			else if (b1==b2)
			{
				++a1_matches;
			}
			else
			{
				++a1_mismatches;
			}
		}

		QByteArray adapter2 = NGSHelper::changeSeq(seq2.left(offset), true, true).left(params_.adapter_overlap);
		int a2_matches = 0;
		int a2_mismatches = 0;
		int a2_invalid = 0;
		for (int i=0; i<adapter2.count(); ++i)
		{
			//forward
			char b1 = adapter2.constData()[i];
			char b2 = params_.a2.constData()[i];

			if (b1=='N' || b2=='N')
			{
				++a2_invalid;
			}
			else if (b1==b2)
			{
				++a2_matches;
			}
			else
			{
				++a2_mismatches;
			}
		}

		if (offset<10) //when the adapter fragment is short => check only number of mismatches
		{
			int max_mm = 2;
			if (offset<6) max_mm = 1;
			if (offset<3) max_mm = 0;
			if (a1_mismatches<=max_mm || a2_mismatches<=max_mm)
			{
				if (params_.debug) debug_out << "  adapter overlap passed! mismatches1:" << a1_mismatches << " mismatches2:" << a2_mismatches << endl;
			}
			else
			{
				if (params_.debug) debug_out << "  adapter overlap failed! mismatches1:" << a1_mismatches << " mismatches2:" << a2_mismatches << endl;
				continue;
			}
		}
		else //when the adapter fragment is short => require non-random adapter sequence hit
		{
			double p1 = matchProbability(a1_matches, a1_mismatches);
			double p2 = matchProbability(a2_matches, a2_mismatches);
			if (p1*p2>params_.mep)
			{
				if (params_.debug) debug_out << "  adapter overlap failed! mep1:" << p1 << " mep2:" << p2 << endl;
				continue;
			}
			else
			{
				if (params_.debug) debug_out << "  adapter overlap passed! mep1:" << p1 << " mep2:" << p2 << endl;
			}
		}

		if (p<best_p)
		{
			best_p = p;
			best_offset = offset;
		}
	}

	//we have found insert match => remove adapter
	if (best_offset!=-1)
	{
		//update sequence data
		int new_length = length_s2_orig-best_offset;

		if (e1_->bases.count()>new_length)
		{
			e1_->bases.resize(new_length);
			e1_->qualities.resize(new_length);
		}
		if (e2_->bases.count()>new_length)
		{
			e2_->bases.resize(new_length);
			e2_->qualities.resize(new_length);
		}

		//update consensus adapter sequence
		QByteArray adapter1 = seq1.mid(new_length);
		if (adapter1.count()>40) adapter1.resize(40);
		for (int i=0; i<adapter1.count(); ++i)
		{
			stats_.acons1[i].inc(adapter1.at(i));
		}
		QByteArray adapter2 = NGSHelper::changeSeq(seq2.left(best_offset), true, true);
		if (adapter2.count()>40) adapter2.resize(40);
		for (int i=0; i<adapter2.count(); ++i)
		{
			stats_.acons2[i].inc(adapter2.at(i));
		}

		//update statistics
		reads_trimmed_insert += 2.0;

		if (params_.debug)
		{
			debug_out << "###Insert sequence hit - offset=" << best_offset << " prob=" << best_p << " adapter1=" << adapter1 << " adapter2=" << adapter2 << endl;
		}

		//error correction
		if (params_.ec) correctErrors(debug_out);
	}

	//step 2: trim by adapter match - forward read
	else
	{
		int offset_forward = -1;
		const char* a1_data = params_.a1.constData();
		for (int offset=0; offset<length_s1_orig; ++offset)
		{
			int matches = 0;
			int mismatches = 0;
			int invalid = 0;
			for (int i=0; i<params_.a_size; ++i)
			{
				if (offset+i>=length_s1_orig) break;

				//forward
				char b1 = seq1_data[offset+i];
				char b2 = a1_data[i];

				if (b1=='N' || b2=='N')
				{
					++invalid;
				}
				else if (b1==b2)
				{
					++matches;
				}
				else
				{
					++mismatches;
				}
			}
			if (100.0*matches/(matches+mismatches) < params_.match_perc) continue;
			double p = matchProbability(matches, mismatches);
			if (p>params_.mep) continue;

			//debug output
			if (params_.debug)
			{
				QByteArray adapter = e1_->bases.right(length_s1_orig-offset);
				adapter.truncate(20);
				debug_out << "###Adapter 1 hit - offset=" << offset << " prob=" << p << " matches=" << matches << " mismatches=" << mismatches << " invalid=" << invalid << " adapter=" << adapter << endl;
			}

			//trim read
			e1_->bases.resize(offset);
			e1_->qualities.resize(offset);
			offset_forward = offset;

			break;
		}

		//step 3: trim by adapter match - reverse read
		int offset_reverse = -1;
		seq2 = e2_->bases;
		seq2_data = seq2.constData();
		const char* a2_data = params_.a2.constData();
		for (int offset=0; offset<length_s2_orig; ++offset)
		{
			int matches = 0;
			int mismatches = 0;
			int invalid = 0;
			for (int i=0; i<params_.a_size; ++i)
			{
				if (offset+i>=length_s2_orig) break;

				//forward
				char b1 = seq2_data[offset+i];
				char b2 = a2_data[i];

				if (b1=='N' || b2=='N')
				{
					++invalid;
				}
				else if (b1==b2)
				{
					++matches;
				}
				else
				{
					++mismatches;
				}
			}

			if (100.0*matches/(matches+mismatches) < params_.match_perc) continue;
			double p = matchProbability(matches, mismatches);
			if (p>params_.mep) continue;

			//debug output
			if (params_.debug)
			{
				QByteArray adapter = e2_->bases.right(length_s2_orig-offset);
				adapter.truncate(20);
				debug_out << "###Adapter 2 hit - offset=" << offset << " prob=" << p << " matches=" << matches << " mismatches=" << mismatches << " invalid=" << invalid << " adapter=" << adapter << endl;
			}

			//trim read
			e2_->bases.resize(offset);
			e2_->qualities.resize(offset);

			//update statistics
			offset_reverse = offset;

			break;
		}

		//we have found at least one adapter hit => react on it
		if (offset_forward!=-1 || offset_reverse!=-1)
		{
			//update statistics
			reads_trimmed_adapter += 2;

			//if only one adapter has been trimmed => trim the other read as well
			if (offset_forward==-1 && e1_->bases.count()>offset_reverse)
			{
				e1_->bases.resize(offset_reverse);
				e1_->qualities.resize(offset_reverse);
			}
			if (offset_reverse==-1 && e2_->bases.count()>offset_forward)
			{
				e2_->bases.resize(offset_forward);
				e2_->qualities.resize(offset_forward);
			}
		}
	}

	//quality trimming
	if (params_.qcut>0)
	{
		if (e1_->trimQuality(params_.qcut, params_.qwin, params_.qoff)>0) ++reads_trimmed_q;
		if (e2_->trimQuality(params_.qcut, params_.qwin, params_.qoff)>0) ++reads_trimmed_q;
	}

	//N trimming
	if (params_.ncut>0)
	{
		if (e1_->trimN(params_.ncut)>0) ++reads_trimmed_n;
		if (e2_->trimN(params_.ncut)>0) ++reads_trimmed_n;
	}

	if (params_.debug)
	{
		debug_out << "Read 1 out: " << e1_->bases << endl;
		debug_out << "Read 2 out: " << e2_->bases << endl;
	}

	//write output
	if (e1_->bases.count()>=params_.min_len && e2_->bases.count()>=params_.min_len)
	{
		data_.out1_out2_mutex.lock();
		data_.out1->write(*e1_);
		data_.out2->write(*e2_);
		data_.out1_out2_mutex.unlock();
	}
	else if (data_.out3!=nullptr && e1_->bases.count()>=params_.min_len)
	{
		reads_removed += 1;
		data_.out3->write(*e1_);
	}
	else if (data_.out4!=nullptr && e2_->bases.count()>=params_.min_len)
	{
		reads_removed += 1;
		data_.out4->write(*e2_);
	}
	else
	{
		reads_removed += 2;
	}

	//update statistics (mutex to make it thread-safe)
	stats_.mutex.lock();
	stats_.read_num += 2;
	stats_.reads_trimmed_insert += reads_trimmed_insert;
	stats_.reads_trimmed_adapter += reads_trimmed_adapter;
	stats_.reads_trimmed_n += reads_trimmed_n;
	stats_.reads_trimmed_q += reads_trimmed_q;
	stats_.reads_removed += reads_removed;
	stats_.bases_remaining[e1_->bases.length()] += 1;
	stats_.bases_remaining[e2_->bases.length()] += 1;
	if (length_s1_orig>0)
	{
		stats_.bases_perc_trim_sum += (double)(length_s1_orig - e1_->bases.count()) / length_s1_orig;
	}
	if (length_s2_orig>0)
	{
		stats_.bases_perc_trim_sum += (double)(length_s2_orig - e2_->bases.count()) / length_s2_orig;
	}
	stats_.mutex.unlock();
}
