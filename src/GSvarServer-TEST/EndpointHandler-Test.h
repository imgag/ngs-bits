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
            Request request {};
            request.method = Request::MethodType::GET;
            request.return_type = ContentType::TEXT_PLAIN;
            QString token = ServerHelper::generateUniqueStr();
            SessionManager::addNewSession(token, Session{"test_user", QDateTime::currentDateTime()});
            request.url_params.insert("token", token);

            IS_TRUE(EndpointHandler::isEligibileToAccess(request));
            I_EQUAL(token.length(), 36);
            I_EQUAL(token.count("-"), 4);
    }
};
