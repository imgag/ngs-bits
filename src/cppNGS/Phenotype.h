#ifndef PHENOTYPE_H
#define PHENOTYPE_H

#include "cppNGS_global.h"
#include <QString>
#include <QList>
#include "Exceptions.h"

//Phenotye source enumeration.
enum class PhenotypeSource : int {HPO, OMIM, CLINVAR, DECIPHER, HGMD, GENCC};
//Phenotype evidence level enumeration.
enum class PhenotypeEvidenceLevel : int {NA, AGAINST, LOW, MEDIUM, HIGH};
//Phenotype combination mode enumeration.
enum class PhenotypeCombimnationMode {MERGE, INTERSECT};

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


	///Returns a set of all phenotype sources.
	static QSet<PhenotypeSource> allSourceValues();
	///Returns a string representation for the given phenotype source.
	static QString sourceToString(PhenotypeSource src);
	///Returns the phenotype source for a given string.
	static PhenotypeSource sourceFromString(QString s);

	///Returns a set of all phenotype evidence levels.
	static QSet<PhenotypeEvidenceLevel> allEvidenceValues(bool include_against);
	///Returns a string representation for the given evidence.
	static QString evidenceToString(const PhenotypeEvidenceLevel& e);
	///Returns the evidence level for a given string.
	static PhenotypeEvidenceLevel evidenceFromString(QString e);

protected:
	QByteArray accession_;
	QByteArray name_;
};

//Settings used to convert phenotyes to gene lists.
struct PhenotypeSettings
{
	QSet<PhenotypeSource> sources = Phenotype::allSourceValues();
	QSet<PhenotypeEvidenceLevel> evidence_levels = Phenotype::allEvidenceValues(false);
	PhenotypeCombimnationMode mode = PhenotypeCombimnationMode::MERGE;

	//Reverts settings to default-constructed state
	void revert()
	{
		this->operator=(PhenotypeSettings());
	}
	//Equality operator
	bool operator==(const PhenotypeSettings& rhs) const
	{
		return sources==rhs.sources && evidence_levels==rhs.evidence_levels && mode==rhs.mode;
	}
	//Inequality operator
	bool operator!=(const PhenotypeSettings& rhs) const
	{
		return !operator==(rhs);
	}

};

//Required to make classes hashable by Qt, e.g. to use it in QSet or QHash
inline uint qHash(const Phenotype& pheno)
{
	return qHash(pheno.accession());
}
inline uint qHash(const PhenotypeEvidenceLevel& evidence)
{
	return qHash(static_cast<int>(evidence));
}
inline uint qHash(const PhenotypeSource& source)
{
	return qHash(static_cast<int>(source));
}

#endif // PHENOTYPE_H
