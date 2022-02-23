#include "ChainFileReader.h"
#include "Exceptions.h"
#include "zlib.h"

ChainFileReader::ChainFileReader(QString filepath, double percent_deletion):
	filepath_(filepath)
  , file_(filepath)
  , percent_deletion_(percent_deletion)
{
	load();
}

ChainFileReader::~ChainFileReader()
{
}

void ChainFileReader::load()
{
	QList<QByteArray> lines = getLines();

	// read first alignment line:
	QByteArray line = lines[0];
	line = line.trimmed();
	GenomicAlignment currentAlignment = parseChainLine(line.split(' '));

	for(int i=1; i<lines.size(); i++)
	{
		line = lines[i].trimmed();
		if (line.length() == 0) continue;

		QList<QByteArray> parts;

		if (line.startsWith("chain"))
		{
			parts = line.split(' ');
			// add last chain alignment to the chromosomes:
			if (! chromosomes_.contains(currentAlignment.ref_chr))
			{
				chromosomes_.insert(currentAlignment.ref_chr, QList<GenomicAlignment>());
			}
			chromosomes_[currentAlignment.ref_chr].append(currentAlignment);

			// parse the new Alignment
			currentAlignment = parseChainLine(parts);
		}
		else
		{
			if (line.contains('\t'))
			{
				parts = line.split('\t');
			}
			else
			{
				parts = line.split(' ');
			}

			if (parts.length() == 1)
			{
				currentAlignment.addAlignmentLine(parts[0].toInt(), 0, 0);
			}
			else if (parts.length() == 3)
			{
				currentAlignment.addAlignmentLine(parts[0].toInt(), parts[1].toInt(), parts[2].toInt());
			}
			else
			{
				THROW(FileParseException, "Alignment Data line with neither 3 nor a single number. " + line);
			}
		}
	}
}

QList<QByteArray> ChainFileReader::getLines()
{
	file_ = VersatileFile(filepath_);

	if (! file_.open(QFile::ReadOnly))
	{
		THROW(FileAccessException, "Could not open chain-file for reading: '" + filepath_ + "'!");
	}

	if (filepath_.endsWith(".chain"))
	{
		return file_.readAll().split('\n');
	}
	else if (filepath_.endsWith(".gz"))
	{

		QByteArray compressed = file_.readAll();
		if (compressed.length() == 0)
		{
			THROW(ProgrammingException, "Reading file gave no compressed data.");
		}

		QByteArray decompressed;
		int ret;
		int decompress_buffer_size = 1024*128; // 128kb
		char out[decompress_buffer_size];
		//set zlib vars
		z_stream infstream;
		infstream.zalloc = Z_NULL;
		infstream.zfree = Z_NULL;
		infstream.opaque = Z_NULL;
		infstream.avail_in = 0;
		infstream.next_in = Z_NULL;
		ret = inflateInit2(&infstream, 16+MAX_WBITS);
		if (ret != Z_OK)
		{
			THROW(ProgrammingException, "Error while initializing inflate. Error code: " + QString::number(ret));
		}

		// setup "compressed_block.data()" as the input and "out" as the uncompressed output
		infstream.avail_in = compressed.size(); // size of input
		infstream.next_in = (Bytef *)compressed.data(); // input char array

		do {
				infstream.avail_out = decompress_buffer_size;
				infstream.next_out = (Bytef *) out;
				ret = inflate(&infstream, Z_NO_FLUSH);
				if(ret != Z_OK && ret != Z_STREAM_END && ret != Z_BUF_ERROR)  /* state not clobbered */
				{
					switch (ret) {
						case Z_STREAM_ERROR:
							inflateEnd(&infstream);
							THROW(FileParseException, "Zlib stream Error while decompressing file!");
							break;
						case Z_DATA_ERROR:
							inflateEnd(&infstream);
							// means that either the data is not a zlib stream to begin with, or that the data was corrupted somewhere along the way since it was compressed
							THROW(FileParseException, "Zlib data Error while decompressing file!");
							break;
						case Z_MEM_ERROR:
							// Z_MEM_ERROR, memory allocation for internal state of inflate() failed.
							inflateEnd(&infstream);
							THROW(FileParseException, "Zlib memory Error while decompressing file!");
							break;
						case Z_VERSION_ERROR:
							inflateEnd(&infstream);
							THROW(FileParseException, "Zlib Version Error while decompressing file!");
							break;
						default:
							inflateEnd(&infstream);
							THROW(FileParseException, "Unknown zlib error while decompressing file! Error Code: " + QString::number(ret));
							break;
					}
				}
				decompressed.append(out, decompress_buffer_size-infstream.avail_out);
			} while (infstream.avail_out == 0);

		inflateEnd(&infstream);
		return decompressed.split('\n');

	}
	else
	{
		THROW(ArgumentException, "File doesn't end with .chain or .gz. Unknown filetype.")
	}
}

