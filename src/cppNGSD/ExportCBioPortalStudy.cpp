#include "ExportCBioPortalStudy.h"

#include <QFile>
#include <Helper.h>
#include "Exceptions.h"
#include "SomaticReportHelper.h"

//
///METAFILE:
//

MetaFile::MetaFile()
{
}

void MetaFile::store(const QString& out)
{
	QSharedPointer<QFile> out_file = Helper::openFileForWriting(out);

	foreach(const QString& key, values_.keys())
	{
		out_file->write(key.toUtf8() + ": " + values_[key].toUtf8() + "\n");
	}
}

void MetaFile::load(const QString& in)
{
	QSharedPointer<QFile> in_file = Helper::openFileForReading(in);

	while(! in_file->atEnd())
	{
		QByteArray line = in_file->readLine();

		QByteArrayList parts = line.split(':');

		if (parts.count() != 2)
		{
			THROW(FileParseException, "Expected meta file line to consist of two parts ")
		}
	}
}

//
///CBIOPORTALEXPORTSETTINGS:
//

CBioPortalExportSettings::CBioPortalExportSettings(StudyData study, bool ngsd_test)
	: study(study)
	, db_(ngsd_test)
{
}

CBioPortalExportSettings::CBioPortalExportSettings(const CBioPortalExportSettings& other)
	: db_( ! other.db_.isProductionDb())
{
	study = other.study;
	cancer = other.cancer;
	sample_list = other.sample_list;
	report_settings = other.report_settings;
	sample_files = other.sample_files;
	ps_ids = other.ps_ids;
	ps_data = other.ps_data;

	sample_attributes = other.sample_attributes;
	patient_map = other.patient_map;
}

void CBioPortalExportSettings::addSample(SomaticReportSettings settings, PatientData patient, SampleFiles files)
{
	QString name = settings.tumor_ps;

	if (sample_list.contains(name)) THROW(ArgumentException, "Given sample: '" + name + "' was already added to the sample list of the export.");

	sample_list.append(name);
	patient_map.insert(name, patient);
	sample_files.append(files);
	//TODO check if all necessary infos were entered
	report_settings.append(settings);

	QString ps_id = db_.processedSampleId(name);
	ps_ids.append(ps_id);
	ps_data.append(db_.getProcessedSampleData(ps_id));
}

double CBioPortalExportSettings::getMsiStatus(int sample_idx)
{
	double msi = std::numeric_limits<double>::quiet_NaN();
	if (QFile::exists(sample_files[sample_idx].msi_file))
	{
		TSVFileStream msi_file(sample_files[sample_idx].msi_file);
		//Use step wise difference (-> stored in the first line of MSI status file) for MSI status
		QByteArrayList data = msi_file.readLine();
		if(data.count() > 0) msi = data[1].toDouble();
	}

	return msi;
}

float CBioPortalExportSettings::getPloidy(int sample_idx)
{
	if (QFile::exists(sample_files[sample_idx].clincnv_file))
	{
		QStringList cnv_data = Helper::loadTextFile(sample_files[sample_idx].clincnv_file, true, QChar::Null, true);

		foreach(const QString& line, cnv_data)
		{
			if (line.startsWith("##ploidy:"))
			{
				QStringList parts = line.split(':');
				return parts[1].toDouble();
				break;
			}

			if (! line.startsWith("##"))
			{
				break;
			}
		}
	}

	return std::numeric_limits<double>::quiet_NaN();
}

float CBioPortalExportSettings::getPurityHist(int sample_idx)
{
	QList<SampleDiseaseInfo> disease_details = db_.getSampleDiseaseInfo(ps_ids[sample_idx], "tumor fraction");
	if (disease_details.count() >1)
	{
		THROW(ArgumentException, "Sample '" + sample_list[sample_idx] + "' has more than one entry for tumor fraction in the disease details.");
	}
	else if (disease_details.count() == 0)
	{
		return std::numeric_limits<double>::quiet_NaN();
	}

	return disease_details[0].disease_info.toDouble() / 100.0;
}

float CBioPortalExportSettings::getPurityCnvs(int sample_idx)
{
	if (QFile::exists(sample_files[sample_idx].clincnv_file))
	{
		CnvList cnvs;
		cnvs.load(sample_files[sample_idx].clincnv_file);
		return SomaticReportHelper::getCnvMaxTumorClonality(cnvs);
	}
	return std::numeric_limits<double>::quiet_NaN();
}

QString CBioPortalExportSettings::getProcessingSystem(int sample_idx)
{
	return ps_data[sample_idx].processing_system;
}

QString CBioPortalExportSettings::getComments(int sample_idx)
{
	return ps_data[sample_idx].comments;
}

