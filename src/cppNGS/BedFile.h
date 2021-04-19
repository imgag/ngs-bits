#ifndef BEDFILE_H
#define BEDFILE_H

#include "cppNGS_global.h"
#include "Chromosome.h"
#include "BasicStatistics.h"

#include <QVector>
#include <QSet>
#include <QByteArrayList>

///Representation of a BED file line (1-based)
class CPPNGSSHARED_EXPORT BedLine
{
public:
	///Default constructor (creates an invalid region - needed for containers only).
    BedLine();
    ///Constructor.
	BedLine(const Chromosome& chr, int start, int end, const QByteArrayList& annotations = QByteArrayList());

	///Returns if the region is valid.
	bool isValid() const
	{
		return chr_!="" && start_>=0 && start_<=end_;
	}

    ///Returns the chromosome.
    const Chromosome& chr() const
    {
        return chr_;
    }
    ///Sets the choromosome.
    void setChr(const Chromosome& chr)
    {
        chr_ = chr;
    }

    ///Returns the start position (1-based).
    int start() const
    {
        return start_;
    }
    ///Sets the start position (1-based).
    void setStart(int start)
    {
        start_ = start;
    }

    ///Returns the end position (1-based).
    int end() const
    {
        return end_;
    }
    ///Sets the end position (1-based).
    void setEnd(int end)
    {
        end_ = end;
    }

    ///Return the lentgh of the chromosomal range.
    int length() const
    {
		// +1 since the BED coordinates have been converted into a 1-based fully-closed format
		return end_ - start_ + 1;
    }

    ///Read-only access to the annotations list.
	const QByteArrayList& annotations() const
    {
        return annotations_;
    }
    ///Read-write access to the annotations list.
	QByteArrayList& annotations()
    {
        return annotations_;
    }

    ///Less-than operator.
    bool operator<(const BedLine& rhs) const;
    ///Equality oerator (does not compare the annotations!).
    bool operator==(const BedLine& rhs) const
    {
        return chr_==rhs.chr_ && start_==rhs.start_ && end_==rhs.end_;
    }
	///Overlap check for chromosome and position range.
    bool overlapsWith(const Chromosome& chr, int start, int end) const
    {
        return chr_==chr && overlapsWith(start, end);
    }
    ///Overlap check for position range only.
    bool overlapsWith(int start, int end) const
    {
		return BasicStatistics::rangeOverlaps(start_, end_, start, end);
    }
    ///Adjacent check - with no gap in between - for chromosome and position range.
	bool adjacentTo(const Chromosome& chr, int start, int end) const
    {
        return chr_==chr && adjacentTo(start, end);
    }
    ///Adjacent check - with no gap in between - for position range only.
	bool adjacentTo(int start, int end) const
    {
        return (start_==end+1 || end_==start-1);
	}

	///Converts the position part of the line to a string
	QString toString(bool human_readable) const
	{
		if (human_readable)
		{
			return chr().strNormalized(true) + ":" + QString::number(start()) + "-" + QString::number(end());
		}
		else
		{
			return chr().strNormalized(true) + "\t" + QString::number(start()) + "\t" + QString::number(end());
		}
	}

	///Parses a chromosomal region string and constructs a line from it. An invalid line is returned, if the string cannot be parsed.
	static BedLine fromString(QString str);

protected:
    Chromosome chr_;
    int start_;
    int end_;
	QByteArrayList annotations_;
};

///Representation of a BED file (header lines are not handled, 1-based)
class CPPNGSSHARED_EXPORT BedFile
{
public:
    ///Default constructor
    BedFile();
	///Convenience constructor that constructs a file with a single-region.
	BedFile(const Chromosome& chr, int start, int end);
    ///Adds a line. Throws ArgumentException.
    void append(const BedLine& line);
    ///Read-only accessor to a single line.
    const BedLine& operator[](int index) const
    {
        return lines_[index];
    }
    ///Read-write accessor to a single line.
    BedLine& operator[](int index)
    {
        return lines_[index];
    }
    ///Returns the line count.
    int count() const
    {
        return lines_.count();
	}
    ///Returns the number of bases summed up over all elements. This method does not consider if elements overlap - they are counted several times then.
    long long baseCount() const;
	///Retuns if there are no lines.
	bool isEmpty() const
	{
		return lines_.isEmpty();
	}

	///Returns the contained chromosomes
	QSet<Chromosome> chromosomes() const;
	const QVector<QByteArray>& headers() const
	{
		return headers_;
	}
	void setHeaders(const QVector<QByteArray>& headers)
	{
		headers_ = headers;
	}
	void appendHeader(const QByteArray& header)
	{
		headers_.append(header);
	}

    ///Loads a 0-based BED file and converts it to 1-based positions. Throws ParseException.
	void load(QString filename, bool stdin_if_empty=true);
	///Stores this 1-based representation to a BED file with 0-based coordinates. If no filename is given, the output goes to the console. Throws ParseException and FileException.
	void store(QString filename, bool stdout_if_empty=true) const;
	///Convert this 1-based representation to a BED-like text. Throws ParseException and FileException.
	QString toText() const;

    ///Removes all content.
    void clear()
    {
		headers_.clear();
        lines_.clear();
    }
    ///Clears annotations of all regions.
    void clearAnnotations();
	///Clears headers.
	void clearHeaders();

	///Sorts the lines accoring to chromosome (lexicographical) and start/end position (ascending).
	void sort();
	///Sorts the lines accoring to chromosome (lexicographical), start/end position (ascending) and name column (ascending).
	void sortWithName();
	///Removes duplicate entries. Throws a ProgrammingException if not sorted.
	void removeDuplicates();

	///Merges overlapping regions (by default also merges back-to-back/bookshelf regions).
	void merge(bool merge_back_to_back = true, bool merge_names = false, bool merged_names_unique = false);
    ///Extends the regions by @p n bases in both directions.
    void extend(int n);
    ///Shrinks the regions by @p n bases in both direactions.
    void shrink(int n);
	///Adds the regions from the given file.
	void add(const BedFile& file2);
    ///Subtracts the regions in the given file.
    void subtract(const BedFile& file2);
    ///Removed all parts of regions that do not overlap with the given file.
    void intersect(const BedFile& file2);
    ///Removes all regions that do not overlap with the given file.
    void overlapping(const BedFile& file2);
    ///Splits all regions to chunks of an approximate size.
    void chunk(int size);

    ///Returns if the lines are sorted. See sort().
    bool isSorted() const;
    ///Returns if the lines are merged.
    bool isMerged() const;
    ///Returns if the lines are merged and sorted (can be computed faster than calling both methods separately).
    bool isMergedAndSorted() const;
	///Returns if the given chromosomal position is in the BED file region. Note that is method is slow when too many lines are present. Use ChromosomalIndex<BedFile> in this case!
    bool overlapsWith(const Chromosome& chr, int start, int end) const;

	///Creates a BED file from a string
	static BedFile fromText(const QByteArray& string);

protected:
    ///Removes empty lines.
    void removeInvalidLines();

	QVector<QByteArray> headers_;
	QVector<BedLine> lines_;

private:
	///Comparator helper class used by sortWithName
	class LessComparatorWithName
	{
		public:
			bool operator()(const BedLine &a, const BedLine &b) const;
	};
};

#endif // BEDFILE_H
