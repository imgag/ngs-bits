#include "ChunkProcessor.h"
#include "VcfFile.h"
#include "TabixIndexedFile.h"
#include <zlib.h>
#include <QFileInfo>

#include "ToolBase.h"
#include "Exceptions.h"
#include "Helper.h"
#include "Transcript.h"
#include <QDebug>
#include <QHash>
#include <QRegularExpressionMatchIterator>
#include "NGSHelper.h"
#include "BasicStatistics.h"

ChunkProcessor::ChunkProcessor(AnalysisJob& job, const MetaData& meta, const Parameters& params)
	: QObject()
	, QRunnable()
	, job_(job)
	, meta_(meta)
	, params_(params)
	, reference_(meta.reference)
	
{
	timer_.start();
	if (params_.debug) QTextStream(stdout) << "ChunkProcessor(): " << job_.index << endl;
}

ChunkProcessor::~ChunkProcessor()
{
	if (params_.debug) QTextStream(stdout) << "~ChunkProcessor(): " << job_.index << " (processing time:" << Helper::elapsedTime(timer_) << ")" << endl;
}

// utility functions
int ChunkProcessor::hashseq(const QByteArray& sequence)
{
	QList<int> pow_four{1,4,16,64,256,1024,4096,16384};
    int result = 0;
    int seqlength = sequence.length();
	for (int i = 0; i<seqlength; i++)
	{
        result += base_to_int(sequence[i]) * pow_four[seqlength - i - 1];
    }
    return result;
}


bool ChunkProcessor::is_valid_sequence(const Sequence& sequence)
{
	return QRegExp("[ACGT]*").exactMatch(sequence);
}

int ChunkProcessor::base_to_int(char base)
{
	switch(base)
	{
		case 'A':
			return 0;
		case 'C':
			return 1;
		case 'G':
			return 2;
		case 'T':
			return 3;
		default:
			THROW(Exception, QString("Unknown base encountered: ") + base);
	}
}


QList<Sequence> ChunkProcessor::get_seqs(const Variant& variant, const int& slice_start, const int& slice_end, const int& length, const FastaFileIndex& reference, const Transcript& transcript)
{
    Sequence ref_seq = reference.seq(variant.chr(), slice_start, length + variant.end() - variant.start());
    Sequence alt_base = variant.obs();

    int variant_position_in_slice = variant.start() - slice_start;
	if (transcript.isMinusStrand())
	{
        ref_seq.reverseComplement();
        alt_base.reverseComplement();
        variant_position_in_slice = slice_end - slice_start - variant_position_in_slice;
    }
    Sequence alt_seq = ref_seq;
    alt_seq = alt_seq.replace(variant_position_in_slice, variant.ref().length(), alt_base);

	return QList<Sequence>{ref_seq, alt_seq};
}

float ChunkProcessor::score5_consensus(const Sequence& sequence)
{
    char seq_pos_3 = sequence[3];
    char seq_pos_4 = sequence[4];
    return cons15_.value(seq_pos_3)*cons25_.value(seq_pos_4)/(bgd_.value(seq_pos_3)*bgd_.value(seq_pos_4));
}

float ChunkProcessor::score5_rest(const Sequence& sequence)
{
    QByteArray rest_seq = sequence.mid(0,3) + sequence.mid(5);
    return meta_.score5_rest_[rest_seq];
}

float ChunkProcessor::score5(const Sequence& sequence)
{
    float consensus_score = score5_consensus(sequence);
    float rest_score = score5_rest(sequence);
    return log2(consensus_score * rest_score);
}

float ChunkProcessor::score3(const Sequence& sequence)
{
    float consensus_score = score3_consensus(sequence);
    float rest_score = score3_rest(sequence);
    return log2(consensus_score * rest_score);
}

float ChunkProcessor::score3_consensus(const Sequence& sequence)
{
    char seq_pos_18 = sequence[18];
    char seq_pos_19 = sequence[19];
    return cons13_.value(seq_pos_18)*cons23_.value(seq_pos_19)/(bgd_.value(seq_pos_18)*bgd_.value(seq_pos_19));
}

