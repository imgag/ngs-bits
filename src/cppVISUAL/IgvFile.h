#ifndef IGVFILE_H
#define IGVFILE_H

#include "Chromosome.h"
#include <QByteArrayList>
#include <QVector>
#include <algorithm>
#include <utility>

// Numeric IGV record with 1-based, inclusive coordinates.
class IgvFileLine
{
public:
	IgvFileLine(const Chromosome& chr, int start, int end, double value)
		: chr_(chr), start_(start), end_(end), value_(value)
	{
	}

	const Chromosome& chr() const { return chr_; }
	int start() const { return start_; }
	int end() const { return end_; }
	double value() const { return value_; }
	bool overlapsWith(int start, int end) const { return start_ <= end && end_ >= start; }
	bool operator<(const IgvFileLine& rhs) const
	{
		if (chr_ != rhs.chr_) return chr_ < rhs.chr_;
		if (start_ != rhs.start_) return start_ < rhs.start_;
		return end_ < rhs.end_;
	}

private:
	Chromosome chr_;
	int start_;
	int end_;
	double value_;
};

// Compact records plus the headers used for track names and default settings.
class IgvFile
{
public:
	IgvFile(QVector<IgvFileLine> lines, QByteArrayList headers)
		: lines_(std::move(lines)), headers_(std::move(headers))
	{
		std::sort(lines_.begin(), lines_.end());
	}

	const QByteArrayList& headers() const { return headers_; }
	int count() const { return lines_.count(); }
	const IgvFileLine& operator[](int index) const { return lines_[index]; }
	bool isSorted() const { return true; }

private:
	QVector<IgvFileLine> lines_;
	QByteArrayList headers_;
};

#endif // IGVFILE_H
