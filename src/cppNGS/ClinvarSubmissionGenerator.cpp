#include "ClinvarSubmissionGenerator.h"
#include "XmlHelper.h"
#include "Helper.h"
#include <QXmlStreamWriter>

void ClinvarSubmissionData::check() const
{
	checkNotEmpty("local_key", local_key);

	checkNotEmpty("submission_id", submission_id);
	checkNotEmpty("submitter_id", submitter_id);
	checkNotEmpty("organization_id", organization_id);

	if (!variant.isValid()) THROW(ArgumentException, "ClinVar submission variant is not valid!");
	if (variant.ref()=="" || variant.ref()=="-") THROW(ArgumentException, "ClinVar submission variant is not in VCF format! Reference base '" + variant.ref() + "' is not valid!");
	if (variant.obs()=="" || variant.obs()=="-") THROW(ArgumentException, "ClinVar submission variant is not in VCF format! Alternate base '" + variant.ref() + "' is not valid!");
	checkNotEmpty("variant_classification", variant_classification);
	checkIn("variant_classification", variant_classification, validClassifications(), false);
	checkNotEmpty("variant_inheritance", variant_inheritance);
	checkIn("variant_inheritance", variant_inheritance, validInheritanceModes(), false);

	checkNotEmpty("sample_name", sample_name);
	checkIn("sample_gender", sample_gender, validGenders(), true);
}

QStringList ClinvarSubmissionData::validGenders()
{
	return QStringList() << "male" << "female";
}

QStringList ClinvarSubmissionData::validInheritanceModes()
{
	//Documentation: ftp://ftp.ncbi.nlm.nih.gov/pub/GTR/standard_terms/Mode_of_inheritance.txt

	return QStringList() << "Autosomal dominant inheritance" << "Autosomal dominant inheritance with maternal imprinting" << "Autosomal dominant inheritance with paternal imprinting" << "Autosomal recessive inheritance" << "Autosomal unknown" << "Codominant" << "Genetic anticipation" << "Mitochondrial inheritance" << "Multifactorial inheritance" << "Oligogenic inheritance" << "Sex-limited autosomal dominant" << "Somatic mutation" << "Sporadic" << "Unknown mechanism" << "X-linked dominant inheritance" << "X-linked inheritance" << "X-linked recessive inheritance" << "Y-linked inheritance";
}

QStringList ClinvarSubmissionData::validClassifications()
{
	//Documentation: https://www.ncbi.nlm.nih.gov/clinvar/docs/clinsig/#clinsig_options_scv

	return QStringList() << "Benign" << "Likely benign" << "Uncertain significance" << "Likely pathogenic" << "Pathogenic";
}

void ClinvarSubmissionData::checkNotEmpty(QString name, QString value)
{
	value = value.trimmed();
	if (value.isEmpty())
	{
		THROW(ArgumentException, "ClinVar submission data property '" + name + "' must not be empty, but is!");
	}
}

void ClinvarSubmissionData::checkIn(QString name, QString value, QStringList valid, bool empty_is_valid)
{
	value = value.trimmed();

	if (value.isEmpty() && empty_is_valid) return;

	if (!valid.contains(value))
	{
		THROW(ArgumentException, "ClinVar submission data property '" + name + "' is '" + value + "', but must be one of '" + valid.join("', '") + "'!");
	}
}

QString ClinvarSubmissionGenerator::generateXML(const ClinvarSubmissionData& data)
{
	data.check();

	QString output;
	generateXML(data, output);

	validateXML(output);

	return output;
}

