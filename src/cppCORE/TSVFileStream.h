#ifndef TSVFILESTREAM_H
#define TSVFILESTREAM_H

#include "cppCORE_global.h"
#include "Exceptions.h"
#include <QFile>
#include <QVector>

/**
  @brief TSV file parser as stream.

  Assumes that comments (double-quoted) and header (single-quoted, only one) are only at the beginning of the file.
*/
class CPPCORESHARED_EXPORT TSVFileStream
{
public:
	///Constructor. Reads from stdin if @p filename is empty.
	TSVFileStream(QString filename, char separator = '\t', char comment = '#');
    ///Destructor.
    ~TSVFileStream();

	///Returns if the stream is at the end.
	bool atEnd() const
	{
		return file_.atEnd() && next_line_.isNull();
	}

	///Returns the current line.
	QList<QByteArray> readLine();

	///Returns the split header line. If no header is present, a list with empty string is returned.
	const QList<QByteArray>& header() const
	{
		return header_;
	}
	///Returns the comment lines.
	const QVector<QByteArray>& comments() const
	{
		return comments_;
	}
	///Returns the number of columns in the file.
	int columns() const
	{
		return columns_;
	}

	///Returns the 0-based index of the last read line.
	int lineIndex() const
	{
		return line_;
	}

	///Checks and converts a comma-separated list of columns (names or 1-based indices) to 0-based numeric indices.
	QVector<int> checkColumns(QString columns, bool numeric);

protected:
	QString filename_;
	char separator_;
	char comment_;
	QFile file_;
	QByteArray next_line_;
	QVector<QByteArray> comments_;
	QList<QByteArray> header_;
	int columns_;
	int line_;

    //declared away methods
    TSVFileStream(const TSVFileStream& );
    TSVFileStream& operator=(const TSVFileStream&);
    TSVFileStream();
};


#endif
