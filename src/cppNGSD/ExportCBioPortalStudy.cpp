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

	sample_attributes = other.sample_attributes;
//	patient_map = other.patient_map;
}

void CBioPortalExportSettings::addSample(SomaticReportSettings settings, SampleFiles files)
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
	return ps_qc.value("QC:2000126", true).asString().toInt();
}

float CBioPortalExportSettings::getTmb(int sample_idx)
{
	QCCollection ps_qc = db_.getQCData(ps_ids[sample_idx]);
	return  ps_qc.value("QC:2000053",true).asString().toFloat();
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
//			return patient_map[sample_list[sample_idx]].patient_id;
			return s_data[sample_idx].patient_identifier;

		case Attribute::PROCESSING_SYSTEM:
			return getProcessingSystem(sample_idx);
		case Attribute::CLINICAL_PHENOTYPE:
			return getClinicalPhenotype(sample_idx);
		case Attribute::COMMENT:
			return getComments(sample_idx).replace("\n", ", ").replace("\t", " ");
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

void ExportCBioPortalStudy::exportStudy(const QString& out_folder)
{
	exportStudyFiles(out_folder);
	exportCancerType(out_folder);
	exportPatientData(out_folder);
	exportSampleData(out_folder);
	exportSnvs(out_folder);
	exportCnvs(out_folder);
//	exportSvs(out_folder);
	exportCaseList(out_folder);
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
	header_lines_patient[0] << "Patient Identifier" << "Gender";
	header_lines_patient[1] << "Patient identifier" << "Gender of patient";
	header_lines_patient[2] << "STRING" << "STRING";
	header_lines_patient[3] << "1" << "9";
	header_lines_patient[4] << "PATIENT_ID" << "GENDER";


	for (int i=0; i<4; i++) {
		const QStringList& header = header_lines_patient[i];
		data_patients->write("#" + header.join("\t").toUtf8() + "\n");
	}
	data_patients->write(header_lines_patient[4].join("\t").toUtf8() + "\n");

	QSet<QString> pat_ids;
	for(int i=0; i<settings_.sample_list.count(); i++)
	{
		if (pat_ids.contains(settings_.s_data[i].patient_identifier)) continue;

		QStringList line;
		line << settings_.s_data[i].patient_identifier;
		line << settings_.s_data[i].gender;

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
	meta_snv_file.addValue("namespaces", "TEST");
	meta_snv_file.addValue("data_filename", "data_mutations.txt");
	meta_snv_file.store(out_folder + "/meta_mutations.txt");

	//data file:
	// gene_symbol,(entrez_gene_id) ncbi_build, chromosome, start, end, variant_classification (missense, silent, inframe_deletion, ...), ref_allele, tum_allele, sample_id, hgvsp_short

	QSharedPointer<QFile> out_file = Helper::openFileForWriting(out_folder + "/data_mutations.txt");

	QByteArrayList columns;
	columns << "Hugo_Symbol" /*<< "Entrez_Gene_Id"*/ << "NCBI_Build" << "Chromosome" << "Start_Position" << "End_Position" << "Variant_Classification" << "Reference_Allele" << "Tumor_Seq_Allele2" << "Tumor_Sample_Barcode"
			<< "HGVSp_Short" << "t_alt_count" << "t_ref_count" << "n_alt_count" << "n_ref_count" << "TEST.string";

	out_file->write(columns.join("\t") + "\n");
	for(int idx=0; idx < settings_.sample_list.count(); idx++)
	{
		VariantList vl_somatic;
		vl_somatic.load(settings_.sample_files[idx].gsvar_somatic);
		//filter
		vl_somatic = SomaticReportSettings::filterVariants(vl_somatic, settings_.report_settings[idx]);

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

		if ( ! transcript.isValid())
		{
			transcript = transcripts[0];
			consequence = hgvs_annotator.annotate(transcripts[0], var);
		}

		line_parts << var.annotations()[idx_gene_anno];
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


		line_parts << "Test-Variant_number: " + QByteArray::number(i+1);
		out_file->write(line_parts.join("\t") + "\n");
	}
}

void ExportCBioPortalStudy::exportCnvs(const QString& out_folder)
{
	MetaFile meta_cnv;
	meta_cnv.addValue("cancer_study_identifier", settings_.study.identifier);
	meta_cnv.addValue("genetic_alteration_type", "COPY_NUMBER_ALTERATION");
	meta_cnv.addValue("datatype", "DISCRETE");
	meta_cnv.addValue("stable_id", "cna");
	meta_cnv.addValue("profile_name", "Copy Number Variants");
	meta_cnv.addValue("show_profile_in_analysis_tab", "true");
	meta_cnv.addValue("profile_description", "Values: -2 = homozygous deletion; -1 = hemizygous deletion; 0 = neutral / no change; 1 = gain; 2 = high level amplification.");
	meta_cnv.addValue("data_filename", "data_CNV.txt");
	meta_cnv.store(out_folder + "/meta_CNV.txt");


	QSharedPointer<QFile> out_file = Helper::openFileForWriting(out_folder + "/data_CNV.txt");

	QByteArrayList columns;
	columns << "Hugo_Symbol" /*<< "Entrez_Gene_Id"*/;

	QList<QMap<QByteArray, int>> data;

	GeneSet all_genes;

	for(int idx=0; idx < settings_.sample_list.count(); idx++)
	{
		columns << settings_.getSampleId(idx).toUtf8();

		CnvList cnvs;
		cnvs.load(settings_.sample_files[idx].clincnv_file);
		//filter
		cnvs = SomaticReportSettings::filterCnvs(cnvs, settings_.report_settings[idx]);

		data.append(QMap<QByteArray, int>());

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

		qDebug() << settings_.getSampleId(idx) << ": found genes with CNVs - " << data[idx].count();
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

void ExportCBioPortalStudy::exportSvs(const QString& out_folder)
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




