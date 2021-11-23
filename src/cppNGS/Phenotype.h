#ifndef PHENOTYPE_H
#define PHENOTYPE_H

#include "cppNGS_global.h"
#include <QString>
#include <QList>

///Phenotype (representation of an HPO term)

// TODO throw errors when casting goes wrong!

/// Strength of the evidence for a given relation
enum PhenotypeEvidence {NA, AGAINST, LOW, MED, HIGH};
/// returns a integer representing the Strength of the evidence: lower less evidence, higher better evidence
static int rank(const PhenotypeEvidence& e)
{
	switch (e) {
		case PhenotypeEvidence::NA:
			return 0;
		case PhenotypeEvidence::AGAINST:
			return 1;
		case PhenotypeEvidence::LOW:
			return 2;
		case PhenotypeEvidence::MED:
			return 3;
		case PhenotypeEvidence::HIGH:
			return 4;
		default:
			return -1;
	}
}
/// returns a QString representation für the given evidence
static QString evidenceToString(PhenotypeEvidence e)
{
	switch (e) {
		case PhenotypeEvidence::NA:
			return "NA";
		case PhenotypeEvidence::AGAINST:
			return "AGAINST";
		case PhenotypeEvidence::LOW:
			return "LOW";
		case PhenotypeEvidence::MED:
			return "MED";
		case PhenotypeEvidence::HIGH:
			return "HIGH";
		default:
			return "";
	}
}
/// turns a given HPO Evidence value into one from the Evidences enum
static PhenotypeEvidence translateHpoEvidence(QString hpoEvi)
{
	/*
	 *	IEA (inferred from electronic annotation): Annotations extracted by parsing the Clinical Features sections of the Online Mendelian Inheritance in Man resource are assigned the evidence code “IEA”.
	 *	PCS (published clinical study) is used for used for information extracted from articles in the medical literature. Generally, annotations of this type will include the pubmed id of the published study in the DB_Reference field.
	 *	TAS (traceable author statement) is used for information gleaned from knowledge bases such as OMIM or Orphanet that have derived the information from a published source..
	 */
	if (hpoEvi == "IEA") {
		return PhenotypeEvidence::LOW;
	} else if (hpoEvi == "TAS") {
		return PhenotypeEvidence::MED;
	} else if (hpoEvi == "PCS") {
		return PhenotypeEvidence::HIGH;
	} else {
		return PhenotypeEvidence::NA;
	}
}
/// turns a given OMIM Evidence value into one from the Evidences enum
static PhenotypeEvidence translateOmimEvidence(QByteArray omimEvi)
{
	/*
		# Phenotype Mapping key - Appears in parentheses after a disorder :
		# -----------------------------------------------------------------
		#
		# 1 - The disorder is placed on the map based on its association with
		# a gene, but the underlying defect is not known.
		# 2 - The disorder has been placed on the map by linkage or other
		# statistical method; no mutation has been found.
		# 3 - The molecular basis for the disorder is known; a mutation has been
		# found in the gene.
		# 4 - A contiguous gene deletion or duplication syndrome, multiple genes
		# are deleted or duplicated causing the phenotype.
	 */
	if (omimEvi == "(1)") {
		return PhenotypeEvidence::LOW;
	} else if (omimEvi == "(2)") {
		return PhenotypeEvidence::LOW;
	} else if (omimEvi == "(3)") {
		return PhenotypeEvidence::HIGH;
	} else if (omimEvi == "(4)") {
		return PhenotypeEvidence::HIGH;
	} else {
		return PhenotypeEvidence::NA;
	}
}
/// turns a given Decipher Evidence value into one from the Evidences enum
static PhenotypeEvidence translateDecipherEvidence(QByteArray decipherEvi)
{
	//disease confidence: One value from the list of possible categories: both DD and IF, confirmed, possible, probable
	/*
	 * Confirmed 	Plausible disease-causing mutations* within, affecting or encompassing an interpretable functional region** of a single gene identified in multiple (>3) unrelated cases/families with a developmental disorder***
					Plausible disease-causing mutations within, affecting or encompassing cis-regulatory elements convincingly affecting the expression of a single gene identified in multiple (>3) unrelated cases/families with a developmental disorder
					As definition 1 and 2 of Probable Gene (see below) with addition of convincing bioinformatic or functional evidence of causation e.g. known inborn error of metabolism with mutation in orthologous gene which is known to have the relevant deficient enzymatic activity in other species; existence of animal mode which recapitulates the human phenotype
	   Probable 	Plausible disease-causing mutations within, affecting or encompassing an interpretable functional region of a single gene identified in more than one (2 or 3) unrelated cases/families or segregation within multiple individuals within a single large family with a developmental disorder
					Plausible disease-causing mutations within, affecting or encompassing cis-regulatory elements convincingly affecting the expression of a single gene identified in in more than one (2 or 3) unrelated cases/families with a developmental disorder
					As definitions of Possible Gene (see below) with addition of convincing bioinformatic or functional evidence of causation e.g. known inborn error of metabolism with mutation in orthologous gene which is known to have the relevant deficient enzymatic activity in other species; existence of animal mode which recapitulates the human phenotype
	   Possible 	Plausible disease-causing mutations within, affecting or encompassingan interpretable functional region of a single gene identified in one case or segregation within multiple individuals within a small family with a developmental disorder
					Plausible disease-causing mutations within, affecting or encompassing cis-regulatory elements convincingly affecting the expression of a single gene identified in one case/family with a developmental disorder
					Possible disease-causing mutations within, affecting or encompassing an interpretable functional region of a single gene identified in more than one unrelated cases/families or segregation within multiple individuals within a single large family with a developmental disorder
	   Both RD and IF 	Plausible disease-causing mutations within, affecting or encompassing the coding region of a single gene identified in multiple (>3) unrelated cases/families with both the relevant disease (RD) and an incidental disorder
	*/
	if (decipherEvi == "both DD and IF") { // meaning?
		return PhenotypeEvidence::LOW;
	} else if (decipherEvi == "probable") {
		return PhenotypeEvidence::LOW;
	} else if (decipherEvi == "possible") {
		return PhenotypeEvidence::MED;
	} else if (decipherEvi == "confirmed") {
		return PhenotypeEvidence::HIGH;
	} else {
		return PhenotypeEvidence::NA;
	}
}
/// turns a given GenCC Evidence value into one from the Evidences enum
static PhenotypeEvidence translateGenccEvidence(QByteArray genccEvi)
{
	//Definitive, Strong, Moderate, Supportive, Limited, Disputed, Refuted, Animal, No Known
	if (genccEvi == "No Known") {
		return PhenotypeEvidence::NA;
	} else if (genccEvi == "No Known Disease Relationship") {
		return PhenotypeEvidence::NA;
	} else if (genccEvi == "Animal") {
		return PhenotypeEvidence::LOW;
	} else if (genccEvi == "Refuted") {
		return PhenotypeEvidence::AGAINST;
	} else if (genccEvi == "Disputed") {
		return PhenotypeEvidence::AGAINST;
	}  else if (genccEvi == "Limited") {
		return PhenotypeEvidence::LOW;
	} else if (genccEvi == "Supportive") {
		return PhenotypeEvidence::LOW;
	} else if (genccEvi == "Moderate") {
		return PhenotypeEvidence::MED;
	} else if (genccEvi == "Strong") {
		return PhenotypeEvidence::HIGH;
	} else if (genccEvi == "Definitive") {
		return PhenotypeEvidence::HIGH;
	} else {
		return PhenotypeEvidence::NA;
	}
}

