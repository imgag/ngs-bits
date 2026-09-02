#ifndef FASTFILEINFO_H
#define FASTFILEINFO_H

#include "cppREST_global.h"
#include <QDateTime>

class CPPRESTSHARED_EXPORT FastFileInfo
{
public:
	explicit FastFileInfo(const QString& absolute_file_path);
	qint64 size() const;
	bool exists() const;
	QString absoluteFilePath() const;
	QString absolutePath() const;
	QString fileName() const;
	QDateTime lastModified() const;

private:
	static bool cachingEnabled();

	QString absolute_file_path_;
	QString absolute_path_;
	QString filename_;
	qint64 size_;
	bool exists_;
	QDateTime last_modified_;
};

#endif // FASTFILEINFO_H
