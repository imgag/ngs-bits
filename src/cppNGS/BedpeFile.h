#ifndef BEDPEFILE_H
#define BEDPEFILE_H

#include "cppNGS_global.h"
#include "Chromosome.h"
#include "Helper.h"
#include "BedFile.h"
#include "GeneSet.h"
#include "VariantList.h"
#include <QByteArrayList>
#include <QMap>

enum StructuralVariantType
{
	DEL, //deletion
	DUP, //duplication (tandem)
	INS, //insertion
	INV, //inversion
	BND,  //breakpoint (translocations, etc)
	UNKNOWN
};

///Converts a StructuralVariantType to QString
QString CPPNGSSHARED_EXPORT StructuralVariantTypeToString(StructuralVariantType type);
///Converts a QString to the correct StructuralVariantType
StructuralVariantType CPPNGSSHARED_EXPORT StructuralVariantTypeFromString(QString type_string);

class CPPNGSSHARED_EXPORT BedpeLine
{
public:
	BedpeLine();
	BedpeLine(const Chromosome& chr1, int start1, int end1, const Chromosome& chr2, int start2, int end2, StructuralVariantType type, const QList<QByteArray>& annotations);

	const Chromosome& chr1() const
	{
		return chr1_;
	}
	void setChr1(const Chromosome& chr1)
	{
		chr1_ = chr1;
	}

	int start1() const
	{
		return start1_;
	}

	void setStart1(int start1)
	{
		start1_ = start1;
	}
	int end1() const
	{
		return end1_;
	}
	void setEnd1(int end1)
	{
		end1_ = end1;
	}
	int start2() const
	{
		return start2_;
	}
	void setStart2(int start2)
	{
		start2_ = start2;
	}
	int end2() const
	{
		return end2_;
	}
	void setEnd2(int end2)
	{
		end2_ = end2;
	}
	const Chromosome& chr2() const
	{
		return chr2_;
	}
	void setChr2(const Chromosome& chr2)
	{
		chr2_ = chr2;
	}

	StructuralVariantType type() const
	{
		return type_;
	}

	const QList<QByteArray>& annotations() const
	{
		return annotations_;
	}
	void setAnnotations(const QList<QByteArray>& annos)
	{
		annotations_ = annos;
	}

	///Converts line into tsv format
	QByteArray toTsv() const;

	///Lessthan: Compares two bedpe-lines by their first two columns (chr1 and start1), if equal it uses chr2 and start2.
	bool operator<(const BedpeLine& rhs) const
	{
		if(chr1_ < rhs.chr1()) return true;
		if(chr1_ == rhs.chr1() && start1_ < rhs.start1()) return true;
		if(chr1_ == rhs.chr1() && start1_ == rhs.start1() && chr2_ < rhs.chr2()) return true;
		if(chr1_ == rhs.chr1() && start1_ == rhs.start1() && chr2_ == rhs.chr2() && start2_ < rhs.start2()) return true;

		return false;
	}

	///Returns if a structural variant intersects with the given regions
	///    (if imprecise_breakpoints == true also include confidence intervall for BND and INS into intersection)
	bool intersectsWith(const BedFile& regions, bool imprecise_breakpoints = false) const;

	///Returns position 1 as string
	QString position1() const;
	///Returns position 2 as string
	QString position2() const;
	///Returns the range (for DEL, DUP, INV) or position 1 (for INS, BND) as string
	QString positionRange() const;

	///Returns the size: range for DEL, DUP and INV; -1 for INS and BND.
	int size() const;

	///Returns the affected chromosomal region als BED file
	BedFile affectedRegion() const;

	///Returns the SV as String
	QString toString();

	///Returns the value of a given FORMAT key:
	///		(only for germline single samples)
    QByteArray formatValueByKey(QByteArray format_key, const QList<QByteArray>& annotation_headers, bool error_on_mismatch=true, QByteArray format_header_name="FORMAT", int sample_idx = 0) const;

	///Returns the genes as GeneSet
	GeneSet genes(const QList<QByteArray>& annotation_headers, bool error_on_mismatch=true) const;


protected:
	Chromosome chr1_;
	int start1_;
	int end1_;

	Chromosome chr2_;
	int start2_;
	int end2_;

	StructuralVariantType type_;

	QList<QByteArray> annotations_;

	/// Convert position to string representation
	static QByteArray posToString(int in)
	{
		if(in == -1) return ".";

		return QByteArray::number(in);
	}
};

