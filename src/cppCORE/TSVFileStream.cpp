#include "TSVFileStream.h"
#include <QStringList>

TSVFileStream::TSVFileStream(QString filename, char separator, char comment)
	: filename_(filename)
	, separator_(separator)
	, comment_(comment)
	, file_(filename)
	, columns_(-1)
	, line_(0)
{
	//open
	bool open_status = true;
	if (filename=="")
	{
		open_status = file_.open(stdin, QFile::ReadOnly | QFile::Text);
	}
	else
	{
		open_status = file_.open(QFile::ReadOnly | QFile::Text);
	}
	if (!open_status)
	{
		THROW(FileAccessException, "Could not open file for reading: '" + filename + "'!");
	}

	//read comments and headers
	QByteArray double_quote = QByteArray(2, comment);
	next_line_ = double_quote;
	while(next_line_.startsWith(comment))
	{
		if (next_line_.startsWith(double_quote))
		{
			if (next_line_!=double_quote)
			{
				comments_.append(next_line_);
			}
		}
		else if (next_line_.startsWith(comment))
		{
			header_ = next_line_.mid(1).split(separator);
			columns_ = header_.count();
		}

		next_line_ = file_.readLine();
		while (next_line_.endsWith('\n') || next_line_.endsWith('\r')) next_line_.chop(1);
		++line_;
	}

	//no first line
	if (file_.atEnd() && next_line_=="") next_line_ = QByteArray();

	//determine number of columns if no header is present
	if (columns_==-1)
	{
		columns_ = next_line_.split(separator).count();
		for(int i=0; i<columns_; ++i)
		{
			header_.append("");
		}
	}
}

TSVFileStream::~TSVFileStream()
{
	file_.close();
}

QList<QByteArray> TSVFileStream::readLine()
{
	//handle first content line
	if (!next_line_.isNull())
	{
		QList<QByteArray> parts = next_line_.split(separator_);
		if (parts.count()!=columns_)
		{
			THROW(FileParseException, "Expected " + QString::number(columns_) + " columns, but got " + QString::number(parts.count()) + " columns in line 1 of file " + filename_);
		}
		next_line_ = QByteArray();
		return parts;
	}

	//handle second to last content line
	QByteArray line = file_.readLine();
	while (line.endsWith('\n') || line.endsWith('\r')) line.chop(1);
	++line_;

	QList<QByteArray> parts = line.split(separator_);
	if (parts.count()!=columns_)
	{
		THROW(FileParseException, "Expected " + QString::number(columns_) + " columns, but got " + QString::number(parts.count()) + " columns in line " + QString::number(line_) + " of file " + filename_);
	}
	return parts;
}

QVector<int> TSVFileStream::checkColumns(QString columns, bool numeric)
{
	QVector<int> cols;

	QStringList parts = columns.split(",");
	if (numeric)
	{
		foreach(QString part, parts)
		{
			bool ok = true;
			int col = part.toInt(&ok);
			if (!ok) THROW(CommandLineParsingException, "Could not convert column number '" + part + "' to 1-based integer!");
			if (col<1 || col>columns_)
			{
				THROW(CommandLineParsingException, "1-based column number '" + part + "' out of range (max is " + QString::number(columns_) + ")!");
			}
			cols.append(col-1);
		}
	}
	else
	{
		foreach(QString part, parts)
		{
			QVector<int> hits;
			for (int i=0; i<header_.count(); ++i)
			{
				if (part==header_[i])
				{
					hits.append(i);
				}
			}
			if (hits.count()==0) THROW(CommandLineParsingException, "Could not find column name '" + part + "' in column headers!");
			if (hits.count()>1) THROW(CommandLineParsingException, "Found column name '" + part + "' more than once in column headers!");
			cols.append(hits[0]);
		}
	}

	return cols;
}
