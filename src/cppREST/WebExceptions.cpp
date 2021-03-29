#include "WebExceptions.h"

BadRequest::BadRequest(QString message, QString file, int line)
	: Exception(message, file, line)
{
}

Unauthorized::Unauthorized(QString message, QString file, int line)
	: Exception(message, file, line)
{
}

Forbidden::Forbidden(QString message, QString file, int line)
	: Exception(message, file, line)
{
}

NotFound::NotFound(QString message, QString file, int line)
	: Exception(message, file, line)
{
}

UnsupportedMediaType::UnsupportedMediaType(QString message, QString file, int line)
	: Exception(message, file, line)
{
}

InternalError::InternalError(QString message, QString file, int line)
	: Exception(message, file, line)
{
}

ServiceUnavailable::ServiceUnavailable(QString message, QString file, int line)
	: Exception(message, file, line)
{
}