int CBioPortalExportSettings::getHrdScore(int sample_idx)
{
	QCCollection ps_qc = db_.getQCData(ps_ids[sample_idx]);
	return ps_qc.value("QC:2000126", true).asInt();
}

float CBioPortalExportSettings::getTmb(int sample_idx)
{
	QCCollection ps_qc = db_.getQCData(ps_ids[sample_idx]);
	return  ps_qc.value("QC:2000053",true).asDouble();
}

QStringList CBioPortalExportSettings::getIcd10(int sample_idx)
{
	QStringList icd10;
	QList<SampleDiseaseInfo> disease_details = db_.getSampleDiseaseInfo(ps_ids[sample_idx], "ICD10 code");
	foreach (const auto& dd, disease_details)
	{
		icd10.append(dd.disease_info);
	}
	return icd10;
}

QStringList CBioPortalExportSettings::getHpoTerms(int sample_idx)
{
	QStringList hpo;
	QList<SampleDiseaseInfo> disease_details = db_.getSampleDiseaseInfo(ps_ids[sample_idx], "HPO term id");
	foreach (const auto& dd, disease_details)
	{
		hpo.append(dd.disease_info);
	}
	return hpo;
}

QString CBioPortalExportSettings::getClinicalPhenotype(int sample_idx)
{
	QStringList clin_phenotype;
	QList<SampleDiseaseInfo> disease_details = db_.getSampleDiseaseInfo(ps_ids[sample_idx], "clinical phenotype (free text)");
	foreach (const auto& dd, disease_details)
	{
		clin_phenotype.append(dd.disease_info);
	}
	return clin_phenotype.join(", ");
}

QString CBioPortalExportSettings::getFormatedAttribute(Attribute att, int sample_idx)
{
	switch (att) {
		case Attribute::SAMPLE_ID:
			return report_settings[sample_idx].tumor_ps + "-" + report_settings[sample_idx].normal_ps;
		case Attribute::PATIENT_ID:
			return patient_map[sample_list[sample_idx]].patient_id;
		case Attribute::PROCESSING_SYSTEM:
			return getProcessingSystem(sample_idx);
		case Attribute::CLINICAL_PHENOTYPE:
			return getClinicalPhenotype(sample_idx);
		case Attribute::COMMENT:
			return getComments(sample_idx);
		case Attribute::HPO_TERMS:
			return getHpoTerms(sample_idx).join(", ");
		case Attribute::HRD_SCORE:
			return QString::number(getHrdScore(sample_idx));
		case Attribute::ICD10:
			return getIcd10(sample_idx).join(", ");
		case Attribute::MSI_STATUS:
		{
			double msi_value = getMsiStatus(sample_idx);
			if(ps_data[sample_idx].processing_system_type == "WES")
			{
				return msi_value <= 0.4 ? "kein Hinweis auf eine MSI" : "Hinweise auf MSI";
			}
			else
			{
				return msi_value <= 0.16 ? "kein Hinweis auf eine MSI" : "Hinweise auf MSI";
			}
		}
		case Attribute::PLOIDY:
			return QString::number(getPloidy(sample_idx), 'f', 2);
		case Attribute::PURITY_CNVS:
			return QString::number(getPurityCnvs(sample_idx), 'f', 2);
		case Attribute::PURITY_HIST:
			return QString::number(getPurityHist(sample_idx), 'f', 2);
		case Attribute::TMB:
			return QString::number(getTmb(sample_idx), 'f', 2);
	}
}

//
//EXPORTCBIOPORTALSTUDY
//

ExportCBioPortalStudy::ExportCBioPortalStudy(CBioPortalExportSettings settings, bool test_db)
	: db_(test_db)
	, settings_(settings)
{
	gatherData();
}


void ExportCBioPortalStudy::gatherData()
{

}

void ExportCBioPortalStudy::exportStudy(const QString& out_folder)
{
	exportStudyFiles(out_folder);
	exportCancerType(out_folder);
}

void ExportCBioPortalStudy::exportStudyFiles(const QString& out_folder)
{
	MetaFile meta_study;
	meta_study.addValue("type_of_cancer", settings_.study.cancer_type);
	meta_study.addValue("name", settings_.study.name);
	meta_study.addValue("cancer_study_identifier", settings_.study.identifier);
	meta_study.addValue("description", settings_.study.description);
	meta_study.addValue("add_global_case_list", "true");
	meta_study.addValue("reference_genome", settings_.study.reference_genome);

	meta_study.store(out_folder + "/meta_study.txt");
}

