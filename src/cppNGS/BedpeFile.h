#ifndef BEDPEFILE_H
#define BEDPEFILE_H

#include "cppNGS_global.h"
#include "Chromosome.h"
#include <QByteArray>
#include <QByteArrayList>
#include <QList>
#include <QVector>
#include <QMap>


class CPPNGSSHARED_EXPORT BedpeLine
{
public:
	BedpeLine();
	BedpeLine(const Chromosome& chr1, int start1, int end1, const Chromosome& chr2, int start2, int end2, const QList<QByteArray>& annotations);

	const Chromosome& chr1()
	{
		return chr1_;
	}
	void setChr1(const Chromosome& chr1)
	{
		chr1_ = chr1;
	}

	int start1()
	{
		return start1_;
	}

	void setStart1(int start1)
	{
		start1_ = start1;
	}
	int end1()
	{
		return end1_;
	}
	void setEnd1(int end1)
	{
		start1_ = end1;
	}


	int start2()
	{
		return start2_;
	}
	void setStart2(int start2)
	{
		start2_ = start2;
	}
	int end2()
	{
		return end2_;
	}
	void setEnd2(int end2)
	{
		end2_ = end2;
	}
	const Chromosome& chr2()
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

	QByteArray toTsv()
	{
		QByteArrayList tmp_out;

		tmp_out << chr1_.str() << QByteArray::number(start1_) << QByteArray::number(end1_) << chr2_.str() << QByteArray::number(start2_) << QByteArray::number(end2_);
		foreach(QByteArray anno, annotations_) tmp_out << anno;

		return tmp_out.join("\t");
	}

private:
	Chromosome chr1_;
	int start1_;
	int end1_;

	Chromosome chr2_;
	int start2_;
	int end2_;

	QList<QByteArray> annotations_;
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
	///Read-wrote access to members
	BedpeLine& operator[](int index)
	{
		return lines_[index];
	}

	///returns number of annotation, -1 if not found
	int annotationIndexByName(const QByteArray& name, bool error_on_mismatch = true);


	const QList<QByteArray> annotationHeaders() const
	{
		return annotation_headers_;
	}

	///Get description of annotations as written in vcf comments, e.g. FORMAT
	QMap <QByteArray,QByteArray> annotationDescriptionByID(const QByteArray& name);

private:
	QList<QByteArray> annotation_headers_;
	QList<QByteArray> comments_;
	QVector<BedpeLine> lines_;

	///Returns all information fields with "NAME=" as list of QMAP containing key value pairs
	QList< QMap<QByteArray,QByteArray> > getInfos(QByteArray name);

	///returns map with key-value pairs for vcf info line in header, beginning after e.g. INFO= or FORMAT=
	QMap <QByteArray,QByteArray> parseInfoField(QByteArray unparsed_fields);
};

#endif // BEDPEFILE_H
