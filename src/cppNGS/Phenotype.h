#ifndef PHENOTYPE_H
#define PHENOTYPE_H

#include "cppNGS_global.h"
#include <QString>

///Phenotype (representation of an HPO term)
class CPPNGSSHARED_EXPORT Phenotype
{
public:
	Phenotype(QString accession="", QString name="");

	QString accession() const
	{
		return accession_;
	}
	void setAccession(QString accession)
	{
		accession_ = accession;
	}

	QString name() const
	{
		return name_;
	}
	void setName(QString name)
	{
		name_ = name;
	}

protected:
	QString accession_;
	QString name_;
};

#endif // PHENOTYPE_H
