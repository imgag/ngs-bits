#ifndef CHAINFILEREADER_H
#define CHAINFILEREADER_H

#include "cppNGS_global.h"
#include <memory>
#include <QFile>
#include "Helper.h"
#include <iostream>
#include "BedFile.h"
#include "ChromosomalIndex.h"

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

	QString toString() const
	{
		return QString::number(size) + "\t" + QString::number(ref_dt) + "\t" + QString::number(q_dt);
	}
};

struct GenomePosition
{
	Chromosome chr;
	int pos;

	GenomePosition(Chromosome chr, int pos):
		chr(chr)
	  , pos(pos)
	{
	}

	QString toString() const
	{
		return QString(chr.strNormalized(true)) + ":" + QString::number(pos);
	}
};

struct GenomicAlignment
{
	double score;

	Chromosome ref_chr;
	int ref_chr_size;
	int ref_start;
	int ref_end;
	bool ref_on_plus;

	Chromosome q_chr;
	int q_chr_size;
	int q_start;
	int q_end;
	bool q_on_plus;

	int id;

	QList<AlignmentLine> alignment;


	GenomicAlignment():
		score(0)
	  , ref_chr("")
	  , ref_chr_size(0)
	  , ref_start(0)
	  , ref_end(0)
	  , ref_on_plus(false)
	  , q_chr("")
	  , q_chr_size(0)
	  , q_start(0)
	  , q_end(0)
	  , q_on_plus(false)
	  , id(0)
	{
	}

	GenomicAlignment(double score, Chromosome ref_chr, int ref_chr_size, int ref_start, int ref_end, bool ref_on_plus, Chromosome q_chr, int q_chr_size, int q_start, int q_end, bool q_on_plus, int id):
		score(score)
	  , ref_chr(ref_chr)
	  , ref_chr_size(ref_chr_size)
	  , ref_start(ref_start)
	  , ref_end(ref_end)
	  , ref_on_plus(ref_on_plus)
	  , q_chr(q_chr)
	  , q_chr_size(q_chr_size)
	  , q_start(q_start)
	  , q_end(q_end)
	  , q_on_plus(q_on_plus)
	  , id(id)
	{
	}

	BedLine lift(int start, int end) const
	{
		int lifted_start = -1;
		int lifted_end = -1;

		int ref_current_pos = ref_start;
		int q_current_pos = q_start;

//		std::cout << "start: " << start << " - end:" << end << "\n";
		for (int i=0; i<alignment.size(); i++)
		{
			const AlignmentLine& line = alignment[i];
			std::cout << "current pos: " << ref_current_pos << " - current_q_pos: " << q_current_pos  << " - line size: " << line.size << "\t- line ref_dt: " << line.ref_dt << "\t- line q_dt: " << line.q_dt <<"\n";
			// try to lift start and end:
			if (ref_current_pos <= start && ref_current_pos + line.size >= start)
			{
				std::cout << "start current pos in alignment: " << ref_current_pos << " - line size: " << line.size << "\n";
				lifted_start = q_current_pos + (start - ref_current_pos);

			}
			// if start was in an unmapped or deleted region of the last alignment line - take the next possible position:
			if (ref_current_pos > start && lifted_start == -1)
			{
				std::cout << "start in unmapped: " << "\n";
				lifted_start = q_current_pos;
			}
			//it's not in the same alignment piece but there is no gap in the reference:
			if (ref_current_pos <= end && ref_current_pos + line.size >= end)
			{
				std::cout << "end current pos in alignment: " << ref_current_pos << " - line size: " << line.size << "\n";
				lifted_end = q_current_pos + (end - ref_current_pos);
			}

			// if end is in the next unmapped region - take the last possible position:
			if (ref_current_pos +line.size < end && end < ref_current_pos +line.size + line.ref_dt && lifted_end == -1)
			{
				std::cout << "end in unmapped: " << "\n";
				lifted_end = q_current_pos + line.size;
			}

			ref_current_pos += line.size;
			q_current_pos += line.size;

			// break when the current position is after the start:
			if (ref_current_pos > end)
			{
				break;
			}
			else
			{
				ref_current_pos += line.ref_dt;
				q_current_pos += line.q_dt;
			}

		}

		if (lifted_start != -1 && lifted_end != -1)
		{
			if (q_on_plus)
			{
//				std::cout << "plus strand!\n";
				return BedLine(q_chr, lifted_start, lifted_end);
			}
			else
			{
//				std::cout << "minus strand!\n";
				return BedLine(q_chr, q_chr_size- lifted_end, q_chr_size- lifted_start);
			}
		}
		else
		{
			if (lifted_start == -1 && lifted_end == -1)
			{
				// both start and end are mapped to gaps in this alignment. Return "empty" result, to continue search in other alignment.
				return BedLine("", -1, -1);
			}

			// One was lifted so the region was split or partially deleted:
			THROW(ArgumentException, "Region was split or partially deleted! lifted start: " + QByteArray::number(lifted_start) + " - lifted end: " + QByteArray::number(lifted_end));
		}
	}

