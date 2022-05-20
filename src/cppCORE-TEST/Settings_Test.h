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

	void stringList()
	{
		QString key = "unit_test_stringlist";

		QStringList random;
		random << "str_" + QString::number(Helper::randomNumber(1,1000));
		random << "str_" + QString::number(Helper::randomNumber(1,1000));
		random << "str_" + QString::number(Helper::randomNumber(1,1000));
		Settings::setStringList(key, random);
		I_EQUAL(Settings::stringList(key).count(), 3)
		S_EQUAL(Settings::stringList(key)[0], random[0])
		S_EQUAL(Settings::stringList(key)[1], random[1])
		S_EQUAL(Settings::stringList(key)[2], random[2])

		IS_TRUE(Settings::contains(key));
		Settings::remove(key);
		IS_FALSE(Settings::contains(key));
	}

	void map()
	{
		QString key = "unit_test_map";

		QMap<QString, QVariant> random;
		random.insert("0", QVariant((int)Helper::randomNumber(1,100)));
		random.insert("1", QVariant(Helper::randomNumber(1,100)));
		random.insert("2", QVariant("str_" + QString::number(Helper::randomNumber(1,1000))));
		Settings::setMap(key, random);
		I_EQUAL(Settings::map(key).count(), 3)
		IS_TRUE(Settings::map(key)["0"]==random["0"])
		IS_TRUE(Settings::map(key)["1"]==random["1"])
		IS_TRUE(Settings::map(key)["2"]==random["2"])

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
		QString path = Helper::canonicalPath(QCoreApplication::applicationDirPath());
		if (!path.endsWith(QDir::separator())) path += QDir::separator();
		Settings::setPath(key, path);
		S_EQUAL(Settings::path(key), path)

		IS_TRUE(Settings::contains(key));
		Settings::remove(key);
		IS_FALSE(Settings::contains(key));
	}

	void allKeys_clear()
	{
		Settings::setBoolean("unit_test_boolean", 1);
		QStringList keys = Settings::allKeys();
		IS_TRUE(keys.contains("unit_test_boolean"));

		Settings::clear();
		IS_TRUE(Settings::allKeys().count() <= keys.count()); //not 0 because clear() only deletes user-specific settings
	}
};

