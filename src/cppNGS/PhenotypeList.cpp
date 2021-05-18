#include "PhenotypeList.h"
#include <QDebug>

PhenotypeList::PhenotypeList()
	: QList<Phenotype>()
	, accessions_()
{
}

PhenotypeList& PhenotypeList::operator<<(const Phenotype& pheno)
{
	append(pheno);

	accessions_ << pheno.accession();

    return *this;
}

PhenotypeList& PhenotypeList::operator<<(const QSet<Phenotype>& set)
{
	foreach(const Phenotype& pheno, set)
	{
		*this << pheno;
	}
	return *this;
}

PhenotypeList& PhenotypeList::operator<<(const PhenotypeList& list)
{
	foreach(const Phenotype& pheno, list)
	{
		*this << pheno;
	}

	return *this;
}

void PhenotypeList::removeAt(int i)
{
	QList<Phenotype>::removeAt(i);

	//removed element > update accession list (the same element can be in the list twice, thus we cannot remove the accession from accessions_)
	accessions_.clear();
	foreach(const Phenotype& pheno, *this)
	{
		accessions_ << pheno.accession();
	}
}

void PhenotypeList::clear()
{
	QList<Phenotype>::clear();
	accessions_.clear();
}

void PhenotypeList::sortByName()
{
	std::sort(begin(), end(), [](const Phenotype& a, const Phenotype& b){ return a.name()<b.name(); });
}

void PhenotypeList::sortByAccession()
{
	std::sort(begin(), end(), [](const Phenotype& a, const Phenotype& b){ return a.accession()<b.accession(); });
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
