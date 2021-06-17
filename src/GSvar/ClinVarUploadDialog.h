#ifndef CLINVARUPLOADDIALOG_H
#define CLINVARUPLOADDIALOG_H

#include <QDialog>

struct ClinvarUploadData
{
	//clinvarSubmisson

		//assertionCriteria
			//citation
				//db
				//id
				//url
			//method

		//clinicalSignificance
			//citation
				//db
				//id
				//url
			//clinicalSignificanceDescription
			//comment
			//dateLastEvaluated
			//modeOfInheritance


		//clinvarAccession

		//conditionSet
			//condition
				//db
				//id
				//name

		//localID

		//localKey

		//observedIn
			//affectedStatus
			//alleleOrigin
			//clinicalFeatures
				//db
				//id
				//name
			//clinicalFeaturesComment
			//collectionMethod
			//numberOfIndividuals
			//structVarMethodType

		//recordStatus

		//releaseStatus

		//variantSet
			//variant
				//chromosomeCoordinates
					//accession
					//alternateAllele
					//assembly
					//chromosome
					//innertStart
					//innertStop
					//outerStart
					//outerStop
					//referenceAllele
					//start
					//stop
					//variantLength
				//copyNumber
				//gene
					//id
					//symbol
				//hgvs
				//referenceCopyNumber
				//variantType


};

namespace Ui {
class ClinvarUploadDialog;
}

class ClinvarUploadDialog
		: public QDialog
{
	Q_OBJECT

public:
	explicit ClinvarUploadDialog(QWidget *parent = 0);
	~ClinvarUploadDialog();

private:
	Ui::ClinvarUploadDialog *ui;
};

#endif // CLINVARUPLOADDIALOG_H
