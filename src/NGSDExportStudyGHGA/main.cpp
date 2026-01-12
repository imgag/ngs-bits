#include "ToolBase.h"
#include "NGSD.h"
#include "Helper.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDir>

class ConcreteTool
		: public ToolBase
{
	Q_OBJECT

public:
	ConcreteTool(int& argc, char *argv[])
		: ToolBase(argc, argv)
	{
	}

	virtual void setup()
	{
		setDescription("Exports meta data of a study from NGSD to a JSON format for import into GHGA.");
		addInfile("samples", "TSV file with pseudonym, SAP ID and processed sample ID (and optional sample folder)", false);
		addInfile("data", "JSON file with data that is not contained in NGSD.", false);
		addFlag("include_bam", "Add BAM files to output.");
		addFlag("include_vcf", "Add VCF files to output.");
		addFlag("use_sample_folder", "Use file names from sample folder provided in '-samples'.");
		addFlag("group_analyses", "Combine all samples from one patient into a combined analysis (e. g. for tumor-normal).");

		addOutfile("out", "Output JSON file.", false, false);

		//optional
		addFlag("test", "Test mode: uses the test NGSD");

		changeLog(2025,  2, 7, "Added option to combine analyses.");
		changeLog(2025,  2, 5, "Added option to read files from sample folder.");
		changeLog(2024,  9, 11, "Updated schema to version 2.0.0.");
		changeLog(2023,  1, 31, "Initial implementation (version 0.9.0 of schema).");
	}

	//returns an array from the input JSON
	QStringList getArray(const QJsonObject& data, QString key)
	{
		if (!data.contains(key)) THROW(FileParseException, "JSON input file does not contain key '" + key + "'!");

		QJsonValue value = data.value(key);
		if (!value.isArray()) THROW(FileParseException, "JSON input file does contain key '" + key + "' with invalid type (not array)!");

		QStringList output;
        for (const QJsonValue& entry : value.toArray())
		{
			output << entry.toString();
		}
		return output;
	}

	//returns a string from the input JSON
	QString getString(const QJsonObject& data, QString key)
	{
		if (!data.contains(key)) THROW(FileParseException, "JSON input file does not contain key '" + key + "'!");

		QJsonValue value = data.value(key);
		if (!value.isString()) THROW(FileParseException, "JSON input file does contain key '" + key + "' with invalid type (not string)!");

		return value.toString();
	}

	//returns a string from the input JSON
	int getInteger(const QJsonObject& data, QString key)
	{
		if (!data.contains(key)) THROW(FileParseException, "JSON input file does not contain key '" + key + "'!");

		QJsonValue value = data.value(key);
		if (!value.isDouble()) THROW(FileParseException, "JSON input file does contain key '" + key + "' with invalid type (not integer)!");

		return value.toInt();
	}

	QString systemTypeToExperimentDescription(QString sys_type)
	{
		if (sys_type=="lrGS") return "long-read sequencing";
		if (sys_type=="cfDNA") return "short-read sequencing";
		if (sys_type=="cfDNA (patient-specific)") return "short-read sequencing";
		if (sys_type=="WGS") return "short-read sequencing";
		if (sys_type=="WES") return "short-read sequencing";
		if (sys_type=="RNA") return "short-read sequencing";


		THROW(NotImplementedException, "Unhandled system type '" + sys_type + "' in CV conversion!");
	}

	QString systemTypeToExperimentType(QString sys_type)
	{
		if (sys_type=="WGS") return "WGS";
		if (sys_type=="WES") return "WXS";
		if (sys_type=="RNA") return "Total RNA";
		if (sys_type=="cfDNA") return "cfDNA";
		if (sys_type=="cfDNA (patient-specific)") return "cfDNA";

		THROW(NotImplementedException, "Unhandled system type '" + sys_type + "' in CV conversion!");
	}

	QString systemTypeToLibraryType(QString sys_type)
	{
		if (sys_type=="WGS") return "WGS";
		if (sys_type=="WES") return "WXS";
		if (sys_type=="RNA") return "Total RNA";
		if (sys_type=="cfDNA") return "OTHER";
		if (sys_type=="cfDNA (patient-specific)") return "OTHER";

		THROW(NotImplementedException, "Unhandled system type '" + sys_type + "' in CV conversion!");
	}

	QString systemTypeToDatasetType(QString sys_type)
	{
		if (sys_type=="WGS") return "Whole genome sequencing";
		if (sys_type=="WES") return "Exome sequencing";
		if (sys_type=="RNA") return "Transcriptome profiling by high-throughput sequencing";

		THROW(NotImplementedException, "Unhandled system type '" + sys_type + "' in CV conversion!");
	}

	QString deviceTypeToSequencingInstrument(QString device_type)
	{
		if (device_type=="NextSeq500") return "NEXTSEQ_500";
		if (device_type=="NovaSeq6000") return "ILLUMINA_NOVASEQ_6000";
		if (device_type=="NovaSeqXPlus") return "ILLUMINA_NOVASEQ_X";
		if (device_type=="MGI-2000") return "MGISEQ-2000RS";

		THROW(NotImplementedException, "Unhandled device type '" + device_type + "' in CV conversion!");
	}

	QString flowcellTypeToflowcellType(QString flowcell_type)
	{
		if (flowcell_type=="Illumina NovaSeq S2") return "ILLUMINA_NOVA_SEQ_S2";
		if (flowcell_type=="Illumina NovaSeq S4") return "ILLUMINA_NOVA_SEQ_S4";
		if (flowcell_type=="Illumina NovaSeq S1") return "OTHER";
		if (flowcell_type=="Illumina NovaSeq SP") return "OTHER";
		if (flowcell_type=="n/a") return "OTHER";

		THROW(NotImplementedException, "Unhandled flowcell type '" + flowcell_type + "' in CV conversion!");
	}

	QString sampleTypeToSampleType(QString sample_type, bool is_ffpe)
	{
		if (!is_ffpe && (sample_type=="DNA" || sample_type=="DNA (amplicon)" || sample_type=="DNA (native)")) return "GENOMIC_DNA";
		if (!is_ffpe && sample_type=="RNA") return "TOTAL_RNA";
		if (!is_ffpe && sample_type=="cfDNA") return "CF_DNA";
		if (!is_ffpe && sample_type=="cfDNA (patient-specific)") return "CF_DNA";

		if (is_ffpe && (sample_type=="DNA" || sample_type=="DNA (amplicon)" || sample_type=="DNA (native)")) return "FFPE_DNA";
		if (is_ffpe && sample_type=="RNA") return "FFPE_TOTAL_RNA";

		THROW(NotImplementedException, "Unhandled sample type '" + sample_type + "' " + (is_ffpe ? "(FFPE)" : "") + " in CV conversion!");
	}

	QString diseaseStatusToCaseControl(QString disease_status)
	{
		if (disease_status == "Affected") return "CASE";
		if (disease_status == "Unaffected") return "CONTROL";
		if (disease_status == "Unclear") return "OTHER";
		if (disease_status == "n/a") return "UNKNOWN";

		THROW(NotImplementedException, "Unhandled disease status '" + disease_status + "' in CV conversion!");
	}

	QString sampleGenderToSex(QString gender)
	{

		if (gender=="female") return "female";
		if (gender=="male") return "male";
		if (gender=="n/a") return "unknown";

		THROW(NotImplementedException, "Unhandled gender '" + gender + "' in CV conversion!");
	}

	QString sampleAncestryToAncestry(QString ancestry)
	{
		if (ancestry=="AFR") return "African";
		if (ancestry=="EUR") return "European";
		if (ancestry=="SAS") return " South Asian";
		if (ancestry=="EAS") return "East Asian";
		if (ancestry=="ADMIXED/UNKNOWN" || ancestry=="") return "";

		THROW(NotImplementedException, "Unhandled ancestry '" + ancestry + "' in CV conversion!");
	}

	QStringList getFilesFromFolder(const QString& folder, QString file_extension, bool allow_multiple=true, QString substring_filter = "")
	{
		QStringList files = QDir(folder).entryList(QStringList() << "*." + file_extension, QDir::Files);

		// filter by substring
		if (substring_filter != "")
		{
			QStringList filtered_files;
			foreach (const QString& file, files)
			{
				if (file.contains(substring_filter)) filtered_files << file;
			}
			files = filtered_files;
		}

		if (files.isEmpty()) THROW(ArgumentException, "No matching file in folder '" + folder + "' found for extension '" + file_extension + "'!");
		if (!allow_multiple && files.size() > 1) THROW(ArgumentException, "Multiple matching file in folder '" + folder + "' found for extension '" + file_extension + "'!");

		return files;
	}

	//Processed sample helper struct
	struct PSData
	{
		QString ps_id;
		QString name;
		QString pseudonym;
		SampleData s_info;
		ProcessedSampleData ps_info;
		PhenotypeList phenotypes;
		QString patient_id;
		QString ps_folder;
		QStringList research_data_files;
		QStringList processed_data_files;
	};

	//Common data helper struct
	struct CommonData
	{
		//general data
		QString version;
		bool test_mode;
		bool include_vcf;
		bool include_bam;
		bool use_sample_folder;
		bool group_analyses;

		//study
		QString study_name;
		QString study_description;
		QStringList study_types;
		QStringList study_affilitions;

		//analysis
		QString analysis_type;
		QString analysis_description; //"cfDNA data analysis using megSAP: https://github.com/imgag/megSAP"
		QString workflow_name;
		QString workflow_version;
		QString workflow_doi;


		//data access
		QString dac_email;
		QString dac_organization;
		QString dap_text;
		QString dap_url;
		QString dap_term; //attention, this is a CV!
		QString dap_id; //attention, this is a CV!
		QStringList dap_modifier_terms; //attention, this is a CV!
		QStringList dap_modifier_ids; //attention, this is a CV!

		//publication
		QString publication_title;
		QString publication_abstract;
		QString publication_author;
		int publication_year = -1;
		QString publication_journal;
		QString publication_doi;

		//processed samples
		QList<PSData> ps_list;
	};


	void addAnalyses(QJsonObject& parent, const CommonData& data)
	{
		QJsonArray array;
		if (data.include_bam || data.include_vcf) //only neccesary if processed sample files are included
		{

			if(data.group_analyses) //combined analysis
			{
				//combine research files
				QMap<QString,QStringList> research_files;
				foreach(const PSData& ps_data, data.ps_list)
				{
					if (research_files.contains(ps_data.patient_id))
					{
						QStringList files = research_files.value(ps_data.patient_id) + ps_data.research_data_files;
						research_files.insert(ps_data.patient_id, files);
					}
					else
					{
						research_files.insert(ps_data.patient_id, ps_data.research_data_files);
					}

				}

				//create analyses
				foreach(const QString& patient_id, research_files.keys())
				{
					QJsonObject obj;

					obj.insert("analysis_method", "ANAM_" + patient_id);
					obj.insert("title", "ANA_" + patient_id);
					obj.insert("description", data.analysis_description);
					obj.insert("type", data.analysis_type); //e.g. "cfDNA"
					//optional
					//obj.insert("ega_accession", QJsonValue());
					obj.insert("research_data_files", QJsonArray::fromStringList(research_files.value(patient_id)));
					obj.insert("alias", "ANA_" + patient_id);
					array.append(obj);
				}



			}
			else //single sample analysis
			{
				foreach(const PSData& ps_data, data.ps_list)
				{
					QJsonObject obj;

					obj.insert("analysis_method", "ANAM_" + ps_data.pseudonym);
					obj.insert("title", "ANA_" + ps_data.pseudonym);
					obj.insert("description", data.analysis_description);
					obj.insert("type", data.analysis_type); //e.g. "cfDNA"
					//optional
					//obj.insert("ega_accession", QJsonValue());
					obj.insert("research_data_files", QJsonArray::fromStringList(ps_data.research_data_files));
					obj.insert("alias", "ANA_" + ps_data.pseudonym);
					array.append(obj);
				}
			}

		}


		parent.insert("analyses", array);
	}

	void addAnalysesMethods(QJsonObject& parent, const CommonData& data)
	{
		QJsonArray array;
		QSet<QString> processed_patient_ids;

		if (data.include_bam || data.include_vcf) //only neccesary if processed sample files are included
		{

			foreach(const PSData& ps_data, data.ps_list)
			{
				QJsonObject obj;
				QString method_name = ps_data.pseudonym;
				if (data.group_analyses)
				{
					method_name = ps_data.patient_id;
					if (processed_patient_ids.contains(method_name)) continue; //already processed
					processed_patient_ids.insert(method_name);
				}

				obj.insert("name", "ANAM_" + method_name);
				obj.insert("description", data.analysis_description);
				obj.insert("type", data.analysis_type);
				obj.insert("workflow_name", "megSAP");
				obj.insert("workflow_version", data.workflow_version);
				obj.insert("workflow_repository", "https://github.com/imgag/megSAP");
				obj.insert("workflow_doi", data.workflow_doi);
				//optional:
				//obj.insert("workflow_tasks", "Pipeline?");
				//obj.insert("parameters", QJsonArray());
				//obj.insert("software_versions", QJsonArray());
				obj.insert("alias", "ANAM_" + method_name);

				array.append(obj);
			}
		}
		parent.insert("analysis_methods", array);
	}

	void addAnalysesMethodSupportingFiles(QJsonObject& parent)
	{
		QJsonArray array;

		//optional:
		parent.insert("analysis_method_supporting_files", array);
	}

	void addDataAccessCommittee(QJsonObject& parent, const CommonData& data)
	{
		QJsonObject obj;
		obj.insert("email", data.dac_email);
		obj.insert("institute", data.dac_organization);
		//optional:
		//obj.insert("ega_accession", QJsonObject());
		obj.insert("alias", data.dac_email);

		parent.insert("data_access_committees", QJsonArray() << obj);

	}

	void addDataAccessPolicy(QJsonObject& parent, const CommonData& data)
	{
		QJsonArray array;

		QJsonObject obj;
		obj.insert("name", "Data access policy for study "+data.study_name);
		obj.insert("description", "Data access policy for study "+data.study_name);
		obj.insert("policy_text", data.dap_text);
		obj.insert("policy_url", data.dap_url);
		obj.insert("data_use_permission_term", data.dap_term);
		obj.insert("data_use_permission_id", data.dap_id);
		obj.insert("data_use_modifier_terms", QJsonArray::fromStringList(QStringList() << data.dap_modifier_terms));
		obj.insert("data_use_modifier_ids", QJsonArray::fromStringList(QStringList() << data.dap_modifier_ids));
		//optional
		//obj.insert("ega_accession", QJsonObject());
		obj.insert("data_access_committee", data.dac_email);
		obj.insert("alias", "DAP_"+data.study_name);

		array.append(obj);

		parent.insert("data_access_policies", array);
	}

	void addDatasets(QJsonObject& parent, const CommonData& data)
	{
		QJsonArray array;

		QJsonObject obj;
		obj.insert("title", "Dataset for " + data.study_name);
		obj.insert("description", "Dataset for " + data.study_name);
		obj.insert("types", QJsonArray() << "Sequencing data");
		//optional
		//obj.insert("ega_accession", QJsonObject());
		obj.insert("data_access_policy", "DAP_" + data.study_name);
		obj.insert("study", data.study_name);
		obj.insert("alias", "DS_" + data.study_name);

		array.append(obj);

		parent.insert("datasets", array);
	}

	void addExperiments(QJsonObject& parent, const CommonData& data)
	{
		QJsonArray experiments;

		foreach(const PSData& ps_data, data.ps_list)
		{
			QJsonObject obj;
			obj.insert("experiment_method", "EXPM_" + ps_data.pseudonym);
			obj.insert("title", "EXP_" + ps_data.pseudonym);
			obj.insert("description", systemTypeToExperimentDescription(ps_data.ps_info.processing_system_type));
			obj.insert("type", systemTypeToExperimentType(ps_data.ps_info.processing_system_type));
			//optional
			//obj.insert("ega_accession", QJsonValue());
			obj.insert("sample", "SAM_" + ps_data.pseudonym);
			//optional
			//obj.insert("attributes", QJsonArray());
			obj.insert("alias", "EXP_" + ps_data.pseudonym);

			experiments.append(obj);
		}

		parent.insert("experiments", experiments);
	}

	void addExperimentMethods(QJsonObject& parent, const CommonData& data, NGSD& db)
	{
		QJsonArray experiment_methods;

		foreach(const PSData& ps_data, data.ps_list)
		{

			QJsonObject obj;
			obj.insert("name", "EXPM_" + ps_data.pseudonym);
			obj.insert("description", ps_data.ps_info.processing_system);
			obj.insert("type", ps_data.ps_info.processing_system);
			obj.insert("library_type", systemTypeToLibraryType(ps_data.ps_info.processing_system_type));
			obj.insert("library_selection_methods", QJsonArray() << "OTHER");
			obj.insert("library_preparation", "unspecified");
			//optional
			//obj.insert("library_preparation_kit_retail_name", ps_data.ps_info.processing_system);
			//obj.insert("library_preparation_kit_manufacturer", QJsonValue());
			//obj.insert("primer", QJsonValue());
			//obj.insert("end_bias", QJsonValue());
			//obj.insert("target_regions", QJsonArray() << "unspecified");
			//obj.insert("rnaseq_strandedness", QJsonValue());
			QString device_type = db.getValue("SELECT d.type FROM device d, sequencing_run r WHERE r.device_id=d.id AND r.name='" + ps_data.ps_info.run_name + "'").toString();
			obj.insert("instrument_model", deviceTypeToSequencingInstrument(device_type));
			//optional:
			//obj.insert("sequencing_center", QJsonValue());
			//obj.insert("sequencing_read_length", QJsonValue());
			obj.insert("sequencing_layout", "PE");
			//optional:
			//obj.insert("target_coverage", QJsonValue());
			QString fc_id = db.getValue("SELECT fcid FROM sequencing_run WHERE name='" + ps_data.ps_info.run_name + "'").toString();
			obj.insert("flow_cell_id", fc_id);
			QString fc_type = db.getValue("SELECT flowcell_type FROM sequencing_run WHERE name='" + ps_data.ps_info.run_name + "'").toString();
			obj.insert("flow_cell_type", flowcellTypeToflowcellType(fc_type));
			//optional:
			//obj.insert("sample_barcode_read", QJsonValue());
			//obj.insert("ega_accession", QJsonValue());
			//obj.insert("attributes", QJsonArray());
			obj.insert("alias", "EXPM_" + ps_data.pseudonym);

			experiment_methods.append(obj);
		}

		parent.insert("experiment_methods", experiment_methods);
	}

	void addExperimentMethodSupportingFiles(QJsonObject& parent) //, const CommonData& data, NGSD& db)
	{
		QJsonArray experiment_method_supporting_files;

		//optional:
		parent.insert("experiment_method_supporting_files", experiment_method_supporting_files);
	}

	void addIndividuals(QJsonObject& parent, const CommonData& data)
	{
		QJsonArray array;

		QSet<QString> processed_ids;

		foreach(const PSData& ps_data, data.ps_list)
		{
			//skip if patient is already processed
			if (processed_ids.contains(ps_data.patient_id)) continue;
			QJsonObject obj;
			//optional:
			//obj.insert("phenotypic_features_terms", QJsonValue());
			//obj.insert("phenotypic_features_ids", QJsonValue());
			//obj.insert("diagnosis_ids", QJsonValue());
			//obj.insert("diagnosis_terms", QJsonValue());
			obj.insert("sex", sampleGenderToSex(ps_data.s_info.gender).toUpper());
			//optional:
			//obj.insert("geographical_region_term", QJsonValue());
			//obj.insert("geographical_region_id", QJsonValue())
			//obj.insert("ancestry_terms", QJsonValue());
			//obj.insert("ancestry_ids", QJsonValue());
			obj.insert("alias", ps_data.patient_id);
			array.append(obj);
			processed_ids.insert(ps_data.patient_id);
		}

		parent.insert("individuals", array);
	}

	void addIndividualSupportingFiles(QJsonObject& parent) //, const CommonData& data, NGSD& db)
	{
		QJsonArray array;

		//optional:
		parent.insert("individual_supporting_files", array);
	}

	void addProcessDataFiles(QJsonObject& parent, CommonData& data)
	{
		QJsonArray array;

		for (int i = 0; i < data.ps_list.size(); ++i)
		{
			PSData& ps_data = data.ps_list[i];
			//add BAM (if required)
			if (data.include_bam)
			{
				QJsonObject obj;
				obj.insert("format", "BAM");
				obj.insert("analysis", "ANA_" + ps_data.pseudonym);
				if (data.use_sample_folder)
				{
					QString bam_file_name = getFilesFromFolder(ps_data.ps_folder, "bam", false, "").at(0);
					obj.insert("name", bam_file_name);
				}
				else
				{
					obj.insert("name", ps_data.pseudonym + ".bam");
				}
				obj.insert("dataset", "DS_" + data.study_name);
				//optional:
				//obj.insert("ega_accession", QJsonValue());
				obj.insert("included_in_submission", true);
				obj.insert("alias", "BAM_" + ps_data.pseudonym);
				ps_data.processed_data_files.append("BAM_" + ps_data.pseudonym);

				array.append(obj);
			}

			//add VCF (if required)
			if (data.include_vcf)
			{
				QJsonObject obj;
				obj.insert("format", "VCF");
				obj.insert("analysis", "ANA_" + ps_data.pseudonym);

				if (data.use_sample_folder)
				{
					QString vcf_file_name = getFilesFromFolder(ps_data.ps_folder, "vcf", false, "").at(0);
					obj.insert("name", vcf_file_name);
				}
				else
				{
					obj.insert("name", ps_data.pseudonym + ".vcf");
				}
				obj.insert("dataset", "DS_" + data.study_name);
				//optional
				//obj.insert("ega_accession", QJsonValue());
				obj.insert("included_in_submission", true);
				obj.insert("alias", "VCF_" + ps_data.pseudonym);
				ps_data.processed_data_files.append("VCF_" + ps_data.pseudonym);

				array.append(obj);
			}
		}

		parent.insert("process_data_files", array);
	}

	void addPublications(QJsonObject& parent, const CommonData& data)
	{
		QJsonObject obj;

		if (!data.publication_title.trimmed().isEmpty())
		{
			obj.insert("study", data.study_name);
			obj.insert("title", data.publication_title);

			//optional
			if (!data.publication_abstract.trimmed().isEmpty()) obj.insert("abstract", data.publication_abstract);
			if (!data.publication_author.trimmed().isEmpty()) obj.insert("author", data.publication_author);
			if (data.publication_year != -1) obj.insert("year", data.publication_year);
			if (!data.publication_journal.trimmed().isEmpty()) obj.insert("journal", data.publication_journal);

			obj.insert("doi", data.publication_doi);
			//optional
			//obj.insert("xref", QJsonObject());
			obj.insert("alias", "PUB_" + data.study_name);
			parent.insert("publications", QJsonArray() << obj);
		}
		else
		{
			parent.insert("publications", QJsonArray());
		}

	}

	void addResearchDataFiles(QJsonObject& parent, CommonData& data)
	{
		QJsonArray array;

		for (int i = 0; i < data.ps_list.size(); ++i)
		{
			PSData& ps_data = data.ps_list[i];
			QJsonObject fastq_template;
			fastq_template.insert("format", "FASTQ");
			fastq_template.insert("technical_replicate", 1);
			//optional:
			//fastq_template.insert("sequencing_lane_id",   QJsonValue());
			fastq_template.insert("experiments",  QJsonArray() << "EXP_" + ps_data.pseudonym);
			fastq_template.insert("dataset", "DS_" + data.study_name);
			//optional:
			//obj.insert("ega_accession", QJsonValue());
			fastq_template.insert("included_in_submission", true);

			//add FASTQ
			if (data.use_sample_folder)
			{
				//get R1 & R2 files
				QStringList fastq_files_r1 = getFilesFromFolder(ps_data.ps_folder, "fastq.gz", true, "R1");
				QStringList fastq_files_r2 = getFilesFromFolder(ps_data.ps_folder, "fastq.gz", true, "R2");

				//R1
				int iterator = 1;
				foreach (const QString& fastq_file, fastq_files_r1)
				{
					QJsonObject obj = fastq_template;
					obj.insert("name", fastq_file);
					obj.insert("alias", "FASTQ_R1_" + QString::number(iterator).rightJustified(3, '0') + "_" + ps_data.pseudonym);
					ps_data.research_data_files.append("FASTQ_R1_" + QString::number(iterator).rightJustified(3, '0') + "_" + ps_data.pseudonym);
					array.append(obj);
					iterator++;
				}

				//R2
				iterator=1;
				foreach (const QString& fastq_file, fastq_files_r2)
				{
					QJsonObject obj = fastq_template;
					obj.insert("name", fastq_file);
					obj.insert("alias", "FASTQ_R2_" + QString::number(iterator).rightJustified(3, '0') + "_" + ps_data.pseudonym);
					ps_data.research_data_files.append("FASTQ_R2_" + QString::number(iterator).rightJustified(3, '0') + "_" + ps_data.pseudonym);
					array.append(obj);
					iterator++;
				}

			}
			else
			{
				//R1
				QJsonObject obj = fastq_template;
				obj.insert("name", ps_data.pseudonym + "_R1.fastq.gz");
				obj.insert("alias", "FASTQ_R1_" + ps_data.pseudonym);
				ps_data.research_data_files.append("FASTQ_R1_" + ps_data.pseudonym);
				array.append(obj);
				//R2
				obj = fastq_template;
				obj.insert("name", ps_data.pseudonym + "_R2.fastq.gz");
				obj.insert("alias", "FASTQ_R2_" + ps_data.pseudonym);
				ps_data.research_data_files.append("FASTQ_R2_" + ps_data.pseudonym);
				array.append(obj);
			}
		}

		parent.insert("research_data_files", array);
	}

	void addSamples(QJsonObject& parent, const CommonData& data)
	{
		QJsonArray array;

		foreach(const PSData& ps_data, data.ps_list)
		{
			//library
			QJsonObject obj;

			obj.insert("individual", ps_data.patient_id);
			obj.insert("name", "SAM_" + ps_data.pseudonym);
			obj.insert("type", sampleTypeToSampleType(ps_data.s_info.type, ps_data.s_info.is_ffpe));
			//optional:
			//obj.insert("biological_replicate", QJsonValue());
			obj.insert("description", (ps_data.s_info.is_tumor)?("Tumor sample of "+ps_data.patient_id):("Germline sample of "+ps_data.patient_id));
			//optional:
			//obj.insert("storage", QJsonValue());
			//obj.insert("disease_or_healthy", QJsonValue());
			obj.insert("case_control_status", diseaseStatusToCaseControl(ps_data.s_info.disease_status));
			//obj.insert("ega_accession", QJsonValue());
			//obj.insert("xref", QJsonValue());
			//obj.insert("biospecimen_name", QJsonValue());
			//obj.insert("biospecimen_type", QJsonValue());
			//obj.insert("biospecimen_description", QJsonValue());
			obj.insert("biospecimen_age_at_sampling", "UNKNOWN");
			//obj.insert("biospecimen_vital_status_at_sampling", QJsonValue());
			obj.insert("biospecimen_tissue_term", "UNKNOWN");
			obj.insert("biospecimen_tissue_id", "UNKNOWN");
			//obj.insert("biospecimen_isolation", QJsonValue());
			//obj.insert("biospecimen_storage", QJsonValue());
			//obj.insert("attributes", QJsonArray());
			obj.insert("alias", "SAM_" + ps_data.pseudonym);
			array.append(obj);
		}

		parent.insert("samples", array);
	}

	void addStudy(QJsonObject& parent, const CommonData& data)
	{
		QJsonObject obj;
		obj.insert("title", data.study_name);
		obj.insert("description", data.study_description);
		obj.insert("types", QJsonArray::fromStringList(data.study_types));
		//optional:
		//obj.insert("ega_accession", QJsonValue());
		obj.insert("affiliations", QJsonArray::fromStringList(data.study_affilitions));
		//optional:
		//obj.insert("attributes", QJsonArray());
		obj.insert("alias", data.study_name);

		parent.insert("studies",  QJsonArray() << obj);
	}

	virtual void main()
	{
		//init
		QTextStream stream(stdout);
		QJsonParseError parse_error;
		QJsonDocument data_doc = QJsonDocument::fromJson(Helper::loadTextFile(getInfile("data")).join("\n").toUtf8(), &parse_error);
		if (data_doc.isNull()) THROW(FileParseException, "Could not read '" + getInfile("data") + "': " + parse_error.errorString());

		QJsonObject data_obj = data_doc.object();

		CommonData data;
		data.version = "2.0.0";
		data.test_mode = getFlag("test");
		data.include_vcf = getFlag("include_vcf");
		data.include_bam = getFlag("include_bam");
		data.use_sample_folder = getFlag("use_sample_folder");
		data.group_analyses = getFlag("group_analyses");

		data.study_name = getString(data_obj, "study");
		data.study_description = getString(data_obj, "study_description");
		data.study_types = getArray(data_obj, "study_types"); // https://docs.ghga.de/metadata/data_dictionary/Study/#types
		data.study_affilitions = getArray(data_obj, "study_affiliations");

		data.analysis_type = getString(data_obj, "analysis_type");
		data.analysis_description = getString(data_obj, "analysis_description");
		data.workflow_name = getString(data_obj, "workflow_name");
		data.workflow_version = getString(data_obj, "workflow_version");
		data.workflow_doi = getString(data_obj, "workflow_doi");

		data.dac_email = getString(data_obj, "data_access_committee_email");
		data.dac_organization = getString(data_obj, "data_access_committee_organization");

		data.dap_text = getString(data_obj, "data_access_policy_text");
		data.dap_url = getString(data_obj, "data_access_policy_url");
		data.dap_term = getString(data_obj, "data_use_permission_term"); //see https://www.ebi.ac.uk/ols4/
		data.dap_id = getString(data_obj, "data_use_permission_id");
		data.dap_modifier_terms = getArray(data_obj, "data_use_modifier_terms");
		data.dap_modifier_ids = getArray(data_obj, "data_use_modifier_ids");

		data.publication_title = getString(data_obj, "publication_title");
		data.publication_abstract = getString(data_obj, "publication_abstract");
		data.publication_author = getString(data_obj, "publication_author");
		data.publication_year = getInteger(data_obj, "publication_year");
		data.publication_journal = getString(data_obj, "publication_journal");
		data.publication_doi = getString(data_obj, "publication_doi");


		NGSD db(data.test_mode);

		//load processed samples to export
        stream << QDateTime::currentDateTime().toString("dd.MM.yyyy hh:mm:ss") << " Loading processed sample list..." << Qt::endl;
		auto handle = Helper::openFileForReading(getInfile("samples"), false);
		while(!handle->atEnd())
		{
			QByteArray line = handle->readLine().trimmed();
			if (line.isEmpty() || line[0]=='#') continue;

			QByteArrayList parts = line.split('\t');
			if (data.use_sample_folder)
			{
				//allow only 5 columns
				if (parts.count()!=5) THROW(FileParseException, "Invalid sample line: " + line);
			}
			else
			{
				//allow only 3/4 columns
				if (parts.count()<3) THROW(FileParseException, "Invalid sample line: " + line);
				if (parts.count()>4) THROW(FileParseException, "Invalid sample line: " + line);
			}


			QString pseudonym = parts[0];

			QString ps = parts[2].trimmed();
			if(ps.startsWith("Skipped - ")) continue; // samples which were skipped in the preparation dialog

			//patient id
			QString patient_id = parts[0]; // use pseudonym if no patient id is provided
			if (parts.count()>3) patient_id = parts[3];

			QString ps_id = db.processedSampleId(ps);

			QString s_id = db.sampleId(ps);

			QString ps_folder;
			if (data.use_sample_folder)
			{
				ps_folder = parts[4].trimmed();
				if (ps_folder.isEmpty()) THROW(FileParseException, "No sample folder set in line: " + line);
			}

			data.ps_list << PSData{ps_id, ps, pseudonym, db.getSampleData(s_id), db.getProcessedSampleData(ps_id), db.samplePhenotypes(s_id), patient_id, ps_folder, QStringList(), QStringList()};
		}
        stream << QDateTime::currentDateTime().toString("dd.MM.yyyy hh:mm:ss") << " Writing JSON for " << data.ps_list.count() << " samples..." << Qt::endl;

		//create JSON
		QJsonObject root;

		//get files
		addProcessDataFiles(root, data); //may change data, so have to be run first
		addResearchDataFiles(root, data); //may change data, so have to be run first

		addAnalyses(root, data);
		addAnalysesMethods(root, data);
		addAnalysesMethodSupportingFiles(root);
		addDataAccessCommittee(root, data);
		addDataAccessPolicy(root, data);
		addDatasets(root, data);
		addExperiments(root, data);
		addExperimentMethods(root, data, db);
		addExperimentMethodSupportingFiles(root);
		addIndividuals(root, data);
		addIndividualSupportingFiles(root);
		addPublications(root, data);
		addSamples(root, data);
		addStudy(root, data);

		//store JSON
		QJsonDocument doc(root);
		Helper::storeTextFile(getOutfile("out"), QStringList() << doc.toJson());

        stream << QDateTime::currentDateTime().toString("dd.MM.yyyy hh:mm:ss") << " Done" << Qt::endl;
	}
};

#include "main.moc"

int main(int argc, char *argv[])
{
	ConcreteTool tool(argc, argv);
	return tool.execute();
}
