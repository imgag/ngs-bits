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

const QByteArray& strToPointer(const QByteArray& str);
const unsigned char& strToPointer(const unsigned char& str);

enum InfoFormatType {INFO_DESCRIPTION, FORMAT_DESCRIPTION};
using FormatIDToValueHash = OrderedHash<QByteArray, QByteArray>;
using SampleIDToIdxPtr = QSharedPointer<OrderedHash<QByteArray, int>>;
using FormatIDToIdxPtr = QSharedPointer<OrderedHash<QByteArray, int>>;
using InfoIDToIdxPtr = QSharedPointer<OrderedHash<QByteArray, int>>;
using ListOfFormatIds = QByteArray; //an array of all ordered FORMAT ids to use as hash key
using ListOfInfoIds = QByteArray; //an array of all ordered INFO ids to use as hash key

struct CPPNGSSHARED_EXPORT VcfHeaderLine
{

	QByteArray value;
	QByteArray key;

	void storeLine(QTextStream& stream) const
	{
		stream << "##" << key << "=" << value << "\n";
	}
};
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
struct CPPNGSSHARED_EXPORT FilterLine
{
	QByteArray id;
	QString description;

	void storeLine(QTextStream& stream) const
	{
		stream << "##FILTER=<ID=" << id << ",Description=\"" << description  << "\">" << "\n";
    }
};

///struct representing a vcf header.
/// most important information is stored in seperate variables, additional information
/// is in in 'unspecific_header_lines'
class CPPNGSSHARED_EXPORT VCFHeader
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
	void setCommentLine(QByteArray& line, const int line_number);
    void setInfoLine(QByteArray& line, const int line_number);
    void setFormatLine(QByteArray& line, const int line_number);

	void setFilterLine(QByteArray& line, const int line_number);
	void setFormat(QByteArray& line);
	void storeHeaderInformation(QTextStream& stream) const;

	InfoFormatLine infoLineByID(const QByteArray& id, bool error_not_found = true) const;
	InfoFormatLine formatLineByID(const QByteArray& id, bool error_not_found = true) const;
	FilterLine filterLineByID(const QByteArray& id, bool error_not_found = true) const;
	AnalysisType type(bool allow_fallback_germline_single_sample) const;
	int vepIndexByName(const QString& name, bool error_if_not_found = true) const;

private:

	static const QByteArrayList InfoTypes;
	static const QByteArrayList FormatTypes;

	QByteArray fileformat_;
	QVector<VcfHeaderLine> file_comments_;

	QVector<InfoFormatLine> info_lines_;
	QVector<FilterLine> filter_lines_;
	QVector<InfoFormatLine> format_lines_;

	bool parseInfoFormatLine(QByteArray& line,InfoFormatLine& info_format_line, QByteArray type, const int line_number);
	InfoFormatLine lineByID(const QByteArray& id, const QVector<InfoFormatLine>& lines, bool error_not_found = true) const;
};

///representation of a line of a vcf file
class CPPNGSSHARED_EXPORT  VCFLine
{

public:

	///Default constructor.
	VCFLine();
	///Constructor with basic entries
	VCFLine(const Chromosome& chr, int pos, const Sequence& ref, const QVector<Sequence>& alt, QByteArrayList format_ids = QByteArrayList(), QByteArrayList sample_ids = QByteArrayList(), QList<QByteArrayList> list_of_format_values = QList<QByteArrayList>());

	const Chromosome& chr() const
	{
		return chr_;
	}
	const int& pos() const
	{
		return pos_;
	}
	const int& start() const
	{
		return pos_;
	}
	const Sequence& ref() const
	{
		return ref_;
	}
	int end() const
	{
		if(ref().length() <= 0) THROW(ArgumentException, "Reference can not have length zero in a VCF File.")
		return (pos() + ref().length() - 1);
	}
	const QVector<Sequence>& alt() const
	{
		return alt_;
	}
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
	QByteArrayList format() const
	{
		if(!formatIdxOf_)
		{
			QByteArrayList empty_list;
			empty_list.push_back(".");
			return empty_list;
		}
		else
		{
			return formatIdxOf_->keys();
		}
	}
    QByteArrayList infoKeys()
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

