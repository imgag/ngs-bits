#include "RowPacker.h"


int RowPacker::insert(int start, int end)
{
	for (int row =0; row < rows_.size(); ++row)
	{
		if (fits(row, start, end))
		{
			insertIntoRow(row, start, end);
			return row;
		}
	}

	rows_.append(QVector<Interval>());
	int row = rows_.size() - 1;
	insertIntoRow(row, start, end);
	return row;
}

void RowPacker::restore(int row, int start, int end)
{
	while (rows_.size() <= row) rows_.append(QVector<Interval>());
	insertIntoRow(row, start, end);
}


int RowPacker::insertionPoint(int row, int start) const
{
	const auto& intervals = rows_[row];
	int lo =0, hi = intervals.size();
	while (lo < hi)
	{
		int mid = (lo + hi) / 2;
		if (intervals[mid].first < start) lo = mid + 1;
		else hi = mid;
	}
	return lo;
}

bool RowPacker::fits(int row, int start, int end) const
{
	int pos = insertionPoint(row, start);
	const auto& intervals = rows_[row];

	if (pos > 0 && intervals[pos - 1].second > start) return false;
	if (pos < intervals.size() && end > intervals[pos].first) return false;
	return true;
}

void RowPacker::insertIntoRow(int row, int start, int end)
{
	int pos = insertionPoint(row, start);
	rows_[row].insert(pos, {start, end});
}
