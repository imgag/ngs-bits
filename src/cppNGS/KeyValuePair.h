#ifndef KEYVALUEPAIR_H
#define KEYVALUEPAIR_H

#include <QString>

struct KeyValuePair
{
	KeyValuePair(const QString& k, const QString& v)
		: key(k)
		, value(v)
	{
	}

	QString key;
	QString value;
};

#endif // KEYVALUEPAIR_H
