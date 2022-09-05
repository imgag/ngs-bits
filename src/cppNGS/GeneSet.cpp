#include "GeneSet.h"
#include "Helper.h"

GeneSet::GeneSet()
	: QList<QByteArray>()
{
}

void GeneSet::insert(const QByteArray& gene)
{
	QByteArray tmp = gene.trimmed().toUpper();
	if (tmp.isEmpty() || set_.contains(tmp)) return;

	auto it = std::lower_bound(begin(), end(), tmp);
	QList<QByteArray>::insert(it, tmp);

	set_.insert(tmp);
}

bool GeneSet::containsAll(const GeneSet& genes) const
{
	foreach(const QByteArray& gene, genes)
	{
		if (!contains(gene)) return false;
	}

	return true;
}

GeneSet GeneSet::intersect(const GeneSet& genes) const
{
	GeneSet output;

	QSet<QByteArray> tmp = set_;
	tmp.intersect(genes.set_);
	foreach(const QByteArray& gene, tmp)
	{
		output.insert(gene);
	}

	return output;
}

void GeneSet::store(QString filename) const
{
	auto handle = Helper::openFileForWriting(filename);
	foreach(const QByteArray& gene, *this)
	{
		handle->write(gene + "\n");
	}
}

GeneSet GeneSet::createFromFile(QString filename)
{
	GeneSet output;

	auto handle = Helper::openFileForReading(filename, true);
	while (!handle->atEnd())
	{
		QByteArray line = handle->readLine();
		if (line.startsWith("#")) continue;
		output.insert(line);
	}

	return output;
}

GeneSet GeneSet::createFromText(const QByteArray& text, char seperator)
{
	GeneSet output;

	QList<QByteArray> lines = text.split(seperator);
	foreach(const QByteArray& line, lines)
	{
		if (line.startsWith("#")) continue;
		output.insert(line);
	}

	return output;
}

GeneSet GeneSet::createFromStringList(const QStringList& list)
{
	GeneSet output;

	foreach(const QString& line, list)
	{
		output.insert(line.toUtf8());
	}

	return output;
}

QStringList GeneSet::toStringList() const
{
	QStringList output;

	foreach(const QByteArray& gene, *this)
	{
		output.append(gene);
	}

	return output;
}

