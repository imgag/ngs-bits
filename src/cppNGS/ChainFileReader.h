#ifndef CHAINFILEREADER_H
#define CHAINFILEREADER_H

#include "cppNGS_global.h"
#include "VersatileFile.h"
#include "BedFile.h"



class CPPNGSSHARED_EXPORT ChainFileReader
{
public:
	//Constructor
	ChainFileReader(QString filepath, double percent_deletion);
	~ChainFileReader();

	// lifts the given region to the new genome
	BedLine lift(const Chromosome& chr, int start, int end) const;

private:
	// internal class to represent the genomic alignment between the reference and query genome
	class GenomicAlignment
	{
	private:
		// helper struct to build an index improving performance
		// (saves checkpoints within an alignment which reduces calculations within an alignment)
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
		//Constructor
		GenomicAlignment(double score, Chromosome ref_chr, int ref_chr_size, int ref_start, int ref_end, bool ref_on_plus, Chromosome q_chr, int q_chr_size, int q_start, int q_end, bool q_on_plus, int id);
		GenomicAlignment();

		///
		/// \brief try to lift given cooridinates in this alignment
		/// \param start start coordinate (0-based)
		/// \param end end coordinate
		/// \param percent_deletion how many percent of bases in the ref genome are allowed to be unmapped/deleted
		/// \return BedLine("", -1,-1) if no lifting possible else the lifted region.
		///
		BedLine lift(int start, int end, double percent_deletion) const;

		// add newly parsed alignment line (also builds the index when more than "index_frequency" lines are added)
		void addAlignmentLine(int size, int ref_dt, int q_dt);

		// return wheather a given position/region is within the reference region of this alignment
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
		// how often the index saves "checkpoints" every X alignmentLines.
		// around 20-50 seems best
		const static int index_frequency = 25;
	};

	//parse file and generate genomicAlignments
	void load();
	GenomicAlignment parseChainLine(QList<QByteArray> parts);

	QString filepath_;
	double percent_deletion_;

	QHash<Chromosome, QList<GenomicAlignment>> chromosomes_;
	QHash<Chromosome, int> ref_chrom_sizes_;

};

#endif // CHAINFILEREADER_H