void ExportCBioPortalStudy::exportCancerType(const QString& out_folder)
{
	MetaFile meta_cancer_type;
	meta_cancer_type.addValue("genetic_alteration_type", "CANCER_TYPE");
	meta_cancer_type.addValue("datatype", "CANCER_TYPE");
	meta_cancer_type.addValue("data_filename", "data_cancer_type.txt" );
	meta_cancer_type.store(out_folder + "/meta_cancer_type.txt");

	QSharedPointer<QFile> out_file = Helper::openFileForWriting(out_folder + "/data_cancer_type.txt");
	QString line = settings_.study.cancer_type + "\t" + settings_.cancer.description + "\t" + settings_.cancer.color + "\t" + settings_.cancer.parent;
	out_file->write(line.toUtf8());
}

void ExportCBioPortalStudy::exportPatientData(const QString &out_folder)
{
	MetaFile meta_clinical_patient;
	meta_clinical_patient.addValue("cancer_study_identifier", settings_.study.identifier);
	meta_clinical_patient.addValue("genetic_alteration_type", "CLINICAL");
	meta_clinical_patient.addValue("datatype", "PATIENT_ATTRIBUTES");
	meta_clinical_patient.addValue("data_filename", "data_clinical_patients.txt");
	meta_clinical_patient.store(out_folder + "/meta_clinical_patients.txt");

	QSharedPointer<QFile> data_patients = Helper::openFileForWriting(out_folder + "/meta_clinical_patients.txt");

	QVector<QStringList> header_lines_patient(5);
	foreach (SampleAttribute attribute, settings_.sample_attributes) {
		header_lines_patient[0] << "Patient Identifier" << "Gender";
		header_lines_patient[1] << "Patient identifier" << "Gender of patient";
		header_lines_patient[2] << "STRING" << "STRING";
		header_lines_patient[3] << "1" << "9";
		header_lines_patient[4] << "PATIENT_IDENTIFIER" << "GENDER";
	}

	foreach (QStringList header, header_lines_patient) {
		data_patients->write("#" + header.join("\t").toUtf8() + "\n");
	}

	QSet<QString> pat_ids;
	foreach (PatientData pd, settings_.patient_map)
	{
		if (pat_ids.contains(pd.patient_id)) continue;

		QStringList line;
		line << pd.patient_id;
		line << pd.gender;

		data_patients->write(line.join("\t").toUtf8() + "\n");
	}
}

void ExportCBioPortalStudy::exportSampleData(const QString &out_folder)
{
	MetaFile meta_clinical_sample;
	meta_clinical_sample.addValue("cancer_study_identifier", settings_.study.identifier);
	meta_clinical_sample.addValue("genetic_alteration_type", "CLINICAL");
	meta_clinical_sample.addValue("datatype", "SAMPLE_ATTRIBUTES");
	meta_clinical_sample.addValue("data_filename", "data_clinical_samples.txt");
	meta_clinical_sample.store(out_folder + "/meta_clinical_samples.txt");


	QSharedPointer<QFile> out_file = Helper::openFileForWriting(out_folder + "/data_clinical_samples.txt");
	//data file headers:
	QVector<QStringList> header_lines_samples(5);
	foreach (const SampleAttribute& attribute, settings_.sample_attributes) {
		header_lines_samples[0] << attribute.name;
		header_lines_samples[1] << attribute.description;
		header_lines_samples[2] << attribute.datatype;
		header_lines_samples[3] << QString::number(attribute.priority);
		header_lines_samples[4] << attribute.db_name;
	}

	foreach (const QStringList& header, header_lines_samples) {
		out_file->write("#" + header.join("\t").toUtf8() + "\n");
	}

	//data file content:
	for (int idx=0; idx < settings_.sample_list.count(); idx++)
	{
		QStringList line;
		foreach (const SampleAttribute& attribute, settings_.sample_attributes)
		{
			line << settings_.getFormatedAttribute(attribute.attribute, idx);
		}
		out_file->write(line.join("\t").toUtf8() + "\n");
	}
}

void ExportCBioPortalStudy::exportSnvs(const QString& out_folder)
{
	MetaFile meta_snv_file;
	meta_snv_file.addValue("cancer_study_identifier", settings_.study.identifier);
	meta_snv_file.addValue("genetic_alteration_type", "MUTATION_EXTENDED");
	meta_snv_file.addValue("datatype", "MAF");
	meta_snv_file.addValue("stable_id", "mutations");
	meta_snv_file.addValue("show_profile_in_analysis_tab", "true");
	meta_snv_file.addValue("profile_description", "Mutation data");
	meta_snv_file.addValue("profile_name", "Mutations");
	meta_snv_file.addValue("data_filename", "data_mutations.txt");
	meta_snv_file.store(out_folder + "/meta_mutations.txt");

	//data file:
	//"minimum" entrez_gene_id can be parsed from hgnc_db_file in GRCh38/share/db/HGNC/
	// gene_symbol,(entrez_gene_id) ncbi_build, chromosome, start, end, variant_classification (missense, silent, inframe_deletion, ...), ref_allele, tum_allele, sample_id, hgvsp_short
}





