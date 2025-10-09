#ifndef PIPELINESETTINGS_H
#define PIPELINESETTINGS_H

#include "cppNGS_global.h"
#include <QStringList>
#include <QMap>

///Parses megSAP pipeline settings file (used for GSvar server only)
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
	static QStringList queuesDefault();
	static QStringList queuesResearch();
	static QStringList queuesHighPriority();
	static QStringList queuesHighMemory();
	static QStringList queuesDragen();

private:
	PipelineSettings();
	static PipelineSettings& instance();
	static void checkInitialized();

	QString root_dir_;
	QString data_folder_;
	QMap<QString, QString>  projects_folder_;
	QStringList queues_default_;
	QStringList queues_research_;
	QStringList queues_high_priority_;
	QStringList queues_high_mem_;
	QStringList queues_dragen_;
};

#endif // PIPELINESETTINGS_H
