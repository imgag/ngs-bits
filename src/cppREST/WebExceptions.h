#ifndef WEBEXCEPTIONS_H
#define WEBEXCEPTIONS_H

#include "cppREST_global.h"
#include "Exceptions.h"

// Error 400
class CPPRESTSHARED_EXPORT BadRequest
		: public Exception
{
public:
	BadRequest(QString message, QString file, int line);
};

// Error 401
class CPPRESTSHARED_EXPORT Unauthorized
		: public Exception
{
public:
	Unauthorized(QString message, QString file, int line);
};

// Error 403
class CPPRESTSHARED_EXPORT Forbidden
		: public Exception
{
public:
	Forbidden(QString message, QString file, int line);
};

// Error 404
class CPPRESTSHARED_EXPORT NotFound
		: public Exception
{
public:
	NotFound(QString message, QString file, int line);
};

// Error 415
class CPPRESTSHARED_EXPORT UnsupportedMediaType
		: public Exception
{
public:
	UnsupportedMediaType(QString message, QString file, int line);
};

// Error 500
class CPPRESTSHARED_EXPORT InternalError
		: public Exception
{
public:
	InternalError(QString message, QString file, int line);
};

// Error 503
class CPPRESTSHARED_EXPORT ServiceUnavailable
		: public Exception
{
public:
	ServiceUnavailable(QString message, QString file, int line);
};

#endif // WEBEXCEPTIONS_H
