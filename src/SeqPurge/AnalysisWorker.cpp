#include "AnalysisWorker.h"
#include "cmath"
#include "NGSHelper.h"
#include "BasicStatistics.h"

QVector<double> AnalysisWorker::fak_cache = QVector<double>();

AnalysisWorker::AnalysisWorker(FastqEntry* e1, FastqEntry* e2, TrimmingParameters& params, TrimmingStatistics& stats, TrimmingData& data)
    : QRunnable()
	, e1_(e1)
	, e2_(e2)
	, params_(params)
	, stats_(stats)
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
		double q = std::pow(0.75, count-i) * std::pow(0.25, i) * fak(count) / fak(matches) / fak(mismatches);
		p += q;
	}

	//check that result is valid
	if (!BasicStatistics::isValidFloat(p))
	{
		THROW(ProgrammingException, "Calculated probabilty for " + QString::number(matches) + " matches and " + QString::number(mismatches) + " mismatches is not a valid float!");
	}

	return p;
}

void AnalysisWorker::run()
{	
	//check that headers match
	QByteArray h1 = e1_->header.split(' ').at(0);
	QByteArray h2 = e2_->header.split(' ').at(0);
	if (h1!=h2)
	{
		if (h1.endsWith("/1") && h2.endsWith("/2"))
		{
			h1.chop(2);
			h2.chop(2);
			if (h1!=h2)
			{
				THROW(Exception, "Headers of read do not match:\n" + h1 + "\n" + h2);
			}
		}
	}

	if (params_.debug)
	{
		qDebug() << "#############################################################################";
		qDebug() << "Header:     " << h1;
		qDebug() << "Read 1 in:  " << e1_->bases;
		qDebug() << "Read 2 in:  " << e2_->bases;
		qDebug() << "Quality 1:  " << e1_->qualities;
		qDebug() << "Quality 2:  " << e2_->qualities;
	}

	//make sure the sequences have the same length
	QByteArray seq1 = e1_->bases;
	QByteArray seq2 = NGSHelper::changeSeq(e2_->bases, true, true);
	int length_s1_orig = seq1.count();
	int length_s2_orig = seq2.count();
	if (length_s1_orig>length_s2_orig)
	{
		seq2 = seq2.leftJustified(length_s1_orig, 'N');
	}
	else if (length_s1_orig<length_s2_orig)
	{
		seq1 = seq1.leftJustified(length_s2_orig, 'N');
	}

	//check length
	int length = seq1.count();
	if (length+1>stats_.bases_remaining.capacity())
	{
		THROW(ProgrammingException, "Read length unsupported! A maximum read length of " + QString::number(stats_.bases_remaining.capacity()) + " is supported!");
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
	for (int offset=1; offset<length; ++offset)
	{
		//optimization: we can abort when we have reached the maximum possible number of mismatches
		//              for the current offset and read length. Like that we can avoid about 75% of
		//              the base comparisons we would actually have to make.
		int max_mismatches = (int)(std::ceil((1.0-params_.match_perc/100.0) * (length-offset)));

		int matches = 0;
		int mismatches = 0;
		int invalid = 0;
		for (int j=offset; j<length; ++j)
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
		//qDebug() << offset << matches << mismatches << (100.0*matches/(matches + mismatches));

		if (100.0*matches/(matches + mismatches) <= params_.match_perc) continue;
		if (params_.debug)
		{
			qDebug() << "  offset: " << offset;
			qDebug() << "  match_perc: " << (100.0*matches/(matches + mismatches)) << "%";
		}

		//calculate the probability of seeing n or more matches at random
		double p = matchProbability(matches, mismatches);
		if (p>params_.mep) continue;
		if (params_.debug) qDebug() << "  mep: " << p;

		//check that at least on one side the adapter is present - if not continue
		if (offset>=10)
		{
			QByteArray adapter1 = seq1.mid(length - offset, params_.adapter_overlap);
			int matches = 0;
			int mismatches = 0;
			int invalid = 0;
			for (int i=0; i<params_.adapter_overlap; ++i)
			{
				//forward
				char b1 = adapter1.constData()[i];
				char b2 = params_.a1.constData()[i];

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
			double p1 = matchProbability(matches, mismatches);

			QByteArray adapter2 = NGSHelper::changeSeq(seq2.left(offset), true, true).left(params_.adapter_overlap);
			matches = 0;
			mismatches = 0;
			invalid = 0;
			for (int i=0; i<params_.adapter_overlap; ++i)
			{
				//forward
				char b1 = adapter2.constData()[i];
				char b2 = params_.a2.constData()[i];

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
			double p2 = matchProbability(matches, mismatches);
			if (p1*p2>params_.mep)
			{
				if (params_.debug) qDebug() << "  adatper overlap failed: " << p1 << p2;
				continue;
			}
			else
			{
				if (params_.debug) qDebug() << "  adatper overlap passed: " << p1 << p2;
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
		int new_length_s1 = std::min(length-best_offset, length_s1_orig);
		e1_->bases.resize(new_length_s1);
		e1_->qualities.resize(new_length_s1);
		int new_length_s2 = std::min(length-best_offset, length_s2_orig);
		e2_->bases.resize(new_length_s2);
		e2_->qualities.resize(new_length_s2);

		//update consensus adapter sequence
		QByteArray adapter1 = seq1.mid(length - best_offset);
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

		if (params_.debug) qDebug() << "###Insert sequence hit - offset=" << best_offset << " prob=" << best_p << " adapter1= " << adapter1 << " adapter2=" << adapter2;
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

			if (100.0*matches/(matches+mismatches) <= params_.match_perc) continue;
			double p = matchProbability(matches, mismatches);
			if (p>params_.mep) continue;

			//update consensus adapter sequence
			QByteArray adapter = e1_->bases.right(length_s1_orig-offset);
			if (adapter.count()>40) adapter.resize(40);
			for (int i=0; i<adapter.count(); ++i)
			{
				stats_.acons1[i].inc(adapter.at(i));
			}

			//trim read
			e1_->bases.resize(offset);
			e1_->qualities.resize(offset);
			offset_forward = offset;

			if (params_.debug) qDebug() << "###Adapter 1 hit - offset=" << offset << " prob=" << p << " matches=" << matches << " mismatches=" << mismatches << " invalid=" << invalid << " adapter=" << adapter;
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

			if (100.0*matches/(matches+mismatches) <= params_.match_perc) continue;
			double p = matchProbability(matches, mismatches);
			if (p>params_.mep) continue;

			//update consensus adapter sequence
			QByteArray adapter = e2_->bases.right(length_s2_orig-offset);
			if (adapter.count()>40) adapter.resize(40);
			for (int i=0; i<adapter.count(); ++i)
			{
				stats_.acons2[i].inc(adapter.at(i));
			}

			//trim read
			e2_->bases.resize(offset);
			e2_->qualities.resize(offset);

			//update statistics
			offset_reverse = offset;

			if (params_.debug)
			{
				qDebug() << "###Adapter 2 hit - offset=" << offset << " prob=" << p << " matches=" << matches << " mismatches=" << mismatches << " invalid=" << invalid << " adapter=" << adapter;
			}
			break;
		}

		//we have found at least one adapter hit => react on it
		if (offset_forward!=-1 || offset_reverse!=-1)
		{
			//update statistics
			reads_trimmed_adapter += 2;

			//if only one adapter has been trimmed => trim the other read as well
			if (offset_forward==-1)
			{
				e1_->bases.resize(offset_reverse);
				e1_->qualities.resize(offset_reverse);
			}
			if (offset_reverse==-1)
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
		qDebug() << "Read 1 out: " << e1_->bases;
		qDebug() << "Read 2 out: " << e2_->bases;
	}

	//write output
	if (e1_->bases.count()>=params_.min_len_s &&
		e2_->bases.count()>=params_.min_len_s &&
		e1_->bases.count() + e2_->bases.count()>=params_.min_len_p)
	{
		data_.out1_out2_mutex.lock();
		data_.out1->write(*e1_);
		data_.out2->write(*e2_);
		data_.out1_out2_mutex.unlock();
	}
	else if (data_.out3 && e1_->bases.count()>=params_.min_len_s)
	{
		reads_removed += 1;
		data_.out3->write(*e1_);
	}
	else if (data_.out4 && e2_->bases.count()>=params_.min_len_s)
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
	if (length+1>stats_.bases_remaining.count())
	{
		stats_.bases_remaining.resize(length+1);
	}
	stats_.bases_remaining[e1_->bases.length()] += 1;
	stats_.bases_remaining[e2_->bases.length()] += 1;
	stats_.bases_perc_trim_sum += (double)(length_s1_orig - e1_->bases.count()) / length_s1_orig;
	stats_.bases_perc_trim_sum += (double)(length_s2_orig - e2_->bases.count()) / length_s2_orig;
	stats_.mutex.unlock();

	//delete data
	delete e1_;
	delete e2_;
}
