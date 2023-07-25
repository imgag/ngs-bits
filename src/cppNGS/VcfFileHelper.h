#pragma once

#include "cppNGS_global.h"

#include "Sequence.h"
#include "Exceptions.h"
#include "FastaFileIndex.h"
#include "BasicStatistics.h"
#include "BedFile.h"

///Data structure containing a vector of keys (to retrieve its order) and a hash of the key to value
template<typename K, typename V>
class CPPNGSSHARED_EXPORT OrderedHash {

public:

	//add key=value pair to the end of OrderedHash
	void push_back(const K& key,const V& value)
	{
		auto iter = hash_.find(key);
		if (iter != hash_.end()) {
			return;
		}

		ordered_keys_.push_back(key);
		hash_.insert(key, value);
	}

	//access value by key
	const V& operator[](const K& key) const
	{
		auto iter = hash_.find(key);
		if (iter == hash_.end())
		{
			THROW(ArgumentException, "Key " + key + " does not exist in OrderedHash.");
		}
		return iter.value();
	}

	//check if key exists
	int indexOf(const K& key) const
	{
		auto iter = hash_.find(key);
		if (iter == hash_.end()) return -1;
		return iter.value();
	}

	//access value by order
	typename QHash<K, V>::const_iterator at(int i) const
	{
		if(i >= size())
		{
			THROW(ArgumentException, "Index " + QString::number(i) + " is out of range for OrderedHash of size " + size() + ".");
		}
		K key = ordered_keys_.at(i);
		return hash_.find(key);
	}

	//get the number of inserted key=value pairs
	int size() const
	{
		return ordered_keys_.size();
	}

	//check if the OrderedHash is empty
	bool empty() const
	{
		return ordered_keys_.empty();
	}

	//returns keys in order
	const QList<K>& keys()
	{
		return ordered_keys_;
	}

private:

	QList<K> ordered_keys_;
	QHash<K, V> hash_;

};

//storing all QByteArrays in a list of unique QByteArrays
const QByteArray& strToPointer(const QByteArray& str);

enum InfoFormatType {INFO_DESCRIPTION, FORMAT_DESCRIPTION};

using SampleIDToIdxPtr = QSharedPointer<OrderedHash<QByteArray, int>>; //Hashing sample ID to position in value list
using FormatIDToIdxPtr = QSharedPointer<OrderedHash<QByteArray, int>>; //Hashing format ID to position in value list
using InfoIDToIdxPtr = QSharedPointer<OrderedHash<QByteArray, int>>; //Hashing info ID to position in value list

//basic header line storing comments (every comment must be a key=value pair)
struct CPPNGSSHARED_EXPORT VcfHeaderLine
{
	QByteArray value;
	QByteArray key;

	void storeLine(QTextStream& stream) const
	{
		stream << "##" << key << "=" << value << "\n";
	}
};

//storing header informations for an INFO or FORMAT line
struct CPPNGSSHARED_EXPORT InfoFormatLine
{
	QByteArray id;
	QByteArray number;
	QByteArray type;
	QString description;

	void storeLine(QTextStream& stream, InfoFormatType line_type) const
	{
		line_type==InfoFormatType::INFO_DESCRIPTION ? stream << "##INFO" : stream << "##FORMAT";
		stream << "=<ID=" << id << ",Number=" << number << ",Type=" << type << ",Description=\"" << description << "\">" << "\n";
	}

};

//storing header informations for a FILTER line
struct CPPNGSSHARED_EXPORT FilterLine
{
	QByteArray id;
	QString description;

	void storeLine(QTextStream& stream) const
	{
		stream << "##FILTER=<ID=" << id << ",Description=\"" << description  << "\">" << "\n";
	}
};

///Struct representing a vcf header.
///It contains the fileformat, comments, information lines, format lines and filter lines.
class CPPNGSSHARED_EXPORT VcfHeader
{
public:

	void clear();

	const QByteArray& fileFormat() const
	{
		return fileformat_;
	}
	const QVector<VcfHeaderLine>& comments() const
	{
		return 	file_comments_;
	}
	const QVector<InfoFormatLine>& infoLines() const
	{
		return info_lines_;
	}
	const QVector<FilterLine>& filterLines() const
	{
		return filter_lines_;
	}
	const QVector<InfoFormatLine>& formatLines() const
	{
		return format_lines_;
	}

