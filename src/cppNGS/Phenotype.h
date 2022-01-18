#ifndef PHENOTYPE_H
#define PHENOTYPE_H

#include "cppNGS_global.h"
#include <QString>
#include <QList>
#include "Exceptions.h"


///PhenotypeEvidence a 'static class' to contain an enum and helperfunctions to represent possible evidence levels of databases for phenotype-gene relations
struct CPPNGSSHARED_EXPORT PhenotypeEvidence
{
	PhenotypeEvidence() = delete;
	/// Strength of the evidence for a given relation
	/// If the evidence values are changed remember to adjust allEvidenceValues and the other functions below if needed.
	enum Evidence {NA=0, AGAINST=1, LOW=2, MED=3, HIGH=4};

	/// returns all possible values for the evidence enum
	static QList<Evidence> allEvidenceValues()
	{
		return QList<Evidence>{Evidence::NA, Evidence::AGAINST, Evidence::LOW, Evidence::MED, Evidence::HIGH};
	}

	/// returns a QString representation für the given evidence
	static QString evidenceToString(const Evidence& e)
	{
		switch (e) {
			case Evidence::NA:
				return "n/a";
			case Evidence::AGAINST:
				return "against";
			case Evidence::LOW:
				return "low";
			case Evidence::MED:
				return "medium";
			case Evidence::HIGH:
				return "high";
			default:
				THROW(ProgrammingException, "Given PhenotypeEvidence::Evidence value has no cast to string. Please add it to the function.")
				return "";
		}
	}
	/// turns a given HPO Evidence value into one from the Evidences enum
	static Evidence translateHpoEvidence(const QString& hpo_evi)
	{
		/*
		 *	IEA (inferred from electronic annotation): Annotations extracted by parsing the Clinical Features sections of the Online Mendelian Inheritance in Man resource are assigned the evidence code “IEA”.
		 *	PCS (published clinical study) is used for used for information extracted from articles in the medical literature. Generally, annotations of this type will include the pubmed id of the published study in the DB_Reference field.
		 *	TAS (traceable author statement) is used for information gleaned from knowledge bases such as OMIM or Orphanet that have derived the information from a published source..
		 */
		if (hpo_evi == "IEA")
		{
			return PhenotypeEvidence::LOW;
		}
		else if (hpo_evi == "TAS")
		{
			return PhenotypeEvidence::MED;
		}
		else if (hpo_evi == "PCS")
		{
			return PhenotypeEvidence::HIGH;
		}
		else
		{
			THROW(ArgumentException, "Given Evidence is not a HPO evidence value: " + QString(hpo_evi));
		}
	}
	/// turns a given OMIM Evidence value into one from the Evidences enum
	static Evidence translateOmimEvidence(const QByteArray& omim_evi)
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
		if (omim_evi == "(1)")
		{
			return PhenotypeEvidence::LOW;
		}
		else if (omim_evi == "(2)")
		{
			return PhenotypeEvidence::LOW;
		}
		else if (omim_evi == "(3)")
		{
			return PhenotypeEvidence::HIGH;
		}
		else if (omim_evi == "(4)")
		{
			return PhenotypeEvidence::HIGH;
		}
		else
		{
			THROW(ArgumentException, "Given Evidence is not a Omim evidence value: " + QString(omim_evi));
		}
	}
	/// turns a given Decipher Evidence value into one from the Evidences enum
	static Evidence translateDecipherEvidence(const QByteArray& decipher_evi)
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
		if (decipher_evi == "\"both RD and IF\"")
		{ // meaning?
			return PhenotypeEvidence::LOW;
		}
		else if (decipher_evi == "possible" || decipher_evi == "limited")
		{
			return PhenotypeEvidence::LOW;
		}
		else if (decipher_evi == "probable")
		{
			return PhenotypeEvidence::MED;
		}
		else if (decipher_evi == "confirmed" || decipher_evi == "definitive" || decipher_evi == "strong")
		{
			return PhenotypeEvidence::HIGH;
		}
		else
		{
			THROW(ArgumentException, "Given Evidence is not a Decipher evidence value.: " + QString(decipher_evi));
		}
	}
	/// turns a given GenCC Evidence value into one from the Evidences enum
	static Evidence translateGenccEvidence(const QByteArray& gencc_evi)
	{
		//Definitive, Strong, Moderate, Supportive, Limited, Disputed, Refuted, Animal, No Known
		if (gencc_evi == "No Known")
		{
			return PhenotypeEvidence::NA;
		}
		else if (gencc_evi == "No Known Disease Relationship")
		{
			return PhenotypeEvidence::NA;
		}
		else if (gencc_evi == "Animal")
		{
			return PhenotypeEvidence::LOW;
		}
		else if (gencc_evi == "Refuted" || gencc_evi == "Refuted Evidence")
		{
			return PhenotypeEvidence::AGAINST;
		}
		else if (gencc_evi == "Disputed" || gencc_evi == "Disputed Evidence")
		{
			return PhenotypeEvidence::AGAINST;
		}
		else if (gencc_evi == "Limited")
		{
			return PhenotypeEvidence::LOW;
		}
		else if (gencc_evi == "Supportive")
		{
			return PhenotypeEvidence::LOW;
		}
		else if (gencc_evi == "Moderate")
		{
			return PhenotypeEvidence::MED;
		}
		else if (gencc_evi == "Strong")
		{
			return PhenotypeEvidence::HIGH;
		}
		else if (gencc_evi == "Definitive")
		{
			return PhenotypeEvidence::HIGH;
		}
		else
		{
			THROW(ArgumentException, "Given Evidence is not a GenCC evidence value: " + QString(gencc_evi));
		}
	}
	/// returns the appropriate evidence value for a given string
	static Evidence evidenceFromString(QString e)
	{
		e = e.toUpper();
		if  (e=="AGAINST")
		{
			return PhenotypeEvidence::AGAINST;
		}
		else if (e=="NA" || e=="N/A")
		{
			return PhenotypeEvidence::NA;
		}
		else if (e=="LOW") {
			return PhenotypeEvidence::LOW;
		}
		else if (e=="MED" || e=="MEDIUM")
		{
			return PhenotypeEvidence::MED;
		}
		else if (e=="HIGH")
		{
			return PhenotypeEvidence::HIGH;
		}

		THROW(TypeConversionException, "Cannot convert String: '" + e + "' to Phenotype Evidence.")
	}
};