enum BedpeFileFormat
{
	BEDPE_GERMLINE_SINGLE,
	BEDPE_SOMATIC_TUMOR_ONLY,
	BEDPE_SOMATIC_TUMOR_NORMAL,
    BEDPE_GERMLINE_MULTI,
    BEDPE_GERMLINE_TRIO,
	BEDPE_RNA,
};

class CPPNGSSHARED_EXPORT BedpeFile
{
public:
	///Default constructor (of an invalid file).
	BedpeFile();

	///Loads the file
	void load(const QString& file_name);

	///Returns if the file is valid. It is invalid e.g. after default-construction or calling clear().
	bool isValid() const;

	///Returns if the file contains somatic structural variants.
	bool isSomatic() const;

	void clear()
	{
		annotation_headers_.clear();
		comments_.clear();
		lines_.clear();
	}

	int count() const
	{
		return lines_.count();
	}
	const QList<QByteArray> comments() const
	{
		return comments_;
	}

	///Read-only access to members
	const BedpeLine& operator[](int index) const
	{
		return lines_[index];
	}
	///Read-write access to members
	BedpeLine& operator[](int index)
	{
		return lines_[index];
	}

	///Append BedpeLine to BedpeFile
	void append(const BedpeLine& line)
	{
		lines_.append(line);
	}

    ///Remove structural variant from list at index.
    void removeAt(int index)
    {
        lines_.removeAt(index);
    }

	///returns index of annotation, -1 if not found
	int annotationIndexByName(const QByteArray& name, bool error_on_mismatch = true) const;

	///returns annotation headers
	const QList<QByteArray> annotationHeaders() const
	{
		return annotation_headers_;
	}
	///Sets the annotation headers for the BEDPE file
	void setAnnotationHeaders(QList<QByteArray> annotation_headers)
	{
		annotation_headers_ = annotation_headers;
	}

    ///returns the sample header info of multisample BEDPE files
    const SampleHeaderInfo sampleHeaderInfo() const
    {
        return sample_header_info_;
    }

	QByteArray annotationDescriptionByName(QByteArray name)
	{
		return annotation_descriptions_.value(name, "");
	}

	///Get description of INFO columns by ID, e.g. INFO,FILTER,FORMAT
	QMap<QByteArray,QByteArray> metaInfoDescriptionByID(const QByteArray& name);

	///Sorts Bedpe file (by columns chr1, start1, chr2, start2)
	void sort();

	///Stores file as TSV
	void toTSV(QString file_name);

	///Returns bedpe type according entry in file comments ##fileformat=
	BedpeFileFormat format() const;

	///Converts type string to enum
	static StructuralVariantType stringToType(const QByteArray& str);
	static QByteArray typeToString(StructuralVariantType type);

	///Returns the estimated SV size (absolut) for a given SV (index)
	///		If SVLEN in INFO_A is given, this length is returned.
	///		For INS without SVLEN the sum of LEFT_SVINSSEQ and RIGHT_SVINSSEQ (known left/right inserted bases)
	///		For BND return -1.
	int estimatedSvSize(int index) const;

	///Returns the index of the BedpeLine which matches the given SV, -1 if not found
	///     NOTICE: 'deep_ins_compare' will perform a left-shift and a sequence comparison. In this case headers
	///             of the given BedpeLine has to match the headers of this file
	///		NOTICE: if 'compare_ci' is set to true, SVs which have overlapping confidence intervalls are also considered as match
	///
	int findMatch(const BedpeLine& sv, bool deep_ins_compare = false, bool error_on_mismatch = true, bool compare_ci=true) const;

private:
    void parseSampleHeaderInfo();
	QList<QByteArray> annotation_headers_;
    /// annotation description in file header: ##DESCRIPTION=KEY=VALUE
	QMap<QByteArray, QByteArray> annotation_descriptions_;
	QList<QByteArray> comments_;
    SampleHeaderInfo sample_header_info_; //contains sample info of trio/multisample
	QList<BedpeLine> lines_;

	///Returns all information fields with "NAME=" as list of QMAP containing key value pairs
	QList< QMap<QByteArray,QByteArray> > getInfos(QByteArray name);

	///Returns map with key-value pairs for vcf info line in header, beginning after e.g. INFO= or FORMAT=
	QMap<QByteArray,QByteArray> parseInfoField(QByteArray unparsed_fields);

	///Converts input position to integer, we set position to -1 if invalid input
	static int parsePosIn(const QByteArray& in)
	{
		bool ok = false;

		int out = in.trimmed().toInt(&ok);
		if(!ok) return -1;

		return out;
	}
};

#endif // BEDPEFILE_H
