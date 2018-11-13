#ifndef CLINCNVLIST_H
#define CLINCNVLIST_H

#include "cppNGS_global.h"
#include "Chromosome.h"
#include "GeneSet.h"
#include <QList>
#include <QByteArrayList>

class CPPNGSSHARED_EXPORT ClinCopyNumberVariant
{
public:
	///Default constructor.
	ClinCopyNumberVariant();
	///Main constructor.
	ClinCopyNumberVariant(const Chromosome& chr, int start, int end, double copy_number, double log_likelihood, GeneSet genes, QByteArrayList annotations);

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

	///Returns the log likelihood
	double likelihood() const
	{
		return log_likelihood_;
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

protected:
	Chromosome chr_;
	int start_;
	int end_;
	double copy_number_;
	double log_likelihood_;
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
	const ClinCopyNumberVariant& operator[](int index) const
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
	void append(const ClinCopyNumberVariant& add)
	{
		variants_.append(add);
	}
	///Copies Annotation Header and Comments
	void copyMetaData(const ClinCnvList& rhs);

protected:
	QByteArrayList comments_;
	QList<ClinCopyNumberVariant> variants_;
	QByteArrayList annotation_headers_;
};

#endif // CLINCNVLIST_H
