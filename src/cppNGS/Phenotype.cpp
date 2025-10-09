#include "Phenotype.h"
#include "Exceptions.h"

Phenotype::Phenotype(const QByteArray& accession, const QByteArray& name)
	: accession_(accession)
	, name_(name)
{
}

QSet<PhenotypeSource> Phenotype::allSourceValues()
{
	return QSet<PhenotypeSource>{PhenotypeSource::HPO, PhenotypeSource::OMIM, PhenotypeSource::CLINVAR, PhenotypeSource::G2P, PhenotypeSource::HGMD, PhenotypeSource::GENCC};
}

QSet<PhenotypeEvidenceLevel> Phenotype::allEvidenceValues(bool include_against)
{
	QSet<PhenotypeEvidenceLevel> output {PhenotypeEvidenceLevel::NA, PhenotypeEvidenceLevel::LOW, PhenotypeEvidenceLevel::MEDIUM, PhenotypeEvidenceLevel::HIGH};

	if (include_against) output << PhenotypeEvidenceLevel::AGAINST;

	return output;
}

QString Phenotype::evidenceToString(const PhenotypeEvidenceLevel& e)
{
	switch (e)
	{
		case PhenotypeEvidenceLevel::NA:
			return "n/a";
		case PhenotypeEvidenceLevel::AGAINST:
			return "against";
		case PhenotypeEvidenceLevel::LOW:
			return "low";
		case PhenotypeEvidenceLevel::MEDIUM:
			return "medium";
		case PhenotypeEvidenceLevel::HIGH:
			return "high";
	}

	THROW(ProgrammingException, "Cannot convert PhenotypeEvidenceLevel to string!")
}

PhenotypeEvidenceLevel Phenotype::evidenceFromString(QString e)
{
	e = e.toLower().trimmed();
	if  (e=="against")
	{
		return PhenotypeEvidenceLevel::AGAINST;
	}
	else if (e=="n/a")
	{
		return PhenotypeEvidenceLevel::NA;
	}
	else if (e=="low")
	{
		return PhenotypeEvidenceLevel::LOW;
	}
	else if (e=="medium")
	{
		return PhenotypeEvidenceLevel::MEDIUM;
	}
	else if (e=="high")
	{
		return PhenotypeEvidenceLevel::HIGH;
	}

	THROW(ProgrammingException, "Cannot convert string '" + e + "' to phenotype evidence level!")
}

QString Phenotype::sourceToString(PhenotypeSource src)
{
	switch (src)
	{
		case PhenotypeSource::HPO:
			return "HPO";
			break;
		case PhenotypeSource::OMIM:
			return "OMIM";
			break;
		case PhenotypeSource::CLINVAR:
			return "ClinVar";
			break;
		case PhenotypeSource::G2P:
			return "G2P";
			break;
		case PhenotypeSource::HGMD:
			return "HGMD";
			break;
		case PhenotypeSource::GENCC:
			return "GenCC";
			break;
	}
	THROW(ProgrammingException, "Cannot convert PhenotypeSource value has to string!")
}

PhenotypeSource Phenotype::sourceFromString(QString s)
{
	s = s.toLower().trimmed();
	if (s == "hpo")
	{
		return PhenotypeSource::HPO;
	}
	else if (s == "omim")
	{
		return PhenotypeSource::OMIM;
	}
	else if (s == "clinvar")
	{
		return PhenotypeSource::CLINVAR;
	}
	else if (s == "g2p")
	{
		return PhenotypeSource::G2P;
	}
	else if (s == "hgmd")
	{
		return PhenotypeSource::HGMD;
	}
	else if (s == "gencc")
	{
		return PhenotypeSource::GENCC;
	}
	THROW(ProgrammingException, "Cannot convert string '" + s + "' to phenotype source!")
}