	//functions to add or move content of the vcf header
	void addInfoLine(const InfoFormatLine& info_line)
	{
		info_lines_.push_back(info_line);
	}
	void removeInfoLine(int pos)
	{
		info_lines_.removeAt(pos);
	}
	void addFormatLine(const InfoFormatLine& format_line)
	{
		format_lines_.push_back(format_line);
	}
	void moveFormatLine(int from, int to)
	{
		format_lines_.move(from, to);
	}
	void addFilter(const QByteArray& filter, const QString& description = "no description available");
	void addFilterLine(const FilterLine& filter_line)
	{
		filter_lines_.push_back(filter_line);
	}

	void removeCommentLine(int pos)
	{
		file_comments_.removeAt(pos);
	}

	//functions setting header content from a whole vcf line as QByteArray
	void setCommentLine(const QByteArray& line, const int line_number);
	void setInfoLine(const QByteArray& line, const int line_number);
	void setFormatLine(const QByteArray& line, const int line_number);
	void setFilterLine(const QByteArray& line, const int line_number);
	void setFormat(const QByteArray& line);
	void storeHeaderInformation(QTextStream& stream) const;

	//functions to check if info, format filter lines exist
	bool infoIdDefined(const QByteArray& id) const;
	bool formatIdDefined(const QByteArray& id) const;
	bool filterIdDefined(const QByteArray& id) const;

	//functions returning single info, format, filter lines by its ID
	const InfoFormatLine& infoLineByID(const QByteArray& id, bool error_not_found = true) const
	{
		return lineByID(id, infoLines(), error_not_found);
	}
	const InfoFormatLine& formatLineByID(const QByteArray& id, bool error_not_found = true) const
	{
		return lineByID(id, formatLines(), error_not_found);
	}
	const FilterLine& filterLineByID(const QByteArray& id, bool error_not_found = true) const;

	//looks up the position of name in the list of VEP annotations from the info line in the header (CSQ line)
	int vepIndexByName(const QString& name, bool error_if_not_found = true) const;

private:

	static const QByteArrayList InfoTypes;
	static const QByteArrayList FormatTypes;

	QByteArray fileformat_;
	QVector<VcfHeaderLine> file_comments_;

	QVector<InfoFormatLine> info_lines_;
	QVector<FilterLine> filter_lines_;
	QVector<InfoFormatLine> format_lines_;

	bool parseInfoFormatLine(const QByteArray& line,InfoFormatLine& info_format_line, QByteArray type, const int line_number);
	const InfoFormatLine& lineByID(const QByteArray& id, const QVector<InfoFormatLine>& lines, bool error_not_found = true) const;
};

///Representation of a line of a VCF file
class CPPNGSSHARED_EXPORT VcfLine
{
public:
	///Default constructor.
	VcfLine();
	///Constructor with basic entries
	/// (chromosome, start position, reference base(s), List of alternative base(s), List of FormatIDs, List of sampleIDs, List containing for every sample a list of values for every format)
	VcfLine(const Chromosome& chr, int start, const Sequence& ref, const QList<Sequence>& alt, QByteArrayList format_ids = QByteArrayList(), QByteArrayList sample_ids = QByteArrayList(), QList<QByteArrayList> list_of_format_values = QList<QByteArrayList>());

	const Chromosome& chr() const
	{
		return chr_;
	}
	int start() const
	{
		return pos_;
	}
	int end() const
	{
		return start() + ref().length() - 1;
	}
	const Sequence& ref() const
	{
		return ref_;
	}
	const QList<Sequence>& alt() const
	{
		return alt_;
	}
	//Concatenates all alternatives bases to a comma seperated string
	Sequence altString() const
	{
		QByteArrayList alt_sequences;
		foreach(const Sequence& seq, alt_)
		{
			alt_sequences.push_back(seq);
		}
		return alt_sequences.join(',');
	}
	const Sequence& alt(int pos) const
	{
		if (pos<0 || pos>=alt_.length()) THROW(ArgumentException, "Invalid alternative sequence index " + QString::number(pos) + " for variant " + toString());
		return alt_.at(pos);
	}
	const QByteArrayList& id() const
	{
		return id_;
	}
	double qual() const
	{
		return qual_;
	}
	//Returns the filter entries. ATTENTION: PASS entry is also contained - use filtersFailed() to check if all filters are passed or not.
	const QByteArrayList& filters() const
	{
		return filter_;
	}
	bool filtersPassed() const
	{
		return filter_.isEmpty() || (filter_.count()==1 && filter_[0]=="PASS");
	}
	//Returns a list of all format IDs
	const QByteArrayList& formatKeys() const
	{
		static QByteArrayList empty;
		if(!formatIdxOf_)
		{
			return empty;
		}
		return formatIdxOf_->keys();
	}

