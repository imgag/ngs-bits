#ifndef CHAINFILEREADER_H
#define CHAINFILEREADER_H

#include "cppNGS_global.h"
#include <QFile>
#include "BedFile.h"



class CPPNGSSHARED_EXPORT ChainFileReader
{
public:
	ChainFileReader(QString filepath, double percent_deletion);
	~ChainFileReader();

	BedLine lift(const Chromosome& chr, int start, int end) const;

private:
	class GenomicAlignment
	{
	private:
		struct IndexLine
		{
			int ref_start;
			int q_start;
			int alignment_line_index;

			IndexLine(int ref_start, int q_start, int idx):
				ref_start(ref_start)
			  , q_start(q_start)
			  , alignment_line_index(idx)
			{
			}
		};

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

	public:
		GenomicAlignment(double score, Chromosome ref_chr, int ref_chr_size, int ref_start, int ref_end, bool ref_on_plus, Chromosome q_chr, int q_chr_size, int q_start, int q_end, bool q_on_plus, int id);
		GenomicAlignment();

		BedLine lift(int start, int end, double percent_deletion) const;

		void addAlignmentLine(int size, int ref_dt, int q_dt);

		bool contains(const Chromosome& chr, int pos) const;
		bool overlapsWith(int start, int end) const;

		QString toString(bool with_al_lines=true) const;

		double score;
		int id;

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

		QList<AlignmentLine> alignment;

		QList<IndexLine> index;
		const static int index_frequency = 25;
	};


	void load();
	GenomicAlignment parseChainLine(QList<QByteArray> parts);

	QString filepath_;
	QFile file_;
	double percent_deletion_;

	QHash<Chromosome, QList<GenomicAlignment>> chromosomes_;
	QHash<Chromosome, int> ref_chrom_sizes_;

};

#endif // CHAINFILEREADER_H
