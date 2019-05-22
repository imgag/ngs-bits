#ifndef CLINCNVLIST_H
#define CLINCNVLIST_H

#include "cppNGS_global.h"
#include "Chromosome.h"
#include "GeneSet.h"
#include <QList>
#include <QByteArrayList>

///Supported analysis types
enum ClinCnvAnalysisType
{
	CLINCNV_TUMOR_NORMAL_PAIR,
	CLINCNV_GERMLINE_SINGLE,
	CLINCNV_GERMLINE_TRIO,
	CLINCNV_GERMLINE_MULTI
};


class CPPNGSSHARED_EXPORT ClinCnvVariant
{
public:
	///Default constructor.
	ClinCnvVariant();
	///Main constructor.
	ClinCnvVariant(const Chromosome& chr, int start, int end, double copy_number, const QList<double>& log_likelihoods, const QList<double>& qvalues, GeneSet genes, QByteArrayList annotations);

	///Returns the chromosome.
	const Chromosome& chr() const
	{
		return chr_;
	}
	///Returns the start position (1-based).
	int start() const
	{
		return start_;
	}
	///Returns the end position (1-based).
	int end() const
	{
		return end_;
	}
	///Returns the copy-numbers (per sub-region).
	double copyNumber() const
	{
		return copy_number_;
	}

	///Returns all likelihoods (multiple samples/trios) as list, contains only 1 value in case of single samples
	const QList<double>& likelihoods() const
	{
		return log_likelihoods_;
	}

	///Returns all q-scores (multiple samples/trios) as list, contains only 1 value in case of single samples
	const QList<double>& qvalues() const
	{
		return qvalues_;
	}

	///Returns the annotated genes.
	const GeneSet& genes() const
	{
		return genes_;
	}

	///Returns the overall variant size.
	int size() const
	{
		return end_ - start_ + 1;
	}
	///Convert range to string.
	QString toString() const
	{
		return chr_.str() + ":" + QString::number(start_) + "-" + QString::number(end_);
	}
	///Generic annotations (see also CnvList::annotationHeaders()).
	const QByteArrayList& annotations() const
	{
		return annotations_;
	}

	bool overlaps(const Chromosome& chr, int start, int end) const
	{
		return (chr == chr_ && start >= start_ && end <= end_);
	}

private:
	Chromosome chr_;
	int start_;
	int end_;
	double copy_number_;
	QList<double> log_likelihoods_;
	QList<double> qvalues_;
	GeneSet genes_;
	QByteArrayList annotations_;
};

class CPPNGSSHARED_EXPORT ClinCnvList
{
public:
	///Default constructor.
	ClinCnvList();
	///Clears content.
	void clear();
	///Loads CNV text file (TSV format from ClinCNV).
	void load(QString filename);

	///Returns the comment header lines (without leading '##').
	const QByteArrayList& comments() const
	{
		return comments_;
	}
	///Returns the number of variants
	int count() const
	{
		return variants_.count();
	}

	bool isEmpty() const
	{
		if(variants_.count() == 0) return true;
		else return false;
	}

	///Returns a variant by index.
	const ClinCnvVariant& operator[](int index) const
	{
		return variants_[index];
	}
	///Returns annotation headers
	const QByteArrayList& annotationHeaders() const
	{
		return annotation_headers_;
	}

	///Returns annotation index by name
	int annotationIndexByName(const QByteArray& name, bool error_on_mismatch = true) const;

	///Appends copy number variant
	void append(const ClinCnvVariant& add)
	{
		variants_.append(add);
	}
	///Copies Annotation Header and Comments
	void copyMetaData(const ClinCnvList& rhs);

	///Total size of all CNVs in list
	int totalCNVsize();


	ClinCnvAnalysisType type() const;

protected:
	QByteArrayList comments_;
	QList<ClinCnvVariant> variants_;
	QByteArrayList annotation_headers_;
};

#endif // CLINCNVLIST_H
