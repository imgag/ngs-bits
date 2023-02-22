#include "ExportCBioPortalStudy.h"

#include <QFile>
#include <Helper.h>



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

ExportCBioPortalStudy::ExportCBioPortalStudy()
{

}

void ExportCBioPortalStudy::setStudy(StudyData study)
{
	study_ = study;
}

void ExportCBioPortalStudy::setCancer(CancerData cancer)
{
	cancer_ = cancer;
}

void ExportCBioPortalStudy::gatherData()
{
	//TODO study, cancer, sample



	//TODO validate data
}

void ExportCBioPortalStudy::exportStudy(const QString& out_folder)
{
	exportStudyFiles(out_folder);
	exportCancerType(out_folder);
}

void ExportCBioPortalStudy::exportStudyFiles(const QString& out_folder)
{
	MetaFile meta_study;
	meta_study.addValue("type_of_cancer", study_.cancer_type);
	meta_study.addValue("name", study_.name);
	meta_study.addValue("cancer_study_identifier", study_.identifier);
	meta_study.addValue("description", study_.description);
	meta_study.addValue("add_global_case_list", "true");
	meta_study.addValue("reference_genome", study_.reference_genome);

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
	QString line = study_.cancer_type + "\t" + cancer_.description + "\t" + cancer_.color + "\t" + cancer_.parent;
	out_file->write(line.toUtf8());
}

void ExportCBioPortalStudy::exportClinicalData(const QString &out_folder)
{
	MetaFile meta_clinical_patient;
	meta_clinical_patient.addValue("cancer_study_identifier", study_.identifier);
	meta_clinical_patient.addValue("genetic_alteration_type", "CLINICAL");
	meta_clinical_patient.addValue("datatype", "PATIENT_ATTRIBUTES");
	meta_clinical_patient.addValue("data_filename", "data_clinical_patients.txt");
	meta_clinical_patient.store(out_folder + "/meta_clinical_patients.txt");

	QSharedPointer<QFile> data_patients = Helper::openFileForWriting(out_folder + "/meta_clinical_patients.txt");

	QVector<QStringList> header_lines_patient(5);
	foreach (SampleAttribute attribute, sample_attributes) {
		header_lines_patient[0] << "Patient Identifier" << "Gender";
		header_lines_patient[1] << "Patient identifier" << "Gender of patient";
		header_lines_patient[2] << "STRING" << "STRING";
		header_lines_patient[3] << "1" << "9";
		header_lines_patient[4] << "PATIENT_IDENTIFIER" << "GENDER";
	}

	foreach (QStringList header, header_lines_patient) {
		data_patients->write("#" + header.join("\t").toUtf8() + "\n");
	}

	data_patients->write("");

	MetaFile meta_clinical_sample;
	meta_clinical_sample.addValue("cancer_study_identifier", study_.identifier);
	meta_clinical_sample.addValue("genetic_alteration_type", "CLINICAL");
	meta_clinical_sample.addValue("datatype", "SAMPLE_ATTRIBUTES");
	meta_clinical_sample.addValue("data_filename", "data_clinical_samples.txt");
	meta_clinical_sample.store(out_folder + "/meta_clinical_samples.txt");


	QSharedPointer<QFile> out_file = Helper::openFileForWriting(out_folder + "/data_clinical_samples.txt");
	//data file headers:
	QVector<QStringList> header_lines_samples(5);
	foreach (SampleAttribute attribute, sample_attributes) {
		header_lines_samples[0] << attribute.name;
		header_lines_samples[1] << attribute.description;
		header_lines_samples[2] << attribute.datatype;
		header_lines_samples[3] << QString::number(attribute.priority);
		header_lines_samples[4] << attribute.db_name;
	}

	foreach (QStringList header, header_lines_samples) {
		out_file->write("#" + header.join("\t").toUtf8() + "\n");
	}

	//data file content:



}





