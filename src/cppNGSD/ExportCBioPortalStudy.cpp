#include "ExportCBioPortalStudy.h"

#include <QFile>
#include <Helper.h>
#include "Exceptions.h"
#include "SomaticReportHelper.h"
#include "VariantHgvsAnnotator.h"

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
	s_data = other.s_data;
	s_mtb_data = other.s_mtb_data;

	sample_attributes = other.sample_attributes;
//	patient_map = other.patient_map;
}

void CBioPortalExportSettings::addSample(SomaticReportSettings settings, SampleFiles files, SampleMTBmetadata mtb_data)
{
	QString name = settings.tumor_ps;

	if (sample_list.contains(name)) THROW(ArgumentException, "Given sample: '" + name + "' was already added to the sample list of the export.");

	sample_list.append(name);
	sample_files.append(files);
	//TODO check if all necessary infos were entered
	report_settings.append(settings);

	QString ps_id = db_.processedSampleId(name);
	ps_ids.append(ps_id);
	ps_data.append(db_.getProcessedSampleData(ps_id));
	s_data.append(db_.getSampleData(db_.sampleId(name)));
	s_mtb_data.append(mtb_data);
}

double CBioPortalExportSettings::getMsiStatus(int sample_idx)
{
	double msi = std::numeric_limits<double>::quiet_NaN();
	if (VersatileFile(sample_files[sample_idx].msi_file).exists())
	{
		TSVFileStream msi_file(sample_files[sample_idx].msi_file);
		//Use step wise difference (-> stored in the first line of MSI status file) for MSI status
		QByteArrayList data = msi_file.readLine();
		qDebug() << "MSI string:" << data;
		if(data.count() > 0) msi = data[1].toDouble();
	}
	return msi;
}

float CBioPortalExportSettings::getPloidy(int sample_idx)
{
	if (VersatileFile(sample_files[sample_idx].clincnv_file).exists())
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
	QList<SampleDiseaseInfo> disease_details = db_.getSampleDiseaseInfo(db_.sampleId(sample_list[sample_idx]), "tumor fraction");
	if (disease_details.count() >1)
	{
		THROW(ArgumentException, "Sample '" + sample_list[sample_idx] + "' has more than one entry for tumor fraction in the disease details.");
	}
	else if (disease_details.count() == 0)
	{
		qDebug() << "HIST Purity (disease info) not found in DB";
		return std::numeric_limits<double>::quiet_NaN();
	}

	qDebug() << "HIST Purity (disease info): " << disease_details[0].disease_info;

	return disease_details[0].disease_info.toDouble() / 100.0;
}

float CBioPortalExportSettings::getPurityCnvs(int sample_idx)
{
	if (VersatileFile(sample_files[sample_idx].clincnv_file).exists())
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
	if (ps_qc.contains("QC:2000126", true))
	{
		return ps_qc.value("QC:2000126", true).asInt();
	}

	return -1;
}

float CBioPortalExportSettings::getTmb(int sample_idx)
{
	QCCollection ps_qc = db_.getQCData(ps_ids[sample_idx]);
	if (ps_qc.contains("QC:2000053", true))
	{
		return ps_qc.value("QC:2000053", true).asDouble();
	}
	return  -1;
}

QStringList CBioPortalExportSettings::getIcd10(int sample_idx)
{
	QStringList icd10;
	QList<SampleDiseaseInfo> disease_details = db_.getSampleDiseaseInfo(db_.sampleId(sample_list[sample_idx]), "ICD10 code");
	foreach (const auto& dd, disease_details)
	{
		icd10.append(dd.disease_info);
	}
	return icd10;
}

QStringList CBioPortalExportSettings::getHpoTerms(int sample_idx)
{
	QStringList hpo;
	QList<SampleDiseaseInfo> disease_details = db_.getSampleDiseaseInfo(db_.sampleId(sample_list[sample_idx]), "HPO term id");
	foreach (const auto& dd, disease_details)
	{
		hpo.append(dd.disease_info);
	}
	return hpo;
}

QString CBioPortalExportSettings::getClinicalPhenotype(int sample_idx)
{
	QStringList clin_phenotype;
	QList<SampleDiseaseInfo> disease_details = db_.getSampleDiseaseInfo(db_.sampleId(sample_list[sample_idx]), "clinical phenotype (free text)");
	foreach (const auto& dd, disease_details)
	{
		clin_phenotype.append(dd.disease_info);
	}
	return clin_phenotype.join(", ");
}

QString CBioPortalExportSettings::getSampleId(int sample_idx)
{
	return report_settings[sample_idx].tumor_ps;
}

QString CBioPortalExportSettings::getPatientId(int sample_idx)
{
	return s_data[sample_idx].patient_identifier;
}

