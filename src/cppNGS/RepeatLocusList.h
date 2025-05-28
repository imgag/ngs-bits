#ifndef REPEATLOCUSLIST_H
#define REPEATLOCUSLIST_H

#include "cppNGS_global.h"
#include "BedFile.h"
#include <QDateTime>

//Repeat locus
class CPPNGSSHARED_EXPORT RepeatLocus
{
public:
	RepeatLocus();

	//Returns if the locus is valid
	bool isValid() const;

	const QByteArray& name() const { return name_; }
	void setName(const QByteArray& name) { name_ = name.trimmed(); }
	//extracts the gene symbol from the locus name
	QByteArray geneSymbol() const;

	const BedLine& region() const { return region_; }
	void setRegion(const BedLine& region) { region_ = region; }

	const QByteArray& unit() const { return unit_; }
	void setUnit(const QByteArray& unit) { unit_ = unit.trimmed(); }

	const QByteArray& allele1() const { return allele1_; }
	void setAllele1(const QByteArray& allele1);
	//Returns the number of repeat units as integer. Throws an error, if casting to int is not possible.
	int allele1asInt();

	const QByteArray& allele2() const { return allele2_; }
	void setAllele2(const QByteArray& allele2);
	//Returns the number of repeat units as integer. Throws an error, if casting to int is not possible.
	int allele2asInt();

	//Returns the repeat lengths for all alleles. If allele 2 is missing, it is omitted.
	QByteArray alleles() const;

	const QByteArrayList& filters() const { return filters_; }
	void setFilters(const QByteArrayList& filters);

	const QByteArray& coverage() const { return coverage_; }
	void setCoverage(const QByteArray& coverage);

	const QByteArray& readsFlanking() const { return reads_flanking_; }
	void setReadsFlanking(const QByteArray& reads_flanking) { reads_flanking_ = reads_flanking.trimmed(); }

	//Note: for Straglr only only supporting reads are returned. They are stored in this variable. Flainking/spanning reads are empty in this case.
	const QByteArray& readsInRepeat() const { return reads_in_repeat_; }
	void setReadsInRepeat(const QByteArray& reads_in_repeat) { reads_in_repeat_ = reads_in_repeat.trimmed(); }

	const QByteArray& readsSpanning() const { return reads_spanning_; }
	void setReadsSpanning(const QByteArray& reads_spanning) { reads_spanning_ = reads_spanning.trimmed(); }

	const QByteArray& confidenceIntervals() const { return confidence_intervals_; }
	void setConfidenceIntervals(const QByteArray& confidence_intervals) { confidence_intervals_ = confidence_intervals.trimmed(); }

	bool sameRegionAndLocus(const RepeatLocus& rhs) const;

	QString toString(bool add_region_unit, bool add_genotypes) const;

protected:
	QByteArray name_;
	BedLine region_;
	QByteArray unit_;

	QByteArray allele1_;
	QByteArray allele2_;

	//QC data
	QList<QByteArray> filters_;
	QByteArray coverage_;
	QByteArray reads_flanking_;
	QByteArray reads_in_repeat_;
	QByteArray reads_spanning_;
	QByteArray confidence_intervals_;
};

///CNV list types
enum class ReCallerType
{
	INVALID,
	EXPANSIONHUNTER,
	STRAGLR
};

//Repeat expansion list
class CPPNGSSHARED_EXPORT RepeatLocusList
{
public:
	RepeatLocusList();

	//Load repeat expansions from VCF file.
	void load(QString filename);

	//Returns the caller
	ReCallerType caller() const;
	//Returns the caller. Throws exception if invalid.
	QByteArray callerAsString() const;
	//Returns the caller version
	const QByteArray callerVersion() const;
	//Returns the calling date
	const QDate& callingDate() const;

	//Clears content
	void clear()
	{
		caller_ = ReCallerType::INVALID;
		caller_version_.clear();
		call_date_ = QDate();
		variants_.clear();
	}

	//Returns if the list is valid
	bool isValid() const
	{
		return caller_!=ReCallerType::INVALID && !caller_version_.isEmpty() && call_date_.isValid() && !variants_.isEmpty();
	}

	//Returns the number of variants
	int count() const
	{
		return variants_.count();
	}

	//Returns if the list is empty
	bool isEmpty() const
	{
		return variants_.isEmpty();
	}

	//Returns a variant by index.
	const RepeatLocus& operator[](int index) const
	{
		return variants_[index];
	}

	//Appends copy number variant
	void append(const RepeatLocus& add)
	{
		variants_.append(add);
	}

protected:

	ReCallerType caller_;
	QByteArray caller_version_;
	QDate call_date_;
	QList<RepeatLocus> variants_;
};

#endif // REPEATEXPANSIONLIST_H
