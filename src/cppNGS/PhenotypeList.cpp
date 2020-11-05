#include "PhenotypeList.h"

PhenotypeList::PhenotypeList()
{
}

PhenotypeList& PhenotypeList::operator<<(const Phenotype& pheno)
{
	append(pheno);
}

PhenotypeList& PhenotypeList::operator<<(const QSet<Phenotype>& set)
{
	foreach(const Phenotype& pheno, set)
	{
		*this << pheno;
	}
	return *this;
}

void PhenotypeList::sortByName()
{
	std::sort(begin(), end(), [](const Phenotype& a, const Phenotype& b){ return a.name()<b.name(); });
}

QString PhenotypeList::toString(QString seperator) const
{
	return toStringList().join(seperator);
}

QStringList PhenotypeList::toStringList() const
{
	QStringList output;

	foreach(const Phenotype& phenotype, *this)
	{
		output << phenotype.name();
	}

	return output;
}