QString CBioPortalExportSettings::getGenomeBuild(int sample_idx)
{
	int sys_id = db_.processingSystemIdFromProcessedSample(sample_list[sample_idx]);
	return db_.getProcessingSystemData(sys_id).genome;
}

QString CBioPortalExportSettings::getFormatedAttribute(Attribute att, int sample_idx)
{
	switch (att) {
		case Attribute::SAMPLE_ID:
			return getSampleId(sample_idx);
		case Attribute::PATIENT_ID:
			return QString::number(s_mtb_data[sample_idx].sap_id);

		case Attribute::PROCESSING_SYSTEM:
			return getProcessingSystem(sample_idx);
		case Attribute::CLINICAL_PHENOTYPE:
			return getClinicalPhenotype(sample_idx);
		case Attribute::COMMENT:
			return getComments(sample_idx).replace("\n", ", ").replace("\t", " ");
		case Attribute::HPO_TERMS:
			return getHpoTerms(sample_idx).join(", ");
		case Attribute::HRD_SCORE:
		{
			int hrd = getHrdScore(sample_idx);
			if (hrd != -1)
			{
				return QString::number(hrd);
			}
			else
			{
				return "";
			}
		}
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
		{
			double tmb = getTmb(sample_idx);
			if(tmb != -1)
			{
				return QString::number(getTmb(sample_idx), 'f', 2);
			}
			else
			{
				return "";
			}
		}
		case Attribute::GENLAB_PAT_ID:
			return s_data[sample_idx].patient_identifier;
		case Attribute::MTB_CASE_ID:
			return s_mtb_data[sample_idx].mtb_case_id;

	}
	THROW(ArgumentException, "Unknown Attribute value!");
}

//
//EXPORTCBIOPORTALSTUDY
//

ExportCBioPortalStudy::ExportCBioPortalStudy(CBioPortalExportSettings settings, bool test_db)
	: db_(test_db)
	, settings_(settings)
{
	gatherData();

	int count = settings_.sample_list.count();

	if (settings_.sample_files.count() != count) THROW(ArgumentException, "SampleFile data not set correctly. Number is not equal to samples.");
	if (settings_.report_settings.count() != count) THROW(ArgumentException, "ReportSettings data not set correctly. Number is not equal to samples.");
	if (settings_.ps_data.count() != count) THROW(ArgumentException, "ps_data not set correctly. Number is not equal to samples.");
	if (settings_.ps_ids.count() != count) THROW(ArgumentException, "ps_ids not set correctly. Number is not equal to samples.");
	if (settings_.s_data.count() != count) THROW(ArgumentException, "s_data not set correctly. Number is not equal to samples.");
}


void ExportCBioPortalStudy::gatherData()
{

}

void ExportCBioPortalStudy::exportStudy(const QString& out_folder, bool debug)
{
	QDir folder(out_folder);
	if (! folder.exists())
	{
		QDir().mkdir(out_folder);
	}

	exportStudyFiles(out_folder);
	exportCancerType(out_folder);
	exportPatientData(out_folder);
	exportSampleData(out_folder);
	qDebug() << "Exporting SNVs.";
	exportSnvs(out_folder, debug);
	qDebug() << "Exporting CNVs.";
	exportCnvs(out_folder, debug);
	qDebug() << "Exporting Fusion.";
	exportFusions(out_folder, debug);
//	exportSvs(out_folder);
	qDebug() << "Exporting Caselists.";
	exportCaseList(out_folder);
	qDebug() << "DONE!";
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
	out_file->write("\n");
}

