#include "ChromosomeInfo.h"
#include "Exceptions.h"

ChromosomeInfo::ChromosomeInfo(BamReader& reader)
	: filename_(QString::fromStdString(reader.GetFilename()))
{
	const RefVector& ref_data = reader.GetReferenceData();
	chrs_.reserve(ref_data.size());
	for (unsigned int i=0; i<ref_data.size(); ++i)
	{
		chrs_.append(Chromosome(ref_data[i].RefName));
		info_.insert(chrs_.last().num(), ChrInfo_{(int)i, ref_data[i].RefLength});
	}
}

const QList<Chromosome>& ChromosomeInfo::chromosomes() const
{
	return chrs_;
}

const Chromosome& ChromosomeInfo::chromosome(int ref_id) const
{
	if (ref_id>=chrs_.size())
	{
		THROW(ArgumentException, "Chromosome RefID '" + QString::number(ref_id) + "' out of bounds!");
	}
	return chrs_[ref_id];
}

int ChromosomeInfo::refID(const Chromosome& chr) const
{
	if (!info_.contains(chr.num()))
	{
		THROW(ArgumentException, "Could not find chromosome '" + chr.strNormalized(true) + "' in BAM file '" + filename_ + "'!");
	}
	return info_[chr.num()].ref_id;
}

int ChromosomeInfo::size(const Chromosome& chr) const
{
	if (!info_.contains(chr.num()))
	{
		THROW(ArgumentException, "Could not find chromosome '" + chr.strNormalized(true) + "' in BAM file '" + filename_ + "'!");
	}
	return info_[chr.num()].size;
}

double ChromosomeInfo::genomeSize(bool nonspecial_only) const
{
	double sum = 0.0;
	foreach(const Chromosome& c, chrs_)
	{
		if (!nonspecial_only || c.isNonSpecial())
		{
			sum += size(c);
		}
	}
	return sum;
}

int ChromosomeInfo::refID(BamReader& reader, const Chromosome& chr)
{
	//search chromosome ID
	int id = reader.GetReferenceID(std::string(chr.strNormalized(true).data()));
	if (id==-1)
	{
		THROW(ArgumentException, "Could not find chromosome '" + chr.strNormalized(true) + "' in BAM file '" + QString::fromStdString(reader.GetFilename()) + "'!");
	}

	return id;
}
