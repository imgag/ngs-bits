#ifndef PHENOTYPELIST_H
#define PHENOTYPELIST_H

#include "cppNGS_global.h"
#include "Phenotype.h"
#include <QList>
#include <QSet>

class CPPNGSSHARED_EXPORT PhenotypeList
	: public QList<Phenotype>
{
public:
	//Default constructor
	PhenotypeList();

	///Adds a phenotype
	PhenotypeList& operator<<(const Phenotype& pheno);
	///Adds a phenotype list
	PhenotypeList& operator<<(const QSet<Phenotype>& set);

	//Sort by name
	void sortByName();
	//Sort by accession
	void sortByAccession();

	//Converts the phenotype list to a string
	QString toString(QString seperator = "; ") const;
	//converts the phenotype list to a string list
	QStringList toStringList() const;
};

#endif // PHENOTYPELIST_H
