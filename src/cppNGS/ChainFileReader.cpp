#include "ChainFileReader.h"
#include "Helper.h"
#include <iostream>

ChainFileReader::ChainFileReader()
{
	percent_deletion_ = 0.05;
}


ChainFileReader::~ChainFileReader()
{

}

BedLine ChainFileReader::lift_tree(const Chromosome& chr, int start, int end) const
{
	if (end <= start)
	{
		THROW(ArgumentException, "End is smaller or equal to start!");
	}

	if ( ! chromosomes_tree.contains(chr))
	{
		THROW(ArgumentException, "Position to lift is in unknown chromosome. Tried to lift: " + chr.strNormalized(true));
	}
	if (start < 0 || end > ref_chrom_sizes_[chr])
	{
		THROW(ArgumentException, "Position to lift is outside of the chromosome size for chromosome. Tried to lift: " + chr.strNormalized(true) +": " + QByteArray::number(start) + "-" + QByteArray::number(end));
	}

	//get alignments that overlap with the given region
	QList<GenomicAlignment> alignments = chromosomes_tree[chr].query(start, end);

//	std::cout << "Alignments found: " << alignments.size() << "\n";

	foreach(const GenomicAlignment& a, alignments)
	{
		//std::cout << a.toString(false).toStdString() << "\n";
		BedLine result = a.lift(start, end, percent_deletion_);

		if (result.start() == -1)
		{
			continue;
		}
		else
		{
			return result;
		}
	}

	THROW(ArgumentException, "Region is unmapped.");
}

BedLine ChainFileReader::lift_list(const Chromosome& chr, int start, int end) const
{
	if (end <= start)
	{
		THROW(ArgumentException, "End is smaller or equal to start!");
	}

	if ( ! chromosomes_tree.contains(chr))
	{
		THROW(ArgumentException, "Position to lift is in unknown chromosome. Tried to lift: " + chr.strNormalized(true));
	}
	if (start < 0 || end > ref_chrom_sizes_[chr])
	{
		THROW(ArgumentException, "Position to lift is outside of the chromosome size for chromosome. Tried to lift: " + chr.strNormalized(true) +": " + QByteArray::number(start) + "-" + QByteArray::number(end));
	}

	//get alignments that overlap with the given region
	QList<GenomicAlignment> alignments = chromosomes_list[chr];

	foreach(const GenomicAlignment& a, alignments)
	{
		if( ! a.overlapsWith(start, end))
		{
			continue;
		}

//		if ( ! (a.q_start<= 146039389 && 146039390 <= a.q_end) || a.ref_start == 10000)
//		{
//			continue;
//		}

		BedLine result = a.lift(start, end, percent_deletion_);

//		std::cout << "\n" << a.toString(false).toStdString() << "\n################################################\n\n";
//		std::cout << result.toString(true).toStdString() << "\n\n";

		if (result.start() == -1)
		{
			continue;
		}
		else
		{
			return result;
		}
	}

	THROW(ArgumentException, "Region is unmapped or more than " + QByteArray::number(percent_deletion_*100) + "% deleted/unmapped bases.");
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
			//
			if (! chromosomes_list.contains(currentAlignment.ref_chr))
			{
				chromosomes_list.insert(currentAlignment.ref_chr, QList<GenomicAlignment>());
			}
			chromosomes_list[currentAlignment.ref_chr].append(currentAlignment);

			// to interval tree:
			if (! chromosomes_tree.contains(currentAlignment.ref_chr))
			{
				chromosomes_tree.insert(currentAlignment.ref_chr, IntervalTree(0, currentAlignment.ref_chr_size));
			}
			chromosomes_tree[currentAlignment.ref_chr].addInterval(currentAlignment);

			// to chromosomal index:

			alignments_index_.append(currentAlignment);

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
	if ( ! q_chrom_sizes_.contains(q_chr))
	{
		q_chrom_sizes_.insert(q_chr, q_chr_size);
	}


	bool q_plus_strand = parts[9] == "+";
	int q_start = parts[10].toInt();
	int q_end = parts[11].toInt();
	int chain_id = parts[12].toInt();

	return GenomicAlignment(score, ref_chr, ref_chr_size, ref_start, ref_end, ref_plus_strand, q_chr, q_chr_size, q_start, q_end, q_plus_strand, chain_id);
}

ChainFileReader::IntervalTree::IntervalTree(int min, int max):
	min_(min)
  , max_(max)
  , center_((min+max) / 2)
  , left_(nullptr)
  , right_(nullptr)
  , sorted_by_start_()
  , sorted_by_end_()
{
}

ChainFileReader::IntervalTree::IntervalTree():
	min_(0)
  , max_(0)
  , center_(0)
  , left_(nullptr)
  , right_(nullptr)
  , sorted_by_start_()
  , sorted_by_end_()
{
}

ChainFileReader::IntervalTree::~IntervalTree()
{
}

QList<GenomicAlignment> ChainFileReader::IntervalTree::query(int start, int end) const
{
	if (end < start)
	{
		THROW(ArgumentException, "End cannot be smaller than start!")
	}

	QList<GenomicAlignment> result;

	if (start < center_)
	{
		if (left_)
		{
			result.append(left_->query(start, end));
		}
	}

	if (end < center_)
	{
		//region to the left of center: search sorted by start until no more intervals overlap
		for (int i=0; i<sorted_by_start_.size(); i++)
		{
			//std::cout << "sorted by start: " << sorted_by_start_[i].toString(false).toStdString() << "\n";
			if (sorted_by_start_[i].overlapsWith(start, end))
			{
				result.append(sorted_by_start_[i]);
			}
			else
			{
				//break;
			}
		}
	}
	else if (start > center_)
	{
		//region to the right of center: search sorted by end until no more intervals overlap
		for (int i=0; i<sorted_by_end_.size(); i++)
		{

			//std::cout << "sorted by end: " << sorted_by_end_[i].toString(false).toStdString() << "\n";
			if (sorted_by_end_[i].overlapsWith(start, end))
			{
				result.append(sorted_by_end_[i]);
			}
			else
			{
				//break;
			}
		}
	}
	else if (start <= center_ && center_ <= end)
	{
		// interval overlaps center so all intervals here are overlapping!
		result.append(sorted_by_start_);
	}
	else
	{
		std::cout << "start: " << start << " end: " << end << "\n";
		std::cout << "center: " << center_ << "\n";
		THROW(ProgrammingException, "This shouldn't be possible... Query Interval neither to the left, nor to the right and also not overlapping a value.")
	}

	if (end > center_)
	{
		if (right_)
		{
			result.append(right_->query(start, end));
		}
	}
	return result;
}

void ChainFileReader::IntervalTree::addInterval(GenomicAlignment alignment)
{
	if (alignment.ref_end < center_)
	{
		if (! left_)
		{
			left_.reset(new IntervalTree(min_, center_));
		}
		left_->addInterval(alignment);
	}
	else if (alignment.ref_start > center_)
	{
		if (! right_)
		{
			right_.reset(new IntervalTree(center_, max_));
		}
		right_->addInterval(alignment);
	}
	else
	{
		sorted_by_end_.append(alignment);
		sorted_by_start_.append(alignment);
	}
}

void ChainFileReader::IntervalTree::sort()
{
	if (left_)
	{
		left_->sort();
	}

	if (right_)
	{
		right_->sort();
	}

	if (! sorted_by_end_.empty())
	{
		std::sort(sorted_by_start_.begin(), sorted_by_start_.end(), AlignmentStartComp());
		std::sort(sorted_by_end_.begin(), sorted_by_end_.end(), AlignmentEndComp());
	}
}



