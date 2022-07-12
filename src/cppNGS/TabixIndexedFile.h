#ifndef TABIXINDEXEDFILE_H
#define TABIXINDEXEDFILE_H

#include "cppNGS_global.h"
#include "Chromosome.h"

#include "htslib/tbx.h"

#include <QByteArrayList>
#include <QHash>

class CPPNGSSHARED_EXPORT TabixIndexedFile
{
public:
	TabixIndexedFile();
	~TabixIndexedFile();

	///Open the file and loads the index.
	void load(QByteArray filename);

	///Clear all resources.
	void clear();

	///Returns lines that overlap the region (1-based)
	QByteArrayList getMatchingLines(const Chromosome& chr, int start, int end, bool ignore_missing_chr = false) const;

protected:
	QByteArray filename_;
	htsFile* file_;
	tbx_t* tbx_;
	QHash<int, int> chr2chr_; //dictionary to translate ngs-bits chromosome IDs to tabix chromosome IDs
};

#endif // TABIXINDEXEDFILE_H
