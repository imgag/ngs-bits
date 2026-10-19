#ifndef SEGFILE_H
#define SEGFILE_H

#include "IgvFile.h"
#include "BasicStatistics.h"
#include <cmath>
#include <limits>

//SEG coordinates are 1-based and inclusive. The first (sample ID) column is ignored.
class SegFileLine : public IgvFileLine
{
public:
	SegFileLine(const Chromosome& chr, int start, int end, QByteArray annotation, double value)
		: IgvFileLine(chr, start, end, value), annotation_(std::move(annotation))
	{
		bool ok = false;
		variance_ = annotation_.toDouble(&ok);
		if (!ok || !std::isfinite(variance_) || variance_ < 0) variance_ = std::numeric_limits<double>::quiet_NaN();
	}

	const QByteArray& annotation() const { return annotation_; }
	double variance() const { return variance_; }

private:
	QByteArray annotation_;
	double variance_;
};

class SegFile
{
public:
	SegFile(QVector<SegFileLine> lines, QByteArrayList headers, QByteArray annotation_name)
		: lines_(std::move(lines)), headers_(std::move(headers)), annotation_name_(std::move(annotation_name))
	{
		std::sort(lines_.begin(), lines_.end());
		QVector<double> variances;
		variances.reserve(lines_.size());
		for (const auto& line : lines_)
		{
			if (std::isfinite(line.variance())) variances.append(line.variance());
		}
		if (!variances.isEmpty())
		{
			std::sort(variances.begin(), variances.end());
			variance_median_ = BasicStatistics::median(variances);
			variance_mad_ = BasicStatistics::mad(variances, variance_median_);
			variance_stdev_ = 1.482 * variance_mad_;
		}
	}

	const QByteArrayList& headers() const { return headers_; }
	const QByteArray& annotationName() const { return annotation_name_; }
	int count() const { return lines_.count(); }
	const SegFileLine& operator[](int index) const { return lines_[index]; }
	bool isSorted() const { return true; }
	double varianceMedian() const { return variance_median_; }
	double varianceMad() const { return variance_mad_; }
	double varianceStdev() const { return variance_stdev_; }

	//Only unusually high variances fade. Missing variances remain fully opaque.
	double varianceAlpha(double variance) const
	{
		if (!std::isfinite(variance) || variance < 0 || !std::isfinite(variance_median_)) return 1.0;
		if (variance_stdev_ == 0) return variance > variance_median_ ? 0.3 : 1.0;
		const double z = (variance - variance_median_) / variance_stdev_;
		return 1.0 - 0.7 * std::clamp((z - 2.5) / 1.5, 0.0, 1.0);
	}

private:
	QVector<SegFileLine> lines_;
	QByteArrayList headers_;
	QByteArray annotation_name_;
	double variance_median_ = std::numeric_limits<double>::quiet_NaN();
	double variance_mad_ = 0;
	double variance_stdev_ = 0;
};

#endif // SEGFILE_H
