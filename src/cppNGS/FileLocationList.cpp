#include "FileLocationList.h"

FileLocationList::FileLocationList()
{
}

FileLocationList FileLocationList::filterById(const QString& id) const
{
	FileLocationList output;

	foreach(const FileLocation& loc, *this)
	{
		if (loc.id!=id) continue;

		output << loc;
	}

	return output;
}

QStringList FileLocationList::asStringList() const
{
	QStringList output;
	for (int i = 0; i < count(); i++)
	{
		output.append(operator[](i).filename);
	}
	return output;
}
