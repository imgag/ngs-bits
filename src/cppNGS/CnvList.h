#ifndef CNVLIST_H
#define CNVLIST_H

#include "cppNGS_global.h"
#include "Chromosome.h"
#include "GeneSet.h"
#include "BasicStatistics.h"
#include "KeyValuePair.h"
#include "GenomeBuild.h"
#include <QList>
#include <QByteArrayList>
#include <QMap>
#include <QJsonObject>
#include <QDateTime>

///Copy-number variant composed of sub-regions.
class CPPNGSSHARED_EXPORT CopyNumberVariant
{
	public:
		///Default constructor.
		CopyNumberVariant();
		///Minimal constructor.
		CopyNumberVariant(const Chromosome& chr, int start, int end);
		///Main constructor.
		CopyNumberVariant(const Chromosome& chr, int start, int end, int num_regs, GeneSet genes, QByteArrayList annotations);

		///Returns if two CNVs are equal
		bool hasSamePosition(const CopyNumberVariant& rhs) const
		{
			return chr_==rhs.chr_ && start_==rhs.start_ && end_==rhs.end_;
		}

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

		///Sets the start position (1-based).
		void setStart(int start)
		{
			start_ = start;
		}
		///Sets the end position (1-based).
		void setEnd(int end)
		{
			end_ = end;
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
		///Sets the annotated genes.
		void setGenes(const GeneSet& genes)
		{
			genes_ = genes;
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

		///Convert cnv to string wit additional data (size, regions).
		QString toStringWithMetaData() const;


		///Generic annotations (see also CnvList::annotationHeaders()).
		const QByteArrayList& annotations() const
		{
			return annotations_;
		}

		///Returns if a variant overlaps a genomic range.
		bool overlapsWith(const Chromosome& chr, int start, int end) const
		{
			return chr == chr_ && BasicStatistics::rangeOverlaps(start_, end_, start, end);
		}

		///Returns the copy-number. If not available, ProgrammingException is thrown, or '-1' is returned.
		int copyNumber(const QByteArrayList& annotation_headers, bool throw_if_not_found=true) const;
		///Sets the copy-number. If not available, ProgrammingException is thrown, or '-1' is returned. (germline only)
		void setCopyNumber(int cn, const QByteArrayList& annotation_headers, bool throw_if_not_found=true);

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
	CLINCNV_TUMOR_NORMAL_PAIR,
	CLINCNV_TUMOR_ONLY
};

struct CnvListCallData
{
	QString caller = "";
	QString caller_version = "";
	QDateTime call_date;
	QJsonObject quality_metrics;
};

///CNV list
class CPPNGSSHARED_EXPORT CnvList
{
	public:
		///Default constructor.
		CnvList();

		///Returns if the file is valid. It is invalid e.g. after default-construction or calling clear().
		bool isValid() const;

		///Clears content.
		void clear();

		///Loads CNV file (TSV format).
		void load(QString filename);
		///Loads header of CNV file only.
		void loadHeaderOnly(QString filename);

		///Stores CNV text file (TSV format).
		void store(QString filename);

		///Returns the CNV list type
		CnvListType type() const
		{
			return type_;
		}
		///Returns the CNV list type
		QString typeAsString() const;

		///Returns the CNV caller
		CnvCallerType caller() const;
		///Returns the CNV caller as string
		QString callerAsString() const;


		///Returns the genome build from the header or an empty string if it could not be determined.
		QByteArray build();

		///Returns the comment header lines (with leading '##').
		const QByteArrayList& comments() const
		{
			return comments_;
		}
		///Parses the qc metric value from the comment header lines.
		QByteArray qcMetric(QString name, bool throw_if_missing=true) const;

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
		int annotationIndexByName(const QByteArray& name, bool throw_on_error, bool contains = false) const;

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
			annotation_header_desc_ = rhs.annotation_header_desc_;
		}

		///Returns the size sum of all all CNVs
		long long totalCnvSize() const;

		///Returns call data from original file (e.g. version, caller...), specify ps_name in case of CNVHunter samples
		static CnvListCallData getCallData(const CnvList& cnvs, QString filename, QString ps_name = "", bool ignore_inval_header_lines = false);


		///Returns the reference copy number for a given CNV
		static int determineReferenceCopyNumber(const CopyNumberVariant& cnv, const QString& gender, GenomeBuild build);

	protected:
		CnvListType type_;
		QByteArrayList comments_;
		QByteArrayList annotation_headers_;
		QMap<QByteArray, QByteArray> annotation_header_desc_;
		QList<CopyNumberVariant> variants_;

		///split key-value pair from file header based on separator
		static KeyValuePair split(const QByteArray& string, char sep);
		///loads file with our without header.
		void loadInternal(QString filename, bool header_only);
};

#endif // CNVLIST_H