///PhenotypeSource a 'static class' to contain an enum and helperfunctions to represent possible source databases for phenotype-gene relations
struct CPPNGSSHARED_EXPORT PhenotypeSource
{
	PhenotypeSource() = delete;
	/// Source for a given relation
	/// when changing the source enum the functions below have to be adjusted!
	enum Source {HPO, OMIM, CLINVAR, DECIPHER, HGMD, GENCC };

	///returns a list of all source enum values
	static QList<Source> allSourceValues()
	{
		return QList<Source>{Source::HPO, Source::OMIM, Source::CLINVAR, Source::DECIPHER, Source::HGMD, Source::GENCC};
	}

	/// returns a QString representation für the given phenotype source
	static QString sourceToString(Source src)
	{
		switch (src) {
			case Source::HPO:
				return "HPO";
				break;
			case Source::OMIM:
				return "OMIM";
				break;
			case Source::CLINVAR:
				return "ClinVar";
				break;
			case Source::DECIPHER:
				return "Decipher";
				break;
			case Source::HGMD:
				return "HGMD";
				break;
			case Source::GENCC:
				return "GenCC";
				break;
			default:
				THROW(ProgrammingException, "Given PhenotypeSource::Source value has no cast to string. Please add it to the function.")
				return "";
				break;
		}
	}
	/// return the corresponding phenotype source for a given string
	static Source SourceFromString(QString s)
	{
		s = s.toLower();
		if (s == "hpo")
		{
			return Source::HPO;
		}
		else if (s == "omim")
		{
			return Source::OMIM;
		}
		else if (s == "clinvar")
		{
			return Source::CLINVAR;
		}
		else if (s == "decipher")
		{
			return Source::DECIPHER;
		}
		else if (s == "hgmd")
		{
			return Source::HGMD;
		}
		else if (s == "gencc")
		{
			return Source::GENCC;
		}
		THROW(TypeConversionException, "Cannot convert String: '" + s + "' to Phenotype Source.")
	}
	/// return the corresponding phenotype source for a given string
	static Source SourceFromString(const QByteArray& s)
	{
		return SourceFromString(QString(s));
	}
};

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