	//Returns a list of all info IDs
	const QList<QByteArray>& infoKeys() const
	{
		static QList<QByteArray> empty;
		if(!infoIdxOf_)
		{
			return empty;
		}
		return infoIdxOf_->keys();
	}

	//Returns the value for an info ID as key
	const QByteArray& info(const QByteArray& key, bool error_if_key_absent = false) const
	{
		static QByteArray empty;

		int info_pos = infoIdxOf_->indexOf(key);
		if(info_pos==-1)
		{
			if (error_if_key_absent) THROW(ArgumentException, "Key ' " + key + "' not found in INFO entries of variant " + toString());
			return empty;
		}
		//qDebug() << __FILE__ << __LINE__ << key << info_pos << info_.count();

		return info_.at(info_pos);
	}


	///Returns a list, which stores for every sample a list of the values for every format ID
	const QList<QByteArrayList>& samples() const
	{
		return sample_values_;
	}
	///Returns a list of all values for every format ID for the sample sample_name
	const QByteArrayList& sample(const QByteArray& sample_name) const
	{
		return sample_values_.at((*sampleIdxOf_)[sample_name]);
	}
	///Returns a list of all values for every format ID for the sample at position pos
	const QByteArrayList& sample(int pos) const
	{
		if(pos >= sample_values_.count()) THROW(ArgumentException, QString::number(pos) + " is out of range for SAMPLES. The VCF file provides " + QString::number(samples().size()) + " SAMPLES");
		return sample_values_.at(pos);
	}
	///Returns the value for a format and sample ID
	const QByteArray& formatValueFromSample(const QByteArray& format_key, const QByteArray& sample_name) const
	{
		static QByteArray empty;

		int sample_pos = sampleIdxOf_->indexOf(sample_name);
		int format_pos = formatIdxOf_->indexOf(format_key);
		if(sample_pos!=-1 && format_pos!=-1)
		{
			return sample_values_.at(sample_pos).at(format_pos);;
		}
		else
		{
			return empty;
		}
	}
	///Returns the value for a format ID and sample position (default is first sample)
	const QByteArray& formatValueFromSample(const QByteArray& format_key, int sample_pos = 0) const
	{
		static QByteArray empty;
		if(sample_pos >= samples().size()) THROW(ArgumentException, QString::number(sample_pos) + " is out of range for SAMPLES. The VCF file provides " + QString::number(samples().size()) + " SAMPLES");

		int format_pos = formatIdxOf_->indexOf(format_key);
		if(format_pos!=-1)
		{
			return sample_values_[sample_pos][format_pos];
		}
		else
		{
			return empty;
		}
	}

	void setChromosome(const Chromosome& chr)
	{
		chr_ =  chr;
	}
	void setPos(int pos)
	{
		pos_ = pos;
	}
	void setRef(const Sequence& ref)
	{
		ref_ = ref;
	}
	void addAlt(const Sequence& alt)
	{
		alt_ << alt;
	}
	//set the alternative base(s) with only one alternative Sequence
	void setSingleAlt(const Sequence& alt)
	{
		alt_.clear();
		addAlt(alt);
	}
	void setId(const QByteArrayList& id)
	{
		id_ = id;
	}
	void setQual(double qual)
	{
		qual_ = qual;
	}
	void setFilters(const QByteArrayList& filter_list)
	{
		foreach(const QByteArray& filter, filter_list)
		{
			addFilter(filter);
		}
	}
	void addFilter(QByteArray tag)
	{
		tag = tag.trimmed();
		if (tag.isEmpty() || tag==".") return;
		filter_.push_back(strToPointer(tag));
	}
	//Set the list storing all info values. Make sure to call setInfoIdToIdxPtr before this method!
	void setInfo(const QByteArrayList& info_values);
	//Set the list, which stores for every sample a list of all format values
	void setSample(const QList<QByteArrayList>& sample)
	{
		sample_values_ = sample;
	}
	void addFormatValues(const QByteArrayList& format_value_list)
	{
		sample_values_.push_back(format_value_list);
	}
	//Set the pointer storing the ordered list of sample IDs
	void setSampleIdToIdxPtr(const SampleIDToIdxPtr& ptr)
	{
		sampleIdxOf_ = ptr;
	}
	//Set the pointer storing the ordered list of format IDs
	void setFormatIdToIdxPtr(const FormatIDToIdxPtr& ptr)
	{
		formatIdxOf_ = ptr;
	}
	//Set the pointer storing the ordered list of info IDs
	void setInfoIdToIdxPtr(const InfoIDToIdxPtr& ptr)
	{
		infoIdxOf_ = ptr;
	}

