#include "TestFramework.h"
#include "ServerHelper.h"

TEST_CLASS(ServerHelper_Test)
{
private:
	TEST_METHOD(test_generateUniqueStr)
	{
		QString token = ServerHelper::generateUniqueStr();		
		I_EQUAL(token.length(), 36);
		I_EQUAL(token.count("-"), 4);
	}

	TEST_METHOD(test_getAppName)
	{
		QString app_name = ServerHelper::getAppName();
		S_EQUAL(app_name, "cppREST-TEST");
	}

	TEST_METHOD(test_strToInt)
	{
		int number = ServerHelper::strToInt("1");
		I_EQUAL(number, 1);

		IS_THROWN(ArgumentException, ServerHelper::strToInt("blabla"));
	}

	TEST_METHOD(test_canConvertToInt)
	{
		bool is_number = ServerHelper::canConvertToInt("1");
		IS_TRUE(is_number);

		is_number = ServerHelper::canConvertToInt("String");
		IS_FALSE(is_number);
	}
};
