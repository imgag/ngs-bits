#include "StatHelper.h"

void StatHelper::countCoverageWithBaseQuality(int min_baseq, QVector<int>& roi_cov, int start, int ol_start, int ol_end, QBitArray& baseQualities, const BamAlignment& al)
{
	int quality_pos = std::max(start, al.start()) - al.start();
	al.qualities(baseQualities, min_baseq, al.end() - al.start() + 1);
	for (int p=ol_start; p<=ol_end; ++p)
	{
		if(baseQualities.testBit(quality_pos))
		{
			++roi_cov[p];
		}
		++quality_pos;
	}
}

void StatHelper::countCoverageWithoutBaseQuality(QVector<int>& roi_cov, int ol_start, int ol_end)
{
	for (int p=ol_start; p<=ol_end; ++p)
	{
		++roi_cov[p];
	}
}

void StatHelper::countCoverageWGSWithBaseQuality(int min_baseq, QVector<unsigned char>& cov, int start, int end, QBitArray& baseQualities, const BamAlignment& al)
{
	al.qualities(baseQualities, min_baseq, end - start);
	int quality_pos = 0;
	for (int p=start; p<end; ++p)
	{
		if(baseQualities.testBit(quality_pos))
		{
			if (cov[p]<254) ++cov[p];
		}
		++quality_pos;
	}
}

void StatHelper::countCoverageWGSWithoutBaseQuality(int start, int end, QVector<unsigned char>& cov)
{
	for (int p=start; p<end; ++p)
	{
		if (cov[p]<254) ++cov[p];
	}
}
