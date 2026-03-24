#ifndef ERRORHANDLER_CPP
#define ERRORHANDLER_CPP

#include "ErrorHandler.h"

ErrorHandler::ErrorHandler(QObject* parent)
	:QObject(parent)
{
}

ErrorHandler* ErrorHandler::instance()
{
	static ErrorHandler error_handler;
	return &error_handler;
}

void ErrorHandler::displayError(QString error)
{
	emit instance()->displayErrorReq(error);
}
#endif // ERRORHANDLER_CPP
