#include "AnalysisWorker.h"
#include "cmath"
#include "NGSHelper.h"
#include "BasicStatistics.h"

AnalysisWorker::AnalysisWorker(AnalysisJob& job, TrimmingParameters& params, TrimmingStatistics& stats, ErrorCorrectionStatistics& ecstats)
	: QObject()
	, QRunnable()
	, job_(job)
	, params_(params)
	, stats_(stats)
	, ecstats_(ecstats)
{
}

AnalysisWorker::~AnalysisWorker()
{
}

void AnalysisWorker::correctErrors(int r, QTextStream& debug_out)
{
	int mm_count = 0;
    const int count = std::min(job_.r1[r].bases.size(), job_.r2[r].bases.size());
	for (int i=0; i<count; ++i)
	{
		const int i2 = count-i-1;

		//error detected
		if (job_.r1[r].bases[i]!=Sequence::complement(job_.r2[r].bases[i2]))
		{
			++mm_count;
			int q1 = job_.r1[r].quality(i, params_.qoff);
			int q2 = job_.r2[r].quality(i2, params_.qoff);

			//debug output
			if (params_.debug)
			{
				if (mm_count!=0)
				{
                    debug_out << "R1: " << job_.r1[r].bases << QT_ENDL;
                    debug_out << "Q1: "<< job_.r1[r].qualities << QT_ENDL;
                    debug_out << "R2: "<< job_.r2[r].bases << QT_ENDL;
                    debug_out << "Q2: "<< job_.r2[r].qualities << QT_ENDL;
				}
                debug_out << "  MISMATCH index=" << i << " R1=" << job_.r1[r].bases[i] << "/" << q1 << " R2=" << job_.r2[r].bases[i2]<< "/" << q2 << QT_ENDL;
			}

			//correct error
			if (q1>q2)
			{
				char replacement = Sequence::complement(job_.r1[r].bases[i]);
				if (params_.debug)
				{
                    debug_out << "    CORRECTED R2: " << job_.r2[r].bases[i2] << " => " << replacement << QT_ENDL;
				}
				job_.r2[r].bases[i2] = replacement;
				job_.r2[r].qualities[i2] = job_.r1[r].qualities[i];
				++ecstats_.mismatch_r2[i2];
			}
			else if(q1<q2)
			{
				char replacement = Sequence::complement(job_.r2[r].bases[i2]);
				if (params_.debug)
				{
                    debug_out << "    CORRECTED R1: " << job_.r1[r].bases[i] << " => " << replacement << QT_ENDL;
				}
				job_.r1[r].bases[i] = replacement;
				job_.r1[r].qualities[i] = job_.r2[r].qualities[i2];
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
	try
	{
		QTextStream debug_out(stdout);

		//update raw data statistics (before trimming)
		if (!params_.qc.isEmpty())
		{
			stats_.qc_mutex.lock();
			for (int r=0; r<job_.read_count; ++r)
			{
				stats_.qc.update(job_.r1[r], StatisticsReads::FORWARD);
				stats_.qc.update(job_.r2[r], StatisticsReads::REVERSE);
			}
			stats_.qc_mutex.unlock();
		}

		for (int r=0; r<job_.read_count; ++r)
		{
			if (params_.debug)
			{
                debug_out << "#############################################################################" << QT_ENDL;
                debug_out << "Header:     " << job_.r1[r].header << QT_ENDL;
                debug_out << "Read 1 in:  " << job_.r1[r].bases << QT_ENDL;
                debug_out << "Read 2 in:  " << job_.r2[r].bases << QT_ENDL;
                debug_out << "Quality 1:  " << job_.r1[r].qualities << QT_ENDL;
                debug_out << "Quality 2:  " << job_.r2[r].qualities << QT_ENDL;
			}

			//check that headers match
			QByteArray tmp1 = job_.r1[r].header.split(' ').at(0);
			QByteArray tmp2 = job_.r2[r].header.split(' ').at(0);
			if (tmp1.endsWith("/1") && tmp2.endsWith("/2"))
			{
				tmp1.chop(2);
				tmp2.chop(2);
			}
			if (tmp1!=tmp2)
			{
				THROW(ArgumentException, "Headers of reads do not match:\n" + tmp1 + "\n" + tmp2);
			}

			//make sure the sequences have the same length
			Sequence seq1 = job_.r1[r].bases;
			Sequence seq2 = job_.r2[r].bases.toReverseComplement();
            job_.length_r1_orig[r] = seq1.size();
            job_.length_r2_orig[r] = seq2.size();
			int min_length = std::min(job_.length_r1_orig[r], job_.length_r2_orig[r]);
			int max_length = std::max(job_.length_r1_orig[r], job_.length_r2_orig[r]);

			//check length
			if (max_length>=MAXLEN)
			{
				THROW(ArgumentException, "Read length unsupported! A maximum read length of " + QString::number(MAXLEN) + " is supported!");
			}

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

				if ((matches + mismatches)==0 || 100.0*matches/(matches + mismatches) < params_.match_perc) continue;
				if (params_.debug)
				{
                    debug_out << "  offset: " << offset << QT_ENDL;
                    debug_out << "  match_perc: " << (100.0*matches/(matches + mismatches)) << "%" << QT_ENDL;
				}

				//calculate the probability of seeing n or more matches at random
				double p = BasicStatistics::matchProbability(0.25, matches, matches+mismatches);
				if (p>params_.mep) continue;
                if (params_.debug) debug_out << "  mep: " << p << QT_ENDL;

				//check that at least on one side the adapter is present - if not continue
				QByteArray adapter1 = seq1.mid(job_.length_r2_orig[r]-offset, params_.adapter_overlap);
				int a1_matches = 0;
				int a1_mismatches = 0;
				int a1_invalid = 0;
                for (int i=0; i<adapter1.size(); ++i)
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

				QByteArray adapter2 = seq2.left(offset).toReverseComplement().left(params_.adapter_overlap);
				int a2_matches = 0;
				int a2_mismatches = 0;
				int a2_invalid = 0;
                for (int i=0; i<adapter2.size(); ++i)
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
                        if (params_.debug) debug_out << "  adapter overlap passed! mismatches1:" << a1_mismatches << " mismatches2:" << a2_mismatches << QT_ENDL;
					}
					else
					{
                        if (params_.debug) debug_out << "  adapter overlap failed! mismatches1:" << a1_mismatches << " mismatches2:" << a2_mismatches << QT_ENDL;
						continue;
					}
				}
				else //when the adapter fragment is short => require non-random adapter sequence hit
				{
					double p1 = BasicStatistics::matchProbability(0.25, a1_matches, a1_matches+a1_mismatches);
					double p2 = BasicStatistics::matchProbability(0.25, a2_matches, a2_matches+a2_mismatches);
					if (p1*p2>params_.mep)
					{
                        if (params_.debug) debug_out << "  adapter overlap failed! mep1:" << p1 << " mep2:" << p2 << QT_ENDL;
						continue;
					}
					else
					{
                        if (params_.debug) debug_out << "  adapter overlap passed! mep1:" << p1 << " mep2:" << p2 << QT_ENDL;
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
				int new_length = job_.length_r2_orig[r]-best_offset;
				job_.r1[r].bases.truncate(new_length);
				job_.r1[r].qualities.truncate(new_length);
				job_.r2[r].bases.truncate(new_length);
				job_.r2[r].qualities.truncate(new_length);

				//update consensus adapter sequence
				QByteArray adapter1 = seq1.mid(new_length);
                if (adapter1.size()>40) adapter1.resize(40);
                for (int i=0; i<adapter1.size(); ++i)
				{
					stats_.acons1[i].inc(adapter1.at(i));
				}
				QByteArray adapter2 = seq2.left(best_offset).toReverseComplement();
                if (adapter2.size()>40) adapter2.resize(40);
                for (int i=0; i<adapter2.size(); ++i)
				{
					stats_.acons2[i].inc(adapter2.at(i));
				}

				//update statistics
				job_.reads_trimmed_insert += 2.0;

				if (params_.debug)
				{
                    debug_out << "###Insert sequence hit - offset=" << best_offset << " prob=" << best_p << " adapter1=" << adapter1 << " adapter2=" << adapter2 << QT_ENDL;
				}

				//error correction
				if (params_.ec) correctErrors(r, debug_out);
			}

			//step 2: trim by adapter match - forward read
			else
			{
				int offset_forward = -1;
				const char* a1_data = params_.a1.constData();
				for (int offset=0; offset<job_.length_r1_orig[r]; ++offset)
				{
					int matches = 0;
					int mismatches = 0;
					int invalid = 0;
					for (int i=0; i<params_.a_size; ++i)
					{
						if (offset+i>=job_.length_r1_orig[r]) break;

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
					double p = BasicStatistics::matchProbability(0.25, matches, matches+mismatches);
					if (p>params_.mep) continue;

					//debug output
					if (params_.debug)
					{
						QByteArray adapter = job_.r1[r].bases.right(job_.length_r1_orig[r]-offset);
						adapter.truncate(20);
                        debug_out << "###Adapter 1 hit - offset=" << offset << " prob=" << p << " matches=" << matches << " mismatches=" << mismatches << " invalid=" << invalid << " adapter=" << adapter << QT_ENDL;
					}

					//trim read
					job_.r1[r].bases.truncate(offset);
					job_.r1[r].qualities.truncate(offset);
					offset_forward = offset;

					break;
				}

				//step 3: trim by adapter match - reverse read
				int offset_reverse = -1;
				seq2 = job_.r2[r].bases;
				seq2_data = seq2.constData();
				const char* a2_data = params_.a2.constData();
				for (int offset=0; offset<job_.length_r2_orig[r]; ++offset)
				{
					int matches = 0;
					int mismatches = 0;
					int invalid = 0;
					for (int i=0; i<params_.a_size; ++i)
					{
						if (offset+i>=job_.length_r2_orig[r]) break;

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
					double p = BasicStatistics::matchProbability(0.25, matches, matches+mismatches);
					if (p>params_.mep) continue;

					//debug output
					if (params_.debug)
					{
						QByteArray adapter = job_.r2[r].bases.right(job_.length_r2_orig[r]-offset);
						adapter.truncate(20);
                        debug_out << "###Adapter 2 hit - offset=" << offset << " prob=" << p << " matches=" << matches << " mismatches=" << mismatches << " invalid=" << invalid << " adapter=" << adapter << QT_ENDL;
					}

					//trim read
					job_.r2[r].bases.truncate(offset);
					job_.r2[r].qualities.truncate(offset);

					//update statistics
					offset_reverse = offset;

					break;
				}

				//we have found at least one adapter hit => react on it
				if (offset_forward!=-1 || offset_reverse!=-1)
				{
					//update statistics
					job_.reads_trimmed_adapter += 2;

					//if only one adapter has been trimmed => trim the other read as well
					if (offset_forward==-1)
					{
						job_.r1[r].bases.truncate(offset_reverse);
						job_.r1[r].qualities.truncate(offset_reverse);
					}
					if (offset_reverse==-1)
					{
						job_.r2[r].bases.truncate(offset_forward);
						job_.r2[r].qualities.truncate(offset_forward);
					}
				}
			}

			//quality trimming
			if (params_.qcut>0)
			{
				if (job_.r1[r].trimQuality(params_.qcut, params_.qwin, params_.qoff)>0) ++job_.reads_trimmed_q;
				if (job_.r2[r].trimQuality(params_.qcut, params_.qwin, params_.qoff)>0) ++job_.reads_trimmed_q;
			}

			//N trimming
			if (params_.ncut>0)
			{
				if (job_.r1[r].trimN(params_.ncut)>0) ++job_.reads_trimmed_n;
				if (job_.r2[r].trimN(params_.ncut)>0) ++job_.reads_trimmed_n;
			}

			if (params_.debug)
			{
                debug_out << "Read 1 out: " << job_.r1[r].bases << QT_ENDL;
                debug_out << "Read 2 out: " << job_.r2[r].bases << QT_ENDL;
			}
		}

		job_.status = TO_BE_WRITTEN;
		emit done(job_.index);
	}
	catch(Exception& e)
	{
		emit error(job_.index, e.message());
	}
}