float ChunkProcessor::score3_rest(const Sequence& sequence)
{
    QByteArray rest_seq = sequence.mid(0,18) + sequence.mid(20);
    float rest_score = 1;
    rest_score *= meta_.score3_rest_[0][hashseq(rest_seq.mid(0,7))];
    rest_score *= meta_.score3_rest_[1][hashseq(rest_seq.mid(7,7))];
    rest_score *= meta_.score3_rest_[2][hashseq(rest_seq.mid(14,7))];
    rest_score *= meta_.score3_rest_[3][hashseq(rest_seq.mid(4,7))];
    rest_score *= meta_.score3_rest_[4][hashseq(rest_seq.mid(11,7))];
    rest_score /= meta_.score3_rest_[5][hashseq(rest_seq.mid(4,3))];
    rest_score /= meta_.score3_rest_[6][hashseq(rest_seq.mid(7,4))];
    rest_score /= meta_.score3_rest_[7][hashseq(rest_seq.mid(11,3))];
    rest_score /= meta_.score3_rest_[8][hashseq(rest_seq.mid(14,4))];
    return rest_score;
}

float ChunkProcessor::score_maxent(const Sequence& sequence, float (ChunkProcessor::*scorefunc)(const Sequence&))
{
	//calculate score if not in cache
	if (!maxent_cache.contains(sequence))
	{
		maxent_cache[sequence] = (this->*scorefunc)(sequence);
    }

	return maxent_cache[sequence];
}


QPair<float, int> ChunkProcessor::get_max_score(const Sequence& context, const float& window_size, float (ChunkProcessor::*scorefunc)(const Sequence&))
{
	float maxscore = -1.0 *std::numeric_limits<int>::max();
	int frame = -1;
	for (int i = 0; i <= context.length() - window_size; i++)
	{
		float current_score = score_maxent(context.mid(i, window_size), scorefunc);
		if (current_score > maxscore)
		{
            maxscore = current_score;
            frame = i;
        }
    }

	return qMakePair(maxscore, frame);
}

QList<QByteArray> ChunkProcessor::runMES(const Variant& variant, const ChromosomalIndex<TranscriptList>& transcripts, const FastaFileIndex& reference)
{
	QList<QByteArray> all_mes_strings;

    // get splice site regions of transcripts oi
	QVector<int> transcripts_oi = transcripts.matchingIndices(variant.chr(), variant.start(), variant.end());
	foreach(int transcript_index, transcripts_oi)
	{
		const Transcript& current_transcript = transcripts.container()[transcript_index];

		const BedFile& coding_regions = current_transcript.codingRegions();
		for ( int i=0; i<coding_regions.count(); ++i )
		{
			const BedLine& coding_region = coding_regions[i];
            bool overlaps_three_prime = false;
            bool overlaps_five_prime = false;
            int slice_start_three;
            int slice_end_three;
            int slice_start_five;
            int slice_end_five;

            // EEEEEEEEEEEEGUXXXIIIIIIIIIIIIIIAPPPXXAGEEEEEEEEEEEE
            // -----Exon---5'ss-----Intron------3'ss------Exon----

			if (current_transcript.isPlusStrand())
			{
                // check 5 prime ss if it is not the first exon (bc. there is no 5' ss)
				if (i != 0)
				{
                    slice_start_three = coding_region.start() - 20;
                    slice_end_three = coding_region.start() + 2;
                    overlaps_three_prime = variant.overlapsWith(slice_start_three, slice_end_three);
                }
				if (i != coding_regions.count() - 1)
				{
                    slice_start_five = coding_region.end() - 2;
                    slice_end_five = coding_region.end() + 6;
                    overlaps_five_prime = variant.overlapsWith(slice_start_five, slice_end_five);
                }
			}
			else
			{
				if (i != coding_regions.count() - 1)
				{
                    slice_start_three = coding_region.end() - 2;
                    slice_end_three = coding_region.end() + 20;
                    overlaps_three_prime = variant.overlapsWith(slice_start_three, slice_end_three);
                }
				if (i != 0)
				{
                    slice_start_five = coding_region.start() - 6;
                    slice_end_five = coding_region.start() + 2;
                    overlaps_five_prime = variant.overlapsWith(slice_start_five, slice_end_five);
                }
            }

			if (overlaps_three_prime)
			{
                // get sequences
                QList<Sequence> seqs = get_seqs(variant, slice_start_three, slice_end_three, 23, reference, current_transcript);
                Sequence ref_seq = seqs[0];
                Sequence alt_seq = seqs[1];
				if (is_valid_sequence(ref_seq) && is_valid_sequence(alt_seq))
				{
                    // get scores
                    float maxentscan_ref = score_maxent(ref_seq, &ChunkProcessor::score3);
                    float maxentscan_alt = score_maxent(alt_seq, &ChunkProcessor::score3);
					all_mes_strings.append(format_score(maxentscan_ref)+'&'+format_score(maxentscan_alt)+'&'+current_transcript.name());
                }
            }

			if (overlaps_five_prime)
			{
                // get sequences
                QList<Sequence> seqs = get_seqs(variant, slice_start_five, slice_end_five, 9, reference, current_transcript);
                Sequence ref_seq = seqs[0];
                Sequence alt_seq = seqs[1];
				if (is_valid_sequence(ref_seq) && is_valid_sequence(alt_seq))
				{
                    float maxentscan_ref = score_maxent(ref_seq, &ChunkProcessor::score5);
                    float maxentscan_alt = score_maxent(alt_seq, &ChunkProcessor::score5);
					all_mes_strings.append(format_score(maxentscan_ref)+'&'+format_score(maxentscan_alt)+'&'+current_transcript.name());
                }
            }
        }
    }

    return all_mes_strings;
}



