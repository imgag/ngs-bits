#ifndef ROWPACKER_H
#define ROWPACKER_H

#include <QVector>
#include <QPair>


class RowPacker
{
public:
	void clear()
	{
		rows_.clear();
	}

	int rowCount() const {return rows_.size();}

	int insert(int start, int end);

	void restore(int row, int start, int end);



private:
	using Interval = QPair<int, int>;
	QVector<QVector<Interval>> rows_;

	int insertionPoint(int row, int start) const;

	bool fits(int row, int start, int end) const;

	void insertIntoRow(int row, int start, int end);
};

#endif // ROWPACKER_H
