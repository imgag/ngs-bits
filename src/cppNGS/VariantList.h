#ifndef VARIANTLIST_H
#define VARIANTLIST_H

#include "cppNGS_global.h"
#include "BedFile.h"
#include "VariantAnnotationDescription.h"
#include "VariantFilter.h"
#include <QVector>
#include <QStringList>
#include <QtAlgorithms>
#include <QVectorIterator>
#include "FastaFileIndex.h"

///Genetic variant or mutation (1-based).
class CPPNGSSHARED_EXPORT Variant
{
public:
    ///Default constructor.
    Variant();
    ///Convenience constructor.
    Variant(const Chromosome& chr, int start, int end, const Sequence& ref, const Sequence& obs, const QList<QByteArray>& annotations = QList<QByteArray>(), int filter_index = -1);

    ///Returns the chromosome.
    const Chromosome& chr() const
    {
        return chr_;
    }
    ///Sets the chromosome.
    void setChr(const Chromosome& chr)
    {
        chr_ = chr;
    }
    ///Returns the start position (1-based).
    int start() const
    {
        return start_;
    }
    ///Sets the start position (1-based).
    void setStart(int start)
    {
        start_ = start;
    }
    ///Returns the end position (1-based).
    int end() const
    {
        return end_;
    }
    ///Sets the end position (1-based).
    void setEnd(int end)
    {
        end_ = end;
    }
    ///Returns the reference base.
	const Sequence& ref() const
    {
        return ref_;
    }
    ///Sets the reference base.
	void setRef(const Sequence& ref)
    {
        ref_ = ref;
    }
    ///Returns the observed base.
	const Sequence& obs() const
    {
        return obs_;
    }
    ///Sets the observed base.
	void setObs(const Sequence& obs)
    {
        obs_ = obs;
    }

    ///Read-only access to the annotations.
	const QList<QByteArray>& annotations() const
    {
        return annotations_;
    }
    ///Read-write access to the annotations.
	QList<QByteArray>& annotations()
    {
        return annotations_;
    }

    ///Convenience access to the filter annotation column (split by ';', trimmed).
    const QList<QByteArray>& filters() const
    {
        return filters_;
    }

    ///Less-than operator.
    bool operator<(const Variant& rhs) const;
    ///Overlap check for chromosome and position range.
    bool overlapsWith(const Chromosome& chr, int start, int end) const
    {
        return (chr_==chr && overlapsWith(start, end));
    }
    ///Overlap check for position range only.
    bool overlapsWith(int start, int end) const
    {
        return (start_>=start && start_<=end) || (start>=start_ && start<=end_);
    }
	///Overlap check BED file line.
	bool overlapsWith(const BedLine& line) const
	{
		return overlapsWith(line.chr(), line.start(), line.end());
	}
	///Returns if the variant is a SNV
    bool isSNV() const
    {
		return obs_.length()==1 && ref_.length()==1 && obs_!="-" && ref_!="-";
    }
    ///Returns the coordinates and base exchange as a string e.g. "chr1:3435345-3435345 A>G"
    QString toString() const
    {
		return chr_.str() + ":" + QString::number(start_) + "-" + QString::number(end_) + " " + ref_ + ">" + obs_;
    }
    /// Removes the common prefix/suffix from indels, adapts the start/end position and replaces empty sequences with a custom string.
	void normalize(const Sequence& empty_seq="");

    ///Auxilary function: Removes common prefix and suffix bases from indels and adapts the start position accordingly.
	static void normalize(int& start, Sequence& ref, Sequence& obs);
    ///Auxilary function: Returns the smallest repeated subsequence of an indel or the complete input sequence if it has no repeat.
	static Sequence minBlock(const Sequence& seq);
	///Auxilary function: Returns the repeat region of an indel (1-based, closed interval).
	///@note Returns the original start/end position if the variant is a SNV, a complex index or not in a repeat region.
	///@note Expects 1-based closed intervals are positions (insertions are after given position).
	static QPair<int, int> indelRegion(const Chromosome& chr, int start, int end, Sequence ref, Sequence obs, const FastaFileIndex& reference);

protected:
    Chromosome chr_;
    int start_;
    int end_;
	Sequence ref_;
	Sequence obs_;
    QList<QByteArray> filters_;
	QList<QByteArray> annotations_;

};

///Debug output operator for Variant.
QDebug operator<<(QDebug d, const Variant& v);

///A list of genetic variants
class CPPNGSSHARED_EXPORT VariantList
{
public:

    ///Supported file formats.
    enum Format
    {
		VCF,    ///< VCF file. Note: only single-sample files are currently supported.
		VCF_GZ, ///< gzipped VCF file. Note: only single-sample files are currently supported.
		TSV,    ///< Tab-separated file that contains at least the following columns: chr, start (1-based), end (1-based), ref, obs.
		AUTO    ///< Format is automatically determined from the file name extension.
    };

    ///Default constructor
    VariantList();

	///Copies meta data from a variant list (comment, annotations, sample name), but not the variants.
	void copyMetaData(const VariantList& rhs);

