#include "EndpointController.h"


EndpointController::EndpointController()
{
}

EndpointController& EndpointController::instance()
{
	static EndpointController endpoint_controller;
	return endpoint_controller;
}