QList<QByteArray> ChunkProcessor::runSWA(const Variant& variant, const ChromosomalIndex<TranscriptList>& transcripts, const FastaFileIndex& reference)
{
    QList<QByteArray> all_mes_swa_strings;

	QVector<int> transcripts_oi = transcripts.matchingIndices(variant.chr(), variant.start(), variant.end());
	foreach(int transcript_index, transcripts_oi)
	{
		const Transcript& current_transcript = transcripts.container()[transcript_index];
        // 5 prime ss / donor ss
        QList<Sequence> donor_seqs = get_seqs(variant, variant.start()-8, variant.end()+8, 17, reference, current_transcript);
        //qDebug() << donor_seqs;
		Sequence ref_context = donor_seqs[0];
		Sequence alt_context = donor_seqs[1];
        QByteArray ref_donor = "";
        QByteArray alt_donor = "";
        QByteArray comp_donor = "";
		if (is_valid_sequence(ref_context) && is_valid_sequence(alt_context))
		{
			QPair<float, int> max_ref_donor = get_max_score(ref_context, 9, &ChunkProcessor::score5);
			QPair<float, int> max_alt_donor = get_max_score(alt_context, 9, &ChunkProcessor::score5);

            float donor_comp;
			if (variant.ref().length() == variant.obs().length())
			{
				int donor_alt_frame = static_cast<int>(max_alt_donor.second);
                Sequence donor_comp_seq = ref_context.mid(donor_alt_frame, 9);
                donor_comp = score_maxent(donor_comp_seq, &ChunkProcessor::score5);
			}
			else // take the max ref score
			{
				donor_comp = max_ref_donor.first;
            }
			ref_donor = format_score(max_ref_donor.first);
			alt_donor = format_score(max_alt_donor.first);
			comp_donor = QByteArray::number(donor_comp, 'f', params_.decimals);
        }
            
        // 3 prime ss / acceptor ss
        QList<Sequence> acceptor_seqs = get_seqs(variant, variant.start()-22, variant.end()+22, 45, reference, current_transcript);
        //qDebug() << acceptor_seqs;
        ref_context = acceptor_seqs[0];
        alt_context = acceptor_seqs[1];
        QByteArray ref_acceptor = "";
        QByteArray alt_acceptor = "";
        QByteArray comp_acceptor = "";
		if (is_valid_sequence(ref_context) && is_valid_sequence(alt_context))
		{
			QPair<float, int> max_ref_acceptor = get_max_score(ref_context, 23, &ChunkProcessor::score3);
			QPair<float, int> max_alt_acceptor = get_max_score(alt_context, 23, &ChunkProcessor::score3);

            float acceptor_comp;
			if (variant.ref().length() == variant.obs().length())
			{
				int acceptor_alt_frame = static_cast<int>(max_alt_acceptor.second);
                Sequence acceptor_comp_seq = ref_context.mid(acceptor_alt_frame, 23);
                acceptor_comp = score_maxent(acceptor_comp_seq, &ChunkProcessor::score3);
			}
			else // take the max ref score
			{
				acceptor_comp = max_ref_acceptor.first;
            }
			ref_acceptor = format_score(max_ref_acceptor.first);
			alt_acceptor = format_score(max_alt_acceptor.first);
			comp_acceptor = QByteArray::number(acceptor_comp, 'f', params_.decimals);
		}

		all_mes_swa_strings.append(ref_donor+'&'+alt_donor+'&'+comp_donor+'&'+ref_acceptor+'&'+alt_acceptor+'&'+comp_acceptor+'&'+current_transcript.name());
    }

	return all_mes_swa_strings;
}

