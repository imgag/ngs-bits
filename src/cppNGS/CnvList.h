#ifndef CNVLIST_H
#define CNVLIST_H

#include "cppNGS_global.h"
#include "Chromosome.h"
#include "GeneSet.h"
#include "BasicStatistics.h"
#include <QList>
#include <QByteArrayList>
#include <QMap>

///Copy-number variant composed of sub-regions.
class CPPNGSSHARED_EXPORT CopyNumberVariant
{
	public:
		///Default constructor.
		CopyNumberVariant();
		///Main constructor.
		CopyNumberVariant(const Chromosome& chr, int start, int end, int num_regs, GeneSet genes, QByteArrayList annotations);

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
		///Returns the number of regions/exons.
		int regions() const
		{
			return num_regs_;
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

		///Convert cnv to string.
		QString toStringWithMetaData() const;


		///Generic annotations (see also CnvList::annotationHeaders()).
		const QByteArrayList& annotations() const
		{
			return annotations_;
		}

		///Retuns if a variant overlaps a genomic range.
		bool overlapsWith(const Chromosome& chr, int start, int end) const
		{
			return chr == chr_ && BasicStatistics::rangeOverlaps(start_, end_, start, end);
		}

	protected:
		Chromosome chr_;
		int start_;
		int end_;
		int num_regs_;
		GeneSet genes_;
		QByteArrayList annotations_;
};

///CNV caller types
enum class CnvCallerType
{
	INVALID,
	CNVHUNTER,
	CLINCNV
};

///CNV list types
enum class CnvListType
{
	INVALID,
	CNVHUNTER_GERMLINE_SINGLE,
	CNVHUNTER_GERMLINE_MULTI,
	CLINCNV_GERMLINE_SINGLE,
	CLINCNV_GERMLINE_MULTI,
	CLINCNV_TUMOR_NORMAL_PAIR
};

///CNV list
class CPPNGSSHARED_EXPORT CnvList
{
	public:
		///Default constructor.
		CnvList();
		///Clears content.
		void clear();
		///Loads CNV text file (TSV format).
		void load(QString filename);

		///Returns the CNV list type
		CnvListType type() const
		{
			return type_;
		}

		///Returns the CNV caller
		CnvCallerType caller() const;

		///Returns the comment header lines (without leading '##').
		const QByteArrayList& comments() const
		{
			return comments_;
		}

		///Returns annotation headers
		const QByteArrayList& annotationHeaders() const
		{
			return annotation_headers_;
		}
		///Returns the annotation header description or '' if unset.
		QByteArray headerDescription(QByteArray name) const;
		///Sets an annotation header description
		void setHeaderDesciption(QByteArray name, QByteArray desciption);

		///Returns the index of an annotation. -1 is returned if not present and -2 if present multiple times.
		int annotationIndexByName(const QByteArray& name, bool throw_on_error) const;

		///Returns the number of variants
		int count() const
		{
			return variants_.count();
		}

		///Returns if the list is empty
		int isEmpty() const
		{
			return variants_.isEmpty();
		}

		///Returns a variant by index.
		const CopyNumberVariant& operator[](int index) const
		{
			return variants_[index];
		}

		///Appends copy number variant
		void append(const CopyNumberVariant& add)
		{
			variants_.append(add);
		}

		///Copies meta data from 'rhs' object.
		void copyMetaData(const CnvList& rhs)
		{
			type_ = rhs.type_;
			comments_ = rhs.comments_;
			annotation_headers_ = rhs.annotation_headers_;
		}

		///Returns the size sum of all all CNVs
		long long totalCnvSize();

	protected:
		CnvListType type_;
		QByteArrayList comments_;
		QByteArrayList annotation_headers_;
		QMap<QByteArray, QByteArray> annotation_header_desc_;
		QList<CopyNumberVariant> variants_;
};

#endif // CNVLIST_H
