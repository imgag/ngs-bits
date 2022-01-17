#ifndef CHAINFILEREADER_H
#define CHAINFILEREADER_H

#include "cppNGS_global.h"
#include <QSharedPointer>
#include <QFile>
#include "Helper.h"

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

	QString toString() const
	{
		return QString(chr) + ":" + QString::number(pos);
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

	bool contains(const QByteArray& chr, int pos) const
	{
		if (chr != ref_chr) return false;
		if (pos < ref_start || pos > ref_end) return false;

		return true;
	}

	bool contains(const GenomePosition& pos) const
	{
		return contains(pos.chr, pos.pos);
	}

	GenomePosition lift(const GenomePosition& ref_pos_to_lift) const
	{
		if (! contains(ref_pos_to_lift))
		{
			return GenomePosition("", -1);
		}

		int ref_current_pos = ref_start;
		int q_current_pos = q_start;

		foreach(const AlignmentLine& line, alignment)
		{
			if (ref_current_pos + line.size >= ref_pos_to_lift.pos)
			{
				int last_bit = line.size - (ref_pos_to_lift.pos - ref_current_pos);
				return GenomePosition(q_chr, q_current_pos + last_bit);
			}
			else
			{
				ref_current_pos += line.size;
				q_current_pos += line.size;

				if (ref_current_pos + line.ref_dt >= ref_pos_to_lift.pos)
				{
					// if given position lands in a gap return invalid. Given position is not mapped in this alignment
					return GenomePosition("", -1);
				}
				else
				{
					ref_current_pos += line.ref_dt;
					q_current_pos += line.q_dt;
				}
			}
		}
		//Should not be possible if the position is contained in the alignment!
		THROW(ProgrammingException, "Function contains() has an error! Position was not contained in alignment! Pos:" + ref_pos_to_lift.toString() + " Alignment: " + QString(ref_chr) + ":" + QString::number(ref_start) + "-" + QString::number(ref_end) + "\n")
	}

	bool operator >(const GenomicAlignment& other) const
	{
		if (other.ref_chr != ref_chr)
		{
			return ref_chr > other.ref_chr;
		}
		else
		{
			return ref_start > other.ref_start;
		}
	}

	bool operator ==(const GenomicAlignment& other) const
	{
		return (other.ref_chr == ref_chr && other.ref_start == ref_start && other.ref_end == ref_end);
	}
};



class CPPNGSSHARED_EXPORT ChainFileReader
{
public:
	ChainFileReader();
	~ChainFileReader();

	void load(QString filepath);
	GenomePosition lift(const GenomePosition& pos) const;

	const QHash<QByteArray, int>& refChromSizes()
	{
		return ref_chrom_sizes;
	}

	const QHash<QByteArray, int>& qChromSizes()
	{
		return q_chrom_sizes;
	}

private:
	GenomicAlignment parseChainLine(QList<QByteArray> parts);

	QString filepath_;
	QSharedPointer<QFile> fp_;
	QHash<QByteArray, QList<GenomicAlignment>> chromosomes;
	QHash<QByteArray, int> ref_chrom_sizes;
	QHash<QByteArray, int> q_chrom_sizes;
};

#endif // CHAINFILEREADER_H
