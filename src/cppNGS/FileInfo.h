#ifndef FILEINFO_H
#define FILEINFO_H

#include "cppNGS_global.h"
#include <QString>
#include <QDateTime>

///Metadata of a file
struct CPPNGSSHARED_EXPORT FileInfo
{
	QString file_name;
	QDateTime created;
	QDateTime last_modiefied;

	FileInfo()
		: file_name()
		, created()
		, last_modiefied()
	{
	}

	FileInfo(const QString& file_name_, const QDateTime created_, const QDateTime& last_modiefied_)
		: file_name(file_name_)
		, created(created_)
		, last_modiefied(last_modiefied_)
	{
	}

	bool isEmpty()
	{
		return ((this->file_name.isEmpty()) && (this->created.isNull()) && (this->last_modiefied.isNull()));
	}
};

#endif // FILEINFO_H
