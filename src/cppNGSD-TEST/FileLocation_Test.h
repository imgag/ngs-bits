#include "TestFramework.h"
#include "FileLocation.h"

TEST_CLASS(FileLocation_Test)
{
Q_OBJECT
private slots:
	void typeToString_and_stringToType()
	{
		for (int i=0; i<static_cast<int>(PathType::OTHER); ++i)
		{
			PathType type = static_cast<PathType>(i);
			QString type_string = FileLocation::typeToString(type);
			PathType type2 = FileLocation::stringToType(type_string);
			IS_TRUE(type==type2);
		}
	}

	void typeToHumanReadableString()
	{
		for (int i=0; i<static_cast<int>(PathType::OTHER); ++i)
		{
			PathType type = static_cast<PathType>(i);
			QString type_string = FileLocation::typeToHumanReadableString(type);
			IS_TRUE(!type_string.isEmpty())
		}
	}
};
