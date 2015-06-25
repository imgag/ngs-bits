#ifndef BEDLINE_H
#define BEDLINE_H

#include "cppNGS_global.h"
#include <QStringList>

///Representation of a BED file line (1-based)
class CPPNGSSHARED_EXPORT BedLine
{
public:
	///Default constructor (needed mainly for containers).
	BedLine();
	///Constructor.
	BedLine(const QString& chr, int start, int end, const QStringList& annotations = QStringList());

	///Returns the chromosome.
	const QString& chr() const;
	///Sets the choromosome.
	void setChr(const QString& chr);
	///Returns the start position (1-based).
	int start() const;
	///Sets the start position (1-based).
	void setStart(int start);
	///Returns the end position (1-based).
	int end() const;
	///Sets the end position (1-based).
	void setEnd(int end);
	///Return the lentgh of the chromosomal range.
	int length() const;

	///Read-only access to the annotations list.
	const QStringList& annotations() const;
	///Read-write access to the annotations list.
	QStringList& annotations();

	///Less-than operator.
	bool operator<(const BedLine& rhs) const;
	///Equality oerator.
	bool operator==(const BedLine& rhs) const;
	///Overlap check for chromosome and position range.
	bool overlapsWith(const QString& chr, int start, int end) const;
	///Overlap check for position range only.
	bool overlapsWith(int start, int end) const;
	///Adjacent check - with no gap in between - for chromosome and position range.
	bool adjacentTo(const QString& chr, int start, int end);
	///Adjacent check - with no gap in between - for position range only.
	bool adjacentTo(int start, int end);

protected:
	QString chr_;
	int start_;
	int end_;
	QStringList annotations_;
};

#endif // BEDLINE_H
