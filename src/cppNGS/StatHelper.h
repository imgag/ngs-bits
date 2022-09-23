#ifndef STATHELPER_H
#define STATHELPER_H

#include "cppNGS_global.h"
#include "BamReader.h"

class CPPNGSSHARED_EXPORT StatHelper
{
public:	
	static void countCoverageWithBaseQuality(int min_baseq, QVector<int>& roi_cov, int start, int ol_start, int ol_end, QBitArray& baseQualities, const BamAlignment& al);
	static void countCoverageWithoutBaseQuality(QVector<int>& roi_cov, int ol_start, int ol_end);
	static void countCoverageWGSWithBaseQuality(int min_baseq, QVector<unsigned char>& cov, int start, int end, QBitArray& baseQualities, const BamAlignment& al);
	static void countCoverageWGSWithoutBaseQuality(int start, int end, QVector<unsigned char>& cov);

private:
	StatHelper() = delete;
};

#endif // STATHELPER_H
