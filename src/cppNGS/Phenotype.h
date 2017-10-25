#ifndef PHENOTYPE_H
#define PHENOTYPE_H

#include "cppNGS_global.h"
#include <QString>

///Phenotype (representation of an HPO term)
class CPPNGSSHARED_EXPORT Phenotype
{
public:
	Phenotype(QByteArray accession="", QByteArray name="");

	QByteArray accession() const
	{
		return accession_;
	}
	void setAccession(QByteArray accession)
	{
		accession_ = accession;
	}

	QByteArray name() const
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

protected:
	QByteArray accession_;
	QByteArray name_;
};

#endif // PHENOTYPE_H
