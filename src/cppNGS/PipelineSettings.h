#ifndef PIPELINESETTINGS_H
#define PIPELINESETTINGS_H

#include <QVariant>
#include "cppNGS_global.h"

///Parses megSAP pipeline settings file and
class CPPNGSSHARED_EXPORT PipelineSettings
{
public:
	///Load settings from the given megSAP INI file. Throws an exception if the file could not be opened.
	static void loadSettings(QString ini_file);
	///Returns if the settings were initialized.
	static bool isInitialized();

	/* getters for data */
	static QString rootDir();
	static QString projectFolder(QString type);
	static QString dataFolder();
	static QString queueEmail();
	static QStringList queuesDefault();
	static QStringList queuesResearch();
	static QStringList queuesHighPriority();
	static QStringList queuesHighMemory();

private:
	PipelineSettings();
	static PipelineSettings& instance();
	static void checkInitialized();

	QString root_dir_;
	QString data_folder_;
	QMap<QString, QString>  projects_folder_;
	QString queue_email_;
	QStringList queues_default_;
	QStringList queues_research_;
	QStringList queues_high_priority_;
	QStringList queues_high_mem_;
};

#endif // PIPELINESETTINGS_H
