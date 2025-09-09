#include "TestFramework.h"
#include "Settings.h"
#include <QCoreApplication>

#include <QStandardPaths>

TEST_CLASS(Settings_Test)
{
private:

    TEST_METHOD(integer)
	{
		QString key = "unit_test_integer";

		int random = Helper::randomNumber(1,1000);
		Settings::setInteger(key, random);
		I_EQUAL(Settings::integer(key), random)

		IS_TRUE(Settings::contains(key));
		Settings::remove(key);
		IS_FALSE(Settings::contains(key));
	}

	TEST_METHOD(string)
	{
		QString key = "unit_test_string";

		QString random = QString("str_") + QString::number(Helper::randomNumber(1,1000));
		Settings::setString(key, random);
		S_EQUAL(Settings::string(key), random)

		IS_TRUE(Settings::contains(key));
		Settings::remove(key);
		IS_FALSE(Settings::contains(key));
	}

	TEST_METHOD(stringList)
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

	TEST_METHOD(map)
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

	TEST_METHOD(boolean)
	{
		QString key = "unit_test_boolean";

		Settings::setBoolean(key, 1);
		I_EQUAL(Settings::boolean(key), 1)

		IS_TRUE(Settings::contains(key));
		Settings::remove(key);
		IS_FALSE(Settings::contains(key));
	}

	TEST_METHOD(path)
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

	TEST_METHOD(allKeys_clear)
	{
		Settings::setBoolean("unit_test_boolean", 1);
		QStringList keys = Settings::allKeys();
		IS_TRUE(keys.contains("unit_test_boolean"));

		Settings::clear();
		IS_TRUE(Settings::allKeys().count() <= keys.count()); //not 0 because clear() only deletes user-specific settings
	}

	TEST_METHOD(setSettingsOverride)
	{
		Settings::setSettingsOverride(TESTDATA("data_in/settings_override.ini"));

		//files
		QStringList files = Settings::files();
		I_EQUAL(files.count(), 1);
		IS_TRUE(files[0].endsWith("settings_override.ini"));

		//contains
		IS_FALSE(Settings::contains("liftover_hg19_hg38"));
		IS_FALSE(Settings::contains("liftover_hg38_hg19"));
		IS_TRUE(Settings::contains("reference_genome"));

		//allKeys
		QStringList allkeys = Settings::allKeys();
		IS_TRUE(allkeys.count()==1);
		IS_TRUE(allkeys.contains("reference_genome"));

		//getValue
		S_EQUAL(Settings::string("reference_genome"), "GRCh38_override.fa");
	}

};

