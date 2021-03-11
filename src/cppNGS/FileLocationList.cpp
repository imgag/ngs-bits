#include "FileLocationList.h"

FileLocationList::FileLocationList()
{
}

FileLocationList::~FileLocationList()
{
}

QStringList FileLocationList::asStringList()
{
	QStringList output {};
	for (int i = 0; i < this->count(); i++)
	{
		output.append(this->value(i).filename);
	}
	return output;
}