BedLine ChainFileReader::lift(const Chromosome& chr, int start, int end) const
{
	if (end < start)
	{
		THROW(ArgumentException, "End is smaller than start!");
	}

	if ( ! chromosomes_.contains(chr))
	{
		THROW(ArgumentException, "Position to lift is in unknown chromosome. Tried to lift: " + chr.strNormalized(true));
	}
	if (start < 1 || end > ref_chrom_sizes_[chr])
	{
		THROW(ArgumentException, "Position to lift is outside of the chromosome size for chromosome. Tried to lift: " + chr.strNormalized(true) +": " + QByteArray::number(start) + "-" + QByteArray::number(end));
	}

	start = start-1;

	//get alignments that overlap with the given region
	QList<GenomicAlignment> alignments = chromosomes_[chr];

	foreach(const GenomicAlignment& a, alignments)
	{
		if( ! a.overlapsWith(start, end))
		{
			continue;
		}

		BedLine result = a.lift(start, end, percent_deletion_);

		if (result.start() == -1)
		{
			continue;
		}
		else
		{
			result.setStart(result.start() +1);
			return result;
		}
	}

	THROW(ArgumentException, "Region is unmapped or more than " + QByteArray::number(percent_deletion_*100) + "% deleted/unmapped bases.");
}

ChainFileReader::GenomicAlignment ChainFileReader::parseChainLine(QList<QByteArray> parts)
{
	double score = parts[1].toDouble();
	Chromosome ref_chr(parts[2]);
	int ref_chr_size = parts[3].toInt();
	if ( ! ref_chrom_sizes_.contains(ref_chr))
	{
		ref_chrom_sizes_.insert(ref_chr, ref_chr_size);
	}

	bool ref_plus_strand = parts[4] == "+";
	int ref_start = parts[5].toInt();
	int ref_end = parts[6].toInt();

	Chromosome q_chr(parts[7]);
	int q_chr_size = parts[8].toInt();

	bool q_plus_strand = parts[9] == "+";
	int q_start = parts[10].toInt();
	int q_end = parts[11].toInt();
	int chain_id = parts[12].toInt();

	return ChainFileReader::GenomicAlignment(score, ref_chr, ref_chr_size, ref_start, ref_end, ref_plus_strand, q_chr, q_chr_size, q_start, q_end, q_plus_strand, chain_id);
}

ChainFileReader::GenomicAlignment::GenomicAlignment():
	score(0)
  , id(0)
  , ref_chr("")
  , ref_chr_size(0)
  , ref_start(0)
  , ref_end(0)
  , ref_on_plus(false)
  , q_chr("")
  , q_chr_size(0)
  , q_start(0)
  , q_end(0)
  , q_on_plus(false)
{
}

ChainFileReader::GenomicAlignment::GenomicAlignment(double score, Chromosome ref_chr, int ref_chr_size, int ref_start, int ref_end, bool ref_on_plus, Chromosome q_chr, int q_chr_size, int q_start, int q_end, bool q_on_plus, int id):
	score(score)
  , id(id)
  , ref_chr(ref_chr)
  , ref_chr_size(ref_chr_size)
  , ref_start(ref_start)
  , ref_end(ref_end)
  , ref_on_plus(ref_on_plus)
  , q_chr(q_chr)
  , q_chr_size(q_chr_size)
  , q_start(q_start)
  , q_end(q_end)
  , q_on_plus(q_on_plus)
{
	index.append(IndexLine(ref_start, q_start, 0));
}

