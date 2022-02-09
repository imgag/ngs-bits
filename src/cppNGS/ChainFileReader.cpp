#include "ChainFileReader.h"
#include "Helper.h"
#include <iostream>

ChainFileReader::ChainFileReader()
{

}


ChainFileReader::~ChainFileReader()
{

}

void ChainFileReader::testing()
{
	QVector<GenomicAlignment> alignments;

	for (int i=0; i<100; i++)
	{
		GenomicAlignment a;
		a.ref_start = 10*i;
		a.ref_end = 10*i+1;
		a.ref_chr = Chromosome("A");
		alignments.append(a);
	}

	chromosomes_index.reset(new ChromosomalIndex<QVector<GenomicAlignment>>(alignments));

}

GenomePosition ChainFileReader::lift_list(const Chromosome& chr, int pos) const
{
	if ( ! chromosomes_list.contains(chr))
	{
		THROW(ArgumentException, "Position to lift is in unknown chromosome. Tried to lift: " + chr.strNormalized(true) +": " + QByteArray::number(pos));
	}
	if (pos < 0 || pos > ref_chrom_sizes_[chr])
	{
		THROW(ArgumentException, "Position to lift is outside of the chromosome size for chromosome. Tried to lift: " + chr.strNormalized(true) +": " + QByteArray::number(pos));
	}
	QList<GenomicAlignment> alignments = chromosomes_list[chr];

	// TODO change from list to tree strcuture
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

BedLine ChainFileReader::lift_list(const Chromosome &chr, int start, int end) const
{
	BedLine bed;
	GenomePosition lifted_start = lift_list(chr, start);
	GenomePosition lifted_end = lift_list(chr, end);

	for (int i=start+1; i<end; i++)
	{
		GenomePosition test_middle = lift_list(chr, i); //throw error if part of the sequence is in unmapped
	}

	if (lifted_start.chr != lifted_end.chr)
	{
		THROW(ArgumentException, "The start and end of the given region map to different chromosomes!")
	}

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
	bed.setChr(Chromosome(lifted_start.chr));

	if (std::abs(bed.start() - bed.end()) < end-start)
	{
		THROW(ArgumentException, "The new region maps to smaller region!")
	}

	return bed;
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
	//std::cout << "Count of overlapping alignments:" << alignments.count() << "\n";

	QVector<int> liftedPos(end-start+1, -1);
	QByteArray liftedChr = "";
	foreach(const GenomicAlignment& a, alignments)
	{
		//std::cout << a.toString().toStdString() << "\n";
		for (int pos=start; pos<=end; pos++)
		{
			GenomePosition l =  a.lift(chr, pos);

			if (l.pos != -1) // position was lifted!
			{
				if (liftedChr == "") // test for chromosome
				{
					liftedChr = l.chr.strNormalized(true);
				}
				else if (l.chr.strNormalized(true) != liftedChr)
				{
					THROW(ArgumentException, "Given region maps to different chromosomes.")
				}

				if(liftedPos[pos-start] != -1) // test for position
				{
					if (l.pos == liftedPos[pos-start])
					{
						std::cout << "Position was lifted twice successfully to the same position.\n";
					}
					THROW(ProgrammingException, "Position was lifted twice. Available from different alignments!")
				}

				liftedPos[pos-start] = l.pos;
			}
		}
	}

//	for (int i=0; i<liftedPos.size(); i++)
//	{
//		std::cout << liftedPos[i] << "\n";
//	}

	// single position special case:
	if (liftedPos.size() == 1)
	{
		if (liftedPos[0] != -1)
		{
			return BedLine(Chromosome(liftedChr), liftedPos[0], liftedPos[0]);
		}
		else
		{
			THROW(ArgumentException, "Position in unmapped region!")
		}
	}

	// region:

	if (liftedPos[0] != -1 && liftedPos[liftedPos.size()-1] != -1)
	{
		if (liftedPos[0] < liftedPos[liftedPos.size()-1])
		{
			//mapped to plus strand: (everything "normally" ordered)
			for (int i=0; i< liftedPos.size()-1; i++)
			{
				if (liftedPos[i] == -1)
				{
					THROW(ArgumentException, "Partial deletion in query region! 1")
				}

//				if(liftedPos[i]+1 != liftedPos[i+1])
//				{
//					THROW(ArgumentException, "Region is split! 1")
//				}
			}
			return BedLine(Chromosome(liftedChr), liftedPos[0], liftedPos[liftedPos.size()-1]);
		}
		else
		{
			// mapped to minus strand:
			for (int i=0; i< liftedPos.size()-1; i++)
			{
				if (liftedPos[i] == -1)
				{
					THROW(ArgumentException, "Partial deletion in query region! 2")
				}

//				if(liftedPos[i]-1 != liftedPos[i+1])
//				{
//					THROW(ArgumentException, "Region is split! 2")
//				}
			}
			return BedLine(Chromosome(liftedChr), liftedPos[liftedPos.size()-1], liftedPos[0]);
		}
	}
	else
	{
		THROW(ArgumentException, "Partial deletion in query region! 3")
	}
}

BedLine ChainFileReader::lift_index(const Chromosome& chr, int start, int end) const
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
	QVector<int> indices = chromosomes_index->matchingIndices(chr, start, end);
	std::cout << "Count of overlapping alignments:" << indices.count() << "\n";

	QVector<int> liftedPos(end-start+1, -1);
	QByteArray liftedChr = "";
	foreach(int idx, indices)
	{
		GenomicAlignment a = alignments_index_[idx];
		for (int pos=start; pos<=end; pos++)
		{
			GenomePosition l =  a.lift(chr, pos);

			if (l.pos != -1) // position was lifted!
			{
				if (liftedChr == "") // test for chromosome
				{
					liftedChr = l.chr.strNormalized(true);
				}
				else if (l.chr.strNormalized(true) != liftedChr)
				{
					THROW(ArgumentException, "Given region maps to different chromosomes.")
				}

				if(liftedPos[pos-start] != -1) // test for position
				{
					if (l.pos == liftedPos[pos-start])
					{
						std::cout << "Position was lifted twice successfully to the same position.\n";
					}
					THROW(ProgrammingException, "Position was lifted twice. Available from different alignments!")
				}

				liftedPos[pos-start] = l.pos;
			}
		}
	}

	// single position special case:
	if (liftedPos.size() == 1)
	{
		if (liftedPos[0] != -1)
		{
			return BedLine(Chromosome(liftedChr), liftedPos[0], liftedPos[0]);
		}
		else
		{
			THROW(ArgumentException, "Position in unmapped region!")
		}
	}

	// region:

	if (liftedPos[0] != -1 && liftedPos[liftedPos.size()-1] != -1)
	{
		if (liftedPos[0] < liftedPos[liftedPos.size()-1])
		{
			//mapped to plus strand: (everything "normally" ordered)
			for (int i=0; i< liftedPos.size()-1; i++)
			{
				if (liftedPos[i] == -1)
				{
					THROW(ArgumentException, "Partial deletion in query region!")
				}

				if(liftedPos[i]+1 != liftedPos[i+1])
				{
					THROW(ArgumentException, "Region is split!")
				}
			}
			return BedLine(Chromosome(liftedChr), liftedPos[0], liftedPos[liftedPos.size()-1]);
		}
		else
		{
			// mapped to minus strand:
			for (int i=0; i< liftedPos.size()-1; i++)
			{
				if (liftedPos[i] == -1)
				{
					THROW(ArgumentException, "Partial deletion in query region!")
				}

				if(liftedPos[i]-1 != liftedPos[i+1])
				{
					THROW(ArgumentException, "Region is split!")
				}
			}
			return BedLine(Chromosome(liftedChr), liftedPos[liftedPos.size()-1], liftedPos[0]);
		}
	}
	else
	{
		THROW(ArgumentException, "Partial deletion in query region!")
	}
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
			// to naive list:
			if ( ! chromosomes_list.contains(currentAlignment.ref_chr))
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

	chromosomes_index.reset(new ChromosomalIndex<QVector<GenomicAlignment>>(alignments_index_));
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



