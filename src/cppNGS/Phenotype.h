#ifndef PHENOTYPE_H
#define PHENOTYPE_H

#include "cppNGS_global.h"
#include <QString>

///Phenotype (representation of an HPO term)
class CPPNGSSHARED_EXPORT Phenotype
{
public:
	Phenotype(const QByteArray& accession="", const QByteArray& name="");

	const QByteArray& accession() const
	{
		return accession_;
	}
	void setAccession(QByteArray accession)
	{
		accession_ = accession;
	}

	const QByteArray& name() const
	{
		return name_;
	}
	void setName(QByteArray name)
	{
		name_ = name;
	}

	bool operator==(const Phenotype& rhs) const
	{
		return accession_==rhs.accession_;
	}

	QByteArray toString() const
	{
		return accession_ + " - " + name_;
	}

protected:
	QByteArray accession_;
	QByteArray name_;
};

//Required to make Chromosome hashable by Qt, e.g. to use it in QSet or QHash
inline uint qHash(const Phenotype& pheno)
{
	return qHash(pheno.accession());
}

#endif // PHENOTYPE_H
