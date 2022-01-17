#include "ChainFileReader.h"
#include "Helper.h"
#include <iostream>

ChainFileReader::ChainFileReader()
{

}


ChainFileReader::~ChainFileReader()
{

}

GenomePosition ChainFileReader::lift(const GenomePosition& pos) const
{
	if ( ! chromosomes.contains(pos.chr))
	{
		THROW(ArgumentException, "Position to lift is in unknown chromosome. Tried to lift:" + pos.toString());
	}
	if (pos.pos < 0 || pos.pos > ref_chrom_sizes[pos.chr])
	{
		THROW(ArgumentException, "Position to lift is outside of the chromosome size for chromosome. Tried to lift:" + pos.toString());
	}
	QList<GenomicAlignment> alignments = chromosomes[pos.chr];


	// TODO binary search ?
	foreach(const GenomicAlignment& a, alignments)
	{
		if (a.contains(pos))
		{
			GenomePosition lifted = a.lift(pos);
			if (lifted.chr == "") // if returned is invalid the given position might be aligned to a gap in this alignment search for another.
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

		QList<QByteArray> parts = line.split(' ');

		if (line.startsWith("chain"))
		{
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
	if ( ! ref_chrom_sizes.contains(ref_chr))
	{
		ref_chrom_sizes.insert(ref_chr, parts[3].toInt());
	}

	bool ref_plus_strand = parts[4] == "+";
	int ref_start = parts[5].toInt();
	int ref_end = parts[6].toInt();

	QByteArray q_chr = parts[7];
	if ( ! q_chrom_sizes.contains(q_chr))
	{
		q_chrom_sizes.insert(ref_chr, parts[8].toInt());
	}

	bool q_plus_strand = parts[9] == "+";
	int q_start = parts[10].toInt();
	int q_end = parts[11].toInt();
	int chain_id = parts[12].toInt();

	return GenomicAlignment(score, ref_chr, ref_start, ref_end, ref_plus_strand, q_chr, q_start, q_end, q_plus_strand, chain_id);
}


