#ifndef CHROMOSOMALFILEINDEX_H
#define CHROMOSOMALFILEINDEX_H

#include "cppNGS_global.h"
#include "limits"
#include "Chromosome.h"
#include <QHash>
#include <QVector>
#include <QPair>
#include <QMap>
#include <QString>

///Chromosomal index for fast access to indexed tab-separated files with chromosome, start position and end position column.
class CPPNGSSHARED_EXPORT ChromosomalFileIndex
{
public:
	///Constructor.
	ChromosomalFileIndex();
	///Creates an index file for a database.
	void create(const QString& db_file_name, int chr_col, int start_col, int end_col, char comment, int bin_size = 1000);
	///Loads an index corresponding to @p db_file_name.
	void load(const QString& db_file_name);
	///Returns the file position range for the given chromosomal range.
	QPair<long, long> filePosition(const Chromosome& chr, int start, int end);
	///Returns the lines overlapping the given chromosomal range.
	QStringList lines(const Chromosome& chr, int start, int end);

	///Checks if the index file of a database exists and is up-to-date.
	static bool isUpToDate(QString db_file_name);

protected:
	QString db_file_name_;
	QMap<QString, QString> meta_;
	QHash<QString, QVector<QPair<int, long> > > index_;
	int max_length_;

    bool overlap(int start1, int end1, int start2, int end2)
    {
        return (start2<=start1 && end2>=start1) || (start2<=end1 && end2>=end1) || (start2>=start1 && end2<=end1);
    }
    static QString version()
    {
        return "2";
    }

};

#endif // CHROMOSOMALFILEINDEX_H
