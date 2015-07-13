#ifndef BEDFILE_H
#define BEDFILE_H

#include "cppNGS_global.h"
#include "Chromosome.h"

#include <QStringList>
#include <QVector>

///Representation of a BED file line (1-based)
class CPPNGSSHARED_EXPORT BedLine
{
public:
    ///Default constructor (needed mainly for containers).
    BedLine();
    ///Constructor.
    BedLine(const Chromosome& chr, int start, int end, const QStringList& annotations = QStringList());

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
        return end_ - start_ + 1;
    }

    ///Read-only access to the annotations list.
    const QStringList& annotations() const
    {
        return annotations_;
    }
    ///Read-write access to the annotations list.
    QStringList& annotations()
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
        return (start_>=start && start_<=end) || (start>=start_ && start<=end_);
    }
    ///Adjacent check - with no gap in between - for chromosome and position range.
    bool adjacentTo(const Chromosome& chr, int start, int end)
    {
        return chr_==chr && adjacentTo(start, end);
    }
    ///Adjacent check - with no gap in between - for position range only.
    bool adjacentTo(int start, int end)
    {
        return (start_==end+1 || end_==start-1);
    }

	///Converts the position part of the line to a string
	QString toString(char delimiter = ' ')
	{
		return chr().str() + delimiter + QString::number(start()) + delimiter + QString::number(end());
	}

protected:
    Chromosome chr_;
    int start_;
    int end_;
    QStringList annotations_;
};

///Representation of a BED file (header lines are not handled, 1-based)
class CPPNGSSHARED_EXPORT BedFile
{
public:
    ///Default constructor
    BedFile();
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
    long baseCount() const;

    ///Loads a 0-based BED file and converts it to 1-based positions. Throws ParseException.
    void load(QString filename);
	///Stores this 1-based representation to a BED file with 0-based coordinates. If no filename is given, the output goes to the console. Throws ParseException and FileException.
    void store(QString filename, QString header="");

    ///Removes all content.
    void clear()
    {
        lines_.clear();
    }
    ///Clears annotations of all regions.
    void clearAnnotations();

    ///Sorts the lines accoring to chromosome (lexicographical) and start position (ascending). If the @p uniq is @em true, duplicate entries are removed after sorting.
    void sort(bool uniq = false);
	///Merges overlapping regions (by default also merges back-to-back/bookshelf regions).
	void merge(bool merge_back_to_back = true, bool merge_names = false);
    ///Extends the regions by @p n bases in both directions.
    void extend(int n);
    ///Shrinks the regions by @p n bases in both direactions.
    void shrink(int n);
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
    ///Returns if the given chromosomal position is in the BED file region
    bool overlapsWith(const Chromosome& chr, int start, int end) const;

protected:
    ///Removes empty lines.
    void removeInvalidLines();

    QVector<BedLine> lines_;
};

#endif // BEDFILE_H
