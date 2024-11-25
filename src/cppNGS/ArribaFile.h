#ifndef ARRIBAFILE_H
#define ARRIBAFILE_H

#include <TsvFile.h>
#include "Chromosome.h"
#include "Exceptions.h"

class CPPNGSSHARED_EXPORT GenomePosition
{
public:
	GenomePosition()
	{
	};

	GenomePosition(const Chromosome& chr, int pos):
		chr_(chr)
	  , pos_(pos)
	{
	};

	GenomePosition(QString position, const char& sep=':')
	{
		QStringList parts = position.split(sep);

		Chromosome chr = Chromosome(parts[0].trimmed());
		if (! chr.isValid()) THROW(ArgumentException, "Invalid breakpoint: " + position +". Invalid chromosome. Expected: chr[sep]pos");
		chr_ = chr;

		bool ok;
		int pos = parts[1].trimmed().toInt(&ok);
		if (!ok) THROW(ArgumentException, "Invalid breakpoint: " + position +". Invalid position. Expected: chr[sep]pos");
		pos_ = pos;
	};

	GenomePosition(const GenomePosition& gpos)
	{
		chr_ = gpos.chr_;
		pos_ = gpos.pos_;
	}

	QByteArray toByteArray(QByteArray sep=":") const
	{
		return chr_.str() + sep + QByteArray::number(pos_);
	}

	const Chromosome& chr() const
	{
		return chr_;
	}

	int pos() const
	{
		return pos_;
	}

private:
	Chromosome chr_;
	int pos_;
};


class CPPNGSSHARED_EXPORT Fusion
{
public:
	Fusion();
	Fusion(GenomePosition breakpoint1, GenomePosition breakpoint2);
	Fusion(GenomePosition breakpoint1, GenomePosition breakpoint2, QStringList& annotations);
	Fusion(GenomePosition breakpoint1, GenomePosition breakpoint2, QString symbol1, QString transcript1, QString symbol2, QString transcript2, QString type, QString reading_frame, QStringList annotations=QStringList());

	QString getAnnotation(int column_idx) const;
	QString toString() const;

	GenomePosition breakpoint1() const;
	GenomePosition breakpoint2() const;
	QString symbol1() const;
	QString transcript1() const;
	QString symbol2() const;
	QString transcript2() const;
	QString type() const;
	QString reading_frame() const;

	QStringList annotations();

	bool fully_initialized() const;

private:
	GenomePosition breakpoint1_;
	GenomePosition breakpoint2_;
	QString symbol1_;
	QString transcript1_;
	QString symbol2_;
	QString transcript2_;

	QString type_;
	QString reading_frame_;

	QStringList annotations_;

	bool fully_initialized_;
};


class CPPNGSSHARED_EXPORT ArribaFile: public TsvFile
{
public:
	ArribaFile();

	void load(QString filename);

	int count() const;
	Fusion getFusion(int idx) const;

	QByteArray getCallerVersion() const;
	QByteArray getCaller() const;
	QByteArray getCallDate() const;

};




#endif // ARRIBAFILE_H