static PhenotypeEvidence evidenceFromString(QString e)
{
	e = e.toUpper();
	if  (e=="AGAINST") {
		return PhenotypeEvidence::AGAINST;
	} else if (e=="NA") {
		return PhenotypeEvidence::NA;
	} else if (e=="LOW") {
		return PhenotypeEvidence::LOW;
	} else if (e=="MED") {
		return PhenotypeEvidence::MED;
	} else if (e=="HIGH") {
		return PhenotypeEvidence::HIGH;
	}
}

static QList<PhenotypeEvidence> allEvidenceValues()
{
	return QList<PhenotypeEvidence>{PhenotypeEvidence::NA, PhenotypeEvidence::AGAINST, PhenotypeEvidence::LOW, PhenotypeEvidence::MED, PhenotypeEvidence::HIGH};
}
/// Source for a given relation
enum PhenotypeSource {HPO, OMIM, CLINVAR, DECIPHER, HGMC, GENCC };
/// returns a QString representation für the given phenotype source
static QString sourceToString(PhenotypeSource src)
{
	switch (src) {
		case PhenotypeSource::HPO:
			return "HPO";
			break;
		case PhenotypeSource::OMIM:
			return "OMIM";
			break;
		case PhenotypeSource::CLINVAR:
			return "ClinVar";
			break;
		case PhenotypeSource::DECIPHER:
			return "Decipher";
			break;
		case PhenotypeSource::HGMC:
			return "HGMC";
			break;
		case PhenotypeSource::GENCC:
			return "GenCC";
			break;
		default:
			return "";
			break;
	}
}
/// return the corresponding phenotype source for a given string
static PhenotypeSource SourceFromString(QString s)
{
	s = s.toLower();
	if (s == "hpo") {
		return PhenotypeSource::HPO;
	} else if (s == "omim") {
		return PhenotypeSource::OMIM;
	} else if (s == "clinvar") {
		return PhenotypeSource::CLINVAR;
	} else if (s == "decipher") {
		return PhenotypeSource::DECIPHER;
	} else if (s == "hgmc") {
		return PhenotypeSource::HGMC;
	} else if (s == "gencc") {
		return PhenotypeSource::GENCC;
	}
}

static PhenotypeSource SourceFromString(QByteArray s)
{
	return SourceFromString(QString(s));
}

static QList<PhenotypeSource> allSourceValues()
{
	return QList<PhenotypeSource>{PhenotypeSource::HPO, PhenotypeSource::OMIM, PhenotypeSource::CLINVAR, PhenotypeSource::DECIPHER, PhenotypeSource::HGMC, PhenotypeSource::GENCC};
}


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
