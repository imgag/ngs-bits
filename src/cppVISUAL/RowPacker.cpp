#include "RowPacker.h"


int RowPacker::insert(int start, int end, int payload)
{
	for (int row =0; row < rows_.size(); ++row)
	{
		if (fits(row, start, end))
		{
			insertIntoRow(row, start, end, payload);
			return row;
		}
	}

	rows_.append(QVector<Interval>());
	int row = rows_.size() - 1;
	insertIntoRow(row, start, end, payload);
	return row;
}

void RowPacker::restore(int row, int start, int end, int payload)
{
	while (rows_.size() <= row) rows_.append(QVector<Interval>());
	insertIntoRow(row, start, end, payload);
}

bool RowPacker::canRestore(int row, int start, int end) const
{
	if (row >= rows_.size())
		return true;

	return fits(row, start, end);
}

int RowPacker::find(int row, int pos) const
{
	if (row >= rows_.size()) return -1;

	const auto& intervals = rows_[row];
	int lo = 0, hi = intervals.size();

	while (lo < hi)
	{
		int mid = (lo + hi) / 2;
		const auto& it = intervals[mid];
		if (pos >= it.start && pos <= intervals[mid].end)
			return it.payload;        // found the interval
		if (pos < it.start)
			hi = mid;
		else
			lo = mid + 1;
	}
	return -1; // not found
}


int RowPacker::insertionPoint(int row, int start) const
{
	const auto& intervals = rows_[row];
	int lo =0, hi = intervals.size();
	while (lo < hi)
	{
		int mid = (lo + hi) / 2;
		if (intervals[mid].start < start) lo = mid + 1;
		else hi = mid;
	}
	return lo;
}

bool RowPacker::fits(int row, int start, int end) const
{
	int pos = insertionPoint(row, start);
	const auto& intervals = rows_[row];

	if (pos > 0 && intervals[pos - 1].end >= start - 1) return false;
	if (pos < intervals.size() && end >= intervals[pos].start + 1) return false;
	return true;
}

void RowPacker::insertIntoRow(int row, int start, int end, int payload)
{
	int pos = insertionPoint(row, start);
	rows_[row].insert(pos, {start, end, payload});
}
