#ifndef CHAINFILEREADER_H
#define CHAINFILEREADER_H

#include "cppNGS_global.h"
#include <QSharedPointer>
#include <QFile>

struct AlignmentLine
{
	int size;
	int ref_dt;
	int q_dt;

	AlignmentLine():
	  size(-1)
	, ref_dt(-1)
	, q_dt(-1)
	{
	}

	AlignmentLine(int size, int ref_dt, int q_dt):
		size(size)
	  , ref_dt(ref_dt)
	  , q_dt(q_dt)
	{
	}
};

struct GenomePosition
{
	QByteArray chr;
	int pos;

	GenomePosition(QByteArray chr, int pos):
		chr(chr)
	  , pos(pos)
	{
	}
};

struct GenomicAlignment
{
	double score;

	QByteArray ref_chr;
	int ref_start;
	int ref_end;
	bool ref_on_plus;

	QByteArray q_chr;
	int q_start;
	int q_end;
	bool q_on_plus;

	int id;

	QList<AlignmentLine> alignment;

	GenomicAlignment(double score, QByteArray ref_chr, int ref_start, int ref_end, bool ref_on_plus, QByteArray q_chr, int q_start, int q_end, bool q_on_plus, int id):
		score(score)
	  , ref_chr(ref_chr)
	  , ref_start(ref_start)
	  , ref_end(ref_end)
	  , ref_on_plus(ref_on_plus)
	  , q_chr(q_chr)
	  , q_start(q_start)
	  , q_end(q_end)
	  , q_on_plus(q_on_plus)
	  , id(id)
	{
	}

	void addAlignmentLine(AlignmentLine line)
	{
		alignment.append(line);
	}

	bool contains(const QByteArray& chr, int pos)
	{
		if (chr != ref_chr) return false;
		if (pos < ref_start || pos > ref_end) return false;

		return true;
	}

	GenomePosition getLiftedPosition(const QByteArray& chr, int pos)
	{
		//TODO return the correctly lifted position
		return GenomePosition(chr, pos);
	}
};



class CPPNGSSHARED_EXPORT ChainFileReader
{
public:
	ChainFileReader();
	~ChainFileReader();

	void load(QString filepath);

private:
	GenomicAlignment parseChainLine(QList<QByteArray> parts, QHash<QByteArray, int>& ref_chrom_sizes, QHash<QByteArray, int>& q_chrom_sizes);

	QString filepath_;
	QSharedPointer<QFile> fp_;
};

#endif // CHAINFILEREADER_H
