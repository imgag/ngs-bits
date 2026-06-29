#ifndef ROWPACKER_H
#define ROWPACKER_H

#include "cppVISUAL_global.h"
#include <QVector>
#include <QPair>

//handles row assignment for intervals
class CPPVISUALSHARED_EXPORT RowPacker
{
public:
	struct Interval
	{
		int start;
		int end;
		int payload; // payload is, for example, the index of the BamAlignment object.
		// TODO: if this class needs to be made generic, payload should be T*, a generic pointer.
	};
	void clear()
	{
		rows_.clear();
	}

	int rowCount() const {return rows_.size();}

	// greedily finds the first row where [start, end] could fit and puts it there
	int insert(int start, int end, int payload);

	// returns true if it cannot find any object at [start, end] on the provided row
	bool canRestore(int row, int start, int end) const;

	// puts [start, end] on the row. Restoring without checking will break other functionality.
	void restore(int row, int start, int end, int payload);

	// returns the payload of pos \in [start, end] of the corresponding interval on the provided row
	int find(int row, int pos) const;


private:
	QVector<QVector<Interval>> rows_;

	// returns the insertion pos based on start point
	int insertionPoint(int row, int start) const;
	// returns true if [start, end] interval can fit in the row
	bool fits(int row, int start, int end) const;
	// inserts [start, end] with payload to the row i.e just stores the struct above
	void insertIntoRow(int row, int start, int end, int payload);
};

#endif // ROWPACKER_H
