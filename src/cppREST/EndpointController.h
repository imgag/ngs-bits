#ifndef ENDPOINTCONTROLLER_H
#define ENDPOINTCONTROLLER_H

#include "cppREST_global.h"
#include "Log.h"
#include "HttpParts.h"
#include "EndpointManager.h"
#include "UrlManager.h"
#include "HttpResponse.h"
#include "SessionManager.h"
#include <QUrl>
#include <QDir>


class CPPRESTSHARED_EXPORT EndpointController
{
public:	


protected:
	EndpointController();

private:
	static EndpointController& instance();

};

#endif // ENDPOINTCONTROLLER_H
