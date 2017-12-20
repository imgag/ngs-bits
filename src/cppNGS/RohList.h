#ifndef ROHLIST_H
#define ROHLIST_H

#include "cppNGS_global.h"
#include "Chromosome.h"
#include "GeneSet.h"
#include <QList>
#include <QByteArrayList>

///Representation of a run of homozygosity.
class CPPNGSSHARED_EXPORT RunOfHomozygosity
{
	public:
		///Default constructor.
		RunOfHomozygosity();
		///Main constructor.
		RunOfHomozygosity(const Chromosome& chr, int start, int end, int markers, int markers_het, double q_score, GeneSet genes, QByteArrayList annotations);

		///Returns the chromosome.
		const Chromosome& chr() const
		{
			return chr_;
		}
		///Returns the start position (1-based).
		int start() const
		{
			return start_;
		}
		///Returns the end position (1-based).
		int end() const
		{
			return end_;
		}
		///Returns the end position (1-based).
		double qScore() const
		{
			return q_score_;
		}
		///Returns the annotated genes.
		const GeneSet& genes() const
		{
			return genes_;
		}
		///Generic annotations (see also RohList::annotationHeaders()).
		const QByteArrayList& annotations() const
		{
			return annotations_;
		}

		///Returns the overall marker count.
		int markerCount() const
		{
			return markers_;
		}
		///Returns the heterozygous marker count.
		int MarkerCountHet() const
		{
			return markers_het_;
		}
		///Returns the bases count.
		int size() const
		{
			return end_ - start_ + 1;
		}
		///Convert range to string.
		QString toString() const
		{
			return chr_.str() + ":" + QString::number(start_) + "-" + QString::number(end_);
		}

	protected:
		Chromosome chr_;
		int start_;
		int end_;
		int markers_;
		int markers_het_;
		double q_score_;
		GeneSet genes_;
		QByteArrayList annotations_;
};

class CPPNGSSHARED_EXPORT RohList
{
	public:
		///Default constructor.
		RohList();
		///Clears content.
		void clear();
		///Loads ROH text file (TSV format from RohHunter).
		void load(QString filename);

		///Returns the comment header lines (without leading '##').
		const QByteArrayList& comments() const
		{
			return comments_;
		}
		///Returns the number of ROHs.
		int count()
		{
			return rohs_.count();
		}
		///Returns a variant by index.
		const RunOfHomozygosity& operator[](int index) const
		{
			return rohs_[index];
		}
		///Returns annotation headers
		const QByteArrayList& annotationHeaders() const
		{
			return annotation_headers_;
		}

	protected:
		QByteArrayList comments_;
		QList<RunOfHomozygosity> rohs_;
		QByteArrayList annotation_headers_;
};

#endif // ROHLIST_H
