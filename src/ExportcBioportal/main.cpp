#include "ToolBase.h"
#include "FastqFileStream.h"
#include "Helper.h"
#include "TsvFile.h"
#include "ExportCBioPortalStudy.h"
#include <QFile>
#include "FileLocationProviderLocal.h"

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

		int idx_ext_name = samples.columnIndex("external_name");

		for (int i=0; i< samples.rowCount(); i++)
		{
			QStringList row = samples.row(i);
			QString sample_name = db.getValue("SELECT name FROM sample WHERE name_external LIKE '%" + row[idx_ext_name] + "%'").toString();
			QString sample_id = db.sampleId(sample_name, true);

			qDebug() << "Sample: " << sample_name;

			QStringList processed_samples = db.getValues("SELECT CONCAT(s.name,'_',LPAD(ps.process_id,2,'0')) FROM processed_sample ps LEFT JOIN sample s ON s.id = ps.sample_id WHERE ps.sample_id = '" + sample_id + "'");
			foreach(QString ps_name, processed_samples)
			{
				qDebug() << "\tps: " << ps_name;

				QString gsvar_file; // TODO get gsvar file path

				VariantList variants;
				variants.load(gsvar_file);

				FileLocationProviderLocal(gsvar_file, variants.getSampleHeader(), variants.type());

			}

			//gather necessary files
			//get variant lists


			/*
		QString sample = tumor_samples[idx];
		QString rna = rna_samples[idx];

		qDebug() << "gathering Data for: " << sample;
		QString ps_id = db.processedSampleId(sample);
		QString normal_sample = db.normalSample(ps_id);
		QString normal_id = db.processedSampleId(normal_sample);


		qDebug() << "normal sample: " << normal_sample;

		loadFile(GlobalServiceProvider::database().secondaryAnalyses(sample + "-" + normal_sample, "somatic")[0]);
		const FileLocationProvider& fileprovider = GlobalServiceProvider::fileLocationProvider();

		SampleFiles files;
		files.clincnv_file = fileprovider.getAnalysisCnvFile().filename;
		files.msi_file = fileprovider.getSomaticMsiFile().filename;
		files.sv_file = fileprovider.getAnalysisSvFile().filename;
		files.gsvar_germline = GlobalServiceProvider::database().processedSamplePath(normal_id, PathType::GSVAR).filename;
		files.gsvar_somatic = GlobalServiceProvider::database().secondaryAnalyses(sample + "-" + normal_sample, "somatic")[0];

		if (rna != "")
		{
			QString rna_id = db.processedSampleId(rna);
			files.rna_fusions = GlobalServiceProvider::database().processedSamplePath(rna_id, PathType::FUSIONS).filename;
		}
		qDebug() << files.gsvar_somatic << "\n" << files.gsvar_germline << "\n" << files.clincnv_file << "\n" << files.msi_file << "\n" << files.rna_fusions << "\n";

		VariantList somatic_vl;
		somatic_vl.load(files.gsvar_somatic);
		VariantList germline_vl;
		germline_vl.load(files.gsvar_germline);
		CnvList cnvs;
		cnvs.load(files.clincnv_file);


		QStringList messages;
		SomaticReportSettings report_settings;
		report_settings.normal_ps = normal_sample;
		report_settings.tumor_ps = sample;
		somatic_report_settings_.msi_file = GlobalServiceProvider::fileLocationProvider().getSomaticMsiFile().filename;
		somatic_report_settings_.viral_file = GlobalServiceProvider::database().processedSamplePath(ps_id, PathType::VIRAL).filename;

		report_settings.report_config = db.somaticReportConfig(ps_id, normal_id, somatic_vl, cnvs, germline_vl, messages);

		qDebug() << report_settings.report_config.filter();
		report_settings.filters = FilterCascadeFile::load(filterFileName, report_settings.report_config.filter());

		export_settings.addSample(report_settings, files);
			*/
		}




		//compare to Mainwindow export helper
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