void ClinvarSubmissionGenerator::generateXML(const ClinvarSubmissionData& data, QString& output)
{
	//XML schema: https://ftp.ncbi.nlm.nih.gov/pub/clinvar/xsd_submission/
	//Documentation: https://www.ncbi.nlm.nih.gov/projects/clinvar/ClinVarDataDictionary.pdf

	//start document
	QXmlStreamWriter w(&output);
	w.setAutoFormatting(true);
	w.writeStartDocument();
	w.writeStartElement("ClinvarSubmissionSet");
	w.writeAttribute("sub_id", data.submission_id);

	w.writeAttribute("Date", data.date.toString(Qt::ISODate));

	//element "SubmitterOfRecordID"
	{
		w.writeStartElement("SubmitterOfRecordID");
		w.writeCharacters(data.submitter_id);
		w.writeEndElement();
	}

	//element "OrgID"
	{
		w.writeStartElement("OrgID");
		w.writeAttribute("Type", "primary");
		w.writeCharacters(data.organization_id);
		w.writeEndElement();
	}

	//element "ClinvarSubmission"
	{
		w.writeStartElement("ClinvarSubmission");

		w.writeStartElement("RecordStatus");
		w.writeCharacters("novel");
		w.writeEndElement();

		w.writeStartElement("ReleaseStatus");
		w.writeCharacters("public");
		w.writeEndElement();

		w.writeStartElement("ClinvarSubmissionID");
		w.writeAttribute("localKey", data.local_key);
		w.writeAttribute("submitterDate", data.date.toString(Qt::ISODate));
		w.writeEndElement();

		//element "MeasureTrait"
		{
			w.writeStartElement("MeasureTrait");

			w.writeStartElement("Assertion");
			w.writeStartElement("AssertionType");
			w.writeAttribute("val_type", "name");
			w.writeCharacters("variation to disease");
			w.writeEndElement();
			w.writeEndElement();

			w.writeStartElement("ClinicalSignificance");
			w.writeStartElement("ReviewStatus");
			w.writeCharacters("criteria provided, single submitter");
			w.writeEndElement();
			w.writeStartElement("Description");
			w.writeCharacters(data.variant_classification);
			w.writeEndElement();
			w.writeStartElement("DateLastEvaluated");
			w.writeCharacters(data.date.toString(Qt::ISODate));
			w.writeEndElement();
			w.writeEndElement();

			w.writeStartElement("AttributeSet");
			w.writeStartElement("MeasureTraitAttributeType");
			w.writeAttribute("val_type", "name");
			w.writeCharacters("ModeOfInheritance");
			w.writeEndElement();
			w.writeStartElement("Attribute");
			w.writeCharacters(data.variant_inheritance);
			w.writeEndElement();
			w.writeEndElement();

			w.writeStartElement("AttributeSet");
			w.writeStartElement("MeasureTraitAttributeType");
			w.writeAttribute("val_type", "name");
			w.writeCharacters("AssertionMethod");
			w.writeEndElement();
			w.writeStartElement("Attribute");
			w.writeCharacters("ACMG Guidelines, 2015");
			w.writeEndElement();
			w.writeStartElement("Citation");
			w.writeStartElement("ID");
			w.writeAttribute("Source", "PubMed");
			w.writeCharacters("25741868");
			w.writeEndElement();
			w.writeEndElement();
			w.writeEndElement();

			//element "ObservedIn"
			{
				w.writeStartElement("ObservedIn");

				w.writeStartElement("Sample");
				w.writeStartElement("Origin");
				w.writeCharacters("germline");
				w.writeEndElement();
				w.writeStartElement("Species");
				w.writeAttribute("TaxonomyId", "9606");
				w.writeCharacters("human");
				w.writeEndElement();
				w.writeStartElement("AffectedStatus");
				w.writeCharacters("yes");
				w.writeEndElement();
				w.writeStartElement("NumberTested");
				w.writeCharacters("1");
				w.writeEndElement();
				if (!data.sample_gender.isEmpty())
				{
					w.writeStartElement("Gender");
					w.writeCharacters(data.sample_gender);
					w.writeEndElement();
				}
				w.writeEndElement();

				w.writeStartElement("Method");
				w.writeStartElement("MethodType");
				w.writeAttribute("val_type", "name");
				w.writeCharacters("clinical testing");
				w.writeEndElement();
				w.writeEndElement();

				w.writeStartElement("ObservedData");
				w.writeStartElement("ObsAttributeType");
				w.writeAttribute("val_type", "name");
				w.writeCharacters("SampleLocalID");
				w.writeEndElement();
				w.writeStartElement("Attribute");
				w.writeCharacters(data.sample_name);
				w.writeEndElement();
				w.writeEndElement();

				//element "TraitSet"
				if (data.sample_phenotypes.count()>0)
				{
					w.writeStartElement("TraitSet");
					w.writeStartElement("TraitSetType");
					w.writeAttribute("val_type", "name");
					w.writeCharacters("Finding");
					w.writeEndElement();
					foreach(const Phenotype& phenotype, data.sample_phenotypes)
					{
						w.writeStartElement("Trait");
						w.writeStartElement("TraitType");
						w.writeAttribute("val_type", "name");
						w.writeCharacters("Finding");
						w.writeEndElement();
						w.writeStartElement("XRef");
						w.writeAttribute("db", "HP");
						w.writeAttribute("id", phenotype.accession());
						w.writeEndElement();
						w.writeEndElement();
					}
					w.writeEndElement();
				}

				w.writeEndElement();
			}

			w.writeEndElement();
		}


		//element "MeasureSet"
		{
			w.writeStartElement("MeasureSet");

			w.writeStartElement("MeasureSetType");
			w.writeAttribute("val_type", "name");
			w.writeCharacters("Variant");
			w.writeEndElement();

			w.writeStartElement("Measure");
			w.writeStartElement("MeasureType");
			w.writeAttribute("val_type", "name");
			w.writeCharacters("Variation");
			w.writeEndElement();
			w.writeStartElement("SequenceLocation");
			w.writeAttribute("Assembly", "GRCh37");
			w.writeAttribute("Chr", data.variant.chr().strNormalized(false));
			w.writeAttribute("referenceAllele", data.variant.ref());
			w.writeAttribute("alternateAllele", data.variant.obs());
			w.writeAttribute("start", QString::number(data.variant.start()));
			w.writeEndElement();
			w.writeEndElement();

			w.writeEndElement();
		}

		//element "TraitSet"
		{
			w.writeStartElement("TraitSet");
			w.writeStartElement("TraitSetType");
			w.writeAttribute("val_type", "name");
			w.writeCharacters("Disease");
			w.writeEndElement();
			w.writeStartElement("Trait");
			w.writeStartElement("TraitType");
			w.writeAttribute("val_type", "name");
			w.writeCharacters("Disease");
			w.writeEndElement();
			if (data.sample_disease.startsWith("MIM:"))
			{
				w.writeStartElement("XRef");
				w.writeAttribute("db", "OMIM");
				w.writeAttribute("id", data.sample_disease.mid(4).trimmed());
				w.writeAttribute("type", "MIM");
				w.writeEndElement();
			}
			else
			{
				w.writeStartElement("Name");
				w.writeStartElement("ElementValueType");
				w.writeAttribute("val_type", "name");
				w.writeCharacters("Preferred");
				w.writeEndElement();
				w.writeStartElement("ElementValue");
				if (data.sample_disease.trimmed().isEmpty())
				{
					w.writeCharacters("not provided");
				}
				else
				{
					w.writeCharacters(data.sample_disease);
				}
				w.writeEndElement();
				w.writeEndElement();
			}
			w.writeEndElement();
			w.writeEndElement();
		}

		w.writeEndElement();
	}

	//close document
	w.writeEndElement();
	w.writeEndDocument();
}

