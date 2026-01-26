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

void GeneSet::remove(const QByteArray& gene)
{
	QByteArray tmp = gene.trimmed().toUpper();
	removeAll(tmp);

	set_.remove(tmp);
}

void GeneSet::remove(const GeneSet& genes)
{
    for (const QByteArray& gene : genes)
	{
		remove(gene);
	}
}

void GeneSet::remove(const QByteArrayList& genes)
{
    for (const QByteArray& gene : genes)
	{
		remove(gene);
	}
}

bool GeneSet::containsAll(const GeneSet& genes) const
{
    for (const QByteArray& gene : genes)
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
    for (const QByteArray& gene : tmp)
	{
		output.insert(gene);
	}

	return output;
}

void GeneSet::store(QString filename) const
{
	auto handle = Helper::openFileForWriting(filename);
    for (const QByteArray& gene : *this)
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

GeneSet GeneSet::createFromText(const QByteArray& text, char separator)
{
	GeneSet output;

	QList<QByteArray> lines = text.split(separator);
    for (const QByteArray& line : lines)
	{
		if (line.startsWith("#")) continue;
		output.insert(line);
	}

	return output;
}

GeneSet GeneSet::createFromStringList(const QStringList& list)
{
	GeneSet output;

    for (const QString& line : list)
	{
		output.insert(line.toUtf8());
	}

	return output;
}

QByteArrayList GeneSet::toByteArrayList() const
{
	QByteArrayList output;

    for (const QByteArray& gene : *this)
	{
		output.append(gene);
	}

	return output;
}

QStringList GeneSet::toStringList() const
{
	QStringList output;

	for (const QByteArray& gene : *this)
	{
		output.append(gene);
	}

	return output;
}

QByteArray GeneSet::toString(QByteArray sep) const
{
	QByteArray output;

	for (int i=0; i<this->count(); ++i)
	{
		if (i!=0) output.append(sep);
		output.append(this->operator[](i));
	}

	return output;
}

