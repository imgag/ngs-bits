#ifndef CNVLIST_H
#define CNVLIST_H

#include "cppNGS_global.h"
#include "Chromosome.h"
#include "GeneSet.h"
#include <QList>
#include <QByteArrayList>

///Copy-number variant composed of sub-regions as reported by CnvHunter.
class CPPNGSSHARED_EXPORT CopyNumberVariant
{
	public:
		///Default constructor.
		CopyNumberVariant();
		///Main constructor.
		CopyNumberVariant(const Chromosome& chr, int start, int end, QByteArrayList regions, QList<int> cns, QList<double> z_scores, QList<double> afs, GeneSet genes, QByteArrayList annotations);

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
		///Returns the sub-regions.
		const QByteArrayList& regions() const
		{
			return regions_;
		}
		///Returns the copy-numbers (per sub-region).
		const QList<int>& copyNumbers() const
		{
			return cns_;
		}
		///Returns the z-scores (per sub-region).
		const QList<double>& zScores() const
		{
			return z_scores_;
		}
		///Returns the CNV allele frequencies (per sub-region).
		const QList<double>& alleleFrequencies() const
		{
			return afs_;
		}
		///Returns the annotated genes.
		const GeneSet& genes() const
		{
			return genes_;
		}

		///Returns the overall region count.
		int regionCount() const
		{
			return regions_.count();
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
		QByteArrayList regions_;
		QList<int> cns_;
		QList<double> z_scores_;
		QList<double> afs_;
		GeneSet genes_;
		QByteArrayList annotations_;
};

class CPPNGSSHARED_EXPORT CnvList
{
	public:
		///Default constructor.
		CnvList();
		///Clears content.
		void clear();
		///Loads CNV text file (TSV format from CnvHunter).
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
		///Returns a variant by index.
		const CopyNumberVariant& operator[](int index) const
		{
			return variants_[index];
		}
		///Returns annotation headers
		const QByteArrayList& annotationHeaders() const
		{
			return annotation_headers_;
		}
		///Appends copy number variant
		void append(const CopyNumberVariant& add)
		{
			variants_.append(add);
		}
		///Copies Annotation Header and Comments
		void copyMetaData(const CnvList& rhs);

	protected:
		QByteArrayList comments_;
		QList<CopyNumberVariant> variants_;
		QByteArrayList annotation_headers_;
};

#endif // CNVLIST_H
