#include "TestFramework.h"
#include "SessionManager.h"
#include "SessionManager.cpp"

TEST_CLASS(SessionManager_Test)
{
Q_OBJECT
private slots:
	void test_session_management()
	{
		QString username = "username";
		QString token = "secure_token";
		QDateTime login_time = QDateTime::currentDateTime();
		SessionManager::addNewSession(token, username, login_time);

		Session session = SessionManager::getSessionByUserId(username);
		S_EQUAL(session.user_id, username);
		I_EQUAL(session.login_time.toSecsSinceEpoch(), login_time.toSecsSinceEpoch());

		session = SessionManager::getSessionBySecureToken(token);
		S_EQUAL(session.user_id, username);
		I_EQUAL(session.login_time.toSecsSinceEpoch(), login_time.toSecsSinceEpoch());

		SessionManager::removeSession(token);
		IS_THROWN(ArgumentException, SessionManager::removeSession(token));
	}
};
