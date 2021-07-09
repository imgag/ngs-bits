#ifndef PHENOTYPELIST_H
#define PHENOTYPELIST_H

#include "cppNGS_global.h"
#include "Phenotype.h"
#include <QList>
#include <QSet>

/// A list of phenotypes
class CPPNGSSHARED_EXPORT PhenotypeList
	: protected QList<Phenotype>
{
public:
	//Default constructor.
	PhenotypeList();

	using QList<Phenotype>::count;
	using QList<Phenotype>::isEmpty;
	using QList<Phenotype>::const_iterator;
	using QList<Phenotype>::cbegin;
	using QList<Phenotype>::cend;
	using QList<Phenotype>::begin;
	using QList<Phenotype>::end;

	//Adds a phenotype.
	PhenotypeList& operator<<(const Phenotype& pheno);
	//Adds a phenotype list.
	PhenotypeList& operator<<(const QSet<Phenotype>& set);
	//Adds a phenotype list.
	PhenotypeList& operator<<(const PhenotypeList& list);

	//Equality operator
	bool operator==(const PhenotypeList& rhs) const
	{
		return rhs.accessions_==accessions_;
	}
	//Inequality operator
	bool operator!=(const PhenotypeList& rhs) const
	{
		return rhs.accessions_!=accessions_;
	}

	//Const access to phenotype
	const Phenotype& operator[](int i) const
	{
		return QList<Phenotype>::operator[](i);
	}

	//Checks if the given phenotype accession is in the list.
	bool containsAccession(const QByteArray& accession)
	{
		return accessions_.contains(accession);
	}

	//Removes the i-th element.
	void removeAt(int i);
	//Remove content.
	void clear();

	//Sort elements by name.
	void sortByName();
	//Sort elements by accession.
	void sortByAccession();

	//Converts the phenotype list to a string.
	QString toString(QString seperator = "; ") const;
	//converts the phenotype list to a string list.
	QStringList toStringList() const;

protected:
	QSet<QByteArray> accessions_;
};

#endif // PHENOTYPELIST_H
