#ifndef BEDPEFILE_H
#define BEDPEFILE_H

#include "cppNGS_global.h"
#include "Chromosome.h"
#include <QByteArray>
#include <QByteArrayList>
#include <QList>
#include <QVector>
#include <QMap>
#include <QSharedPointer>
#include <QFile>
#include "Helper.h"

class CPPNGSSHARED_EXPORT BedpeLine
{
public:
	BedpeLine();
	BedpeLine(const QList<QByteArray>& input_fields);
	BedpeLine(const Chromosome& chr1, int start1, int end1, const Chromosome& chr2, int start2, int end2, const QList<QByteArray>& annotations);

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

	const QList<QByteArray>& annotations() const
	{
		return annotations_;
	}
	QList<QByteArray>& annotations()
	{
		return annotations_;
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

protected:
	Chromosome chr1_;
	int start1_;
	int end1_;

	Chromosome chr2_;
	int start2_;
	int end2_;

	QList<QByteArray> annotations_;

	///Converts input position to integer, we set position to -1 if invalid input
	int parsePosIn(QByteArray in) const;
	QByteArray parsePosOut(int in) const;
};



class CPPNGSSHARED_EXPORT BedpeFile
{
public:
	BedpeFile();

	enum SV_TYPE{ DELLY, MANTA, UNKNOWN};
	///returns analysis type
	SV_TYPE analysisType();

	///load bedpe file
	void load(const QString& file_name);

	void clear()
	{
		annotation_headers_.clear();
		comments_.clear();
		lines_.clear();
	}

	int count()
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

	///returns index of annotation, -1 if not found
	int annotationIndexByName(const QByteArray& name, bool error_on_mismatch = true);

	///returns annotation headers
	const QList<QByteArray> annotationHeaders() const
	{
		return annotation_headers_;
	}

	///Get description of annotations as written in vcf comments, e.g. FORMAT
	QMap <QByteArray,QByteArray> annotationDescriptionByID(const QByteArray& name);

	///Sorts Bedpe file (by columns chr1, start1, chr2, start2)
	void sort();

	///Stores file as TSV
	void toTSV(QString file_name);

private:
	QList<QByteArray> annotation_headers_;
	QList<QByteArray> comments_;
	QVector<BedpeLine> lines_;

	///Returns all information fields with "NAME=" as list of QMAP containing key value pairs
	QList< QMap<QByteArray,QByteArray> > getInfos(QByteArray name);

	///Returns map with key-value pairs for vcf info line in header, beginning after e.g. INFO= or FORMAT=
	QMap <QByteArray,QByteArray> parseInfoField(QByteArray unparsed_fields);
};

#endif // BEDPEFILE_H
