#ifndef LOG_H
#define LOG_H

#include "cppCORE_global.h"
#include <QString>
#include <QTime>


/**
	@brief Singleton logging handler for command line applications.

	@note The DEBUG level is missing by purpose: Debugging output should be done with QDebug (it can be disabled for
*/
class CPPCORESHARED_EXPORT Log
{
public:
	enum LogLevel
	{
		PERFORMANCE = 1,
		INFO = 2,
		WARNING = 4,
		ERROR = 8
	};

	///Logs message using ERROR level.
	static void error(const QString& message);
	///Logs message using WARNING level.
	static void warn(const QString& message);
	///Logs message using INFO level.
	static void info(const QString& message);
	///Logs message using PERF level (performance data).
	static void perf(const QString& message);
	///Convenice: Logs a performance message appending the human-readable elapsed time.
	static void perf(const QString& message, QTime elapsed);
	///Convenice: Logs basic application info (path, version, ...) using INFO level.
	static void appInfo();

	/// Activates/deactivates the command line logging. Default is enabled.
	static void setCMDEnabled(bool enabled);
	/// Activates/deactivates file logging. Default is disabled.
	static void setFileEnabled(bool enabled);
	///Sets the log file. Default is application name plus '.log' extention.
	static void setFileName(QString filename);
	///Returns the used log file.
	static QString fileName();
	///Enables/disables log levels (bit-wise OR of levels). Default is all levels enabled.
	static void enableLogLevels(int level);

protected:
	///Default constructor not public
	Log();
	///Copy constructor "declared away"
	Log(const Log&);
	///Returns the logger singleton instance.
	static Log& inst();
	///Internal message handler that does the actual logging.
	void logMessage(LogLevel level, const QString& message);
	///Returns the log level string.
	QString levelString(LogLevel level);

	///Command line logging
	bool log_cmd_;
	///File logging
	bool log_file_;
	///Log file name
	QString log_file_name_;
	///Status of log levels
	int enabled_;
};

#endif // LOG_H
