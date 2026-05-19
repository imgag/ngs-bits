#include "Chromosome.h"
#include <QHash>
#include <QMutex>

const QHash<QByteArray, int> Chromosome::str2num_cache_ = {
	{"", 0},
	//our chromosome names
	{"chr1", 1},
	{"chr2", 2},
	{"chr3", 3},
	{"chr4", 4},
	{"chr5", 5},
	{"chr6", 6},
	{"chr7", 7},
	{"chr8", 8},
	{"chr9", 9},
	{"chr10", 10},
	{"chr11", 11},
	{"chr12", 12},
	{"chr13", 13},
	{"chr14", 14},
	{"chr15", 15},
	{"chr16", 16},
	{"chr17", 17},
	{"chr18", 18},
	{"chr19", 19},
	{"chr20", 20},
	{"chr21", 21},
	{"chr22", 22},
	{"chrX", 1001},
	{"chrY", 1002},
	{"chrMT", 1003},
	//Ensembl chromosome names
	{"1", 1},
	{"2", 2},
	{"3", 3},
	{"4", 4},
	{"5", 5},
	{"6", 6},
	{"7", 7},
	{"8", 8},
	{"9", 9},
	{"10", 10},
	{"11", 11},
	{"12", 12},
	{"13", 13},
	{"14", 14},
	{"15", 15},
	{"16", 16},
	{"17", 17},
	{"18", 18},
	{"19", 19},
	{"20", 20},
	{"21", 21},
	{"22", 22},
	{"X", 1001},
	{"Y", 1002},
	{"MT", 1003}
};

const QHash<int, QByteArray> Chromosome::num2str_cache_ = {
	{0, ""},
	{1, "1"},
	{2, "2"},
	{3, "3"},
	{4, "4"},
	{5, "5"},
	{6, "6"},
	{7, "7"},
	{8, "8"},
	{9, "9"},
	{10, "10"},
	{11, "11"},
	{12, "12"},
	{13, "13"},
	{14, "14"},
	{15, "15"},
	{16, "16"},
	{17, "17"},
	{18, "18"},
	{19, "19"},
	{20, "20"},
	{21, "21"},
	{22, "22"},
	{1001, "X"},
	{1002, "Y"},
	{1003,"MT"}
};

Chromosome::Chromosome()
	: str_("")
	, num_(0)
{
}

Chromosome::Chromosome(const QString& chr)
	: str_(chr.trimmed().toUtf8())
	, num_(0)
{
	num_ = numericRepresentation();
}

Chromosome::Chromosome(const QByteArray& chr)
	: str_(chr.trimmed())
	, num_(0)
{
	num_ = numericRepresentation();
}

Chromosome::Chromosome(const char* chr)
	: str_(chr)
	, num_(0)
{
	str_ = str_.trimmed();
	num_ = numericRepresentation();
}

Chromosome::Chromosome(const std::string& chr)
	: str_(chr.c_str())
	, num_(0)
{
	str_ = str_.trimmed();
	num_ = numericRepresentation();
}

Chromosome::Chromosome(QByteArrayView chr)
	: str_(chr.trimmed().toByteArray())
	, num_(0)
{
	num_ = numericRepresentation();
}

QByteArray Chromosome::normalizedStringRepresentation() const
{
	QByteArray tmp = str_.toUpper();
	if (tmp.startsWith("CHR")) tmp = tmp.mid(3);
	if (tmp=="M") tmp = "MT";
	return tmp;
}

int Chromosome::numericRepresentation() const
{
	//speed up for default representations
	if (str2num_cache_.contains(str_)) return str2num_cache_[str_];

	//normalize
	QByteArray tmp = normalizedStringRepresentation();

	//handle special cases empty, X,Y and M/MT
	if (tmp=="")
	{
		return 0;
	}
	if (tmp=="X")
	{
		return 1001;
	}
	if (tmp=="Y")
	{
		return 1002;
	}
	if (tmp=="MT")
	{
		return 1003;
	}

	//calculate numeric representation
	if (!tmp.startsWith('0'))
	{
		bool ok = false;
		uint value = tmp.toUInt(&ok);
		if (ok && value>0 && value<=1000)
		{
			return value;
		}
	}

	//other non-numeric chromosome
	static int next_num = 1004;
	static QHash<QByteArray, int> cache;
	static QMutex mutex; //mutex to protect static members when accessed in parallel from several threads
	QMutexLocker locker(&mutex);
	if (!cache.contains(tmp))
	{
		cache[tmp] = next_num;
		++next_num;
	}

	return cache[tmp];
}
