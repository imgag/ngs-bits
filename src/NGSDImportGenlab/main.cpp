#include "ToolBase.h"
#include "NGSD.h"
#include "Helper.h"
#include "GenLabDB.h"

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
		setDescription("Import sample information from GenLAB into NGSD.");
		setExtendedDescription(QStringList() << "Imports the following data:" << "general meta data: gender, patient ID, year of birth, disease data, studies" << "sample relations: " << "rna tissue: ");
		addString("ps", "Processed sample for which the GenLAB data will be imported.", false);
		//optional
		addFlag("no_relations", "Do not search and import sample relations from GenLAB.");
		addFlag("no_rna_tissue", "Do not import RNA reference tissue from HPO terms.");
		addFlag("no_metadata", "Do not search and import metadata from GenLAB (disease group, ICD10, HPO, ...)");
		addFlag("test", "Uses the test database instead of on the production database.");
		addFlag("dry_run", "Run as specified but do NOT change anything in the database.");
	}

	virtual void main()
	{
		//init
		NGSD db(getFlag("test"));
		if (!GenLabDB::isAvailable()) THROW(DatabaseException, "Genlab database is not available. Can't import data.");
		GenLabDB genlab;

		QString ps = getString("ps");
		bool dry_run = getFlag("dry_run");

		//import
		if (!getFlag("no_metadata"))
		{
			importGenLabMetadata(ps, db, genlab, dry_run);
		}

		if (!getFlag("no_relations"))
		{
			importSampleRelations(ps, db, genlab, dry_run);
		}

		if (!getFlag("no_rna_tissue"))
		{
			importRnaReferenceTissue(ps, db, dry_run);
		}
	}

	void importSampleRelations(const QString& ps_name, NGSD& db, GenLabDB& genlab, bool dry_run)
	{
		QStringList samples = genlab.patientSamples(ps_name);
		if (samples.count() == 0) return;

		QString ps_id = db.processedSampleId(ps_name);
		QString s_id = db.sampleId(ps_name);

		SampleData sample_data = db.getSampleData(s_id);
		ProcessedSampleData ps_data = db.getProcessedSampleData(ps_id);

		// gather related sample data
		QList<SampleData> related_sample_data;
		foreach(const QString& sample, samples)
		{
			QString rel_s_id = db.sampleId(sample, false);
			if (rel_s_id.isEmpty()) continue;

			related_sample_data << db.getSampleData(rel_s_id);
		}

		QStringList ngsd_relations = db.getValues("SELECT relation FROM sample_relations WHERE sample1_id = "+ s_id + " OR sample2_id = " + s_id);
		if (!ngsd_relations.contains("tumor-normal"))
		{
			checkForTumorNormalRelation(db, sample_data, ps_data, related_sample_data, dry_run);
		}
		if (!ngsd_relations.contains("same sample") && sample_data.type=="RNA")
		{
			checkForDnaRnaRelation(db, genlab, sample_data, related_sample_data, dry_run);
		}

		//relatives patient relations (parents, siblings)
		foreach (const SampleRelation& genlab_relation, genlab.relatives(ps_name))
		{
			QSet<int> sample_ids_ngsd = db.relatedSamples(s_id.toInt(), genlab_relation.relation);
			int sample2_id = db.sampleId(genlab_relation.sample1).toInt();

			if (!sample_ids_ngsd.contains(sample2_id))
			{
                QTextStream(stdout) << "Adding relative relation: " << genlab_relation.sample1 << " - " << genlab_relation.relation << " - " << genlab_relation.sample2 << QT_ENDL;
				SqlQuery insert = db.getQuery();
				insert.prepare("INSERT INTO sample_relations (sample1_id, relation, sample2_id) VALUES (:0, :1, :2)");
				insert.bindValue(0, db.sampleId(genlab_relation.sample1));
				insert.bindValue(1, genlab_relation.relation);
				insert.bindValue(2, db.sampleId(genlab_relation.sample2));
				if (!dry_run) insert.exec();
			}
		}
	}

	void checkForTumorNormalRelation(NGSD& db, const SampleData& current_sample_data, const ProcessedSampleData& current_ps_data, const QList<SampleData>& related_sample_data, bool dry_run)
	{
		//determine best match for tumor-normal relation
		ProcessedSampleData best_candidate;
		foreach(const SampleData& data, related_sample_data)
		{
			if (!data.type.startsWith("DNA")) continue;
			if (data.is_tumor == current_sample_data.is_tumor) continue;

			//get processed samples:
			foreach (const QString& ps_id, db.getValues("SELECT id FROM processed_sample WHERE sample_id = '" + db.sampleId(data.name) + "'"))
			{
				ProcessedSampleData ps_data = db.getProcessedSampleData(ps_id);

				if (current_ps_data.processing_system != ps_data.processing_system) continue;
				if (ps_data.quality == "bad") continue;

				QString run_status = db.getValue("SELECT status FROM sequencing_run WHERE name='" + ps_data.run_name + "'").toString();
				if (run_status!="run_started" && run_status!="demultiplexing_started" && run_status!="analysis_started" && run_status!="analysis_finished") continue;

				if (best_candidate.name.isEmpty())
				{
					best_candidate = ps_data;
				}
				else if (isSampleNewer(best_candidate.name, ps_data.name)) // if current best isn't from the latest run take the newer sample
				{
					best_candidate = ps_data;
				}
			}
		}
		if (best_candidate.name.isEmpty()) return;

		//insert new tumor-normal relation
		QString tumor_ps_id;
		QString tumor_ps_name;
		QString normal_ps_id;
		QString normal_ps_name;
		QString normal_sample_name;
		if (current_sample_data.is_tumor)
		{
			tumor_ps_id = db.processedSampleId(current_ps_data.name);
			tumor_ps_name = current_ps_data.name;
			normal_ps_id = db.processedSampleId(best_candidate.name);
			normal_ps_name = best_candidate.name;
			normal_sample_name = current_ps_data.normal_sample_name;
		}
		else
		{
			normal_ps_id = db.processedSampleId(current_ps_data.name);
			normal_ps_name = current_ps_data.name;
			tumor_ps_id = db.processedSampleId(best_candidate.name);
			tumor_ps_name = best_candidate.name;
			normal_sample_name = best_candidate.normal_sample_name;
		}

        QTextStream(stdout) << "Adding new tumor normal relation: " << tumor_ps_name << " tumor-normal " << normal_ps_name << QT_ENDL;

		SqlQuery insert = db.getQuery();
		insert.prepare("INSERT IGNORE INTO sample_relations (sample1_id, relation, sample2_id) VALUES (:0,'tumor-normal',:1)");
		insert.bindValue(0, db.sampleId(tumor_ps_name));
		insert.bindValue(1, db.sampleId(normal_ps_name));
		if (!dry_run) insert.exec();

		//update normal id of tumor
		if (db.normalSample(tumor_ps_id)=="")
		{
            QTextStream(stdout) << "Updating normal ID for tumor sample: " << tumor_ps_name << " - normal sample is: " << normal_ps_name << QT_ENDL;
			if (!dry_run) db.getQuery().exec("UPDATE `processed_sample` SET normal_id = " + normal_ps_id + " WHERE id=" + tumor_ps_id);
		}
	}

	void checkForDnaRnaRelation(NGSD& db, GenLabDB& genlab, const SampleData& current_sample_data, const QList<SampleData>& related_sample_data, bool dry_run)
	{
		// search for a related DNA sample
		ProcessedSampleData genlab_related_sample;

		foreach (const QString& rel_sample_name, genlab.dnaSamplesofRna(current_sample_data.name_external))
		{
			QString rel_sample_id = db.sampleId(rel_sample_name, false);
			if (rel_sample_id == "") continue;

			if(!db.getSampleData(rel_sample_id).type.startsWith("DNA")) continue;

			foreach(const QString& ps_id, db.getValues("SELECT id FROM processed_sample WHERE sample_id = '" + db.sampleId(rel_sample_name) + "'"))
			{
				ProcessedSampleData data = db.getProcessedSampleData(ps_id);

				if (genlab_related_sample.name.isEmpty())
				{
					genlab_related_sample = data;
				}
				else if (isSampleNewer(genlab_related_sample.name, data.name))
				{
					genlab_related_sample = data;
				}
			}
		}

		//found a related sample in genlab -> import and stop
		if (!genlab_related_sample.name.isEmpty())
		{
            QTextStream(stdout) << "Adding DNA-RNA relation based on GenLab v_ngs_dnarna table: " << current_sample_data.name << " same sample " << genlab_related_sample.name << QT_ENDL;
			SqlQuery insert = db.getQuery();
			insert.prepare("INSERT INTO sample_relations (sample1_id, relation, sample2_id) VALUES (:0,'same sample',:1)");
			insert.bindValue(0, db.sampleId(current_sample_data.name));
			insert.bindValue(1, db.sampleId(genlab_related_sample.name));
			if (!dry_run) insert.exec();
			return;
		}

		//if there was nothing found in genlab DnaRna table search for a fitting sample in samples from the same patient.
		ProcessedSampleData best_candidate;
		foreach (const SampleData& data, related_sample_data)
		{
			if (data.is_tumor != current_sample_data.is_tumor) continue;
			if ( ! data.type.startsWith("DNA")) continue;

			//get processed samples
			foreach (const QString& ps_id, db.getValues("SELECT id FROM processed_sample WHERE sample_id = '" + db.sampleId(data.name) + "'"))
			{
				ProcessedSampleData ps_data = db.getProcessedSampleData(ps_id);

				if (ps_data.processing_system_type != "Panel" && ps_data.processing_system_type != "WES" && ps_data.processing_system_type != "WGS") continue;
				QString run_status = db.getValue("SELECT status FROM sequencing_run WHERE name = '" + ps_data.run_name + "'").toString();

				if (run_status!="run_started" && run_status!="demultiplexing_started" && run_status!="analysis_started" && run_status!="analysis_finished") continue;

				if (best_candidate.name.isEmpty())
				{
					best_candidate = ps_data;
				}
				else if (isSampleNewer(best_candidate.name, ps_data.name))
				{
					best_candidate = ps_data;
				}
			}
		}

		if (! best_candidate.name.isEmpty())
		{
            QTextStream(stdout) << "Adding DNA-RNA relation based on NGSD: " << current_sample_data.name << " same sample " << best_candidate.name << QT_ENDL;
			SqlQuery insert = db.getQuery();
			insert.prepare("INSERT INTO sample_relations (sample1_id, relation, sample2_id) VALUES (:0,'same sample',:1)");
			insert.bindValue(0, db.sampleId(current_sample_data.name));
			insert.bindValue(1, db.sampleId(best_candidate.name));
			if (!dry_run) insert.exec();
			return;
		}
	}

	void importGenLabMetadata(QString& ps_name, NGSD& db, GenLabDB& genlab, bool dry_run)
	{
		QString ps_id = db.processedSampleId(ps_name);
		QString s_id = db.sampleId(ps_name);
		SampleData s_data = db.getSampleData(s_id);

		//gender
		QString gender = genlab.gender(ps_name);
		if (gender!="" && s_data.gender=="n/a")
		{
            QTextStream(stdout) << "Adding gender: " << gender << QT_ENDL;
			if (!dry_run) db.getQuery().exec("UPDATE sample SET gender='" + gender + "' WHERE id=" + s_id);
		}

		//patient identifier
		QString patient_identifier = genlab.patientIdentifier(ps_name);
		if (patient_identifier!="" && s_data.patient_identifier=="")
		{
            QTextStream(stdout) << "Adding patient identifier: " << patient_identifier << QT_ENDL;
			if (!dry_run)db.getQuery().exec("UPDATE sample SET patient_identifier='" + patient_identifier + "' WHERE id=" + s_id);
		}

		//year of birth
		QString yob = genlab.yearOfBirth(ps_name);
		if (yob!="" && s_data.year_of_birth=="")
		{
            QTextStream(stdout) << "Adding year of birth: " << yob << QT_ENDL;
			if (!dry_run) db.getQuery().exec("UPDATE sample SET year_of_birth='" + yob + "' WHERE id=" + s_id);
		}

		//order date
		QString order_date = genlab.orderEntryDate(ps_name);
		if (order_date!="" && s_data.order_date=="")
		{
            QTextStream(stdout) << "Adding order date: " << order_date << QT_ENDL;
			if (!dry_run) db.getQuery().exec("UPDATE sample SET order_date='" + order_date + "' WHERE id=" + s_id);
		}

		//sampling date
		QString sampling_date = genlab.samplingDate(ps_name);
		if (sampling_date!="" && s_data.sampling_date=="")
		{
            QTextStream(stdout) << "Adding sampling date: " << sampling_date << QT_ENDL;
			if (!dry_run) db.getQuery().exec("UPDATE sample SET sampling_date='" + sampling_date + "' WHERE id=" + s_id);
		}

		//tissue
		QString tissue = genlab.tissue(ps_name);
		if (tissue!="" && s_data.tissue=="n/a")
		{
            QTextStream(stdout) << "Adding tissue: " << tissue << QT_ENDL;
			if (!dry_run) db.getQuery().exec("UPDATE sample SET tissue='" + tissue + "' WHERE id=" + s_id);
		}

		//disease group and status
		QPair<QString, QString> disease_data = genlab.diseaseInfo(ps_name);
		QString disease_group = disease_data.first;
		QString disease_status = disease_data.second;
		if (disease_group!="n/a" && s_data.disease_group=="n/a")
		{
            QTextStream(stdout) << "Adding disease group: " << disease_group << QT_ENDL;
			if (!dry_run) db.getQuery().exec("UPDATE sample SET disease_group='" + disease_group + "' WHERE id=" + s_id);
		}
		if (disease_status!="n/a" && s_data.disease_status=="n/a")
		{
            QTextStream(stdout) << "Adding disease status: " << disease_status << QT_ENDL;
			if (!dry_run) db.getQuery().exec("UPDATE sample SET disease_status='" + disease_status + "' WHERE id=" + s_id);
		}

		//disease details
		importDiseaseDetails(db, s_id, genlab.anamnesis(ps_name), "clinical phenotype (free text)", dry_run);
		importDiseaseDetails(db, s_id, genlab.orphanet(ps_name), "Orpha number", dry_run);
		importDiseaseDetails(db, s_id, genlab.diagnosis(ps_name), "ICD10 code", dry_run);
		importDiseaseDetails(db, s_id, genlab.tumorFraction(ps_name), "tumor fraction", dry_run);
		PhenotypeList phenotypes = genlab.phenotypes(ps_name);
		QStringList hpo_accessions;
        for (const Phenotype& genlab_v : phenotypes)
		{
			hpo_accessions << genlab_v.accession();
		}
		importDiseaseDetails(db, s_id, hpo_accessions, "HPO term id", dry_run);

		//sudies
		QStringList genlab_studies = genlab.studies(ps_name);
		QStringList ngsd_studies = db.getValues("SELECT st.name FROM study st, study_sample ss, processed_sample ps WHERE ss.study_id=st.id AND ps.id=ss.processed_sample_id AND ps.sample_id=" + s_id);
		foreach(const QString& study, genlab_studies)
		{
			if (!ngsd_studies.contains(study))
			{
				QVariant study_id = db.getValue("SELECT id FROM study WHERE name=:0", true, study);
				if (!study_id.isValid()) INFO(ArgumentException, "GenLab study name '" + study + "' not found in NGSD! Please add the study to NGSD, or correct the study name in GenLab!");

                QTextStream(stdout) << "Adding study: " << study << QT_ENDL;

				if (!dry_run) db.getQuery().exec("INSERT INTO `study_sample`(`study_id`, `processed_sample_id`) VALUES ("+study_id.toString()+", "+ps_id+")");
			}
		}
	}

	void importDiseaseDetails(NGSD& db, QString s_id, QStringList genlab_values, QString type, bool dry_run)
	{
		//get values already in NGSD
		QSet<QString> ngsd_values;
		foreach(const SampleDiseaseInfo& info, db.getSampleDiseaseInfo(s_id, type))
		{
			ngsd_values << info.disease_info;
		}

		//add new values
		foreach (const QString& genlab_v, genlab_values)
		{
			if (!ngsd_values.contains(genlab_v))
			{
                QTextStream(stdout) << "Adding disease details: " << type << " - " << genlab_v << QT_ENDL;

				SampleDiseaseInfo new_entry;
				new_entry.disease_info = genlab_v;
				new_entry.type = type;
				new_entry.user = "genlab_import";
				new_entry.date = QDateTime::currentDateTime();

				if (!dry_run) db.addSampleDiseaseInfo(s_id, new_entry);
			}
		}
	}

	void importRnaReferenceTissue(QString& ps_name, NGSD& db, bool dry_run)
	{
		//check that this is a RNA sample
		QString s_id = db.sampleId(ps_name);
		SampleData s_data = db.getSampleData(s_id);
		if (s_data.type!="RNA") return;

		//load HPO-tissue map
		QStringList lines = Helper::loadTextFile(":/HPO_to_RnaReferenceTissue.tsv", true, '#', true);
		QMap<QString, QString> map;
		foreach(const QString& line, lines)
		{
			QStringList parts = line.split("\t");
			if (parts.count()<2) continue;
			if (map.contains(parts[0])) THROW(FileParseException, "HPO-tissue map contains HPO term more than once:" + parts[0]);
			map.insert(parts[0], parts[1]);
		}

		//determine RNA reference tissue based on HPO terms
		QSet<QString> rna_reference_tissue;
		foreach(const SampleDiseaseInfo& term, db.getSampleDiseaseInfo(s_id, "HPO term id"))
		{
			if(map.contains(term.disease_info))
			{
				rna_reference_tissue << map[term.disease_info];
			}
		}
		if (rna_reference_tissue.count()==1)
		{
			QString tissue = *(rna_reference_tissue.begin());
			importDiseaseDetails(db, s_id, QStringList() << tissue, "RNA reference tissue", dry_run);
		}
		else if (rna_reference_tissue.count()>1)
		{

            QTextStream(stdout) << "WARNING: Cannot determine RNA reference tissue! Sample " +  ps_name + " has multiple HPO terms that are mapped to contradicting RNA reference tissues." << QT_ENDL;
		}
	}

	bool isSampleNewer(const QString& current_sample, const QString& other_sample)
	{
		QRegularExpression number("\\d+");
		QString current_sample_base = number.match(current_sample).captured(0);
		QString other_sample_base = number.match(other_sample).captured(0);

		QString current_sample_ps_id = current_sample.split("_")[1];
		QString other_sample_ps_id = other_sample.split("_")[1];

		if (current_sample_base == other_sample_base)
		{
			return current_sample_ps_id.toInt() < other_sample_ps_id.toInt();
		}

		return current_sample_base.toInt() < other_sample_base.toInt();
	}
};

#include "main.moc"

int main(int argc, char *argv[])
{
	ConcreteTool tool(argc, argv);
	return tool.execute();
}
