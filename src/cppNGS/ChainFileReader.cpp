#include "ChainFileReader.h"
#include "Helper.h"

ChainFileReader::ChainFileReader()
{

}


ChainFileReader::~ChainFileReader()
{

}

void ChainFileReader::load(QString filepath)
{
	filepath_ = filepath;
	fp_ = Helper::openFileForReading(filepath, false);

	QHash<QByteArray, QList<GenomicAlignment>> chromosomes;
	QHash<QByteArray, int> ref_chrom_sizes;
	QHash<QByteArray, int> q_chrom_sizes;

	// read first alignment line:
	QByteArray line = fp_->readLine();
	line = line.trimmed();

	GenomicAlignment currentAlignment = parseChainLine(line.split('\t'), ref_chrom_sizes, q_chrom_sizes);

	while(! fp_->atEnd())
	{
		line = fp_->readLine();
		line = line.trimmed();
		if (line.length() == 0) continue;

		QList<QByteArray> parts = line.split('\t');

		if (line.startsWith("chain"))
		{
			// add last chain alignment to the chromosomes:
			if ( ! chromosomes.contains(currentAlignment.ref_chr))
			{
				chromosomes.insert(currentAlignment.ref_chr, QList<GenomicAlignment>());
			}
			chromosomes[currentAlignment.ref_chr].append(currentAlignment);

			// parse the new Alignment
			currentAlignment = parseChainLine(parts, ref_chrom_sizes, q_chrom_sizes);

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

GenomicAlignment ChainFileReader::parseChainLine(QList<QByteArray> parts, QHash<QByteArray, int>& ref_chrom_sizes, QHash<QByteArray, int>& q_chrom_sizes)
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
