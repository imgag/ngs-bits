#include "TestFramework.h"
#include "SessionManager.h"
#include "EndpointHandler.h"
#include "EndpointHandler.cpp"

TEST_CLASS(EndpointHandler_Test)
{
Q_OBJECT
private slots:
    void test_isEligibileToAccess()
    {
			HttpRequest request {};
			request.setMethod(RequestMethod::GET);
			request.setContentType(ContentType::TEXT_PLAIN);
            QString token = ServerHelper::generateUniqueStr();
            SessionManager::addNewSession(token, Session{"test_user", QDateTime::currentDateTime()});
			request.addUrlParam("token", token);

			IS_TRUE(EndpointHelper::isEligibileToAccess(request));
            I_EQUAL(token.length(), 36);
            I_EQUAL(token.count("-"), 4);
    }
};