QByteArray ChunkProcessor::format_score(float score)
{
	if (score<params_.min_score) score = params_.min_score;

	return QByteArray::number(score, 'f', params_.decimals);
}

// single chunks are processed
void ChunkProcessor::run()
{
	QRegExp base_regexp("[ACGT]*");
	ChromosomalIndex<TranscriptList> transcript_index(meta_.transcripts);
	Variant variant;
	try
	{
		//process data
		QList<QByteArray> lines_new;
		lines_new.reserve(job_.lines.size());
		foreach(const QByteArray& line, job_.lines)
		{
			if (line.trimmed().isEmpty())  continue;

			if (line.startsWith('#')) //header line
			{
				//append header line for new annotation
				if (line.startsWith("#CHROM"))
				{
					lines_new << meta_.annotation_header_lines;
				}

				lines_new << line;
			}
			else //content line
			{
				//split line and extract variant infos
            	QList<QByteArray> parts = line.trimmed().split('\t');
            	if(parts.count() < 8) THROW(FileParseException, "VCF with too few columns: " + line);

            	Chromosome chr = parts[0];
				Sequence ref = parts[3].toUpper();
				Sequence alt = parts[4].toUpper();
            	int start = atoi(parts[1]);
            	int end = start + ref.length() - 1;
				QByteArray& info = parts[7];

				variant = Variant(chr, start, end, ref, alt);


				//write out multi-allelic, variants with non-standard bases and structural variants without annotation
				if(!base_regexp.exactMatch(ref) || !base_regexp.exactMatch(alt) || !variant.isValid())
				{
					lines_new << parts.join('\t') + '\n';
            	    continue;
				}

				if (variant.ref().length() == 1 && variant.obs().length() == 1) // only calculate for SNPs
				{
					QList<QByteArray> all_mes_strings = runMES(variant, transcript_index, reference_);
					if (all_mes_strings.count() > 0) // add to info column & remove . if it was there
					{
						QByteArray anno = params_.tag.toLatin1()+"=" + all_mes_strings.join('|');
						if (info == ".")
						{
							info = anno;
						}
						else
						{
							info += ";" + anno;
						}
					}
				}

				if (params_.swa)
				{
					QList<QByteArray> all_mes_swa_strings = runSWA(variant, transcript_index, reference_);
					if (all_mes_swa_strings.count() > 0) // add to info column & remove . if it was there
					{
						QByteArray anno = params_.tag_swa.toLatin1()+"=" + all_mes_swa_strings.join('|');
						if (info == ".")
						{
							info = anno;
						}
						else
						{
							info += ";" + anno;
						}
					}
				}

				lines_new << parts.join('\t') + '\n';
			}
		}
		job_.lines = lines_new;

		emit done(job_.index);
	}
	catch(Exception& e)
	{
		emit error(job_.index, e.message() + " while processing variant:" + variant.toString());
	}
}
