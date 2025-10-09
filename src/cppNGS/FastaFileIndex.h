#ifndef FASTAFILEINDEX_H
#define FASTAFILEINDEX_H

#include "cppNGS_global.h"
#include "Chromosome.h"
#include "Sequence.h"
#include <QFile>
#include <QHash>

///Fasta file index for fast access to seqences in a FASTA file.
class CPPNGSSHARED_EXPORT FastaFileIndex
{
public:
	///Constructor, loads an index corresponding to @p fasta_file. The index is assumed to have the same name with appended '.fai' extension.
	FastaFileIndex(QString fasta_file);
	///Descructor.
	~FastaFileIndex();

	///Returns the sequence corresponding to the given chromosome.
	Sequence seq(const Chromosome& chr, bool to_upper = true) const;
	///Returns the sequence corresponding to the given chromosome and range (start is 1-based). If the coordinates are invalid, an empty string is returned.
	Sequence seq(const Chromosome& chr, int start, int length, bool to_upper = true) const;

	///Returns the chromosomes in the order of the input FASTA file.
	const QList<Chromosome>& chromosomes() const
	{
		return chrs_;
	}

	///Returns the length of the given chromosome.
	int lengthOf(const Chromosome& chr) const
	{
		return index(chr).length;
	}

	///Returns the number of 'N' bases of the given chromosome.
	int n(const Chromosome& chr) const;

protected:
	QString fasta_name_;
	QString index_name_;
	struct FastaIndexEntry
	{
		QString name;  ///< chromosome name (original string, i.e. not normalized)
		int length;  ///< length of sequence
		long long offset;  ///< bytes offset of sequence from start of file
		int line_blen;  ///< line length in bytes, sequence characters
		int line_len;  ///< line length including newline
	};
	QHash<Chromosome, FastaIndexEntry> index_;
	QList<Chromosome> chrs_;
	mutable QHash<Chromosome, int> n_; //cache for N bases (slow, so it should not be calcualted more than once)
	mutable QFile file_;
	const FastaIndexEntry& index(const Chromosome& chr) const;
	bool isLocal() const;
	void saveEntryToIndex(const QList<QByteArray>& fields);
};

#endif
