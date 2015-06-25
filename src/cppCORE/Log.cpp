#include "Log.h"
#include "Exceptions.h"
#include "Helper.h"
#include <QTextStream>
#include <QCoreApplication>
#include <QFile>


Log::Log()
	: log_cmd_(true)
	, log_file_(false)
	, log_file_name_(QCoreApplication::applicationFilePath().replace(".exe", "") + ".log")
	, enabled_(PERFORMANCE|INFO|WARNING|ERROR)
{
}

Log& Log::inst()
{
	static Log logger;
	return logger;
}

void Log::setCMDEnabled(bool enabled)
{
	inst().log_cmd_ = enabled;
}

void Log::setFileEnabled(bool enabled)
{
	inst().log_file_ = enabled;
}

void Log::setFileName(QString filename)
{
	inst().log_file_name_ = filename;
}

QString Log::fileName()
{
	return inst().log_file_name_;
}

void Log::enableLogLevels(int enabled)
{
	inst().enabled_ = enabled;
}

void Log::error(const QString& message)
{
	inst().logMessage(ERROR, message);
}

void Log::warn(const QString& message)
{
	inst().logMessage(WARNING, message);
}

void Log::info(const QString& message)
{
	inst().logMessage(INFO, message);
}

void Log::perf(const QString& message)
{
	inst().logMessage(PERFORMANCE, message);
}

void Log::perf(const QString& message, QTime elapsed)
{
	inst().logMessage(PERFORMANCE, message.trimmed() + " " + Helper::elapsedTime(elapsed));
}

void Log::appInfo()
{
	inst().logMessage(INFO, "Application path: " + QCoreApplication::applicationFilePath());
	inst().logMessage(INFO, "Application version: " + QCoreApplication::applicationVersion());
	inst().logMessage(INFO, "User: " + Helper::userName());
}

void Log::logMessage(LogLevel level, const QString& message)
{
	if (!(enabled_&level)) return;
	QString level_str = levelString(level);

	//CMD
	if (log_cmd_)
	{
		if (level==ERROR || level==WARNING)
		{
			QTextStream stream(stderr);
			stream << level_str << ": " << message << endl;
		}
		else
		{
			QTextStream stream(stdout);
			stream << level_str << ": " << message << endl;
		}
	}
	//FILE
	if (log_file_)
	{

		QString timestamp = Helper::dateTime("yyyy-MM-ddThh:mm:ss.zzz");
		QString name = QCoreApplication::applicationName().replace(".exe", "");
		QString message_sanitized = QString(message).replace("\t", " ").replace("\n", "");

		QScopedPointer<QFile> out_file(Helper::openFileForWriting(log_file_name_, false, true));
		out_file->write((timestamp + "\t" + name + "\t" + level_str + "\t" + message_sanitized).toLatin1() + "\n");
		out_file->flush();
	}
}

QString Log::levelString(Log::LogLevel level)
{
	switch(level)
	{
		case PERFORMANCE:
			return "PERFORMANCE";
			break;
		case INFO:
			return "INFO";
			break;
		case WARNING:
			return "WARNING";
			break;
		case ERROR:
			return "ERROR";
			break;
	}

	THROW(ProgrammingException, "Unknown log level '" + QString::number(level) + "'!");
}
