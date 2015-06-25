#ifndef EXCEPTIONS_H
#define EXCEPTIONS_H

#include "cppCORE_global.h"
#include <QString>
#include <QDebug>

///Base exception. Rather use derived exception if possible.
class CPPCORESHARED_EXPORT Exception
{
public:
	Exception(QString message, QString file, int line);
	QString message() const;
	QString file() const;
	int line() const;

protected:
	QString message_;
	QString file_;
	int line_;
};

///Exception that is thrown when a function/method receives invalid arguments.
class CPPCORESHARED_EXPORT ArgumentException
		: public Exception
{
public:
	ArgumentException(QString message, QString file, int line);
};

///Exception that is thrown when files cannot be opened/written.
class CPPCORESHARED_EXPORT FileAccessException
		: public Exception
{
public:
	FileAccessException(QString message, QString file, int line);
};

///Exception that is thrown when file content cannot be parsed.
class CPPCORESHARED_EXPORT FileParseException
		: public Exception
{
public:
	FileParseException(QString message, QString file, int line);
};

///Exception for command line parsing errors of tools.
class CPPCORESHARED_EXPORT CommandLineParsingException
		: public Exception
{
public:
	CommandLineParsingException(QString message, QString file, int line);
};

///Exception that is thrown to indicate that the execution of a tool failed. It sets the error code to '1' and prints the message.
class CPPCORESHARED_EXPORT ToolFailedException
		: public Exception
{
public:
	ToolFailedException(QString message, QString file, int line);
};

///Exception for programming errors (forgotten switch cases, null pointers, etc).
class CPPCORESHARED_EXPORT ProgrammingException
		: public Exception
{
public:
	ProgrammingException(QString message, QString file, int line);
};

///Exception for database errors.
class CPPCORESHARED_EXPORT DatabaseException
		: public Exception
{
public:
	DatabaseException(QString message, QString file, int line);
};

///Exception for type conversion errors.
class CPPCORESHARED_EXPORT TypeConversionException
		: public Exception
{
public:
	TypeConversionException(QString message, QString file, int line);
};

///Exception for statistics errors.
class CPPCORESHARED_EXPORT StatisticsException
		: public Exception
{
public:
	StatisticsException(QString message, QString file, int line);
};

#define THROW(name, message) \
throw name(message, __FILE__, __LINE__);

#endif // EXCEPTIONS_H