void ClinvarSubmissionGenerator::validateXML(const QString& text)
{
	//store text to file
	QString tmp_file = Helper::tempFileName(".xml");
	Helper::storeTextFile(tmp_file, QStringList() << text);

	//validate file
	QString xml_error = XmlHelper::isValidXml(tmp_file, ":/Resources/clinvar_submission_1.6.xsd");
	if (xml_error!="")
	{
		foreach(QString line, text.split("\n"))
		{
			qDebug() << line;
		}
		THROW(ProgrammingException, " ClinvarSubmissionData::generateXML produced an invalid XML file: " + xml_error);
	}
}

QString ClinvarSubmissionGenerator::translateClassification(QString classification)
{
	bool ok = true;
	int value = classification.toInt(&ok);
	if (ok && value==1)
	{
		return "Benign";
	}
	else if (ok && value==2)
	{
		return "Likely benign";
	}
	else if (ok && value==3)
	{
		return "Uncertain significance";
	}
	else if (ok && value==4)
	{
		return "Likely pathogenic";
	}
	else if (ok && value==5)
	{
		return "Pathogenic";
	}
	else if (classification=="M")
	{
		return "risk factor";
	}

	return "";
}

QString ClinvarSubmissionGenerator::translateInheritance(QString mode)
{
	if (mode=="AR")
	{
		return "Autosomal recessive inheritance";
	}
	else if (mode=="AD")
	{
		return "Autosomal dominant inheritance";
	}
	else if (mode=="XLR")
	{
		return "X-linked recessive inheritance";
	}
	else if (mode=="XLD")
	{
		return "X-linked dominant inheritance";
	}
	else if (mode=="MT")
	{
		return "Mitochondrial inheritance";
	}

	return "";
}

/**
	### Requirements ###
	1) Several users can submit variants for our institute. The submitter is shown as contact person of the submitted variant and can edit the submission.
	2) Submission via an API that uses a token/key for identification of the user/organization and (b) a data format that can be validated before the submission (the current XSD is not sufficient since the actual logic is in the PDF).

	### Questions ###
	1) Organizations/Groups/Users:
	I created our organiztation:
	https://www.ncbi.nlm.nih.gov/clinvar/submitters/506385/
	If I understand the documentation right, this group is associated to our organization:
	https://submit.ncbi.nlm.nih.gov/groups/clinvar-institute-of-medical-genetics-and-applied-genomics/
	So I would invite our lab staff to join this group to be able to submit on bahalf of our ogganization, right?

	2) Submissions listed
	On the organization page https://www.ncbi.nlm.nih.gov/clinvar/submitters/506385/ no submissions are listed.
	The submission SUB6667177 is probably not listed because it is not processed yet, right?

	### Questions internal###
	1) Upload somatic variants to ClinVar? > Christopher
	2) Report config inheritance modes AR+AD and XLR+XLDnecessary? Add others from ClinVar?
	3) ACMG Guidelines als "AssertionMethod" ok? > Tobias
*/