void ExportCBioPortalStudy::exportCaseList(const QString& out_folder)
{
	QString case_list_dir = out_folder + "/case_lists/";
	QDir().mkdir(case_list_dir);

	//All
//	QSharedPointer<QFile> cases_all = Helper::openFileForWriting(case_list_dir + "/cases_all.txt");
//	cases_all->write("cancer_study_identifier: " + settings_.study.identifier.toUtf8() + "\n");
//	cases_all->write("stable_id: " + settings_.study.identifier.toUtf8() + "_all\n");
//	cases_all->write("case_list_category: all_cases_in_study\n");
//	cases_all->write("case_list_name: All Cases\n");
//	cases_all->write("case_list_description: All cases of study (" + QByteArray::number(settings_.sample_list.count()) + " samples)\n");
//	cases_all->write("case_list_ids: ");

//	QByteArrayList sample_ids;
//	for (int idx=0; idx < settings_.sample_list.count(); idx++)
//	{
//		sample_ids.append(settings_.getSampleId(idx).toUtf8());
//	}

//	cases_all->write(sample_ids.join("\t"));
//	cases_all->write("\n");

	//sequenced -> all samples profiled for mutations:

	QSharedPointer<QFile> cases_sequenced = Helper::openFileForWriting(case_list_dir + "/cases_sequenced.txt");
	cases_sequenced->write("cancer_study_identifier: " + settings_.study.identifier.toUtf8() + "\n");
	cases_sequenced->write("stable_id: " + settings_.study.identifier.toUtf8() + "_sequenced\n");
	cases_sequenced->write("case_list_category: all_cases_with_mutation_data\n");
	cases_sequenced->write("case_list_name: Sequenced Tumors\n");
	cases_sequenced->write("case_list_description: All sequenced samples (" + QByteArray::number(settings_.sample_list.count()) + " samples)\n");
	cases_sequenced->write("case_list_ids: ");

	QByteArrayList sample_ids;
	for (int idx=0; idx < settings_.sample_list.count(); idx++)
	{
		sample_ids.append(settings_.getSampleId(idx).toUtf8());
	}

	cases_sequenced->write(sample_ids.join("\t"));
	cases_sequenced->write("\n");
	cases_sequenced->close();

	QSharedPointer<QFile> cases_with_cnvs = Helper::openFileForWriting(case_list_dir + "/cases_cnv.txt");
	cases_with_cnvs->write("cancer_study_identifier: " + settings_.study.identifier.toUtf8() + "\n");
	cases_with_cnvs->write("stable_id: " + settings_.study.identifier.toUtf8() + "_cna\n");
	cases_with_cnvs->write("case_list_category: all_cases_with_cna_data\n");
	cases_with_cnvs->write("case_list_name: Tumors with CNVs\n");
	cases_with_cnvs->write("case_list_description: All samples with CNV data (" + QByteArray::number(settings_.sample_list.count()) + " samples)\n");
	cases_with_cnvs->write("case_list_ids: ");

	sample_ids.clear();
	for (int idx=0; idx < settings_.sample_list.count(); idx++)
	{
		VersatileFile clin_cnv(settings_.sample_files[idx].clincnv_file);
		if (clin_cnv.exists())
		{
			sample_ids.append(settings_.getSampleId(idx).toUtf8());
		}
	}

	cases_with_cnvs->write(sample_ids.join("\t"));
	cases_with_cnvs->write("\n");
	cases_with_cnvs->close();

	QSharedPointer<QFile> cases_with_svs = Helper::openFileForWriting(case_list_dir + "/cases_sv.txt");
	cases_with_svs->write("cancer_study_identifier: " + settings_.study.identifier.toUtf8() + "\n");
	cases_with_svs->write("stable_id: " + settings_.study.identifier.toUtf8() + "_sv\n");
	cases_with_svs->write("case_list_category: all_cases_with_sv_data\n");
	cases_with_svs->write("case_list_name: Tumors with SVs\n");
	cases_with_svs->write("case_list_description: All samples with SV data (" + QByteArray::number(settings_.sample_list.count()) + " samples)\n");
	cases_with_svs->write("case_list_ids: ");

	sample_ids.clear();
	for (int idx=0; idx < settings_.sample_list.count(); idx++)
	{
		VersatileFile sv_file(settings_.sample_files[idx].sv_file);
		if (sv_file.exists())
		{
			sample_ids.append(settings_.getSampleId(idx).toUtf8());
		}
	}

	cases_with_svs->write(sample_ids.join("\t") + "\n");
	cases_with_cnvs->close();

	//TODO other?
}

