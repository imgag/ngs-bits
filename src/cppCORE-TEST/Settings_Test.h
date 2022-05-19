#include "TestFramework.h"
#include "Settings.h"
#include <QCoreApplication>

#include <QStandardPaths>

TEST_CLASS(Settings_Test)
{
Q_OBJECT
private slots:

	void integer()
	{
		QString key = "unit_test_integer";

		int random = Helper::randomNumber(1,1000);
		Settings::setInteger(key, random);
		I_EQUAL(Settings::integer(key), random)

		IS_TRUE(Settings::contains(key));
		Settings::remove(key);
		IS_FALSE(Settings::contains(key));
	}

	void string()
	{
		QString key = "unit_test_string";

		QString random = QString("str_") + QString::number(Helper::randomNumber(1,1000));
		Settings::setString(key, random);
		S_EQUAL(Settings::string(key), random)

		IS_TRUE(Settings::contains(key));
		Settings::remove(key);
		IS_FALSE(Settings::contains(key));
	}

	void boolean()
	{
		QString key = "unit_test_boolean";

		Settings::setBoolean(key, 1);
		I_EQUAL(Settings::boolean(key), 1)

		IS_TRUE(Settings::contains(key));
		Settings::remove(key);
		IS_FALSE(Settings::contains(key));
	}

	void path()
	{
		QString key = "unit_test_path";
		QString path = QCoreApplication::applicationDirPath();
		Settings::setPath(key, path);
		S_EQUAL(Settings::path(key), path)

		IS_TRUE(Settings::contains(key));
		Settings::remove(key);
		IS_FALSE(Settings::contains(key));
	}

	void allKeys_clear()
	{
		QStringList keys = Settings::allKeys();

		Settings::clear();
		IS_TRUE(Settings::allKeys().count() < keys.count()-1); //not 0 because clear() only deletes user-specific settings
	}
};


/*
	public:

	static QStringList stringList(QString key, bool optional=false);
	static void setStringList(QString key, QStringList value);

	static QMap<QString,QVariant> map(QString key, bool optional=false);
	static void setMap(QString key, QMap<QString,QVariant> value);s
*/
