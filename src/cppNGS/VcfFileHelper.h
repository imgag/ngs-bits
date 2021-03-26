#pragma once

#include "cppNGS_global.h"
#include "KeyValuePair.h"
#include "Exceptions.h"
#include "Log.h"

#include "ChromosomalIndex.h"
#include "Sequence.h"
#include "BedFile.h"
#include "VariantList.h"

#include <QIODevice>
#include <QString>
#include <QMap>
#include <QTextStream>
#include <QByteArray>
#include <QList>

#include <memory>

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
	bool hasKey(const K& key, V& value) const
	{
		auto iter = hash_.find(key);
		if (iter == hash_.end()) return false;
		value = iter.value();
		return true;
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
		return (ordered_keys_.empty());
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
using ListOfFormatIds = QByteArray; //an array of all ordered FORMAT ids to use as hash key
using ListOfInfoIds = QByteArray; //an array of all ordered INFO ids to use as hash key

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

	//functions setting header content from a whole vcf line as QByteArray
	void setCommentLine(const QByteArray& line, const int line_number);
	void setInfoLine(const QByteArray& line, const int line_number);
	void setFormatLine(const QByteArray& line, const int line_number);
	void setFilterLine(const QByteArray& line, const int line_number);
	void setFormat(const QByteArray& line);
	void storeHeaderInformation(QTextStream& stream) const;

	//functions returning single info, format, filter lines by its ID
	InfoFormatLine infoLineByID(const QByteArray& id, bool error_not_found = true) const;
	InfoFormatLine formatLineByID(const QByteArray& id, bool error_not_found = true) const;
	FilterLine filterLineByID(const QByteArray& id, bool error_not_found = true) const;

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
	InfoFormatLine lineByID(const QByteArray& id, const QVector<InfoFormatLine>& lines, bool error_not_found = true) const;
};

///Representation of a line of a vcf file
class CPPNGSSHARED_EXPORT  VcfLine
{

public:

