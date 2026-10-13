#include "Chromosome.h"
#include <deque>
#include <mutex>
#include <shared_mutex>
#include <unordered_map>

struct Chromosome::Pool
{
	struct Hash
	{
		size_t operator()(QByteArrayView str) const noexcept { return ::qHash(str); }
	};
	using Records = std::unordered_map<QByteArrayView, const Data*, Hash>;

	//deque keeps records (and the strings used as map keys) at stable addresses.
	std::deque<Data> data;
	Records common;
	Records other;
	QHash<QByteArray, int> special_numbers;
	int next_num = 1004;
	std::shared_mutex mutex;

	Pool()
	{
		common.reserve(55);
		addCommon(QByteArray(""), QByteArray(""), 0);
		for (int num = 1; num <= 22; ++num)
		{
			const QByteArray name = QByteArray::number(num);
			addCommon(name, name, num);
			addCommon("chr" + name, name, num);
		}
		for (const auto& entry : {std::pair<const char*, int>{"X", 1001}, {"Y", 1002}, {"MT", 1003}, {"M", 1003}})
		{
			const QByteArray name(entry.first);
			const QByteArray normalized = entry.second == 1003 ? QByteArray("MT") : name;
			addCommon(name, normalized, entry.second);
			addCommon("chr" + name, normalized, entry.second);
		}
	}

	void addCommon(const QByteArray& original, const QByteArray& normalized, int num)
	{
		data.push_back({original, normalized, num});
		common.emplace(QByteArrayView(data.back().original), &data.back());
	}
};

const Chromosome::Data* Chromosome::getDataPointer(QByteArrayView chr)
{
	chr = chr.trimmed();
	//Keep records alive even during static destruction in other translation units.
	//Initialization is thread-safe; the common map is immutable after publication.
	static Pool* const pool = new Pool;
	thread_local const Data* last = nullptr;
	if (last != nullptr && QByteArrayView(last->original) == chr) return last;

	const Pool::Records& common_records = pool->common;
	const auto common = common_records.find(chr);
	if (common != common_records.end()) return last = common->second;

	{
		std::shared_lock<std::shared_mutex> lock(pool->mutex);
		const auto found = pool->other.find(chr);
		if (found != pool->other.end()) return last = found->second;
	}

	std::unique_lock<std::shared_mutex> lock(pool->mutex);
	//Another thread may have inserted this spelling while we changed locks.
	const auto found = pool->other.find(chr);
	if (found != pool->other.end()) return last = found->second;

	//Copy the input: callers may supply temporary strings or non-owning views.
	const QByteArray original = chr.toByteArray();
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
		const uint value = normalized.toUInt(&ok);
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
	return last = result;
}

Chromosome::Chromosome()
	: data_(getDataPointer(QByteArrayView()))
{
}

Chromosome::Chromosome(const QString& chr)
	: data_(getDataPointer(chr.trimmed().toUtf8()))
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
