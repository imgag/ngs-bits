#ifndef CHROMOSOMEINFO_H
#define CHROMOSOMEINFO_H

#include "api/BamReader.h"
#include "Chromosome.h"
#include <QHash>
#include "cppNGS_global.h"

using namespace BamTools;

///Information about chromosomes of a BAM file reference.
class CPPNGSSHARED_EXPORT ChromosomeInfo
{
public:
	ChromosomeInfo(BamReader& reader);

	///Returns all chromosomes
	const QList<Chromosome>& chromosomes() const;
	///Returns a chromosome based on the reference ID. If the chomosome is not found, an ArgumentException is thrown.
	const Chromosome& chromosome(int ref_id) const;

	///Returns the chromosome ID (called reference ID in BamTools). If the chomosome is not found, an ArgumentException is thrown.
	int refID(const Chromosome& chr) const;
	///Returns the size (number of bases) of a chromosome. If the chomosome is not found, an ArgumentException is thrown.
	int size(const Chromosome& chr) const;

	///Returns the genome size
	double genomeSize(bool nonspecial_only) const;

	///Returns the chromosome ID (called reference ID in BamTools). If the chomosome is not found, an ArgumentException is thrown.
	static int refID(BamTools::BamReader& reader, const Chromosome& chr);

protected:
	QList<Chromosome> chrs_;
	struct ChrInfo_
	{
		int ref_id;
		int size;
	};
	QHash<int, ChrInfo_> info_;
	QString filename_;
};

#endif // CHROMOSOMEINFO_H
