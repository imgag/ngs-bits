#ifndef CLINVARSUBMISSIONGENERATOR_H
#define CLINVARSUBMISSIONGENERATOR_H

#include "cppNGS_global.h"
#include "Phenotype.h"
#include "VariantList.h"

///Container for ClinVar submission data
struct CPPNGSSHARED_EXPORT ClinvarSubmissionData
{
	//submission meta data
	QString local_key; //local identifier for the submission, e.g. sample identifier plus variant identifiers

	//submitter data
	QString submitter_first_name;
	QString submitter_last_name;
	QString submitter_email;
	QString organization_id; //NCBI organization ID, e.g. https://www.ncbi.nlm.nih.gov/clinvar/submitters/506385/

	//variant data
	Variant variant; //variant in VCF format
	QString variant_classification;
	QString variant_inheritance;

	//sample data
	QString sample_name;
	QString sample_gender; //optional
	QString sample_disease; //optional - optimally contains OMIM disease e.g. "MIM:614381", but may also contains any other disease name
	QList<Phenotype> sample_phenotypes; //optional

	///Checks the data for errors. Throws an exception if it is not valid.
	void check() const;

	///Helper: returns the list of valid genders
	static QStringList validGenders();
	///Helper: returns the list of inheritance modes
	static QStringList validInheritanceModes();
	///Helper: returns the list of valid classifications
	static QStringList validClassifications();

protected:
	static void checkNotEmpty(QString name, QString value);
	static void checkIn(QString name, QString value, QStringList valid, bool empty_is_valid);
};

///Container for ClinVar submission data
class CPPNGSSHARED_EXPORT ClinvarSubmissionGenerator
{
public:
	//Generates the ClinVar XML file. Throws an exception if the input or output is invalid.
	static QString generateXML(const ClinvarSubmissionData& data);

	//Helper: Converts classification from NGSD to ClinVar notation. Returns empty string if no conversion was possible.
	static QString translateClassification(QString classification);
	//Helper: Converts mode of inheritance from NGSD to ClinVar notation. Returns empty string if no conversion was possible.
	static QString translateInheritance(QString mode);

private:
	//main methods
	ClinvarSubmissionGenerator() = delete;
	static void generateXML(const ClinvarSubmissionData& data, QString& output);
	static void validateXML(const QString& text);

};

#endif // CLINVARSUBMISSIONDATA_H