	int start() const
	{
		return ref_start;
	}

	int end() const
	{
		return ref_end;
	}

	Chromosome chr() const
	{
		return ref_chr;
	}

	void addAlignmentLine(AlignmentLine line)
	{
		alignment.append(line);
	}

	bool contains(const Chromosome& chr, int pos) const
	{
		if (chr != ref_chr) return false;
		if (pos < ref_start || pos > ref_end) return false;

		return true;
	}

	bool overlapsWith(int start, int end) const
	{
		return ((ref_start <= start && start <= ref_end) || (ref_start <= end && end <= ref_end));
	}

	QString toString(bool with_al_lines=true) const
	{
		QString res = QString("ref_chr:\t%1\tref_start:\t%2\tref_end:\t%3\tq_chr:\t%4\tq_start:\t%5\tq_end:\t%6\n").arg(QString(ref_chr.strNormalized(true)), QString::number(ref_start), QString::number(ref_end), QString(q_chr.strNormalized(true)), QString::number(q_start), QString::number(q_end));
		res += "ref on plus: " + QString::number(ref_on_plus) + "\tq on plus: " + QString::number(q_on_plus);
		if (with_al_lines)
		{
			foreach (AlignmentLine l, alignment)
			{
				res += l.toString() + "\n";
			}
		}

		return res;
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

	class IntervalTree
	{
	public:
		IntervalTree(int min, int max);
		IntervalTree();
		~IntervalTree();
		QList<GenomicAlignment> query(int start, int end) const;
		void addInterval(GenomicAlignment alignment);
		void sort();

		IntervalTree(const IntervalTree& other)
		: min_(other.min_)
		, max_(other.max_)
		, center_(other.center_)
		, left_(other.left_ ? other.left_->clone() : nullptr)
		, right_(other.right_ ? other.right_->clone() : nullptr)
		, sorted_by_start_(other.sorted_by_start_)
		, sorted_by_end_(other.sorted_by_end_)
		{}

		IntervalTree& operator=(const IntervalTree& other) {
			min_ = other.min_;
			max_ = other.max_;
			center_ = other.center_;
			left_ = other.left_ ? other.left_->clone() : nullptr;
			right_= other.right_ ? other.right_->clone() : nullptr;
			sorted_by_start_ = other.sorted_by_start_;
			sorted_by_end_ = other.sorted_by_end_;
			return *this;
		}

		std::unique_ptr<IntervalTree> clone() const {
			return std::unique_ptr<IntervalTree>(new IntervalTree(*this));
		}

		struct AlignmentStartComp
		{
			inline bool operator() (const GenomicAlignment& left, const GenomicAlignment& right)
			{
				return (left.ref_start < right.ref_start);
			}
		};

		struct AlignmentEndComp
		{
			inline bool operator() (const GenomicAlignment& left, const GenomicAlignment& right)
			{
				return (left.ref_end < right.ref_end);
			}
		};

	private:
		int min_, max_, center_;
		std::unique_ptr<IntervalTree> left_;
		std::unique_ptr<IntervalTree> right_;
		QList<GenomicAlignment> sorted_by_start_;
		QList<GenomicAlignment> sorted_by_end_;

	};

	ChainFileReader();
	~ChainFileReader();

	void load(QString filepath);
	BedLine lift_tree(const Chromosome& chr, int start, int end) const;
	BedLine lift_list(const Chromosome& chr, int start, int end) const;
	BedLine lift_index(const Chromosome& chr, int start, int end) const;
	void testing();

	const QHash<Chromosome, int>& refChromSizes()
	{
		return ref_chrom_sizes_;
	}

	const QHash<Chromosome, int>& qChromSizes()
	{
		return q_chrom_sizes_;
	}

	std::unique_ptr<ChromosomalIndex<QVector<GenomicAlignment>>> chromosomes_index;
	QHash<Chromosome, QList<GenomicAlignment>> chromosomes_list;
private:
	GenomicAlignment parseChainLine(QList<QByteArray> parts);

	QString filepath_;
	QSharedPointer<QFile> fp_;
	double percent_deletion_;

//	QHash<Chromosome, QList<GenomicAlignment>> chromosomes_list;
	QHash<Chromosome, IntervalTree> chromosomes_tree;

	QVector<GenomicAlignment> alignments_index_;

	QHash<Chromosome, int> ref_chrom_sizes_;
	QHash<Chromosome, int> q_chrom_sizes_;

};

#endif // CHAINFILEREADER_H
