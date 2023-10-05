#include "ToolBase.h"
#include "NGSD.h"
#include "Helper.h"
#include <QJsonDocument>
#include <QJsonArray>

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
		addInfile("samples", "TSV file with pseudonym, SAP ID and processed sample ID", false);
		addInfile("data", "JSON file with data that is not contained in NGSD.", false);
		addOutfile("out", "Output JSON file.", false, false);

		//optional
		addFlag("test", "Test mode: uses the test NGSD, does not calculate size/checksum of BAMs, ...");

		changeLog(2023,  1, 31, "Initial implementation (version 0.9.0 of schema).");
	}

	//returns an array from the input JSON
	QStringList getArray(const QJsonObject& data, QString key)
	{
		if (!data.contains(key)) THROW(FileParseException, "JSON input file does not contain key '" + key + "'!");

		QJsonValue value = data.value(key);
		if (!value.isArray()) THROW(FileParseException, "JSON input file does contain key '" + key + "' with invalid type (not array)!");

		QStringList output;
		foreach(const QJsonValue& entry, value.toArray())
		{
			output << entry.toString();
		}
		return output;
	}

	//returns an array from the input JSON
	QString getString(const QJsonObject& data, QString key)
	{
		if (!data.contains(key)) THROW(FileParseException, "JSON input file does not contain key '" + key + "'!");

		QJsonValue value = data.value(key);
		if (!value.isString()) THROW(FileParseException, "JSON input file does contain key '" + key + "' with invalid type (not string)!");

		return value.toString();
	}

	QString systemTypeToExperimentType(QString sys_type)
	{
		if (sys_type=="WGS") return "WGS";
		if (sys_type=="WES") return "WXS";
		if (sys_type=="RNA") return "Total RNA";

		THROW(NotImplementedException, "Unhandled system type '" + sys_type + "' in CV conversion!");
	}

	QString systemTypeToLibraryType(QString sys_type)
	{
		if (sys_type=="WGS") return "WGS";
		if (sys_type=="WES") return "WXS";
		if (sys_type=="RNA") return "Total RNA";

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
		if (device_type=="NextSeq500") return "Illumina NextSeq 500";
		if (device_type=="NovaSeq6000") return "Illumina NovaSeq 6000";
		if (device_type=="NovaSeqXPlus") return "Illumina NovaSeq X Plus";

		THROW(NotImplementedException, "Unhandled device type '" + device_type + "' in CV conversion!");
	}

	QString flowcellTypeToflowcellType(QString flowcell_type)
	{
		if (flowcell_type=="Illumina NovaSeq S2") return "Illumina NovaSeq S2";
		if (flowcell_type=="Illumina NovaSeq S4") return "Illumina NovaSeq S4";

		THROW(NotImplementedException, "Unhandled flowcell type '" + flowcell_type + "' in CV conversion!");
	}

	QString sampleTypeToSampleType(QString sample_type, bool is_ffpe)
	{
		if (!is_ffpe && (sample_type=="DNA" || sample_type=="DNA (amplicon)" || sample_type=="DNA (native)")) return "genomic DNA";
		if (!is_ffpe && sample_type=="RNA") return "total RNA";
		if (!is_ffpe && sample_type=="cfDNA") return "cfDNA";

		if (is_ffpe && (sample_type=="DNA" || sample_type=="DNA (amplicon)" || sample_type=="DNA (native)")) return "FFPE DNA";
		if (is_ffpe && sample_type=="RNA") return "FFPE total RNA";

		THROW(NotImplementedException, "Unhandled sample type '" + sample_type + "' " + (is_ffpe ? "(FFPE)" : "") + " in CV conversion!");
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

	//Processed sample helper struct
	struct PSData
	{
		QString ps_id;
		QString name;
		QString bam; //required
		QString vcf; //optional - skipped when empty
		QString pseudonym;
		SampleData s_info;
		ProcessedSampleData ps_info;
		PhenotypeList phenotypes;
	};

	//Common data helper struct
	struct CommonData
	{
		//general data
		QString version;
		bool test_mode;

		//study
		QString study_name;
		QString study_description;
		QStringList study_affilitions;

		//data access
		QString dac_email;
		QString dac_organization;
		QString dap_text;
		QString dap_url;
		QString dap_use_condition; //attention, this is a CV!
		QStringList dap_use_modifiers; //attention, this is a CV!

		//processed samples
		QList<PSData> ps_list;
	};

	void addStudy(QJsonObject& parent, const CommonData& data)
	{
		QJsonObject obj;

		obj.insert("alias", data.study_name);
		obj.insert("title", data.study_name);
		obj.insert("description", data.study_description);
		obj.insert("affiliation", QJsonArray::fromStringList(data.study_affilitions));
		obj.insert("type", "resequencing");
		obj.insert("has_attribute", QJsonValue());
		obj.insert("has_project", QJsonValue());
		obj.insert("has_publication", QJsonValue());
		obj.insert("schema_type", "CreateStudy");
		obj.insert("schema_version", data.version);

		parent.insert("has_study", obj);
	}

	void addExperiments(QJsonObject& parent, const CommonData& data)
	{
		QJsonArray array;

		foreach(const PSData& ps_data, data.ps_list)
		{
			QJsonObject obj;
			obj.insert("alias", "EXP" + ps_data.pseudonym);
			obj.insert("description", "short-read sequencing");
			obj.insert("type", systemTypeToExperimentType(ps_data.ps_info.processing_system_type));
			QStringList files;
			files << "BAM" + ps_data.pseudonym;
			if (!ps_data.vcf.isEmpty()) files << "VCF" + ps_data.pseudonym;
			obj.insert("has_file", QJsonArray::fromStringList(files));
			obj.insert("has_protocol", QJsonArray::fromStringList(QStringList() << "LIB" + ps_data.pseudonym << "SEQ" + ps_data.pseudonym));
			obj.insert("has_sample", QJsonArray::fromStringList(QStringList() << "SAM" + ps_data.pseudonym));
			obj.insert("has_study", data.study_name);
			obj.insert("biological_replicates", "1");
			obj.insert("technical_replicates", "1");
			obj.insert("title", QJsonValue());
			obj.insert("schema_type", "CreateExperiment");
			obj.insert("schema_version", data.version);

			array.append(obj);
		}

		parent.insert("has_experiment", array);
	}

	void addAnalyses(QJsonObject& parent, const CommonData& data)
	{
		QJsonArray array;

		foreach(const PSData& ps_data, data.ps_list)
		{
			QJsonObject obj;
			obj.insert("alias", "ANA" + ps_data.pseudonym);
			obj.insert("description", "short-read data analysis using megSAP: https://github.com/imgag/megSAP");
			QStringList files;
			files << "BAM" + ps_data.pseudonym;
			obj.insert("has_input", QJsonArray::fromStringList(files));
			files.clear();
			if (!ps_data.vcf.isEmpty()) files << "VCF" + ps_data.pseudonym;
			obj.insert("has_output", QJsonArray::fromStringList(files));
			obj.insert("has_study", data.study_name);
			obj.insert("reference_chromosome", "unspecified");
			obj.insert("reference_genome", "GRCh38");
			obj.insert("type", "BAM");
			obj.insert("schema_type", "CreateAnalysis");
			obj.insert("schema_version", data.version);

			array.append(obj);
		}

		parent.insert("has_analysis", array);
	}

	void addFiles(QJsonObject& parent, const CommonData& data)
	{
		QJsonArray array;

		foreach(const PSData& ps_data, data.ps_list)
		{
			//add BAM (required)
			{
				QJsonObject obj;
				obj.insert("alias", "BAM" + ps_data.pseudonym);
				obj.insert("name", "BAM" + ps_data.pseudonym + ".bam");

				if (data.test_mode)
				{
					obj.insert("size", "6000000");
					obj.insert("checksum", "ABCDEFGHIJKLMNOPQRSTUVWXYZ");
				}
				else
				{
					QFile file(ps_data.bam);
					if (!file.open(QFile::ReadOnly)) THROW(FileAccessException, "Could not open file " + ps_data.bam);
					obj.insert("size", file.size());

					QCryptographicHash hash(QCryptographicHash::Md5);
					if (!hash.addData(&file)) THROW(Exception, "Could not calcualate checksum of " + ps_data.bam);
					obj.insert("checksum", QString(hash.result()));
				}

				obj.insert("format", "BAM");
				obj.insert("checksum_type", "MD5");
				obj.insert("schema_type", "CreateFile");
				obj.insert("schema_version", data.version);

				array.append(obj);
			}

			//add VCF (if available)
			if (!ps_data.vcf.isEmpty())
			{
				QJsonObject obj;
				obj.insert("alias", "VCF" + ps_data.pseudonym);
				obj.insert("name", "VCF" + ps_data.pseudonym + ".vcf.gz");

				if (data.test_mode)
				{
					obj.insert("size", "7000000");
					obj.insert("checksum", "123456789ABCDEFGHIJKLMNO");
				}
				else
				{
					QFile file(ps_data.vcf);
					if (!file.open(QFile::ReadOnly)) THROW(FileAccessException, "Could not open file " + ps_data.vcf);
					obj.insert("size", file.size());

					QCryptographicHash hash(QCryptographicHash::Md5);
					if (!hash.addData(&file)) THROW(Exception, "Could not calcualate checksum of " + ps_data.vcf);
					obj.insert("checksum", QString(hash.result()));
				}

				obj.insert("format", "VCF");
				obj.insert("checksum_type", "MD5");
				obj.insert("schema_type", "CreateFile");
				obj.insert("schema_version", data.version);

				array.append(obj);
			}
		}

		parent.insert("has_file", array);
	}

	void addFixed(QJsonObject& parent)
	{
		parent.insert("has_project", QJsonValue());
		parent.insert("has_publication", QJsonArray());
		parent.insert("has_biospecimen", QJsonArray());
	}

	void addDataAccessCommittee(QJsonObject& parent, const CommonData& data)
	{
		QJsonObject obj;
		obj.insert("alias", data.study_name + " DAC");
		obj.insert("main_contact", data.dac_email);
		obj.insert("has_member", QJsonArray::fromStringList(QStringList() << data.dac_email));
		obj.insert("schema_type", "CreateDataAccessCommittee");
		obj.insert("schema_version", data.version);

		QJsonArray array;
		array.append(obj);
		parent.insert("has_data_access_committee", array);

		//create member object as well
		obj = QJsonObject();
		obj.insert("alias", data.dac_email);
		obj.insert("email", data.dac_email);
		obj.insert("organization", data.dac_organization);
		obj.insert("schema_type", "CreateMember");
		obj.insert("schema_version", data.version);

		QJsonArray array2;
		array2.append(obj);
		parent.insert("has_member", array2);
	}

	void addDataAccessPolicy(QJsonObject& parent, const CommonData& data)
	{
		QJsonArray array;

		QJsonObject obj;
		obj.insert("alias", "DAP_"+data.study_name);
		obj.insert("name", "Data access policy for study "+data.study_name);
		obj.insert("description", "Data access policy for study "+data.study_name);
		obj.insert("policy_text", data.dap_text);
		obj.insert("policy_url", data.dap_url);
		obj.insert("has_data_use_permisson", data.dap_use_condition);
		obj.insert("has_data_use_modifier", QJsonArray::fromStringList(QStringList() << data.dap_use_modifiers));
		obj.insert("has_data_access_committee", data.study_name + " DAC");
		obj.insert("schema_type", "CreateDataAccessPolicy");
		obj.insert("schema_version", data.version);


		array.append(obj);

		parent.insert("has_data_access_policy", array);
	}

	void addDatasets(QJsonObject& parent, const CommonData& data)
	{
		QJsonArray array;

		QJsonObject obj;

		obj.insert("alias", data.study_name + " dataset");
		obj.insert("title", "Dataset of the study " + data.study_name);
		obj.insert("description", "Dataset of the study " + data.study_name);
		obj.insert("has_study", QJsonArray::fromStringList(QStringList() << data.study_name));
		obj.insert("has_data_access_policy", "DAP_"+data.study_name);

		QStringList tmp;
		foreach(const PSData& ps_data, data.ps_list)
		{
			tmp << systemTypeToDatasetType(ps_data.ps_info.processing_system_type);
		}
		obj.insert("type", QJsonArray::fromStringList(tmp));

		tmp.clear();
		foreach(const PSData& ps_data, data.ps_list)
		{
			tmp << "ANA" + ps_data.pseudonym;
		}
		obj.insert("has_analysis", QJsonArray::fromStringList(tmp));

		tmp.clear();
		foreach(const PSData& ps_data, data.ps_list)
		{
			tmp << "EXP" + ps_data.pseudonym;
		}
		obj.insert("has_experiment", QJsonArray::fromStringList(tmp));

		tmp.clear();
		foreach(const PSData& ps_data, data.ps_list)
		{
			tmp << "BAM" + ps_data.pseudonym;
			if (!ps_data.vcf.isEmpty()) tmp << "VCF" + ps_data.pseudonym;
		}
		obj.insert("has_file", QJsonArray::fromStringList(tmp));

		tmp.clear();
		foreach(const PSData& ps_data, data.ps_list)
		{
			tmp << "SAM" + ps_data.pseudonym;
		}
		obj.insert("has_sample", QJsonArray::fromStringList(tmp));

		obj.insert("schema_type", "CreateDataset");
		obj.insert("schema_version", data.version);

		array.append(obj);

		parent.insert("has_dataset", array);
	}

	void addProtocols(QJsonObject& parent, const CommonData& data, NGSD& db)
	{
		QJsonArray array;

		foreach(const PSData& ps_data, data.ps_list)
		{
			//library
			QJsonObject obj;
			obj.insert("alias", "LIB" + ps_data.pseudonym);
			obj.insert("description", ps_data.ps_info.processing_system);
			obj.insert("library_name", "processed_sample.id:"+ps_data.ps_id);
			obj.insert("library_layout", "PE");
			obj.insert("library_type", systemTypeToLibraryType(ps_data.ps_info.processing_system_type));
			obj.insert("library_selection", "unspecified");
			obj.insert("library_preparation_kit_retail_name", ps_data.ps_info.processing_system);
			obj.insert("library_preparation_kit_manufacturer", QJsonValue());
			obj.insert("library_preparation", "unspecified");
			obj.insert("end_bias", QJsonValue());
			obj.insert("primer", QJsonValue());
			obj.insert("has_attribute", QJsonValue());
			obj.insert("rnaseq_strandedness", QJsonValue());
			obj.insert("target_regions", QJsonArray() << "unspecified");
			obj.insert("schema_type", "CreateLibraryPreparationProtocol");
			obj.insert("schema_version", data.version);

			array.append(obj);

			//sequencing
			obj = QJsonObject();
			obj.insert("alias", "SEQ" + ps_data.pseudonym);
			obj.insert("description", "short-read sequencing");
			obj.insert("type", QJsonValue());
			QString device_type = db.getValue("SELECT d.type FROM device d, sequencing_run r WHERE r.device_id=d.id AND r.name='" + ps_data.ps_info.run_name + "'").toString();
			obj.insert("instrument_model", deviceTypeToSequencingInstrument(device_type));
			obj.insert("sequencing_center", QJsonValue());
			obj.insert("sequencing_read_length", QJsonValue());
			obj.insert("seq_forward_or_reverse", QJsonValue());
			obj.insert("target_coverage", QJsonValue());
			QString fc_id = db.getValue("SELECT fcid FROM sequencing_run WHERE name='" + ps_data.ps_info.run_name + "'").toString();
			obj.insert("flow_cell_id", fc_id);
			QString fc_type = db.getValue("SELECT flowcell_type FROM sequencing_run WHERE name='" + ps_data.ps_info.run_name + "'").toString();
			obj.insert("flow_cell_type", flowcellTypeToflowcellType(fc_type));
			obj.insert("cell_barcode_offset", QJsonValue());
			obj.insert("cell_barcode_read", QJsonValue());
			obj.insert("cell_barcode_size", QJsonValue());
			obj.insert("has_attribute", QJsonValue());
			obj.insert("sample_barcode_read", QJsonValue());
			obj.insert("umi_barcode_offset", QJsonValue());
			obj.insert("umi_barcode_read", QJsonValue());
			obj.insert("umi_barcode_size", QJsonValue());
			obj.insert("schema_type", "CreateSequencingProtocol");
			obj.insert("schema_version", data.version);

			array.append(obj);
		}

		parent.insert("has_protocol", array);
	}

	void addTissueArray(QJsonObject& parent, const CommonData& data, QString tissue)
	{
		QJsonArray array;

		QJsonObject obj;
		obj.insert("concept_name", tissue); //NGSD and GHGA use BRENDA Tissue Ontology > no conversion required
		obj.insert("schema_type", "CreateAnatomicalEntity");
		obj.insert("schema_version", data.version);

		array.append(obj);

		parent.insert("has_anatomical_entity", array);
	}

	void addSamples(QJsonObject& parent, const CommonData& data)
	{
		QJsonArray array;

		foreach(const PSData& ps_data, data.ps_list)
		{
			//library
			QJsonObject obj;

			obj.insert("alias", "SAM" + ps_data.pseudonym);
			obj.insert("name", "SAM" + ps_data.pseudonym);
			addTissueArray(obj, data, ps_data.s_info.tissue);
			obj.insert("description", "sample that was sequenced");
			obj.insert("type", sampleTypeToSampleType(ps_data.s_info.type, ps_data.s_info.is_ffpe));

			obj.insert("case_control_status", QJsonValue());
			obj.insert("vital_status_at_sampling", QJsonValue());
			obj.insert("isolation", QJsonValue());
			obj.insert("storage", QJsonValue());
			obj.insert("has_biospecimen", QJsonValue());
			obj.insert("xref", QJsonValue());
			obj.insert("has_attribute", QJsonValue());
			obj.insert("has_individual", "IND" + ps_data.pseudonym);
			obj.insert("schema_type", "CreateSample");
			obj.insert("schema_version", data.version);

			array.append(obj);
		}

		parent.insert("has_sample", array);
	}

	void addPhenotypes(QJsonObject& parent, const CommonData& data, const PhenotypeList& phenotypes)
	{
		QJsonArray array;

		foreach(const Phenotype& pheno, phenotypes)
		{
			//library
			QJsonObject obj;

			obj.insert("concept_name", QString(pheno.accession()));
			obj.insert("schema_type", "CreatePhenotypicFeature");
			obj.insert("schema_version", data.version);

			array.append(obj);
		}

		parent.insert("has_phenotypic_feature", array);
	}

	void addIndividuals(QJsonObject& parent, const CommonData& data)
	{
		QJsonArray array;

		foreach(const PSData& ps_data, data.ps_list)
		{
			//library
			QJsonObject obj;

			obj.insert("alias", "IND" + ps_data.pseudonym);
			obj.insert("sex", sampleGenderToSex(ps_data.s_info.gender));
			obj.insert("age", "unknown");
			addPhenotypes(obj, data, ps_data.phenotypes);
			obj.insert("vital_status", "unknown");
			obj.insert("karyotype", "unknown");
			obj.insert("geographical_region", "unknown");
			QJsonArray ancestry_list;
			QString ancestry = sampleAncestryToAncestry(ps_data.ps_info.ancestry);
			if (ancestry!="")
			{
				QJsonObject obj2;

				obj2.insert("concept_name", ancestry);
				obj2.insert("schema_type", "CreateAncestry");
				obj2.insert("schema_version", data.version);

				ancestry_list.append(obj2);
			}
			obj.insert("has_ancestry", ancestry_list);

			obj.insert("schema_type", "CreateIndividual");
			obj.insert("schema_version", data.version);

			array.append(obj);
		}

		parent.insert("has_individual", array);
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
		data.version = "0.9.0";
		data.test_mode = getFlag("test");
		data.study_name = getString(data_obj, "study");
		data.study_description = getString(data_obj, "study_description");
		data.study_affilitions = getArray(data_obj, "study_affiliations");
		data.dac_email = getString(data_obj, "data_access_committee_email");
		data.dac_organization = getString(data_obj, "data_access_committee_organization");
		data.dap_text = getString(data_obj, "data_access_policy_text");
		data.dap_url = getString(data_obj, "data_access_policy_url");
		data.dap_use_condition = getString(data_obj, "data_access_policy_use_condition");
		data.dap_use_modifiers = getArray(data_obj, "data_access_policy_use_modifiers");

		NGSD db(data.test_mode);

		//load processed samples to export
		stream << QDateTime::currentDateTime().toString("dd.MM.yyyy hh:mm:ss") << " Loading processed sample list..." << endl;
		auto handle = Helper::openFileForReading(getInfile("samples"), false);
		while(!handle->atEnd())
		{
			QByteArray line = handle->readLine().trimmed();
			if (line.isEmpty() || line[0]=='#') continue;

			QByteArrayList parts = line.split('\t');
			if (parts.count()!=3) THROW(FileParseException, "Invalid sample line: " + line);

			QString pseudonym = parts[0];

			QString ps = parts[2].trimmed();
			if(ps.startsWith("Skipped - ")) continue; // samples which were skipped in the preparation dialog

			QString ps_id = db.processedSampleId(ps);

			QString s_id = db.sampleId(ps);
			QString bam = db.processedSamplePath(ps_id, PathType::BAM);
			if (!QFile::exists(bam) && !data.test_mode) THROW(Exception, "Processed sample " + ps + " BAM missing: " + bam);

			QString vcf = db.processedSamplePath(ps_id, PathType::VCF);
			if (!QFile::exists(vcf) && !data.test_mode) vcf.clear();

			data.ps_list << PSData{ps_id, ps, bam, vcf, pseudonym, db.getSampleData(s_id), db.getProcessedSampleData(ps_id), db.samplePhenotypes(s_id)};
		}
		stream << QDateTime::currentDateTime().toString("dd.MM.yyyy hh:mm:ss") << " Writing JSON for " << data.ps_list.count() << " samples..." << endl;

		//create JSON
		QJsonObject root;
		root.insert("schema_type", "CreateSubmission");
		root.insert("schema_version", data.version);
		addStudy(root, data);
		addDataAccessCommittee(root, data);
		addDataAccessPolicy(root, data);
		addExperiments(root, data);
		addAnalyses(root, data);
		addFiles(root, data);
		addDatasets(root, data);
		addProtocols(root, data, db);
		addSamples(root, data);
		addIndividuals(root, data);
		addFixed(root);

		//store JSON
		QJsonDocument doc(root);
		Helper::storeTextFile(getOutfile("out"), QStringList() << doc.toJson());

		stream << QDateTime::currentDateTime().toString("dd.MM.yyyy hh:mm:ss") << " Done" << endl;
	}
};

#include "main.moc"

int main(int argc, char *argv[])
{
	ConcreteTool tool(argc, argv);
	return tool.execute();
}
