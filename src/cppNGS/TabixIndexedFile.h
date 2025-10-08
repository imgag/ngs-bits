#ifndef TABIXINDEXEDFILE_H
#define TABIXINDEXEDFILE_H

#include "cppNGS_global.h"
#include "Chromosome.h"

#include "htslib/tbx.h"

#include <QByteArrayList>
#include <QHash>

///Fast random access for files indexed with tabix using a CSI or TBI index.
class CPPNGSSHARED_EXPORT TabixIndexedFile
{
public:
	TabixIndexedFile();
	~TabixIndexedFile();

	///Open the file and loads the index.
	void load(QByteArray filename);

	///Clear all resources.
	void clear();

	///Returns the filename of the data file (not the index).
	QByteArray filename() const { return filename_; }
	///Returns the filename of the index.
	QByteArray filenameIndex() const { return filename_index_; }
	///Returns the index format: CSI, TBI
	QByteArray format() const;
	///Returns the min_shift parameter from a CSI header. Throws an exception if the index is not a CSI file!
	int minShift() const;

	///Returns lines that overlap the region (1-based)
	QByteArrayList getMatchingLines(const Chromosome& chr, int start, int end, bool ignore_missing_chr = false) const;

protected:
	QByteArray filename_;
	QByteArray filename_index_;
	htsFile* file_;
	tbx_t* tbx_;
	QHash<int, int> chr2chr_; //dictionary to translate ngs-bits chromosome IDs to tabix chromosome IDs
};

#endif // TABIXINDEXEDFILE_H
