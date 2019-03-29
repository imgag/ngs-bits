#ifndef SVLIST_H
#define SVLIST_H

#include "Chromosome.h"
#include <QList>
#include <QByteArrayList>




///structural variant as reported by MANTA
class CPPNGSSHARED_EXPORT StructuralVariant
{
public:
	///default constructor
	StructuralVariant();

	///main constructor
	StructuralVariant(const QByteArray type, const Chromosome& chr, int start, int end, int score, const QByteArray& filter, const Chromosome& mate_chr, int mate_pos, const QByteArray& mate_filter, const QByteArrayList& annotations);

	///Returns the affected Chromosome
	const Chromosome& chr() const
	{
		return chr_;
	}

	///Returns the start position
	int start() const
	{
		return start_;
	}

	///Returns the end position
	int end() const
	{
		return end_;
	}

	///returns the overall variant size, if it is not available -1
	int size() const
	{
		if(end_ == -1)
		{
			return -1;
		}
		else
		{
			return end_ - start_;
		}
	}

	int score() const
	{
		return score_;
	}

	///returns the filter of the variant
	const QByteArray& getFilter() const
	{
		return filter_;
	}

	///returns the variant type: e.g BND, DUP or DEL
	const QByteArray& type() const
	{
		return type_;
	}

	///returns the mate Chromosome
	const Chromosome& mateChr() const
	{
		return mate_chr_;
	}

	int matePos() const
	{
		return mate_pos_;
	}

	const QByteArray& getMateFilter() const
	{
		return mate_filter_;
	}

	const QByteArrayList& annotations() const
	{
		return annotations_;
	}

private:
	QByteArray type_;
	Chromosome chr_;
	int start_;
	int end_;
	int score_;
	QByteArray filter_;

	Chromosome mate_chr_;
	int mate_pos_;
	QByteArray mate_filter_;
	QByteArrayList annotations_;
};

///Provides a list of structural variants and basic operations
class CPPNGSSHARED_EXPORT SvList
{
public:
	///default constructor
	SvList();

	///load data from tab separated file
	void load(QString filename);

	///clear old data
	void clear();

	///returns the available comments
	const QByteArrayList& comments() const
	{
		return comments_;
	}

	///returns the number of structural variants
	int count() const
	{
		return variants_.count();
	}

	///returns a variant by its index
	const StructuralVariant& operator[](int i) const
	{
		return variants_[i];
	}

	///returns the annotation headers
	const QByteArrayList& annotationHeaders() const
	{
		return annotation_headers_;
	}

	///appends a structural variant
	void append(const StructuralVariant& variant)
	{
		variants_.append(variant);
	}

	///returns the index of an annotation by its name (denotated in the corresponding header), if no error on mismatch it returns -1 in case nothing is found
	///and -2 if more than 1 annotation is found
	int annotationIndexByName(const QByteArray& name, bool error_on_mismatch = true);

	///checks with basic rules whether file is consistent and throws errors if a mistake is detected
	void checkValid() const;

private:
	QList<StructuralVariant> variants_;
	QByteArrayList annotation_headers_;
	QByteArrayList comments_;
};


#endif // SVLIST_H
