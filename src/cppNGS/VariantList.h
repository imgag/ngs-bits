#ifndef VARIANTLIST_H
#define VARIANTLIST_H

#include "cppNGS_global.h"
#include "VariantAnnotationDescription.h"
#include "FastaFileIndex.h"
#include "BedFile.h"
#include "ChromosomalIndex.h"
#include "OntologyTermCollection.h"

#include <QVector>
#include <QStringList>
#include <QtAlgorithms>
#include <QVectorIterator>


///Transcript annotations e.g. from SnpEff/VEP.
struct CPPNGSSHARED_EXPORT VariantTranscript
{
	QByteArray gene;
	QByteArray id; //Attention: older GSvar files contain Ensembl transcripts without version number. Since 12/2020 a version number is included.
	QByteArray type;
	QByteArray impact;
	QByteArray exon;
	QByteArray hgvs_c;
	QByteArray hgvs_p;
	QByteArray domain;

	QByteArray toString(char sep) const;

	///Returns true if one the variant type matches the given terms
	bool typeMatchesTerms(const OntologyTermCollection& terms) const;

	///Returns the transcript identifier without
	QByteArray idWithoutVersion() const;
};

///Sample header struct for samples in variant lists.
struct CPPNGSSHARED_EXPORT SampleInfo
{
	QString id; //sample name/identifier
	QString column_name; //sample column in VCF/GSvar format
	int column_index; //column index of the sample genotype column (for germline only, below zero for somatic)
	QMap<QString, QString> properties;

	///Returns if the sample has state 'affected'.
	bool isAffected() const;

	///Returns if the sample is a tumor sample.
	bool isTumor() const;

	///Returns the gender of the sample, or 'n/a' if unknown.
	QString gender() const;
};

///Sample header information.
class CPPNGSSHARED_EXPORT SampleHeaderInfo
	: public QList<SampleInfo>
{
	public:
		///Returns the sample info by sample id.
		const SampleInfo& infoByID(const QString& id) const;
		///Returns the sample info matching the given sample properties. Throws an error if there more/less than one columns that match.
		const SampleInfo& infoByStatus(bool affected, QString gender = "n/a") const;
		///Returns sample genotype column indices of affected/unaffected samples.
		QList<int> sampleColumns(bool affected) const;
};

///VCF representation of a variant in GSvar format.
struct VariantVcfRepresentation
{
	Chromosome chr;
	int pos;
	Sequence ref;
	Sequence alt;
};

///Genetic variant or mutation (1-based).
class CPPNGSSHARED_EXPORT Variant
{
public:
    ///Default constructor.
    Variant();
    ///Convenience constructor.
    Variant(const Chromosome& chr, int start, int end, const Sequence& ref, const Sequence& obs, const QList<QByteArray>& annotations = QList<QByteArray>(), int filter_index = -1);


