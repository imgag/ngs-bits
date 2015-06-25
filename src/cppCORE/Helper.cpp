#include "Helper.h"
#include "Exceptions.h"
#include "cmath"
#include "time.h"
#include <QDir>
#include <QCoreApplication>
#include <QDateTime>

QString Helper::randomString(int length, const QString& chars)
{
	//initialize random number generator
	//initialized only once - otherwise two calls in the same second produce to the same random string
	static bool is_initialized = false;
	if (!is_initialized)
	{
		int seconds = time(NULL);
		int milliseconds = QTime::currentTime().msec();
		int process_id = QCoreApplication::applicationPid();
		srand(seconds + milliseconds + process_id);
		is_initialized = true;
	}

	//create random string
	QString output;
	for (int i=0; i<length; ++i)
	{
		output.append(chars[rand() % (chars.length()-1)]);
	}
	return output;
}

QString Helper::elapsedTime(QTime elapsed)
{
	//calculate minutes and seconds
	double s = elapsed.elapsed()/1000.0;
	double m = floor(s/60.0);
	s -= 60.0 * m;

	//create strings
	QString sec = QString::number(s, 'f', 3) + "s";
	QString min = "";
	if (m>0.0) min = QString::number(m, 'f', 0) + "m ";

	return min + sec;
}


QStringList Helper::loadTextFile(QString file_name, bool trim_lines, QChar skip_header_char, bool skip_empty_lines)
{
	QFile file(file_name);
	if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
	{
		THROW(FileAccessException, "Could not open file for reading: '" + file_name + "'!");
	}

	QStringList output;
	while (!file.atEnd())
	{
		QByteArray line = file.readLine();

		//remove newline or trim
		if (trim_lines)
		{
			line = line.trimmed();
		}
		else
		{
			line.chop(1);
		}

		//skip empty lines
		if (skip_empty_lines && line.count()==0) continue;

		//skip header lines
		if (skip_header_char!=QChar::Null && line.count()!=0 && line[0]==skip_header_char.toLatin1()) continue;

		output.append(line);
	}

	return output;
}

void Helper::storeTextFile(QString file_name, const QStringList& lines, bool stdout_if_file_empty, bool append)
{
	QScopedPointer<QFile> file(openFileForWriting(file_name, stdout_if_file_empty, append));
	foreach(const QString& line, lines)
	{
		file->write(line.toLatin1() + "\n");
	}
}

QString Helper::tempFileName(QString extension, int length)
{
	QString name = Helper::randomString(length);
	if (extension!="")
	{
		if (!extension.startsWith(".")) name.append(".");
		name.append(extension);
	}

	return QDir::toNativeSeparators(QDir::tempPath() + "/" + name);
}

void Helper::findFiles(const QString& directory, const QString& pattern, QStringList& output, bool first_call)
{
	QDir dir(directory);
	if (first_call)
	{
		output.clear();
		if(!dir.exists()) THROW(FileAccessException, "Directory does not exist: " + directory);
	}

	QFileInfoList file_infos = dir.entryInfoList(QStringList() << pattern, QDir::AllDirs|QDir::Files|QDir::NoDotAndDotDot);
	foreach(const QFileInfo& entry, file_infos)
	{
		if(entry.isFile())
		{
			output.append(directory + "/" + entry.fileName());
		}
		else if(entry.isDir())
		{
			findFiles(directory + "/" + entry.fileName(), pattern, output, false);
		}
	}
}

void Helper::findFolders(const QString& directory, const QString& pattern, QStringList& output, bool first_call)
{
	QDir dir(directory);
	if (first_call)
	{
		output.clear();
		if(!dir.exists()) THROW(FileAccessException, "Directory does not exist: " + directory);
	}

	QFileInfoList file_infos = dir.entryInfoList(QStringList() << pattern, QDir::Dirs|QDir::NoDotAndDotDot);
	foreach(const QFileInfo& entry, file_infos)
	{
		output.append(directory + "/" + entry.fileName());
	}
	file_infos = dir.entryInfoList(QDir::AllDirs|QDir::NoDotAndDotDot);
	foreach(const QFileInfo& entry, file_infos)
	{
		findFolders(directory + "/" + entry.fileName(), pattern, output, false);
	}
}

int Helper::levenshtein(const QString& s1, const QString& s2)
{
	const int len1 = s1.size();
	const int len2 = s2.size();
	QVector<int> col(len2+1);
	QVector<int> prevCol(len2+1);

	for (int i = 0; i < prevCol.size(); i++)
	{
		prevCol[i] = i;
	}
	for (int i = 0; i < len1; i++)
	{
		col[0] = i+1;
		for (int j = 0; j < len2; j++)
		{
			col[j+1] = std::min( std::min(prevCol[1 + j] + 1, col[j] + 1), prevCol[j] + (s1[i]==s2[j] ? 0 : 1) );
		}
		col.swap(prevCol);
	}
	return prevCol[len2];
}

QString Helper::userName()
{
#ifdef WIN32
	return qgetenv("USERNAME");
#else
	return qgetenv("USER");
#endif
}

QString Helper::dateTime(QString format)
{
	if (format=="")
	{
		return QDateTime::currentDateTime().toString(Qt::ISODate);
	}

	return QDateTime::currentDateTime().toString(format);
}

QFile* Helper::openFileForReading(QString file_name, bool stdin_if_empty)
{
	QFile* file = new QFile(file_name);
	if (stdin_if_empty && file_name=="")
	{
		file->open(stdin, QFile::ReadOnly);
	}
	else if (!file->open(QFile::ReadOnly))
	{
		delete file;
		THROW(FileAccessException, "Could not open file for reading: '" + file_name + "'!");
	}

	return file;
}

QFile* Helper::openFileForWriting(QString file_name, bool stdout_if_file_empty, bool append)
{
	QFile* file = new QFile(file_name);
	if (stdout_if_file_empty && file_name=="")
	{
		file->open(stdout, QFile::WriteOnly);
	}
	else if (!file->open(QFile::WriteOnly|(append? QFile::Append : QFile::Truncate)))
	{
		delete file;
		THROW(FileAccessException, "Could not open file for writing: '" + file_name + "'!");
	}

	return file;
}
