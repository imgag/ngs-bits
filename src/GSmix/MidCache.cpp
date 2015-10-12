#include "MidCache.h"
#include "NGSD.h"
#include "Exceptions.h"
#include <QDebug>
#include <QSqlError>

const MidCache& MidCache::inst()
{
	static MidCache cache;
	return cache;
}

int MidCache::count() const
{
	return mids.count();
}

const Mid& MidCache::operator[](int index) const
{
	return mids[index];
}

const Mid& MidCache::midById(int id) const
{
	for (int i=0; i<count(); ++i)
	{
		if (mids[i].id==id)
		{
			return mids[i];
		}
	}

	THROW(ProgrammingException, "MidCache::midById received invalid id: " + QString::number(id));
}

MidCache::MidCache()
{
	//load MIDs from NGSD
	NGSD db;
	QSqlQuery q = db.getQuery();

	if (q.exec("SELECT id, name, number, sequence FROM mid"))
	{
		while(q.next())
		{
			int id = q.value(0).toInt();
			QString name = q.value(1).toString() + " " + q.value(2).toString();
			QString sequence = q.value(3).toString();

			mids.append(Mid{id, name.trimmed(), sequence.trimmed()});
		}
	}
	else
	{
		THROW(DatabaseException, q.lastError().text());
	}

	//sort MIDs by name
	std::sort(mids.begin(), mids.end(), [](const Mid& a, const Mid& b){ return a.name<b.name; } );
}

const QString Mid::toString() const
{
	return name + " (" + sequence + ")";
}
