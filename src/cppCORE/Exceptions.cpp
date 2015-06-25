#include "Exceptions.h"

Exception::Exception(QString message, QString file, int line)
	: message_(message)
	, file_(file)
	, line_(line)
{
}

QString Exception::message() const
{
	return message_;
}

QString Exception::file() const
{
	return file_;
}

int Exception::line() const
{
	return line_;
}

ArgumentException::ArgumentException(QString message, QString file, int line)
	: Exception(message, file, line)
{
}

FileAccessException::FileAccessException(QString message, QString file, int line)
	: Exception(message, file, line)
{
}

FileParseException::FileParseException(QString message, QString file, int line)
	: Exception(message, file, line)
{
}

ToolFailedException::ToolFailedException(QString message, QString file, int line)
	: Exception(message, file, line)
{
}
CommandLineParsingException::CommandLineParsingException(QString message, QString file, int line)
	: Exception(message, file, line)
{
}

ProgrammingException::ProgrammingException(QString message, QString file, int line)
	: Exception(message, file, line)
{
}

DatabaseException::DatabaseException(QString message, QString file, int line)
	: Exception(message, file, line)
{
}

TypeConversionException::TypeConversionException(QString message, QString file, int line)
	: Exception(message, file, line)
{
}

StatisticsException::StatisticsException(QString message, QString file, int line)
	: Exception(message, file, line)
{
}