void ExportCBioPortalStudy::exportPatientData(const QString &out_folder)
{
	MetaFile meta_clinical_patient;
	meta_clinical_patient.addValue("cancer_study_identifier", settings_.study.identifier);
	meta_clinical_patient.addValue("genetic_alteration_type", "CLINICAL");
	meta_clinical_patient.addValue("datatype", "PATIENT_ATTRIBUTES");
	meta_clinical_patient.addValue("data_filename", "data_clinical_patients.txt");
	meta_clinical_patient.store(out_folder + "/meta_clinical_patients.txt");

	QSharedPointer<QFile> data_patients = Helper::openFileForWriting(out_folder + "/data_clinical_patients.txt");

	QVector<QStringList> header_lines_patient(5);
	header_lines_patient[0] << "Patient Identifier" << "Gender" << "Genlab Patient ID" << "MTB case ID";
	header_lines_patient[1] << "Patient identifier" << "Gender of patient" << "Patient identifier of genlab" << "ID of the MTB case";
	header_lines_patient[2] << "STRING" << "STRING" << "STRING" << "STRING";
	header_lines_patient[3] << "1" << "9" << "9" << "9";
	header_lines_patient[4] << "PATIENT_ID" << "GENDER" << "GENLAB_PAT_ID" << "MTB_CASE_ID";

	for (int i=0; i<4; i++) {
		const QStringList& header = header_lines_patient[i];
		data_patients->write("#" + header.join("\t").toUtf8() + "\n");
	}
	data_patients->write(header_lines_patient[4].join("\t").toUtf8() + "\n");

	QSet<QString> pat_ids;
	for(int i=0; i<settings_.sample_list.count(); i++)
	{
		QString pat_id = settings_.getFormatedAttribute(Attribute::PATIENT_ID, i);
		if (pat_ids.contains(pat_id)) continue;

		QStringList line;
		line << pat_id;
		line << settings_.s_data[i].gender;
		line << settings_.getFormatedAttribute(Attribute::GENLAB_PAT_ID, i);
		line << settings_.getFormatedAttribute(Attribute::MTB_CASE_ID, i);

		pat_ids.insert(pat_id);

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

	for (int i=0; i<4; i++) {
		const QStringList& header = header_lines_samples[i];
		out_file->write("#" + header.join("\t").toUtf8() + "\n");
	}
	out_file->write(header_lines_samples[4].join("\t").toUtf8() + "\n");

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

void ExportCBioPortalStudy::exportSnvs(const QString& out_folder, bool debug)
{
	QTextStream out(stdout);
	MetaFile meta_snv_file;
	meta_snv_file.addValue("cancer_study_identifier", settings_.study.identifier);
	meta_snv_file.addValue("genetic_alteration_type", "MUTATION_EXTENDED");
	meta_snv_file.addValue("datatype", "MAF");
	meta_snv_file.addValue("stable_id", "mutations");
	meta_snv_file.addValue("show_profile_in_analysis_tab", "true");
	meta_snv_file.addValue("profile_description", "Mutation data");
	meta_snv_file.addValue("profile_name", "Mutations");
	meta_snv_file.addValue("namespaces", "annotation");
	meta_snv_file.addValue("data_filename", "data_mutations.txt");
	meta_snv_file.store(out_folder + "/meta_mutations.txt");

	//data file:
	// gene_symbol,(entrez_gene_id) ncbi_build, chromosome, start, end, variant_classification (missense, silent, inframe_deletion, ...), ref_allele, tum_allele, sample_id, hgvsp_short

	QSharedPointer<QFile> out_file = Helper::openFileForWriting(out_folder + "/data_mutations.txt");

	QByteArrayList columns;
	columns << "Hugo_Symbol" /*<< "Entrez_Gene_Id"*/ << "NCBI_Build" << "Chromosome" << "Start_Position" << "End_Position" << "Variant_Classification" << "Reference_Allele" << "Tumor_Seq_Allele2" << "Tumor_Sample_Barcode"
			<< "HGVSp_Short" << "t_alt_count" << "t_ref_count" << "n_alt_count" << "n_ref_count" << "ANNOTATION.VICC";

	out_file->write(columns.join("\t") + "\n");
	for(int idx=0; idx < settings_.sample_list.count(); idx++)
	{
		if(debug)
		{
			out << "exporting SNVs sample: " << settings_.sample_list[idx];
		}
		VariantList vl_somatic;
		vl_somatic.load(settings_.sample_files[idx].gsvar_somatic);

		//filter
		bool throw_errors = false;
		vl_somatic = SomaticReportSettings::filterVariants(vl_somatic, settings_.report_settings[idx], throw_errors);
		if (debug)
		{
			QString errors;
			for (int i=0;i<settings_.report_settings[idx].filters.count(); i++ ) {
				errors += settings_.report_settings[idx].filters.errors(i).join("; ");
			}

			if (errors.length() > 0)
			{
				out << " - Errors When filtering: " << errors;
			}

			out << "\n"; //flush intended

		}


		writeSnvVariants(out_file, vl_somatic, idx);
	}
}

void ExportCBioPortalStudy::writeSnvVariants(QSharedPointer<QFile> out_file, VariantList filtered_vl, int sample_idx)
{
	//TODO add switch for somatic vs Germline?

	QByteArray build = settings_.getGenomeBuild(sample_idx).toUtf8();
	QByteArray sample_id = settings_.getSampleId(sample_idx).toUtf8();

	FastaFileIndex genome_idx(Settings::string("reference_genome"));
	VariantHgvsAnnotator hgvs_annotator(genome_idx);

	int idx_gene_anno = filtered_vl.annotationIndexByName("gene");
	int idx_co_sp_anno = filtered_vl.annotationIndexByName("coding_and_splicing");
	int idx_tumor_dp = filtered_vl.annotationIndexByName("tumor_dp");
	int idx_tumor_af = filtered_vl.annotationIndexByName("tumor_af");
	int idx_normal_dp = filtered_vl.annotationIndexByName("normal_dp");
	int idx_normal_af = filtered_vl.annotationIndexByName("normal_af");

	for (int i=0; i<filtered_vl.count(); i++)
	{
		QByteArrayList line_parts;
		Variant var = filtered_vl[i];

		TranscriptList transcripts  = db_.transcriptsOverlapping(var.chr(), var.start(), var.end(), 5000);
		transcripts.sortByRelevance();

		GeneSet genes = GeneSet::createFromText(var.annotations()[idx_gene_anno], ',');

		//remove transcripts of close genes
		TranscriptList remove;
		foreach(auto trans, transcripts)
		{
			if (! genes.contains(trans.gene()))
			{
				remove.append(trans);
			}
		}

		foreach(auto trans, remove)
		{
			transcripts.removeAll(trans);
		}


		Transcript transcript;
		VariantConsequence consequence;
		//find prefered transcript and annotate:
		foreach(const Transcript& trans, transcripts)
		{
			if (trans.isPreferredTranscript())
			{
				transcript = trans;
				consequence = hgvs_annotator.annotate(trans, var);
				break;
			}
		}

		if ( ! transcript.isValid() && transcripts.count() > 0)
		{
			transcript = transcripts[0];
			consequence = hgvs_annotator.annotate(transcripts[0], var);
		}

		if (transcript.gene() == "") continue;

		line_parts << transcript.gene(); //var.annotations()[idx_gene_anno];
		line_parts << build;

		line_parts << var.chr().strNormalized(true);
		line_parts << QByteArray::number(var.start());
		line_parts << QByteArray::number(var.end());
		line_parts << formatVariantClassification(transcript, var.annotations()[idx_co_sp_anno]); // variant classification
		line_parts << var.ref();
		line_parts << var.obs();
		line_parts << sample_id;

		line_parts << consequence.hgvs_p;


		int tumor_alt_count = static_cast<int>(std::round(var.annotations()[idx_tumor_dp].toDouble() * var.annotations()[idx_tumor_af].toDouble()));
		int tumor_ref_count = var.annotations()[idx_tumor_dp].toInt() - tumor_alt_count;
		line_parts << QByteArray::number(tumor_alt_count);
		line_parts << QByteArray::number(tumor_ref_count);

		int normal_alt_count = static_cast<int>(std::round(var.annotations()[idx_normal_dp].toDouble() * var.annotations()[idx_normal_af].toDouble()));
		int normal_ref_count = var.annotations()[idx_normal_dp].toInt() - tumor_alt_count;
		line_parts << QByteArray::number(normal_alt_count);
		line_parts << QByteArray::number(normal_ref_count);



		SomaticViccData vicc = db_.getSomaticViccData(var,false);
		if (vicc.isValid())
		{
			QString vicc_class = SomaticVariantInterpreter::viccScoreAsString(vicc);
			line_parts << vicc_class.toUtf8();
		}
		else
		{
			line_parts << "";
		}
		out_file->write(line_parts.join("\t") + "\n");
	}
}

void ExportCBioPortalStudy::exportCnvs(const QString& out_folder, bool debug)
{
	MetaFile meta_cnv;
	meta_cnv.addValue("cancer_study_identifier", settings_.study.identifier);
	meta_cnv.addValue("genetic_alteration_type", "COPY_NUMBER_ALTERATION");
	meta_cnv.addValue("datatype", "DISCRETE");
	meta_cnv.addValue("stable_id", "cna");
	meta_cnv.addValue("profile_name", "Copy Number Variants");
	meta_cnv.addValue("show_profile_in_analysis_tab", "true");
	meta_cnv.addValue("namespaces", "annotation");
	meta_cnv.addValue("profile_description", "Values: -2 = homozygous deletion; -1 = hemizygous deletion; 0 = neutral / no change; 1 = gain; 2 = high level amplification.");
	meta_cnv.addValue("data_filename", "data_CNV.txt");
	meta_cnv.store(out_folder + "/meta_CNV.txt");

	QTextStream out(stdout);
	QSharedPointer<QFile> out_file = Helper::openFileForWriting(out_folder + "/data_CNV.txt");

	QByteArrayList columns;
	columns << "Hugo_Symbol" << "Entrez_Gene_Id";

	QList<QMap<QByteArray, int>> data;

	GeneSet all_genes;

	for(int idx=0; idx < settings_.sample_list.count(); idx++)
	{
		if (debug)
		{
			out << "CNV sample: " << settings_.sample_list[idx] << "\n";
		}
		columns << settings_.getSampleId(idx).toUtf8();
		data.append(QMap<QByteArray, int>());

		if (! VersatileFile(settings_.sample_files[idx].clincnv_file).exists())
		{
			out << "No clincnv file! skipping... " << settings_.sample_list[idx] << "\n";
			continue;
		}

		CnvList cnvs;
		cnvs.load(settings_.sample_files[idx].clincnv_file);
		//filter
		cnvs = SomaticReportSettings::filterCnvs(cnvs, settings_.report_settings[idx]);

		for (int idx_var=0; idx_var < cnvs.count(); idx_var++)
		{
			CopyNumberVariant cnv = cnvs[idx_var];

			all_genes.insert(cnv.genes());

			foreach (QByteArray gene, cnv.genes())
			{

				if (cnv.copyNumber(cnvs.annotationHeaders()) == 0)
				{
					data[idx].insert(gene, -2);
				}
				else if (cnv.copyNumber(cnvs.annotationHeaders()) == 1)
				{
					data[idx].insert(gene, -1);
				}
				else if (cnv.copyNumber(cnvs.annotationHeaders()) == 2)
				{
					data[idx].insert(gene, 0);
				}
				else if (cnv.copyNumber(cnvs.annotationHeaders()) < 5)
				{
					data[idx].insert(gene, 1);
				}
				else
				{
					data[idx].insert(gene, 2);
				}

			}
		}
		if (debug)
		{
			out << settings_.getSampleId(idx) << ": found genes with CNVs - " << data[idx].count() << "\n";
		}
	}

	out_file->write(columns.join("\t") + "\n");

	foreach (QByteArray gene, all_genes)
	{
		QByteArrayList line_parts;
		line_parts << gene;

		for(int idx=0; idx < settings_.sample_list.count(); idx++)
		{
			if (data[idx].contains(gene))
			{
				line_parts << QByteArray::number(data[idx][gene]);
			}
			else
			{
				line_parts << "0";
			}
		}
		out_file->write(line_parts.join("\t") + "\n");

	}
}

void ExportCBioPortalStudy::exportSvs(const QString& out_folder, bool /*debug*/)
{
	MetaFile meta_svs;
	meta_svs.addValue("cancer_study_identifier", settings_.study.identifier);
	meta_svs.addValue("genetic_alteration_type", "STRUCTURAL_VARIANT");
	meta_svs.addValue("datatype", "SV");
	meta_svs.addValue("stable_id", "structural_variants");
	meta_svs.addValue("profile_name", "Structural Variants");
	meta_svs.addValue("show_profile_in_analysis_tab", "true");
	meta_svs.addValue("profile_description", "Structural variants called with manta.");
	meta_svs.addValue("data_filename", "data_SV.txt");
	meta_svs.store(out_folder + "/meta_SV.txt");


	QSharedPointer<QFile> out_file = Helper::openFileForWriting(out_folder + "/data_SV.txt");

	QByteArrayList columns;
	columns << "Sample_ID" << "SV_Status" << "Event_Info" << "Site1_Hugo_Symbol"/* << "Site1_Region_Number" << "Site1_Region" */<< "Site1_Chromosome" << "Site1_Position" << "Site2_Hugo_Symbol" /*<< "Site2_Region_Number" << "Site2_Region"*/ << "Site2_Chromosome" << "Site2_Position" << "Class" << "Tumor_Split_Read_Count" << "Tumor_Paired_End_Read_Count" << "Breakpoint_Type" << "SV_Length" << "Normal_Paired_End_Read_Count" << "Normal_Split_Read_Count";

	out_file->write(columns.join("\t")+ "\n");
	for(int idx=0; idx < settings_.sample_list.count(); idx++)
	{
		BedpeFile bedpe;
		bedpe.load(settings_.sample_files[idx].sv_file);
		int idx_tumor_src = bedpe.annotationIndexByName("TUM_SR_ALT");
		int idx_tumor_prc = bedpe.annotationIndexByName("TUM_PR_ALT");

		int idx_normal_src = bedpe.annotationIndexByName("NOR_SR_ALT");
		int idx_normal_prc = bedpe.annotationIndexByName("NOR_PR_ALT");

		int idx_flags = bedpe.annotationIndexByName("FLAGS");


		// TODO write filter? In somatic report settings?

		for(int idx_var=0; idx_var<bedpe.count(); idx_var++)
		{
			const BedpeLine& var = bedpe[idx_var];

			if (! var.chr1().isNonSpecial() || ! var.chr2().isNonSpecial()) continue;

			QByteArrayList line_parts;
			// << "Sample_Id" << "SV_Status"
			line_parts << settings_.getSampleId(idx).toUtf8();
			line_parts << "SOMATIC";
			line_parts << "Event info";

			//<< "Site1_Hugo_Symbol" << "Site1_Region_Number" << "Site1_Region" << "Site1_Chromosome" << "Site1_Position"

			int pos1 = (var.start1() + var.end1())/2;

			GeneSet pos1_genes = db_.genesOverlapping(var.chr1(), pos1, pos1+1);

			QByteArray gene1;
			if (pos1_genes.count() != 1)
			{
				gene1 = "";
			}
			else
			{
				gene1 = pos1_genes[0];
			}

			line_parts << gene1;

			line_parts << var.chr1().strNormalized(true);
			line_parts << QByteArray::number((var.start1() + var.end1())/2);


			//<< "Site2_Hugo_Symbol" << "Site2_Region_Number" << "Site2_Region" << "Site2_Chromosome" << "Site2_Position"

			int pos2 = (var.start2() + var.end2())/2;

			GeneSet pos2_genes = db_.genesOverlapping(var.chr2(), pos2, pos2+1);

			QByteArray gene2;
			if (pos2_genes.count() != 1)
			{
				gene2 = "";
			}
			else
			{
				gene2 = pos2_genes[0];
			}
			line_parts << gene2;
			line_parts << var.chr2().strNormalized(true);
			line_parts << QByteArray::number((var.start2() + var.end2())/2);


			// << "Class" << "Tumor_Split_Read_Count" << "Tumor_Paired_End_Read_Count"
			// Class
			line_parts << StructuratVariantTypeToStringLong(var.type());
			line_parts << var.annotations()[idx_tumor_src];
			line_parts << var.annotations()[idx_tumor_prc];


			//<< "Breakpoint_Type" << "SV_Length" << "Normal_Paired_End_Read_Count" << "Normal_Split_Read_Count"

			if (var.annotations()[idx_flags].contains("IMPRECISE"))
			{
				line_parts << "IMPRECISE";
			}
			else
			{
				line_parts << "PRECISE";
			}

			line_parts << QByteArray::number(var.size());
			line_parts << var.annotations()[idx_normal_prc];
			line_parts << var.annotations()[idx_normal_src];

			out_file->write(line_parts.join("\t")+ "\n");
		}
	}
}


void ExportCBioPortalStudy::exportFusions(const QString& out_folder, bool /*debug*/)
{
	MetaFile meta_svs;
	meta_svs.addValue("cancer_study_identifier", settings_.study.identifier);
	meta_svs.addValue("genetic_alteration_type", "STRUCTURAL_VARIANT");
	meta_svs.addValue("datatype", "SV");
	meta_svs.addValue("stable_id", "structural_variants");
	meta_svs.addValue("profile_name", "Fusions");
	meta_svs.addValue("show_profile_in_analysis_tab", "true");
	meta_svs.addValue("profile_description", "Fusions called in the RNA with arriba.");
	meta_svs.addValue("data_filename", "data_FU.txt");
	meta_svs.store(out_folder + "/meta_FU.txt");


	QSharedPointer<QFile> out_file = Helper::openFileForWriting(out_folder + "/data_FU.txt");

	QByteArrayList columns;
//	columns << "Sample_ID" << "SV_Status" << "Event_Info" << "Site1_Hugo_Symbol" << "Site1_Ensembl_Transcript_Id" << "Site1_Exon" << "Site1_Region_Number" << "Site1_Region" << "Site1_Chromosome" << "Site1_Position" << "Site2_Hugo_Symbol" << "Site2_Ensembl_Transcript_Id" << "Site2_Exon" << "Site2_Region_Number" << "Site2_Region" << "Site2_Chromosome" << "Site2_Position" << "Class" << "Tumor_Split_Read_Count" << "Tumor_Paired_End_Read_Count" << "Breakpoint_Type";
	columns << "Sample_ID" << "NCBI_Build" << "SV_Status" << "Event_Info" << "Site1_Hugo_Symbol" << "Site1_Ensembl_Transcript_Id" << "Site1_Exon" << "Site1_Chromosome" << "Site1_Position" << "Site2_Hugo_Symbol" << "Site2_Ensembl_Transcript_Id" << "Site2_Exon" << "Site2_Chromosome" << "Site2_Position" << "Site2_Effect_On_Frame" << "Class" << "Tumor_Split_Read_Count" << "Tumor_Paired_End_Read_Count" << "Breakpoint_Type";

	out_file->write(columns.join("\t")+ "\n");
	for(int idx=0; idx < settings_.sample_list.count(); idx++)
	{
		if ( ! VersatileFile(settings_.sample_files[idx].rna_fusions).exists()) continue;

		TsvFile fusions;
		fusions.load(settings_.sample_files[idx].rna_fusions);

		int idx_pos1 = fusions.columnIndex("breakpoint1");
		int idx_pos2 = fusions.columnIndex("breakpoint2");

		int idx_gene1 = fusions.columnIndex("gene1");
		int idx_gene2 = fusions.columnIndex("gene2");

		int idx_reading_frame = fusions.columnIndex("reading_frame");

//		int idx_site1 = fusions.columnIndex("site1");
//		int idx_site2 = fusions.columnIndex("site2");

		int idx_class = fusions.columnIndex("type");
		int idx_split_read1 = fusions.columnIndex("split_reads1");
		int idx_split_read2 = fusions.columnIndex("split_reads2");
		int idx_read_pairs = fusions.columnIndex("discordant_mates");

//		int idx_cov1 = fusions.columnIndex("coverage1");
//		int idx_cov2 = fusions.columnIndex("coverage2");

//		int idx_transcript1_id = fusions.columnIndex("transcript_id1");
//		int idx_transcript2_id = fusions.columnIndex("transcript_id2");

		for(int idx_var=0; idx_var<fusions.rowCount(); idx_var++)
		{
			QByteArrayList line_parts;
			// << "Sample_Id" << "SV_Status"
			line_parts << settings_.getSampleId(idx).toUtf8();
			line_parts << "GRCh38";
			line_parts << "SOMATIC";
			line_parts << "Event info";


			QStringList var_parts = fusions.row(idx_var);

			//<< "Site1_Hugo_Symbol" << "Site1_Ensembl_Transcript_Id"  << "Site1_Exon" << "Site1_Region_Number" << "Site1_Region" << "Site1_Chromosome" << "Site1_Position"


			QByteArray pos1 = var_parts[idx_pos1].split(':')[1].toUtf8();
			QByteArray chr1 = var_parts[idx_pos1].split(':')[0].toUtf8();

			line_parts << var_parts[idx_gene1].toUtf8();

			line_parts << "";
			line_parts << "";

//			line_parts << var_parts[idx_site1].toUtf8();

			line_parts << chr1;
			line_parts << pos1;


			//<< "Site2_Hugo_Symbol" << "Site2_Ensembl_Transcript_Id" << "Site2_Exon" << "Site2_Region_Number" << "Site2_Region" << "Site2_Chromosome" << "Site2_Position"

			QByteArray pos2 = var_parts[idx_pos2].split(':')[1].toUtf8();
			QByteArray chr2 = var_parts[idx_pos2].split(':')[0].toUtf8();

			line_parts << var_parts[idx_gene2].toUtf8();
			line_parts << "";
			line_parts << "";

			line_parts << chr2;
			line_parts << pos2;

			//reading frame:
			QString frame_effect = var_parts[idx_reading_frame];
			if (frame_effect.contains("in-frame"))
			{
				line_parts << "in-frame";
			}
			else if (frame_effect.contains("out-of-frame"))
			{
				line_parts << "frameshift";
			}
			else
			{
				line_parts << "";
			}

			// << "Class" << "Tumor_Split_Read_Count" << "Tumor_Paired_End_Read_Count"
			// Class

			QString type = var_parts[idx_class];
			if (type.contains("inversion"))
			{
				line_parts << "Inversion";
			}
			else if (type.contains("deletion"))
			{
				line_parts << "Deletion";
			}
			else if (type.contains("translocation"))
			{
				line_parts << "Translocation";
			}
			else if (type.contains("insertion"))
			{
				line_parts << "Insertion";
			}
			else if (type.contains("duplication"))
			{
				line_parts << "Duplication";
			}
			else
			{
				line_parts << "";
			}

			line_parts << QByteArray::number(var_parts[idx_split_read1].toInt() + var_parts[idx_split_read2].toInt());
			line_parts << var_parts[idx_read_pairs].toUtf8();


			//<< "Breakpoint_Type"

			line_parts << "PRECISE";

			out_file->write(line_parts.join("\t")+ "\n");
		}
	}

}



QByteArray ExportCBioPortalStudy::StructuratVariantTypeToStringLong(const StructuralVariantType& type)
{
	switch (type)
	{
		case StructuralVariantType::DEL:
			return "Deletion";
		case StructuralVariantType::DUP:
			return "Duplication";
		case StructuralVariantType::INS:
			return "Insertion";
		case StructuralVariantType::INV:
			return "Inversion";
		case StructuralVariantType::BND:
			return "Translocation";
		case StructuralVariantType::UNKNOWN:
			THROW(ArgumentException, "StructuralVariantType::UNKNOWN can only be used for the default constructor.");
		default:
			THROW(NotImplementedException, "Invalid StructuralVariantType!");
	}
}


QByteArray ExportCBioPortalStudy::formatVariantClassification(const Transcript& trans, const QByteArray& coding_splicing)
{
	QByteArrayList annotated = (coding_splicing + ",").split(',');

	foreach (QByteArray trans_anno, annotated)
	{
		if (trans_anno.isEmpty()) continue;

		QByteArrayList parts = trans_anno.split(':');

		if (parts[1] == trans.nameWithVersion())
		{
			return parts[2];
		}
	}

	return annotated[0].split(':')[2];

}




