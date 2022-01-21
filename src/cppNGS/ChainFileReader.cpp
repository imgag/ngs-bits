#include "ChainFileReader.h"
#include "Helper.h"
#include <iostream>

ChainFileReader::ChainFileReader()
{

}


ChainFileReader::~ChainFileReader()
{

}

GenomePosition ChainFileReader::lift(const QByteArray& chr, int pos) const
{
	if ( ! chromosomes.contains(chr))
	{
		THROW(ArgumentException, "Position to lift is in unknown chromosome. Tried to lift: chr" + chr +": " + QByteArray::number(pos));
	}
	if (pos < 0 || pos > ref_chrom_sizes[chr])
	{
		THROW(ArgumentException, "Position to lift is outside of the chromosome size for chromosome. Tried to lift: " + chr +": " + QByteArray::number(pos));
	}
	QList<GenomicAlignment> alignments = chromosomes[chr];

	// TODO binary search ?
	foreach(const GenomicAlignment& a, alignments)
	{
		if (a.contains(chr, pos))
		{
			GenomePosition lifted = a.lift(chr, pos);
			if (lifted.chr == "" || lifted.pos == -1) // if returned is invalid the given position might be aligned to a gap in this alignment search for another.
			{
				continue;
			}
			else
			{
				return lifted;
			}
		}
	}

	THROW(ArgumentException, "The given position is in an unmapped region!")
}

BedLine ChainFileReader::lift(const QByteArray &chr, int start, int end) const
{
	BedLine bed;
	GenomePosition lifted_start = lift(chr, start);
	GenomePosition lifted_end = lift(chr, end);

	if (lifted_start.chr != lifted_end.chr)
	{
		THROW(ArgumentException, "The start and end of the given region map to different chromosomes!")
	}

	if (std::abs(lifted_start.pos - lifted_end.pos) < end-start)
	{
		THROW(ArgumentException, "The new region maps to smaller region!")
	}

	bed.setChr(Chromosome(lifted_start.chr));

	if (lifted_start.pos < lifted_end.pos)
	{
		bed.setStart(lifted_start.pos);
		bed.setEnd(lifted_end.pos);
	}
	else
	{
		bed.setStart(lifted_end.pos);
		bed.setEnd(lifted_start.pos);
	}

	return bed;
}

void ChainFileReader::load(QString filepath)
{
	filepath_ = filepath;
	fp_ = Helper::openFileForReading(filepath, false);

	// read first alignment line:
	QByteArray line = fp_->readLine();
	line = line.trimmed();

	GenomicAlignment currentAlignment = parseChainLine(line.split(' '));

	while(! fp_->atEnd())
	{
		line = fp_->readLine();
		line = line.trimmed();
		if (line.length() == 0) continue;

		QList<QByteArray> parts;

		if (line.startsWith("chain"))
		{
			parts = line.split(' ');
			// add last chain alignment to the chromosomes:
			if ( ! chromosomes.contains(currentAlignment.ref_chr))
			{
				chromosomes.insert(currentAlignment.ref_chr, QList<GenomicAlignment>());
			}
			chromosomes[currentAlignment.ref_chr].append(currentAlignment);

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

			AlignmentLine align;
			if (parts.length() == 1)
			{
				align = AlignmentLine(parts[0].toInt(), 0, 0);
			}
			else if (parts.length() == 3)
			{
				align = AlignmentLine(parts[0].toInt(), parts[1].toInt(), parts[2].toInt());
			}
			else
			{
				THROW(FileParseException, "Alignment Data line with neither 3 nor a single number. " + line);
			}
			currentAlignment.addAlignmentLine(align);
		}
	}
}

GenomicAlignment ChainFileReader::parseChainLine(QList<QByteArray> parts)
{
	double score = parts[1].toDouble();
	QByteArray ref_chr = parts[2];
	int ref_chr_size = parts[3].toInt();
	if ( ! ref_chrom_sizes.contains(ref_chr))
	{
		ref_chrom_sizes.insert(ref_chr, ref_chr_size);
	}


	bool ref_plus_strand = parts[4] == "+";
	int ref_start = parts[5].toInt();
	int ref_end = parts[6].toInt();

	QByteArray q_chr = parts[7];
	int q_chr_size = parts[8].toInt();
	if ( ! q_chrom_sizes.contains(q_chr))
	{
		q_chrom_sizes.insert(q_chr, q_chr_size);
	}


	bool q_plus_strand = parts[9] == "+";
	int q_start = parts[10].toInt();
	int q_end = parts[11].toInt();
	int chain_id = parts[12].toInt();

	return GenomicAlignment(score, ref_chr, ref_chr_size, ref_start, ref_end, ref_plus_strand, q_chr, q_chr_size, q_start, q_end, q_plus_strand, chain_id);
}


