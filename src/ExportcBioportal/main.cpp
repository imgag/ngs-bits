#include "ToolBase.h"
#include "FastqFileStream.h"
#include "Helper.h"
#include "TsvFile.h"
#include "ExportCBioPortalStudy.h"
#include <QFile>
#include "FileLocationProviderLocal.h"
#include <QFileInfo>
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
		setDescription("Converts a FASTQ file to FASTA format.");
		addInfile("samples", "Input TSV file with samples (tumor, normal, rna) to be exported and their clinical data.", false);
		addInfile("study_data", "Input TSV file with Infos about the study that should be created.", false);
		addInfile("attribute_data", "Input TSV file with Infos about the sample attributes that should be contained in the study.", false);
		addString("out", "Output folder that will contain all files for the cBioPortal study.", false);
		addFlag("test", "Uses the test database instead of on the production database.");
		addFlag("debug", "Provide additional debug output on stdout.");
	}

	virtual void main()
	{
		StudyData study;
		CancerData cancer;
		parseStudyData(getInfile("study_data"), study, cancer);

		QList<SampleAttribute> attributes = parseAttributeData(getInfile("attribute_data"));


		CBioPortalExportSettings export_settings(study, false);
		export_settings.cancer = cancer;
		export_settings.setSampleAttributes(attributes);


		TsvFile samples;
		samples.load(getInfile("samples"));

		//gather sample infos:

		NGSD db(getFlag("test"));

//		int idx_ext_name = samples.columnIndex("external_name");
		int idx_tumor_name = samples.columnIndex("tumor_ps_name");
		int idx_normal_name = samples.columnIndex("normal_ps_name");
		int idx_sap_id = samples.columnIndex("sap_id");
		int idx_mtb_case_id = samples.columnIndex("mtb_case_id");
		int idx_mtb_registration_date = samples.columnIndex("mtb_registration_date");
		int idx_mtb_board_date = samples.columnIndex("mtb_board_date");
		int idx_patient_mpi = samples.columnIndex("patient_mpi");
		int idx_icd10_code = samples.columnIndex("icd10_code");
		int idx_icd10_catalog = samples.columnIndex("icd10_catalog");
		int idx_oncotree_code = samples.columnIndex("oncotree_code");

		for (int i=0; i< samples.rowCount(); i++)
		{
			QStringList row = samples.row(i);
			QString sample_id = db.sampleId(row[idx_tumor_name], true);

			SampleMTBmetadata mtb_data;
			mtb_data.mtb_case_id = row[idx_mtb_case_id];
			mtb_data.mtb_board_date = QDate::fromString(row[idx_mtb_board_date], "yyyy-MM-dd");
			mtb_data.mtb_registration_date = QDate::fromString(row[idx_mtb_registration_date], "yyyy-MM-dd");
			mtb_data.sap_id = row[idx_sap_id].toInt();
			mtb_data.patient_mpi_id = row[idx_patient_mpi];
			mtb_data.icd10_code = row[idx_icd10_code];
			mtb_data.icd10_cataloge = row[idx_icd10_catalog];
			mtb_data.oncotree_code = row[idx_oncotree_code];

			QStringList processed_samples = db.getValues("SELECT CONCAT(s.name,'_',LPAD(ps.process_id,2,'0')) FROM processed_sample ps LEFT JOIN sample s ON s.id = ps.sample_id LEFT JOIN project as p ON ps.project_id = p.id WHERE p.type='diagnostic' AND ps.sample_id = '" + sample_id + "'");
			if (processed_samples.count() == 0) THROW(ArgumentException, "No processed samples found for: " + row[idx_tumor_name] + " with sample id: " + sample_id);

			foreach(QString tumor_ps, processed_samples)
			{
				qDebug() << "\tps: " << tumor_ps;
				QString tumor_id = db.processedSampleId(tumor_ps);

				QString normal_ps = db.normalSample(tumor_id);
				QString normal_id = db.processedSampleId(normal_ps);

				if (normal_ps == "") THROW(ArgumentException, "No normal sample set for tumor: " + tumor_ps);
				if (normal_ps != row[idx_normal_name]) THROW(ArgumentException, "The set normal sample in NGSD is a different one compared to the given normale sample. Given: " + row[idx_normal_name] + ", in NGSD set: " + normal_ps);

				QString rna_ps = getRnaSample(db, tumor_ps);

				qDebug() << "\trna: " << rna_ps;

				QString tumor_folder = db.processedSamplePath(tumor_id, PathType::SAMPLE_FOLDER);
				tumor_folder.chop(1); // remove seperator at end

				QString project_folder = QFileInfo(tumor_folder).dir().absolutePath();
				QString somatic_prefix = tumor_ps + "-" + normal_ps;
				QString somatic_folder = project_folder + QDir::separator() + "Somatic_"+somatic_prefix + QDir::separator();

				QString gsvar_file = somatic_folder + somatic_prefix + ".GSvar";

				if (! QFile(gsvar_file).exists())
				{
					qDebug() << "No Gsvar file found for tumor: " + tumor_ps + " - skipping sample. - " + gsvar_file;
					continue;
				}

				VariantList variants;
				variants.load(gsvar_file);

				FileLocationProviderLocal fileprovider (gsvar_file, variants.getSampleHeader(), variants.type());

				SampleFiles files;
				files.clincnv_file = fileprovider.getAnalysisCnvFile().filename;
				files.msi_file = fileprovider.getSomaticMsiFile().filename;
				files.sv_file = fileprovider.getAnalysisSvFile().filename;
				files.gsvar_germline = db.processedSamplePath(normal_id, PathType::GSVAR);
				files.gsvar_somatic = gsvar_file;

				if (rna_ps != "")
				{
					QString rna_id = db.processedSampleId(rna_ps);
					files.rna_fusions = db.processedSamplePath(rna_id, PathType::FUSIONS);
				}

				VariantList somatic_vl;
				somatic_vl.load(files.gsvar_somatic);
				VariantList germline_vl;
				germline_vl.load(files.gsvar_germline);
				CnvList cnvs;
				if (VersatileFile(files.clincnv_file).exists())
				{
					cnvs.load(files.clincnv_file);
				}

				QStringList messages;
				SomaticReportSettings report_settings;
				report_settings.normal_ps = normal_ps;
				report_settings.tumor_ps = tumor_ps;
				report_settings.msi_file = fileprovider.getSomaticMsiFile().filename;
				report_settings.viral_file = db.processedSamplePath(tumor_id, PathType::VIRAL);

				report_settings.report_config = db.somaticReportConfig(tumor_id, normal_id, somatic_vl, cnvs, germline_vl, messages);

//				qDebug() << report_settings.report_config.filter();
				QString filterFileName = QCoreApplication::applicationDirPath() + QDir::separator() + "GSvar_filters.ini";
				report_settings.filters = FilterCascadeFile::load(filterFileName, report_settings.report_config.filter());

				export_settings.addSample(report_settings, files, mtb_data);
			}
		}

		ExportCBioPortalStudy exportStudy(export_settings, getFlag("test"));
		exportStudy.exportStudy(getString("out") + "/" + study.identifier + "/", getFlag("debug"));
	}

	QString getRnaSample(NGSD& db, QString tumor)
	{
		QString tumor_sample_id = db.sampleId(tumor);
		QSet<int> rna_sample_ids = db.relatedSamples(tumor_sample_id.toInt(), "same sample", "RNA"); // TODO bug here!

//		qDebug() << "POTENTIAL RNA IDS: " << rna_sample_ids;

		if (rna_sample_ids.count() == 0)
		{
			return "";
		}
		else
		{
			QStringList rna_processed_samples;
			foreach (int rna_id, rna_sample_ids)
			{
				rna_processed_samples.append(db.getValues("SELECT CONCAT(s.name,'_',LPAD(ps.process_id,2,'0')) as name FROM processed_sample as ps LEFT JOIN sample as s ON s.id = ps.sample_id WHERE s.id = :1 ORDER BY name DESC", QString::number(rna_id)));
			}

			if (rna_processed_samples.count() == 0) return "";


			QString newest = "2000-01-01";
			QString newest_rna = "";

			foreach (QString rna_ps, rna_processed_samples)
			{

				QString seq_id = db.getValue("SELECT sequencing_run_id FROM processed_sample WHERE id=:1", false, db.processedSampleId(rna_ps)).toString();
				QString seq_date = db.getValue("SELECT start_date FROM sequencing_run WHERE id=:1", false, seq_id).toString();

				if (newest < seq_date)
				{
					newest = seq_date;
					newest_rna = rna_ps;
				}
			}

			if (newest_rna == "") THROW(ArgumentException, "Expected to find rna processed sample for the tumor but couldn't. Tumor sample id:" + tumor_sample_id);

			return newest_rna;
		}

	}

	QList<SampleAttribute> parseAttributeData(QString file)
	{
		QList<SampleAttribute> attributes;

		TsvFile attr_data;
		attr_data.load(file);

		QStringList headers = attr_data.headers();

		int idx_attr_name = getIndex(headers, "name");
		int idx_attr_db_name = getIndex(headers, "db_name");
		int idx_desc = getIndex(headers, "description");
		int idx_datatype = getIndex(headers, "datatype");
		int idx_prio = getIndex(headers, "priority");

		for (int i=0; i<attr_data.rowCount(); i++)
		{
			QStringList row = attr_data.row(i);

			SampleAttribute attr;
			attr.name = row[idx_attr_name];
			attr.description = row[idx_desc];
			attr.db_name = row[idx_attr_db_name];
			attr.datatype = row[idx_datatype];
			bool ok;
			int prio = row[idx_prio].toInt(&ok);
			if (!ok)
			{
				THROW(ArgumentException, "Could not convert the priority of " + row[idx_attr_name] + "to integer: '" + row[idx_prio] + "' to integer");
			}
			attr.priority = prio;

			attr.attribute = SampleAttribute::determineAttribute(row[idx_attr_db_name]);

			attributes.append(attr);
		}

		return attributes;
	}


	void parseStudyData(QString file, StudyData& study, CancerData& cancer)
	{
		TsvFile study_data;
		study_data.load(file);

		QStringList keys = study_data.extractColumn(0);

		int idx_study_name = getIndex(keys, "study_name");
		int idx_study_ident = getIndex(keys, "study_identifier");
		int idx_study_desc = getIndex(keys, "study_description");
		int idx_study_ref = getIndex(keys, "study_reference");
		int idx_cancer_name = getIndex(keys, "cancer_name");
		int idx_cancer_desc = getIndex(keys, "cancer_description");
		int idx_cancer_parent = getIndex(keys, "cancer_parent");
		int idx_cancer_color = getIndex(keys, "cancer_color");

		QStringList values = study_data.extractColumn(1);

		study.name = values[idx_study_name];
		study.description = values[idx_study_desc];
		study.identifier = values[idx_study_ident];
		study.reference_genome = values[idx_study_ref];
		study.cancer_type = values[idx_cancer_name];

		cancer.description = values[idx_cancer_desc];
		cancer.parent = values[idx_cancer_parent];
		cancer.color = values[idx_cancer_color];
	}

	int getIndex(QStringList& keys, QString key)
	{
		int tmp = keys.indexOf(key);
		if (tmp == -1)
		{
			THROW(FileParseException, "Couldn't find necessary key '" + key + "' in the study_data file.");
		}
		return tmp;
	}
};



#include "main.moc"

int main(int argc, char *argv[])
{
	ConcreteTool tool(argc, argv);
	return tool.execute();
}
