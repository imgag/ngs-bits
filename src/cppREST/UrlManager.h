#ifndef URLMANAGER_H
#define URLMANAGER_H

#include <QMap>
#include <QMutex>
#include <QDateTime>
#include "cppREST_global.h"
#include "ServerHelper.h"

struct CPPRESTSHARED_EXPORT UrlEntity
{
	QString filename;
	QString path;
	QString filename_with_path;
	QString file_id;
	QDateTime created;

	UrlEntity()
		: filename()
		, path()
		, filename_with_path()
		, file_id()
		, created()
	{
	}

	UrlEntity(QString filename_, QString path_, QString filename_with_path_, QString file_id_, QDateTime created_)
		: filename(filename_)
		, path(path_)
		, filename_with_path(filename_with_path_)
		, file_id(file_id_)
		, created(created_)
	{
	}

	bool isEmpty()
	{
		return ((this->filename.isEmpty()) && (this->created.isNull()));
	}
};

struct CPPRESTSHARED_EXPORT StaticFile
{
	QString filename_with_path;
	QDateTime modified;
	QByteArray content;
	qint64 size;
};

class CPPRESTSHARED_EXPORT UrlManager
{
public:
	static void saveEverythingToFile();
	static void saveUrlToFile(QString id, UrlEntity in);
	static void restoreFromFile();
	static void addNewUrl(QString id, UrlEntity url_entity, bool save_to_file = true);
	static void removeUrl(const QString& id);
	static bool isInStorageAlready(const QString& filename_with_path);
	static UrlEntity getURLById(const QString& id);


public slots:
	static void removeExpiredUrls();

protected:
	UrlManager();	

private:
	static UrlManager& instance();
	QSharedPointer<QFile> backup_file_;
	QMutex mutex_;
	QMap<QString, UrlEntity> url_storage_;
};

#endif // URLMANAGER_H