	///Default constructor.
	VcfLine();
	///Constructor with basic entries
	/// (chromosome, start position, reference base(s), List of alternative base(s), List of FormatIDs, List of sampleIDs, List containing for every sample a list of values for every format)
	VcfLine(const Chromosome& chr, int start, const Sequence& ref, const QVector<Sequence>& alt, QByteArrayList format_ids = QByteArrayList(), QByteArrayList sample_ids = QByteArrayList(), QList<QByteArrayList> list_of_format_values = QList<QByteArrayList>());

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
		if(ref().length() <= 0) THROW(ArgumentException, "Reference can not have length zero in a VCF File.");
		return (start() + ref().length() - 1);
	}
	const Sequence& ref() const
	{
		return ref_;
	}
	const QVector<Sequence>& alt() const
	{
		return alt_;
	}
	//Concatenates all alternatives bases to a comma seperated string
	const Sequence altString() const
	{
		QByteArrayList alt_sequences;
		for(const Sequence& seq : alt())
		{
			alt_sequences.push_back(seq);
		}
		return alt_sequences.join(',');
	}
	const Sequence& alt(int pos) const
	{
		return alt_.at(pos);
	}
	const QByteArrayList& id() const
	{
		return id_;
	}
	const double& qual() const
	{
		return qual_;
	}
	const QByteArrayList& filter() const
	{
		return filter_;
	}
	//Returns a list of all format IDs
	QByteArrayList formatKeys() const
	{
		if(!formatIdxOf_)
		{
			QByteArrayList empty_list;
			return empty_list;
		}
		else
		{
			return formatIdxOf_->keys();
		}
	}
	//Returns a list of all info IDs
	QByteArrayList infoKeys() const
	{
		if(!infoIdxOf_)
		{
			QByteArrayList list;
			return list;
		}
		else
		{
			return infoIdxOf_->keys();
		}
	}
	//Returns a list of all info values in order of the info IDs
	QByteArrayList infoValues()
	{
		if(!infoIdxOf_)
		{
			QByteArrayList list;
			return list;
		}
		else
		{
			return info_;
		}
	}
	//Returns the value for an info ID as key
	QByteArray info(const QByteArray& key, bool error_if_key_absent = false) const
	{
		if(error_if_key_absent)
		{
			int i_idx = (*infoIdxOf_)[key];
			return info_.at(i_idx);
		}
		else
		{
			int info_pos;
			if(infoIdxOf_->hasKey(key, info_pos))
			{
				return info_.at(info_pos);
			}
			else
			{
				return "";
			}
		}
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
	QByteArray formatValueFromSample(const QByteArray& format_key, const QByteArray& sample_name, bool error_if_format_key_absent = false) const
	{
		if(error_if_format_key_absent)
		{
			int s_idx = (*sampleIdxOf_)[sample_name];
			int f_idx = (*formatIdxOf_)[format_key];

			return sample_values_.at(s_idx).at(f_idx);
		}
		else
		{
			int sample_pos;
			int format_pos;
			if(sampleIdxOf_->hasKey(sample_name, sample_pos) && formatIdxOf_->hasKey(format_key, format_pos))
			{
				return sample_values_.at(sample_pos).at(format_pos);;
			}
			else
			{
				return "";
			}
		}
	}
	///Returns the value for a format ID and sample position (default is first sample)
	QByteArray formatValueFromSample(const QByteArray& format_key, int sample_pos = 0, bool error_if_format_key_absent = false) const
	{

		if(sample_pos >= samples().size()) THROW(ArgumentException, QString::number(sample_pos) + " is out of range for SAMPLES. The VCF file provides " + QString::number(samples().size()) + " SAMPLES");

		QByteArrayList format_values = sample_values_.at(sample_pos);

		if(error_if_format_key_absent)
		{
			int f_idx = (*formatIdxOf_)[format_key];
			return sample_values_.at(sample_pos).at(f_idx);
		}
		else
		{
			int format_pos;
			if(formatIdxOf_->hasKey(format_key, format_pos))
			{
				return format_values.at(format_pos);;
			}
			else
			{
				return "";
			}
		}
	}

	void setChromosome(const Chromosome& chr)
	{
		chr_ =  chr;
	}
	void setPos(const int& pos)
	{
		pos_ = pos;
	}
	void setRef(const Sequence& ref)
	{
		ref_ = ref;
	}
	void addAlt(const QByteArrayList& alt)
	{
		for(const Sequence& seq : alt)
		{
			alt_.push_back(strToPointer(seq.toUpper()));
		}
	}
	//set the alternative bases with a list of sequences
	void setAlt(const QList<Sequence>& alt)
	{
		alt_.clear();
		for(const QByteArray alt_element : alt)
		{
			alt_.push_back(alt_element);
		}
	}
	//set the alternative base(s) with only one alternative Sequence
	void setSingleAlt(const Sequence& seq)
	{
		if(alt_.empty())
		{
			alt_.push_back(seq);
		}
		else
		{
			alt_[0] = seq;
		}
	}
	void setId(const QByteArrayList& id)
	{
		id_ = id;
	}
	void setQual(const double& qual)
	{
		qual_ = qual;
	}
	void setFilter(QByteArrayList filter_list)
	{
		if(filter_list.size() == 1 && filter_list.at(0) == ".")
		{
			return;
		}
		for(QByteArray& filter : filter_list)
		{
			filter = filter.trimmed();
			filter_.push_back(strToPointer(filter));
		}
	}
	void addFilter(QByteArray& tag)
	{
		tag = tag.trimmed();
		//if all filters are passed, only the first index is set
		if(filter_.count() == 1 && (filter_.at(0) == "" || filter_.at(0) == "." || filter_.at(0) == "PASS" || filter_.at(0) == "PASSED"))
		{
			filter_.removeFirst();
		}
		filter_.push_back(strToPointer(tag));
	}
	//Set the list storing all info values
	void setInfo(const QByteArrayList& info_values)
	{
		info_ = info_values;
	}
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
			for(const QByteArray alt_seq : alt())
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
	bool isMultiAllelic() const;
	//Returns if the variant is an InDel, can only be called on single allelic variants
	bool isInDel() const;
	//Returns if the chromosome is valid
	bool isValidGenomicPosition() const;
	//Returns all not passed filters
	QByteArrayList failedFilters() const;
	//Return a string of the variant coordinates and reference, alternative base(s)
	QString variantToString() const;
	QByteArrayList vepAnnotations(int field_index) const;

	void leftNormalize(QString reference_genome);
	// Removes the common prefix/suffix from indels, adapts the start/end position and replaces empty sequences with a custom string.
	void normalize(const Sequence& empty_seq="", bool to_gsvar_format=true);
	// copy coordinates of the vcf line into a variant (only single alternative bases)
	void copyCoordinatesIntoVariant(Variant& variant)
	{
		variant.setChr(chr());
		variant.setStart(start());
		variant.setEnd(end());
		variant.setRef(ref());
		variant.setObs(alt(0));
	}

	//Equality operator (only compares the variant location itself, not further annotations).
	bool operator==(const VcfLine& rhs) const;
	//Less-than operator.
	bool operator<(const VcfLine& rhs) const;

private:
	Chromosome chr_;
	int pos_;
	Sequence ref_;
	QVector<Sequence> alt_; //comma seperated list of alternative sequences

	QByteArrayList id_; //; seperated list of id-strings
	double qual_;

	QByteArrayList filter_; //; seperated list of failed filters or "PASS"

	InfoIDToIdxPtr infoIdxOf_;
	QByteArrayList info_;

	SampleIDToIdxPtr sampleIdxOf_;
	FormatIDToIdxPtr formatIdxOf_;
	QList<QByteArrayList> sample_values_;
};
using VcfLinePtr = QSharedPointer<VcfLine>;

namespace VcfFormat
{
//Comparator helper class that used by sort().
class LessComparator
{
public:
	///Constructor.
	LessComparator(bool use_quality);
	bool operator()(const VcfLinePtr& a, const VcfLinePtr& b) const;

private:
	bool use_quality;
};
//Comparator helper class used by sortByFile.
class LessComparatorByFile
{
public:
	//Constructor with FAI file, which determines the chromosome order.
	LessComparatorByFile(QString filename);
	bool operator()(const VcfLinePtr& a, const VcfLinePtr& b) const;

private:
	QString filename_;
	QHash<int, int> chrom_rank_;
};

} //end namespace VcfFormat