	///Returns if the variant is valid
	bool isValid() const
	{
		return chr_.isValid() && start_!=-1 && end_!=-1 && !ref_.isEmpty() && !obs_.isEmpty();
	}

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
		ref_ = ref.trimmed();
    }
    ///Returns the observed base.
	const Sequence& obs() const
    {
        return obs_;
    }
    ///Sets the observed base.
	void setObs(const Sequence& obs)
    {
		obs_ = obs.trimmed();
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

	void setAnnotations(const QList<QByteArray>& annotations)
	{
		annotations_ = annotations;
	}

	///Returns the VEP annotations for an certain annotation field (one for each transcript)
	QByteArrayList vepAnnotations(int csq_index, int field_index) const;

	///Adds the given tag to the filter column.
	void addFilter(QByteArray tag, int filter_column_index);

	///Convenience access to the filter annotation column (split by ';', trimmed, enties the indiacate passing removed).
    const QList<QByteArray>& filters() const
    {
        return filters_;
    }

	///Equality operator (only compares the variant itself, not annotation).
	bool operator==(const Variant& rhs) const;
    ///Less-than operator.
    bool operator<(const Variant& rhs) const;
    ///Overlap check for chromosome and position range.
    bool overlapsWith(const Chromosome& chr, int start, int end) const
    {
		return (chr_==chr && BasicStatistics::rangeOverlaps(start_, end_, start, end));
    }
    ///Overlap check for position range only.
    bool overlapsWith(int start, int end) const
    {
		return BasicStatistics::rangeOverlaps(start_, end_, start, end);
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
	QString toString(bool space_separated=false, int max_sequence_length=-1, bool chr_normalized=false) const;

	///Checks if the variant is valid (without annotations). Throws ArgumentException if not.
	void checkValid() const;
	///Checks if the reference sequence matches the reference genome. Throws ArgumentException if not.
	void checkReferenceSequence(const FastaFileIndex& reference);

	/// Left-align indels in repeat regions. Works for GSvar files only - assumes the variants are normalized.
	void leftAlign(const FastaFileIndex& reference);
    /// Removes the common prefix/suffix from indels, adapts the start/end position and replaces empty sequences with a custom string.
	void normalize(const Sequence& empty_seq="", bool to_gsvar_format=false);
	/// Returns the HGVS.g notation of the variant.
	QString toHGVS(const FastaFileIndex& genome_index) const;
	/// Returns the VCF line notation of the variant up to the INFO column.
	VariantVcfRepresentation toVCF(const FastaFileIndex& genome_index) const;

    ///Auxilary function: Removes common prefix and suffix bases from indels and adapts the start position accordingly.
	static void normalize(int& start, Sequence& ref, Sequence& obs);
    ///Auxilary function: Returns the smallest repeated subsequence of an indel or the complete input sequence if it has no repeat.
	static Sequence minBlock(const Sequence& seq);
	///Auxilary function: Returns the repeat region of an indel (1-based, closed interval).
	///@note Returns the original start/end position if the variant is a SNV, a complex index or not in a repeat region.
	///@note Expects 1-based closed intervals are positions (insertions are after given position).
	static QPair<int, int> indelRegion(const Chromosome& chr, int start, int end, Sequence ref, Sequence obs, const FastaFileIndex& reference);

	///Returns transcript information from the given column index.
	QList<VariantTranscript> transcriptAnnotations(int column_index) const
	{
		return parseTranscriptString(annotations()[column_index]);
	}

	static QList<VariantTranscript> parseTranscriptString(QByteArray text, bool allow_old_format_with_7_columns=false);

	///Returns a normalized variant extracted from user input text. Throws an exception, if it is not valid.
	static Variant fromString(const QString& text);

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

///Supported analysis types.
enum AnalysisType
{
	GERMLINE_SINGLESAMPLE,
	GERMLINE_TRIO,
	GERMLINE_MULTISAMPLE,
	SOMATIC_SINGLESAMPLE,
	SOMATIC_PAIR
};
///Returns the string repesentation of the analysis type (or a human-readable version).
QString analysisTypeToString(AnalysisType type, bool human_readable=false);
///Returns a the  repesentation of the analysis type (does not support the human-readable version).
AnalysisType stringToAnalysisType(QString type);

///A list of genetic variants
class CPPNGSSHARED_EXPORT VariantList
{
public:
    ///Default constructor
    VariantList();

	///Returns the human readable name of the analysis, e.g. for showning in a GUI.
	QString analysisName() const;
	///Returns the name of the main processed sample (child for trio, tumor for tumor-normal, only affected for multi). Throws an exception of no main sample could be determined!
	QString mainSampleName() const;

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
	///Resize variant list.
	void resize(int size)
	{
		variants_.resize(size);
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
	const QList<VariantAnnotationDescription>& annotationDescriptions() const
	{
		return annotation_descriptions_;
	}
	///Non-const access to annotation headers.
	QList<VariantAnnotationDescription>& annotationDescriptions()
	{
		return annotation_descriptions_;
	}

	///get annotation header by name
	VariantAnnotationDescription annotationDescriptionByName(const QString& description_name, bool error_not_found = true) const;

	///Looks up annotation header index by name. If no or several annotations match, -1 is returned (or an error is thrown if @p error_on_mismatch is set).
	int annotationIndexByName(const QString& name, bool exact_match = true, bool error_on_mismatch = true) const;
	///Looks up the index of an annotation in the VEP header (CSQ info field). Keep in mind the that CSQ field of variants conains comma-sparated entries for each transcript!
	int vepIndexByName(const QString& name, bool error_if_not_found = true) const;

	///Adds an annotation column and returns the index of the new column.
	int addAnnotation(QString name, QString description, QByteArray default_value="");
	///Adds an annotation column if it does not exist and returns the index of the new/old column.
	int addAnnotationIfMissing(QString name, QString description, QByteArray default_value="");

	///Prepends an annotation column and returns the index of the new column.
	int prependAnnotation(QString name, QString description, QByteArray default_value="");

	///Removes an annotation column by index.
	void removeAnnotation(int index);
	///Removes an annotation column by name.
	void removeAnnotationByName(QString name, bool exact_match=true, bool error_on_mismatch=true);

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
	///If @p roi is given, only variants that fall into the target regions are loaded.
	///If @p invert is given, only variants that fall outside the target regions are loaded.
	void load(QString filename, const BedFile& roi, bool invert=false);
	void load(QString filename);
    ///Stores the variant list to a file.
	void store(QString filename) const;

	///Default sorting of variants. The order is chromosome (numeric), position, ref, obs, quality (if desired - only for VCF).
	void sort(bool use_quality = false);
	///Sort list alphabetically by annotation
	void sortByAnnotation(int annotation_index);
	///Sorts the lines accoring to FASTA index file. The order is chromosome (as given in the file), position, ref, obs.
    void sortByFile(QString file_name);
	///Costum sorting of variants.
	template <typename T>
	void sortCustom(const T& comarator)
	{
		std::sort(variants_.begin(), variants_.end(), comarator);
	}

    ///Remove duplicate variants.
	void removeDuplicates(bool sort_by_quality);
    ///Removes all content.
    void clear();
    ///Removes the annotations of all variants.
    void clearAnnotations();
	///Removes all variants.
	void clearVariants();

	///Shifts each non complex insert or deletion to the left as far as possible. Then, removes duplicates (@p sort_by_quality is only supported for VCF format).
	void leftAlign(QString ref_file, bool sort_by_quality);

	///Checks if the variants are valid (with annotation). Throws ArgumentException if not.
	void checkValid() const;

	///Parses and returns sample data from variant list header (only for GSvar).
	SampleHeaderInfo getSampleHeader() const;

	///Returns the analysis pipeline and version from the header.
	QString getPipeline() const;
	///Returns the creation date from the header i.e. the date of the annotatated VCF from which the GSvar was created. If not available, a invalid date is returned.
	QDate getCreationDate() const;
	///Returns the analysis type from the header.
	AnalysisType type(bool allow_fallback_germline_single_sample=true) const;

	///Returns whether list contains variant with same chr, start, end, ref and obs
	bool contains(const Variant& var)
	{
		return variants_.contains(var);
	}

protected:
    QStringList comments_;
	QList<VariantAnnotationDescription> annotation_descriptions_;
	QList<VariantAnnotationHeader> annotation_headers_;
	QMap<QString, QString> filters_;
    QVector<Variant> variants_;

	void loadInternal(QString filename, const BedFile* roi = nullptr, bool invert=false);

	///Comparator helper class used by sortByAnnotation
	class LessComparatorByAnnotation
	{
		public:
			LessComparatorByAnnotation(int annotation_index);
			bool operator()(const Variant &a, const Variant &b) const;

		private:
			int annotation_index_;

	};

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
};

#endif // VARIANTLIST_H