    ///Adds a variant. Throws ArgumentException if the variant is not valid or does not contain the required number of annotations.
    void append(const Variant& variant)
    {
        variants_.append(variant);
    }
    ///Removes the variant with the index @p index.
    void remove(int index)
    {
        variants_.remove(index);
    }
    ///Variant accessor to a single variant.
    const Variant& operator[](int index) const
    {
        return variants_[index];
    }
    ///Read-write accessor to a single variant.
    Variant& operator[](int index)
    {
        return variants_[index];
    }
    ///Returns the variant count.
    int count() const
    {
        return variants_.count();
    }

	///Reserves space for a defined number of variants.
	void reserve(int size)
	{
		variants_.reserve(size);
	}

    ///Adds a comment line.
    void addCommentLine(QString comment_line)
    {
        comments_.append(comment_line);
    }
    ///Const access to comment lines
    const QStringList& comments() const
    {
        return comments_;
    }

    ///Const access to annotation headers.
	const QList<VariantAnnotationHeader>& annotations() const
    {
		return annotation_headers_;
    }
    ///Non-const access to annotation headers.
	QList<VariantAnnotationHeader>& annotations()
    {
		return annotation_headers_;
    }

	///Const access to annotation headers.
	const QList<VariantAnnotationDescription>& annotation_descriptions() const
	{
		return annotation_descriptions_;
	}
	///Non-const access to annotation headers.
	QList<VariantAnnotationDescription>& annotation_descriptions()
	{
		return annotation_descriptions_;
	}


	VariantAnnotationDescription annotationDescriptionByName(const QString& description_name, const QString sample_id = NULL);

	///Looks up annotation header index by name. If no or several annotations match, -1 is returned (or an error is thrown if @p error_on_mismatch is set).
	int annotationIndexByName(const QString& name, bool exact_match = true, bool error_on_mismatch = true) const;
	int annotationIndexByName(const QString& name, const QString& sample_id, bool exact_match, bool error_on_mismatch) const;
	///Removes an annotation column by index.
	void removeAnnotation(int index);


	///Const access to filter descriptions.
	const QMap<QString, QString>& filters() const
	{
		return filters_;
	}
	///Non-const access to filter descriptions.
	QMap<QString, QString>& filters()
	{
		return filters_;
	}


    ///Loads a single-sample variant list from a file. Returns the format of the file.
    Format load(QString filename, Format format=AUTO);
    ///Stores the variant list to a file.
    void store(QString filename, Format format=AUTO);

    ///Sorts the variants. The order is chromosome (numeric), position, ref, obs, quality (if desired).
    void sort(bool use_quality = false);
    ///Sorts the lines accoring to FASTA index file. The order is chromosome (as given in the file), position, ref, obs, quality (if desired).
    void sortByFile(QString file_name);

    ///Removes all variants that are not contained in the given chromosomal regions (must be merged and sorted).
	void filterByRegions(const BedFile& regions, bool invert=false);
    ///Removed all variants that do not pass all filters.
    void filterByRules(const QVector<VariantFilter>& filters);
	///Filter all variants by filters given in file
	void filterByFilterColumn();

    ///Remove duplicate variants.
	void removeDuplicates(bool sort_by_quality);
    ///Removes all content.
    void clear();
    ///Removes the annotations of all variants.
    void clearAnnotations();
	///Removes all variants.
	void clearVariants();

    ///Shifts each non complex insert or deletion to the left as far as possible. Then, removes duplicates.
    void leftAlign(QString ref_file);

    ///Checks if the variants are valid. Throws ArgumentException if not. The @p action string is appended to the error message after ', while'.
    void checkValid(QString action) const;

protected:
    QStringList comments_;
	QList<VariantAnnotationDescription> annotation_descriptions_;
	QList<VariantAnnotationHeader> annotation_headers_;
	QMap<QString, QString> filters_;
    QVector<Variant> variants_;
    ///Comparator helper class used by sortByFile.
    class LessComparatorByFile
    {
		public:
			///Constructor with FAI file, which determines the chromosome order.
			LessComparatorByFile(QString filename);
			bool operator()(const Variant &a, const Variant &b ) const;

		private:
			QString filename_;
			QHash<int, int> chrom_rank_;
    };
    ///Comparator helper class that used by sort().
    class LessComparator
    {

		public:
			///Constructor. If @p quality_index not given, the quality is not considered
			LessComparator(int quality_index=-1);
			bool operator()(const Variant& a, const Variant& b) const;

		private:
			int quality_index_;
    };

    ///Loads the variant list from a TSV file.
    void loadFromTSV(QString filename);
    ///Stores the variant list as a TSV file.
    void storeToTSV(QString filename);
    ///Loadss the variant list from a VCF file.
    void loadFromVCF(QString filename);
    ///Stores the variant list as a VCF file.
    void storeToVCF(QString filename);
	///Converts an annotation type to a string (for VCF only)
	static QString annotationTypeToString(VariantAnnotationDescription::AnnotationType type);
};

#endif // VARIANTLIST_H