	//Overlap check for chromosome and position range.
	bool overlapsWith(const Chromosome& input_chr, int input_start, int input_end) const
	{
		return (chr_==input_chr && BasicStatistics::rangeOverlaps(start(), end(), input_start, input_end));
	}
	//Overlap check for position range only.
	bool overlapsWith(int input_start, int input_end) const
	{
		return BasicStatistics::rangeOverlaps(start(), end(), input_start, input_end);
	}
	//Overlap check BED file line.
	bool overlapsWith(const BedLine& line) const
	{
		return overlapsWith(line.chr(), line.start(), line.end());
	}

	//Returns if the variant is a SNV
	//allow_several_alternatives=TRUE if all alternatives in the vector shall be considered,
	//otherwise only the first variant is checked
	bool isSNV(bool allow_several_alternatives = false) const
	{
		if(allow_several_alternatives)
		{
			foreach(const Sequence& alt_seq, alt())
			{
				if(alt_seq.length() != 1) return false;
			}
		}
		else if(alt(0).length() != 1)
		{
			return false;
		}

		return ref_.length()==1 && alt(0)!="-" && ref_!="-";
	}
	//Returns if any VcfLine in the file is multiallelic
	bool isMultiAllelic() const
	{
		return alt().count() > 1;
	}
	//Returns if the variant is a combined insertion and deletion. Cannot be called on multi-allelic variants.
	bool isInDel() const;
	//Returns if the variant is an insertion. Cannot be called on multi-allelic variants.
    bool isIns() const;
	//Returns if the variant is a deletion. Cannot be called on multi-allelic variants.
    bool isDel() const;
	//Returns if the VCF variant is valid (only checks the base variant, i.e. chr, pos, ref, alt)
	bool isValid() const;
	//Overload of the above function that also checks if the reference bases of the variants are correct.
	bool isValid(const FastaFileIndex& reference) const;
	//Returns a string representation of the variant (chr, pos, ref, alt).
	QByteArray toString(bool add_end=false) const
	{
		return chr_.str() + ":" + QByteArray::number(start()) + (add_end ? "-" + QByteArray::number(end()): "") + " " + ref() + ">" + altString();
	}
	QByteArrayList vepAnnotations(int field_index) const;
	// Left-normalize all variants.
	void leftNormalize(FastaFileIndex& reference, bool check_reference)
	{
		normalize(ShiftDirection::LEFT, reference, check_reference);
	}
	//Right-normalize all variants
	void rightNormalize(FastaFileIndex& reference, bool check_reference=true)
	{
		normalize(ShiftDirection::RIGHT, reference, check_reference);
	}
    // Removes the common prefix/suffix from indels, shifts the variant left or right, and adds a common reference base
	enum ShiftDirection {LEFT, RIGHT};
	void normalize(ShiftDirection shift_dir, const FastaFileIndex& reference, bool check_reference, bool add_prefix_base_to_mnps=false);

	//Equality operator (only compares the variant location itself, not further annotations).
	bool operator==(const VcfLine& rhs) const
	{
		return pos_==rhs.start() && chr_==rhs.chr() && ref_==rhs.ref() && altString()==rhs.altString();
	}
	//Less-than operator.
	bool operator<(const VcfLine& rhs) const;

private:
	Chromosome chr_;
	int pos_;
	Sequence ref_;
	QList<Sequence> alt_;

	QByteArrayList id_;
	double qual_;

	QByteArrayList filter_; //list of filter entries. ATTENTION: PASS is contained

	InfoIDToIdxPtr infoIdxOf_;
	QByteArrayList info_;

	SampleIDToIdxPtr sampleIdxOf_;
	FormatIDToIdxPtr formatIdxOf_;
	QList<QByteArrayList> sample_values_;
};
