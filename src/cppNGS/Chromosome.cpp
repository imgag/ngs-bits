#include "Chromosome.h"
#include <QReadWriteLock>
#include <deque>

struct Chromosome::Pool
{
	//deque keeps records (and the strings used as map keys) at stable addresses.
	std::deque<Data> data;
	QHash<QByteArrayView, const Data*> common;
	QHash<QByteArrayView, const Data*> other;
	QHash<QByteArray, int> special_numbers;
	int next_num = 1004;
	QReadWriteLock mutex;

	Pool()
	{
		addCommon(QByteArray(""), QByteArray(""), 0);
		for (int num = 1; num <= 22; ++num)
		{
			QByteArray name = QByteArray::number(num);
			addCommon(name, name, num);
			addCommon("chr" + name, name, num);
		}
		for (const auto& entry : {QPair<QByteArray, int>{"X", 1001}, {"Y", 1002}, {"MT", 1003}, {"M", 1003}})
		{
			QByteArray normalized = entry.second == 1003 ? QByteArray("MT") : entry.first;
			addCommon(entry.first, normalized, entry.second);
			addCommon("chr" + entry.first, normalized, entry.second);
		}
	}

	void addCommon(const QByteArray& original, const QByteArray& normalized, int num)
	{
		data.push_back({original, normalized, num});
		common.emplace(original, &data.back());
	}
};

const Chromosome::Data* Chromosome::getDataPointer(QByteArrayView chr)
{
	chr = chr.trimmed();

	//construct pool of data (shared)
	static Pool* const pool = new Pool;

	//search in common chr names (immutable)
	const auto common = pool->common.constFind(chr);
	if (common != pool->common.cend()) return common.value();

	//search in other chr names (can be changed)
	{
		QReadLocker lock(&pool->mutex);
		const auto found = pool->other.constFind(chr);
		if (found != pool->other.cend()) return found.value();
	}

	//not in other => get write lock
	QWriteLocker lock(&pool->mutex);
	const auto found = pool->other.constFind(chr); //Another thread may have inserted this spelling while we changed locks.
	if (found != pool->other.cend()) return found.value();

	//insert new chr spelling
	QByteArray original = chr.toByteArray();
	QByteArray normalized = original.toUpper();
	if (normalized.startsWith("CHR")) normalized = normalized.mid(3);
	if (normalized == "M") normalized = "MT";

	int num = 0;
	if (normalized == "X") num = 1001;
	else if (normalized == "Y") num = 1002;
	else if (normalized == "MT") num = 1003;
	else if (!normalized.isEmpty())
	{
		bool ok = false;
		int value = normalized.toInt(&ok);
		if (!normalized.startsWith('0') && ok && value > 0 && value <= 1000)
		{
			num = value;
			//Preserve the former normalization cache's behavior for 1-22.
			if (num <= 22) normalized = QByteArray::number(num);
		}
		else
		{
			auto number = pool->special_numbers.constFind(normalized);
			if (number == pool->special_numbers.cend())
			{
				num = pool->next_num;
				pool->special_numbers.insert(normalized, num);
				++pool->next_num;
			}
			else num = number.value();
		}
	}

	pool->data.push_back({original, normalized, num});
	const Data* result = &pool->data.back();
	pool->other.emplace(QByteArrayView(result->original), result);
	return result;
}

Chromosome::Chromosome()
	: data_(getDataPointer(QByteArrayView()))
{
}

Chromosome::Chromosome(const QString& chr)
	: data_(getDataPointer(chr.toUtf8()))
{
}

Chromosome::Chromosome(const QByteArray& chr)
	: data_(getDataPointer(chr))
{
}

Chromosome::Chromosome(const char* chr)
	: data_(getDataPointer(QByteArrayView(chr)))
{
}

Chromosome::Chromosome(const std::string& chr)
	: Chromosome(chr.c_str())
{
}

Chromosome::Chromosome(QByteArrayView chr)
	: data_(getDataPointer(chr))
{
}
