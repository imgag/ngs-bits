#ifndef CHROMOSOMALINDEX_H
#define CHROMOSOMALINDEX_H
#include "cppNGS_global.h"
#include "limits"
#include "Chromosome.h"
#include <QHash>
#include <QVector>
#include <QPair>

///Chromosomal index for fast access to @em sorted containers with chromosomal range elements like BedFile and VariantList.
template <class T>
class CPPNGSSHARED_EXPORT ChromosomalIndex
{
public:
	///Constructor.
	ChromosomalIndex(const T& container, int bin_size = 30);

	///Re-creates the index (only needed if the container content changed after calling the index constructor).
	void createIndex();

	///Returns the underlying container
	const T& container() const { return container_; }

	///Returns a vector of element indices overlapping the given chromosomal range.
	QVector<int> matchingIndices(const Chromosome& chr, int start, int end) const;
	///Returns the index of the first element in the container that overlaps the given chromosomal range, or -1 if no element overlaps.
	int matchingIndex(const Chromosome& chr, int start, int end) const;

protected:
	const T& container_;
	QHash<int, QVector<QPair<int, int> > > index_;
	int max_length_;
	int bin_size_;
	static bool firstOfPairComparator(const QPair<int, int>& a, const QPair<int, int>& b)
	{
		return	a.first < b.first;
	}
};

template <class T>
ChromosomalIndex<T>::ChromosomalIndex(const T& container, int bin_size)
	: container_(container)
	, index_()
	, max_length_(-1)
	, bin_size_(bin_size)
{
	createIndex();
}

template <class T>
void ChromosomalIndex<T>::createIndex()
{
	//clear index
	index_.clear();
	max_length_ = -1;

	//re-create index
	int min = std::numeric_limits<int>::min();
	int max = std::numeric_limits<int>::max();

	int bin_count = 0;
	Chromosome last_chr;
	QVector< QPair<int, int> > chr_indices;
	for (int i=0; i<container_.count(); ++i)
	{
		if (container_[i].chr()!=last_chr)
		{
			if (last_chr.isValid())
			{
				chr_indices.append(QPair<int, int>(max,i-1));
				index_.insert(last_chr.num(), chr_indices);
			}
			chr_indices.clear();
			chr_indices.append(QPair<int, int>(min,i));
			last_chr = container_[i].chr();
			bin_count = 0;
		}
		else if (bin_count==bin_size_)
		{
			chr_indices.append(QPair<int, int>(container_[i].start(),i));
			bin_count = 0;
		}

		max_length_ = std::max(max_length_, container_[i].end() - container_[i].start() + 1);

		++bin_count;
	}

	//add last chromosome index
	chr_indices.append(QPair<int, int>(max,container_.count()-1));
	index_.insert(last_chr.num(), chr_indices);
}

template <class T>
QVector<int> ChromosomalIndex<T>::matchingIndices(const Chromosome& chr, int start, int end) const
{
	QVector<int> matches;

	//chromosome not found
	if (!index_.contains(chr.num())) return matches;

	//find start iterator in index
	const QVector<QPair<int,int> >& pos_array = index_[chr.num()];
	QVector<QPair<int,int> >::const_iterator it = std::lower_bound(pos_array.begin(), pos_array.end(), QPair<int, int>(start, -1), firstOfPairComparator);
	--it;

	//find start in container
	int index = it->second;
	while(index>0 && container_[index].start()>=start-max_length_ && container_[index].chr()==chr)
	{
		--index;
	}
	if (container_[index].chr()!=chr) ++index;

	//find end in container
	while(index<container_.count() && container_[index].start()<end+max_length_ && container_[index].chr()==chr)
	{
		if (container_[index].overlapsWith(start, end))
		{
			matches.append(index);
		}
		++index;
	}

	return matches;
}


template <class T>
int ChromosomalIndex<T>::matchingIndex(const Chromosome& chr, int start, int end) const
{
	//chromosome not found
	if (!index_.contains(chr.num())) return -1;

	//find start iterator in index
	const QVector<QPair<int,int> >& pos_array = index_[chr.num()];
	QVector<QPair<int,int> >::const_iterator it = std::lower_bound(pos_array.begin(), pos_array.end(), QPair<int, int>(start, -1), firstOfPairComparator);
	--it;

	//find start in container
	int index = it->second;
	while(index>0 && container_[index].start()>=start-max_length_ && container_[index].chr()==chr)
	{
		--index;
	}
	if (container_[index].chr()!=chr) ++index;

	//find match
	while(index<container_.count() && container_[index].start()<end+max_length_ && container_[index].chr()==chr)
	{
		if (container_[index].overlapsWith(start, end))
		{
			return index;
		}
		++index;
	}

	return -1;
}


#endif // CHROMOSOMALINDEX_H