	///returns all SAMPLES as a hash of (SAMPLE ID to a hash of (FORMAT ID to value))
	const QList<QByteArrayList>& samples() const
	{
		return sample_values_;
	}
	///access the hash of (FORMAT ID to value) for a SAMPLE by SAMPLE ID
	const QByteArrayList& sample(const QByteArray& sample_name) const
	{
		return sample_values_.at((*sampleIdxOf_)[sample_name]);
	}
	///access the hash of (FORMAT ID to value) for a SAMPLE by position
	const QByteArrayList& sample(int pos) const
	{
		if(pos >= sample_values_.count()) THROW(ArgumentException, QString::number(pos) + " is out of range for SAMPLES. The VCF file provides " + QString::number(samples().size()) + " SAMPLES");
		return sample_values_.at(pos);
	}
	///access the value for a FORMAT and SAMPLE ID
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
	///access the value for a FORMAT ID and SAMPLE position (default is first SAMPLE)
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
	void setAlt(const QByteArrayList& alt)
	{
		alt_.clear();
		for(const QByteArray alt_element : alt)
		{
			alt_.push_back(alt_element);
		}
	}
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
	void setFilter(const QByteArrayList& filter_list)
	{
		if(filter_list.size() == 1 && filter_list.at(0) == ".")
		{
			return;
		}
		for(const QByteArray& filter : filter_list)
		{
			filter_.push_back(strToPointer(filter));
		}
	}
	void addFilter(QByteArray& tag)
	{
		tag = tag.trimmed();
		filter_.push_back(strToPointer(tag));
	}
    void setInfo(const QByteArrayList& info_values)
	{
        info_ = info_values;
	}
    void setSample(const QList<QByteArrayList>& sample)
	{
		sample_values_ = sample;
	}
	void addFormatValues(const QByteArrayList& format_value_list)
	{
		sample_values_.push_back(format_value_list);
	}
    void setSampleIdToIdxPtr(const SampleIDToIdxPtr& ptr)
	{
		sampleIdxOf_ = ptr;
	}
    void setFormatIdToIdxPtr(const FormatIDToIdxPtr& ptr)
	{
		formatIdxOf_ = ptr;
	}
    void setInfoIdToIdxPtr(const InfoIDToIdxPtr& ptr)
    {
        infoIdxOf_ = ptr;
    }

	///Overlap check for chromosome and position range.
	bool overlapsWith(const Chromosome& input_chr, int input_start, int input_end) const
	{
		return (chr_==input_chr && BasicStatistics::rangeOverlaps(pos(), end(), input_start, input_end));
	}
	///Overlap check for position range only.
	bool overlapsWith(int input_start, int input_end) const
	{
		return BasicStatistics::rangeOverlaps(pos(), end(), input_start, input_end);
	}
	///Overlap check BED file line.
	bool overlapsWith(const BedLine& line) const
	{
		return overlapsWith(line.chr(), line.start(), line.end());
	}

	///Returns if the variant is a SNV
	/// allow_several_alternatives=TRUE if all alternatives in the vector shall be considered
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
    ///returns if any VCFLine in the file is multiallelic
    bool isMultiAllelic() const;
    ///returns if the variant is an InDel, can only be called on single allelic variants
    bool isInDel() const;
	//returns if the chromosome is valid
	bool isValidGenomicPosition() const;
	//returns all not passed filters
	QByteArrayList failedFilters() const;
	QString variantToString() const;
	QByteArrayList vepAnnotations(int field_index) const;

	void leftNormalize(QString reference_genome);
	/// Removes the common prefix/suffix from indels, adapts the start/end position and replaces empty sequences with a custom string.
	void normalize(const Sequence& empty_seq="", bool to_gsvar_format=true);

	///Equality operator (only compares the variatn location itself, not further annotations).
	bool operator==(const VCFLine& rhs) const;
	///Less-than operator.
	bool operator<(const VCFLine& rhs) const;

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
using VCFLinePtr = QSharedPointer<VCFLine>;

namespace VcfFormat
{
///Comparator helper class that used by sort().
class LessComparator
{
	public:
		///Constructor.
		LessComparator(bool use_quality);
        bool operator()(const VCFLinePtr& a, const VCFLinePtr& b) const;

	private:
		bool use_quality;
};
///Comparator helper class used by sortByFile.
class LessComparatorByFile
{
	public:
		///Constructor with FAI file, which determines the chromosome order.
		LessComparatorByFile(QString filename);
        bool operator()(const VCFLinePtr& a, const VCFLinePtr& b) const;

	private:
		QString filename_;
		QHash<int, int> chrom_rank_;
};

} //end namespace VcfFormat
