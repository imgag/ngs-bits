#ifndef MIDCACHE_H
#define MIDCACHE_H

#include <QString>
#include <QList>

struct Mid
{
	int id;
	QString name;
	QString sequence;

	const QString toString() const;
};

///Singleton MID cache
class MidCache
{
public:
	static const MidCache& inst();

	int count() const;
	const Mid& operator[](int index) const;
	const Mid& midById(int id) const;

private:
	MidCache();
	MidCache(const MidCache& rhs) = delete;

	QList<Mid> mids;
};

#endif // MIDCACHE_H