BedLine ChainFileReader::GenomicAlignment::lift(int start, int end, double percent_deletion) const
{
	int start_index = 0;
	int ref_current_pos = ref_start;
	int q_current_pos = q_start;

	for (int cur=1; cur<index.size(); cur++)
	{
		if (index[cur].ref_start > start)
		{
			ref_current_pos = index[cur-1].ref_start;
			q_current_pos = index[cur-1].q_start;
			start_index = index[cur-1].alignment_line_index;
			break;
		}

		if (cur == index.size()-1)
		{
			ref_current_pos = index[cur].ref_start;
			q_current_pos = index[cur].q_start;
			start_index = index[cur].alignment_line_index;
			break;
		}
	}

	int lifted_start = -1;
	int lifted_end = -1;
	int unmapped_bases = 0;

	bool start_was_in_unmapped = false;

	// Test if part of the query region is outside of the alignment.
	if (ref_start >= start)
	{
		lifted_start = q_current_pos;
		unmapped_bases += ref_current_pos - start;
	}

	if (ref_end <= end)
	{
		lifted_end = q_end;
		unmapped_bases += end - ref_end;
	}

	for (int i=start_index; i<alignment.size(); i++)
	{
		if (unmapped_bases > percent_deletion * (end-start))
		{
			// break if more than the allowed percentage is unmapped/deleted
			break;
		}

		const AlignmentLine& line = alignment[i];

		// try to lift start and end:

		if (lifted_start == -1)
		{
			if (ref_current_pos <= start && start < ref_current_pos + line.size)
			{
				lifted_start = q_current_pos + (start - ref_current_pos);
			}

			 //if start is in the next unmapped or deleted region of the last alignment line - take the next possible position:
			if (ref_current_pos + line.size <= start && start < ref_current_pos +line.size + line.ref_dt)
			{
				unmapped_bases += (ref_current_pos + line.size + line.ref_dt) - start;
				lifted_start = q_current_pos + line.size + line.q_dt;
				start_was_in_unmapped = true; // make sure the line.ref_dt is only added once
			}
		}

		if (lifted_end == -1)
		{
			//it's not in the same alignment piece but there is no gap in the reference:
			if (ref_current_pos <= end && end < ref_current_pos + line.size)
			{
				lifted_end = q_current_pos + (end - ref_current_pos);
			}

			// if end is in the next unmapped region - take the last possible position:
			if (ref_current_pos +line.size <= end && end < ref_current_pos +line.size + line.ref_dt)
			{
				unmapped_bases += end - (ref_current_pos +line.size); // amount the end is "pulled forward"
				lifted_end = q_current_pos + line.size;
			}

			if (ref_current_pos + line.size +line.ref_dt == end) // neccessary but strange
			{
				unmapped_bases += line.ref_dt;
				lifted_end = q_current_pos + line.size;
			}
		}

		ref_current_pos += line.size + line.ref_dt;
		q_current_pos += line.size + line.q_dt;

		if(lifted_start != -1 && lifted_end == -1 && ! start_was_in_unmapped)
		{
			unmapped_bases += line.ref_dt;
		}
		start_was_in_unmapped = false;


		// break when the current position is after the start:
		if (ref_current_pos > end)
		{
			break;
		}
	}

	if (lifted_start != -1 && lifted_end != -1)
	{

		if (unmapped_bases > percent_deletion * (end-start)) // !Certain that it's 5 percent and > (not >=) and not rounded!
		{
			return BedLine("", -1, -1);
		}

		if (q_on_plus)
		{
			return BedLine(q_chr, lifted_start, lifted_end);
		}
		else
		{
			return BedLine(q_chr, q_chr_size - lifted_end, q_chr_size - lifted_start);
		}
	}

	return BedLine("", -1, -1);

}

void ChainFileReader::GenomicAlignment::addAlignmentLine(int size, int ref_dt, int q_dt)
{
	AlignmentLine line = AlignmentLine(size, ref_dt, q_dt);
	alignment.append(line);

	if (alignment.size() % index_frequency == 0)
	{
		int new_index_ref_start = index[index.size()-1].ref_start;
		int new_index_q_start = index[index.size()-1].q_start;
		for(int i=index[index.size()-1].alignment_line_index; i < alignment.size()-1; i++)
		{
			new_index_ref_start += alignment[i].size + alignment[i].ref_dt;
			new_index_q_start += alignment[i].size + alignment[i].q_dt;
		}
		index.append(IndexLine(new_index_ref_start, new_index_q_start, alignment.size()-1));
	}
}

bool ChainFileReader::GenomicAlignment::contains(const Chromosome& chr, int pos) const
{
	if (chr != ref_chr) return false;
	if (pos < ref_start || pos > ref_end) return false;

	return true;
}

bool ChainFileReader::GenomicAlignment::overlapsWith(int start, int end) const
{
	return ((ref_start <= start && start <= ref_end) || (ref_start <= end && end <= ref_end));
}

QString ChainFileReader::GenomicAlignment::toString(bool with_al_lines) const
{
	QString res = QString("ref_chr:\t%1\tref_start:\t%2\tref_end:\t%3\tq_chr:\t%4\tq_start:\t%5\tq_end:\t%6\n").arg(QString(ref_chr.strNormalized(true)), QString::number(ref_start), QString::number(ref_end), QString(q_chr.strNormalized(true)), QString::number(q_start), QString::number(q_end));
	res += "ref on plus: " + QString::number(ref_on_plus) + "\tq on plus: " + QString::number(q_on_plus);
	if (with_al_lines)
	{
		foreach (const AlignmentLine& l, alignment)
		{
			res += l.toString() + "\n";
		}
	}

	return res;
}
