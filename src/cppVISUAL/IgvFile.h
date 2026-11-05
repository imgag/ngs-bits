#ifndef IGVFILE_H
#define IGVFILE_H

#include "Chromosome.h"
#include <QByteArrayList>
#include <QVector>
#include <algorithm>
#include <utility>
#include <limits>

// Numeric IGV record with 1-based, inclusive coordinates.
class IgvFileLine
{
public:
	IgvFileLine(const Chromosome& chr, int start, int end, double value)
		: IgvFileLine(chr, start, end, QVector<double>{value})
	{
	}

	IgvFileLine(const Chromosome& chr, int start, int end, QVector<double> values)
		: chr_(chr), start_(start), end_(end), values_(std::move(values))
	{
	}

	const Chromosome& chr() const { return chr_; }
	int start() const { return start_; }
	int end() const { return end_; }
	double value(int dataset = 0) const { return values_.value(dataset, std::numeric_limits<double>::quiet_NaN()); }
	int datasetCount() const { return values_.count(); }
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
	QVector<double> values_;
};

// Compact records plus the headers used for track names and default settings.
class IgvFile
{
public:
	IgvFile(QVector<IgvFileLine> lines, QByteArrayList headers)
		: lines_(std::move(lines)), headers_(std::move(headers))
	{
		std::sort(lines_.begin(), lines_.end());
		for (const auto& header : headers_)
		{
			if (!header.startsWith('#'))
			{
				dataset_names_ = header.split('\t').mid(4);
				break;
			}
		}
		int count = dataset_names_.count();
		for (const auto& line : lines_) count = std::max(count, line.datasetCount());
		while (dataset_names_.count() < count) dataset_names_.append(QByteArray());
		for (int i = 0; i < dataset_names_.count(); ++i)
		{
			if (dataset_names_[i].isEmpty()) dataset_names_[i] = "Dataset " + QByteArray::number(i + 1);
		}
	}

	const QByteArrayList& headers() const { return headers_; }
	const QByteArrayList& datasetNames() const { return dataset_names_; }
	int datasetCount() const { return dataset_names_.count(); }
	int count() const { return lines_.count(); }
	const IgvFileLine& operator[](int index) const { return lines_[index]; }
	bool isSorted() const { return true; }

private:
	QVector<IgvFileLine> lines_;
	QByteArrayList headers_;
	QByteArrayList dataset_names_;
};

#endif // IGVFILE_H
