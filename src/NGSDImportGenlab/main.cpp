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
		setDescription("Import sample information from genlab into NGSD.");
		addString("ps_id", "sample for which the genlab data will be imported.", false);
		//optional
		addInfile("rna_tissue_mapping", "TSV file with mapping from HPO to rna reference tissue.", true, true);
		addFlag("no_relations", "Do not search and import sample relations from Genlab");
		addFlag("no_metadata", "Do not search and import metadata from Genlab (disease group, ICD10, HPO, ...)");
		QString desc = "Action for disease details that are already in NGSD: \n"
					   "\t\t\t\tADD: add additional metadata even when NGSD contains metadata of the same type applies to: disease details, relations\n"
					   "\t\t\t\tREPLACE: replace NGSD metadata with data from genlab. Applies to: gender, patient identifier disease group and disease status.\n"
					   "\t\t\t\tIGNORE: if NGSD already contains information of that type, do not import the info from genlab.";
		addEnum("action", desc, true, QStringList{"ADD", "REPLACE", "IGNORE"}, "IGNORE");
		addFlag("test", "Uses the test database instead of on the production database.");
		addFlag("debug", "Enables debug output.");
	}

	virtual void main()
	{
		QString ps_name = getString("ps_id");
		NGSD db(getFlag("test"));
		QString rna_tissue_map = getInfile("rna_tissue_mapping");

		if (! GenLabDB::isAvailable())
		{
			THROW(DatabaseException, "Genlab database is not available. Can't import data.");
		}

		GenLabDB genlab;

		if (! getFlag("no_relations"))
		{
			importSampleRelations(ps_name, db, genlab);
		}

		if (! getFlag("no_metadata"))
		{
			importGenLabMetadata(ps_name, db, genlab);
		}

		if (rna_tissue_map != "")
		{
			importRnaReferenceTissue(ps_name, db, rna_tissue_map);
		}
	}

	void importSampleRelations(const QString& ps_name, NGSD& db, GenLabDB& genlab)
	{
		QString current_ps_id = db.processedSampleId(ps_name);
		QString current_sample_id = db.sampleId(ps_name);

		SampleData current_sample_data = db.getSampleData(current_sample_id);
		ProcessedSampleData current_ps_data = db.getProcessedSampleData(current_ps_id);

		QStringList samples = genlab.patientSamples(ps_name);
		if (getFlag("debug")) qDebug() << "related samples (same patient): " << samples;

		if (samples.count() == 0) return;

		QList<SampleData> related_sample_data;

		// gather related sample data
		foreach(QString sample, samples)
		{
			QString sample_id = db.sampleId(sample);
			related_sample_data << db.getSampleData(sample_id);
		}

		if (current_ps_data.processing_system_type == "Panel" || current_ps_data.processing_system_type == "WES")
		{
			// TODO check if sample already has relation / overwrite is allowed
			checkForTumorNormalRelation(db, current_sample_data, current_ps_data, related_sample_data);
		}
		if (current_ps_data.processing_system_type == "RNA")
		{
			checkForSameSampleRelation(db, genlab, current_sample_data, related_sample_data);
		}

		//relatives patient relations (parents, siblings)

		QList<SampleRelation> relation_list = genlab.relatives(ps_name);
		foreach (SampleRelation genlab_relation, relation_list) {
			QSet<int> sample_ids_ngsd = db.relatedSamples(current_sample_id.toInt(), genlab_relation.relation);
			int sample2_id = db.sampleId(genlab_relation.sample1).toInt();

			if (sample_ids_ngsd.isEmpty() || (! sample_ids_ngsd.contains(sample2_id) && getEnum("action") == "ADD"))
			{
				if (getFlag("debug")) qDebug() << "Adding relative relation: " << genlab_relation.sample1 << " - " << genlab_relation.relation << " - " << genlab_relation.sample2;
				db.addSampleRelation(genlab_relation);
			}
		}
	}

	void checkForTumorNormalRelation(NGSD& db, const SampleData& current_sample_data, const ProcessedSampleData& current_ps_data, const QList<SampleData>& related_sample_data)
	{
		ProcessedSampleData best_candidate;

		foreach (SampleData data, related_sample_data)
		{
			if (data.is_tumor == current_sample_data.is_tumor) continue;
			if (data.type != "DNA") continue;

			//get processed samples:
			QStringList ps_ids = db.getValues("SELECT id FROM processed_sample WHERE sample_id = '" + db.sampleId(current_sample_data.name) + "'");

			foreach (QString ps_id, ps_ids)
			{
				ProcessedSampleData ps_data = db.getProcessedSampleData(ps_id);

				if (ps_data.processing_system_type != "Panel" && ps_data.processing_system_type != "WES") continue;
				if (ps_data.quality == "bad") continue;
				QString run_status = db.getValue("SELECT status FROM sequencing_run WHERE name='" + ps_data.run_name + "'").toString();
				if (run_status == "n/a" || run_status == "run_started" || run_status == "run_aborted" || run_status == "analysis_not_possible") continue;

				if (best_candidate.name.isEmpty())
				{
					best_candidate = ps_data;
				}
				else
				{
					if (best_candidate.processing_system == current_ps_data.processing_system && ps_data.processing_system != current_ps_data.processing_system) continue;

					// if new candidate is from the same run and the current best not always take it
					if (ps_data.run_name == current_ps_data.run_name && best_candidate.run_name != current_ps_data.run_name)
					{
						best_candidate = ps_data;
					}
					// if current best isn't from the same run take the newer sample
					else if (IsSampleNewer(best_candidate.name, ps_data.name))
					{
						best_candidate = ps_data;
					}
				}
			}
		}

		if (best_candidate.name.isEmpty()) return; // No acceptable candidate found.

		SqlQuery insert = db.getQuery();
		insert.prepare("INSERT IGNORE INTO sample_relations (sample1_id, relation, sample2_id) VALUES (:0,'tumor-normal',:1)");

		QString related_sample_name = best_candidate.name.split("_")[0];
		QString tumor_ps_id;
		QString tumor_ps_name;
		QString normal_ps_id;
		QString normal_ps_name;
		QString normal_sample_name;

		if (current_sample_data.is_tumor)
		{
			insert.bindValue(0, current_sample_data.name);
			insert.bindValue(1, related_sample_name);
			tumor_ps_id = db.processedSampleId(current_ps_data.name);
			tumor_ps_name = current_ps_data.name;
			normal_ps_id = db.processedSampleId(best_candidate.name);
			normal_ps_name = best_candidate.name;
			normal_sample_name = current_ps_data.normal_sample_name;
		}
		else
		{
			insert.bindValue(0, related_sample_name);
			insert.bindValue(1, current_sample_data.name);
			normal_ps_id = db.processedSampleId(current_ps_data.name);
			normal_ps_name = current_ps_data.name;
			tumor_ps_id = db.processedSampleId(best_candidate.name);
			tumor_ps_name = best_candidate.name;
			normal_sample_name = best_candidate.normal_sample_name;
		}

		//insert new relation:
		if (getFlag("debug")) qDebug() << "Importing new tumor normal relation: " << tumor_ps_name << " tumor-normal " << normal_ps_name;
//		insert.exec();

		//update normal id:
		if (current_sample_data.is_tumor && normal_sample_name == "")
		{
			if (getFlag("debug")) qDebug() << "Updating normal_id for tumor sample: " << tumor_ps_name << " new normal sample " << normal_ps_name;
//			db.getQuery().exec("UPDATE `processed_sample` SET normal_id = " + normal_ps_id + " WHERE id=" + tumor_ps_id);
		}
	}

	void checkForSameSampleRelation(NGSD& db, GenLabDB& genlab, const SampleData& current_sample_data, const QList<SampleData>& related_sample_data)
	{
		// search for a sample entered in the DnaRna table in GenLab:

		ProcessedSampleData genlab_related_sample;

		foreach (QString rel_sample_name, genlab.dnaSamplesofRna(current_sample_data.name_external))
		{
			QString rel_sample_id = db.sampleId(rel_sample_name, false);
			if (rel_sample_id == "") continue; // not found in NGSD
			if(db.getSampleData(rel_sample_id).type == "RNA") continue;

			foreach(QString ps_id, db.getValues("SELECT id FROM processed_sample WHERE sample_id = '" + db.sampleId(rel_sample_name) + "'"))
			{
				ProcessedSampleData data = db.getProcessedSampleData(ps_id);

				if (genlab_related_sample.name.isEmpty())
				{
					genlab_related_sample = data;
				}
				else
				{
					if (IsSampleNewer(genlab_related_sample.name, data.name))
					{
						genlab_related_sample = data;
					}
				}
			}
		}

		// found a related sample in genlab -> import and stop
		if (! genlab_related_sample.name.isEmpty())
		{
			SqlQuery insert = db.getQuery();
			insert.prepare("INSERT IGNORE INTO 'sample_relations' ('sample1_id', 'relation', 'sample2_id') VALUES (:0,'same sample',:1)");
			insert.bindValue(0, db.sampleId(current_sample_data.name));
			insert.bindValue(1, db.sampleId(genlab_related_sample.name));
			if (getFlag("debug")) qDebug() << "Importing sameSample relation based on genlabs dnarna table: " << current_sample_data.name << " same sample " << genlab_related_sample.name;
//			insert.exec();
			return;
		}

		//if there was nothing found in genlab DnaRna table search for a fitting sample in samples from the same patient.
		ProcessedSampleData best_candidate;

		foreach (SampleData data, related_sample_data)
		{
			if (data.is_tumor != current_sample_data.is_tumor) continue;
			if (data.type != "DNA") continue;

			//get processed samples:
			QStringList ps_ids = db.getValues("SELECT id FROM processed_sample WHERE sample_id = '" + db.sampleId(current_sample_data.name) + "'");

			foreach (QString ps_id, ps_ids)
			{
				ProcessedSampleData ps_data = db.getProcessedSampleData(ps_id);

				if (ps_data.processing_system_type != "Panel" && ps_data.processing_system_type != "WES" && ps_data.processing_system_type != "WGS") continue;
				if (ps_data.quality == "bad") continue;
				QString run_status = db.getValue("SELECT status FROM sequencing_run WHERE name == '" + ps_data.run_name + "'").toString();
				if (run_status == "n/a" || run_status == "run_started" || run_status == "run_aborted" || run_status == "analysis_not_possible") continue;


				if (best_candidate.name.isEmpty())
				{
					best_candidate = ps_data;
				}
				else
				{
					if (IsSampleNewer(best_candidate.name, ps_data.name)) best_candidate = ps_data;
				}
			}
		}

		if (! best_candidate.name.isEmpty())
		{
			SqlQuery insert = db.getQuery();
			insert.prepare("INSERT IGNORE INTO 'sample_relations' ('sample1_id', 'relation', 'sample2_id') VALUES (:0,'same sample',:1)");
			insert.bindValue(0, db.sampleId(current_sample_data.name));
			insert.bindValue(1, db.sampleId(best_candidate.name));
			if (getFlag("debug")) qDebug() << "Importing sameSample relation based on metadata: " << current_sample_data.name << "same sample " << best_candidate.name;
//			insert.exec();
			return;
		}
	}

	void importGenLabMetadata(QString& ps_name, NGSD& db, GenLabDB& genlab)
	{
		QString ps_id = db.processedSampleId(ps_name);
		QString s_id = db.getValue("SELECT sample_id FROM processed_sample WHERE id=:0", false, ps_id).toString();
		SampleData s_data = db.getSampleData(s_id);

		//***gender:
		QString gender = genlab.gender(ps_name);
		if (s_data.gender == "n/a" || (s_data.gender != gender && getEnum("action") == "REPLACE"))
		{
			if (getFlag("debug")) qDebug() << "Importing gender: " << gender;
//			db.getQuery().exec("UPDATE sample SET gender='" + gender + "' WHERE id=" + s_id);
		}

		//***patient_identifier
		QString patient_identifier = genlab.patientIdentifier(ps_name);
		if (s_data.patient_identifier == "" || (/**s_data.patient_identifier != patient_identifier && **/getEnum("action") == "REPLACE"))
		{
			if (getFlag("debug")) qDebug() << "Importing patient identifier: " << patient_identifier;
//			db.getQuery().exec("UPDATE sample SET patient_identifier='" + patient_identifier + "' WHERE id=" + s_id);
		}

		//***disease group and status
		auto disease_data = genlab.diseaseInfo(ps_name);
		QString disease_group = disease_data.first;
		QString disease_status = disease_data.second;
		if (s_data.disease_group == "n/a" || (/**s_data.disease_group != disease_group && **/getEnum("action") == "REPLACE"))
		{
			if (getFlag("debug")) qDebug() << "Importing disease group: " << disease_group;
//			db.getQuery().exec("UPDATE sample SET disease_group='" + disease_group + "' WHERE id=" + s_id);
		}

		if (disease_status != "n/a" && (s_data.disease_status == "n/a" || (/**s_data.disease_status != disease_status && **/getEnum("action") == "REPLACE")))
		{
			if (getFlag("debug")) qDebug() << "Importing disease status: " << disease_status;
//			db.getQuery().exec("UPDATE sample SET disease_status='" + disease_status + "' WHERE id=" + s_id);
		}

		//*** disease details *** 'HPO term id','ICD10 code','OMIM disease/phenotype identifier','Orpha number','CGI cancer type','tumor fraction','age of onset','clinical phenotype (free text)','RNA reference tissue')
		importDiseaseDetails(db, s_id, genlab.anamnesis(ps_name), "clinical phenotype (free text)");
		importDiseaseDetails(db, s_id, genlab.orphanet(ps_name), "Orpha number");
		importDiseaseDetails(db, s_id, genlab.diagnosis(ps_name), "ICD10 code");
		importDiseaseDetails(db, s_id, genlab.tumorFraction(ps_name), "tumor fraction");
		PhenotypeList phenotypes = genlab.phenotypes(ps_name);
		QStringList genlab_phenotype_values;
		foreach (Phenotype genlab_v, phenotypes) {
			genlab_phenotype_values << genlab_v.accession();
		}
		importDiseaseDetails(db, s_id, genlab_phenotype_values, "HPO term id");

		//***Studies:

		QStringList genlab_studies = genlab.studies(ps_name);
		QStringList ngsd_studies = db.getValues("SELECT st.name FROM study st, study_sample ss, processed_sample ps WHERE ss.study_id=st.id AND ps.id=ss.processed_sample_id AND ps.sample_id=" + s_id);

		foreach(QString study, genlab_studies)
		{
			if (ngsd_studies.count() == 0 || (! ngsd_studies.contains(study) && getEnum("action") == "ADD"))
			{
				QVariant study_id = db.getValue("SELECT id FROM study WHERE name=:0", true, study);
				if (!study_id.isValid()) INFO(ArgumentException, "GenLab study name '" + study + "' not found in NGSD! Please add the study to NGSD, or correcte the study name in GenLab!");

				if (getFlag("debug")) qDebug() << "Importing new study: " << study;
//				db.getQuery().exec("INSERT INTO `study_sample`(`study_id`, `processed_sample_id`) VALUES ("+study_id.toString()+", "+ps_id+")");
			}
		}
	}

	void importDiseaseDetails(NGSD& db, QString s_id, QStringList genlab_values, QString type)
	{
		QStringList ngsd_values = ngsdDiseaseDetails(db, s_id, type);
		foreach (QString genlab_v, genlab_values)
		{
			if (ngsd_values.empty() || (! ngsd_values.contains(genlab_v) && getEnum("action") == "ADD"))
			{
				if (getFlag("debug")) qDebug() << "Importing disease details: " << type << " - " << genlab_v;

				QList<SampleDiseaseInfo> disease_details = db.getSampleDiseaseInfo(s_id);
				SampleDiseaseInfo new_entry;
				new_entry.disease_info = genlab_v;
				new_entry.type = type;
				new_entry.user = "genlab_import";
				new_entry.date = QDateTime::currentDateTime();
				disease_details << new_entry;
				//NGSD Import:
//				db.setSampleDiseaseInfo(s_id, disease_details);
			}
		}
	}

	QStringList ngsdDiseaseDetails(NGSD& db, QString sample_id, QString type)
	{
		QStringList values;

		QList<SampleDiseaseInfo> infos = db.getSampleDiseaseInfo(sample_id, type);

		foreach(SampleDiseaseInfo info, infos)
		{
			values << info.disease_info;
		}
		return values;
	}

	void importRnaReferenceTissue(QString& ps_name, NGSD& db, QString rna_tissue_map)
	{
		auto map = loadHPOtoTissueMap(rna_tissue_map);
		QString s_id = db.sampleId(ps_name);

		QList<SampleDiseaseInfo> hpo_terms = db.getSampleDiseaseInfo(s_id, "HPO term id");

		QString rna_reference_tissue= "";
		foreach(SampleDiseaseInfo term, hpo_terms)
		{
			if(map.keys().contains(term.disease_info))
			{
				if (rna_reference_tissue == "")
				{
					rna_reference_tissue = map[term.disease_info];
				}
				else if (rna_reference_tissue != map[term.disease_info])
				{
					//TODO warning or error?
					THROW(ArgumentException, "Cannot determine rna reference tissue! Sample " +  ps_name + " has multiple HPO terms that are mapped to contradicting rna reference tissues.");
				}
			}
		}
		if (rna_reference_tissue != "")
		{
			QStringList tmp;
			tmp << rna_reference_tissue;
			importDiseaseDetails(db, s_id, tmp, "RNA reference tissue");
		}
	}

	QMap<QString, QString> loadHPOtoTissueMap(QString rna_tissue_map)
	{
		QStringList lines = Helper::loadTextFile(rna_tissue_map, true, '#', true);

		QMap<QString, QString> map;
		foreach(const QString& line, lines)
		{
			QStringList parts = line.split("\t");
			map.insert(parts[0], parts[1]);
		}

		return map;
	}

	bool IsSampleNewer (QString current_sample, QString other_sample)
	{
		const QRegularExpression rx(QLatin1Literal("[^0-9]+"));
		QString current_sample_number = rx.match(current_sample).captured(0);
		QString other_sample_number = rx.match(other_sample).captured(0);

		return current_sample_number < other_sample_number;
	}

};

#include "main.moc"

int main(int argc, char *argv[])
{
	ConcreteTool tool(argc, argv);
	return tool.execute();
}
