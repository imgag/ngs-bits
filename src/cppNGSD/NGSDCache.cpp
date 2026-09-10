#include "NGSDCache.h"
#include "Helper.h"
#include <QMutexLocker>
#include <QSqlDriver>
#include <QSqlIndex>

NGSDReferenceDataCache& NGSDReferenceDataCache::instance(const QString& database_context)
{
	static QMutex registry_mutex;
	static QHash<QString, QSharedPointer<NGSDReferenceDataCache>> caches;
	QMutexLocker locker(&registry_mutex);
	if (!caches.contains(database_context)) caches.insert(database_context, QSharedPointer<NGSDReferenceDataCache>(new NGSDReferenceDataCache));
	return *caches.value(database_context);
}

NGSDReferenceDataCache::NGSDReferenceDataCache()
	: gene_transcripts_index_(gene_transcripts_)
{
}

void NGSDReferenceDataCache::clear()
{
	QMutexLocker locker(&mutex_);

	table_infos_.clear();
	same_samples_.clear();
	same_patients_.clear();
	related_samples_.clear();
	same_samples_initialized_ = false;
	same_patients_initialized_ = false;
	related_samples_initialized_ = false;
	approved_gene_names_.clear();
	approved_gene_names_initialized_ = false;
	gene2id_.clear();
	gene2id_initialized_ = false;
	id2gene_.clear();
	id2gene_initialized_ = false;
	enum_values_.clear();
	non_approved_to_approved_gene_names_.clear();
	phenotypes_by_id_.clear();
	phenotypes_by_id_initialized_ = false;
	phenotypes_accession_to_id_.clear();
	phenotype_accession_to_id_initialized_ = false;
	hpo_genes_.clear();
	hpo_genes_initialized_ = false;
	hpo_parent_.clear();
	hpo_parent_initialized_ = false;
	gene_symbol_to_somatic_gene_role_.clear();
	somatic_gene_roles_initialized_ = false;
	gene_id_to_hgnc_.clear();
	gene_id_to_hgnc_initialized_ = false;
	hgnc_id_to_gene_id_.clear();
	hgnc_to_gene_id_initialized_ = false;

	gene_transcripts_.clear();
	gene_transcripts_index_.createIndex();
	gene_transcripts_id2index_.clear();
	gene_transcripts_symbol2indices_.clear();
	gene_transcripts_name2id_.clear();
	transcript_cache_initialized_ = false;

	gene_expression_id2gene_.clear();
	gene_expression_gene2id_.clear();
	gene_expression_cache_initialized_ = false;
}

const QSet<int>& NGSDReferenceDataCache::sameSamples(NGSD& db, int sample_id, SameSampleMode mode)
{
	QMutexLocker locker(&mutex_);
	static const QSet<int> empty_entry;
	QHash<int, QSet<int>>& cache = mode == SameSampleMode::SAME_PATIENT ? same_patients_ : same_samples_;
	bool& initialized = mode == SameSampleMode::SAME_PATIENT ? same_patients_initialized_ : same_samples_initialized_;
	if (!initialized)
	{
		QHash<int, QSet<int>> id2same;
		SqlQuery query = db.getQuery();
		query.exec("SELECT sample1_id, sample2_id FROM sample_relations WHERE (relation='same sample'" + QString(mode == SameSampleMode::SAME_PATIENT ? " OR relation='same patient')" : ")"));
		while (query.next())
		{
			const int id1 = query.value(0).toInt();
			const int id2 = query.value(1).toInt();
			id2same[id1] << id2;
			id2same[id2] << id1;
		}
		for (auto it=id2same.cbegin(); it!=id2same.cend(); ++it)
		{
			const int sample1_id = it.key();
			if (cache.contains(sample1_id)) continue;
			QSet<int> cluster{sample1_id};
			int previous_size = -1;
			while (previous_size != cluster.size())
			{
				previous_size = cluster.size();
				const QSet<int> current_cluster = cluster;
				for (int id : current_cluster) cluster.unite(id2same.value(id));
			}
			for (int id : std::as_const(cluster))
			{
				QSet<int> current = cluster;
				current.remove(id);
				cache[id] = std::move(current);
			}
		}
		if (mode == SameSampleMode::SAME_PATIENT)
		{
			query.exec("SELECT id, patient_identifier FROM sample WHERE patient_identifier IS NOT NULL AND patient_identifier!=''");
			QHash<QString, QSet<int>> by_patient;
			while (query.next())
			{
				const QString patient = query.value(1).toString().trimmed();
				if (!patient.isEmpty()) by_patient[patient] << query.value(0).toInt();
			}
			for (const QSet<int>& sample_ids : std::as_const(by_patient))
			{
				if (sample_ids.size() < 2) continue;
				QSet<int> combined;
				for (int id : sample_ids) { combined << id; combined += cache.value(id); }
				for (int id : std::as_const(combined))
				{
					QSet<int> current = combined;
					current.remove(id);
					cache[id] = std::move(current);
				}
			}
		}
		initialized = true;
	}
	const auto it = cache.constFind(sample_id);
	return it == cache.cend() ? empty_entry : it.value();
}

const QSet<int>& NGSDReferenceDataCache::relatedSamples(NGSD& db, int sample_id)
{
	QMutexLocker locker(&mutex_);
	static const QSet<int> empty_entry;
	if (!related_samples_initialized_)
	{
		SqlQuery query = db.getQuery();
		query.exec("SELECT sample1_id, sample2_id FROM sample_relations");
		while (query.next())
		{
			const int id1 = query.value(0).toInt();
			const int id2 = query.value(1).toInt();
			related_samples_[id1] << id2;
			related_samples_[id2] << id1;
		}
		related_samples_initialized_ = true;
	}
	const auto it = related_samples_.constFind(sample_id);
	return it == related_samples_.cend() ? empty_entry : it.value();
}

const GeneSet& NGSDReferenceDataCache::approvedGeneNames(NGSD& db)
{
	QMutexLocker locker(&mutex_);
	if (!approved_gene_names_initialized_)
	{
		for (const QString& symbol : db.getValues("SELECT symbol FROM gene")) approved_gene_names_.insert(symbol.toUtf8());
		approved_gene_names_initialized_ = true;
	}
	return approved_gene_names_;
}

int NGSDReferenceDataCache::geneId(NGSD& db, const QByteArray& gene)
{
	QMutexLocker locker(&mutex_);
	if (!gene2id_initialized_)
	{
		SqlQuery query = db.getQuery();
		query.exec("SELECT symbol, id FROM gene");
		while (query.next())
		{
			const QByteArray symbol = query.value(0).toByteArray();
			const int id = query.value(1).toInt();
			gene2id_[symbol] = id;
			gene2id_[symbol.trimmed().toUpper()] = id;
		}
		gene2id_initialized_ = true;
	}
	if (gene2id_.contains(gene)) return gene2id_.value(gene);
	const QByteArray normalized_gene = gene.trimmed().toUpper();
	if (gene2id_.contains(normalized_gene))
	{
		const int id = gene2id_.value(normalized_gene);
		gene2id_.insert(gene, id);
		return id;
	}

	SqlQuery query = db.getQuery();
	query.prepare("SELECT g.id FROM gene g, gene_alias ga WHERE g.id=ga.gene_id AND ga.symbol=:0 AND ga.type='previous'");
	query.bindValue(0, gene);
	query.exec();
	if (query.size()==1) { query.next(); return gene2id_[gene] = query.value(0).toInt(); }
	if (query.size()>1) return gene2id_[gene] = -1;
	query.prepare("SELECT g.id FROM gene g, gene_alias ga WHERE g.id=ga.gene_id AND ga.symbol=:0 AND ga.type='synonym'");
	query.bindValue(0, gene);
	query.exec();
	if (query.size()==1) { query.next(); return gene2id_[gene] = query.value(0).toInt(); }
	return gene2id_[gene] = -1;
}

QByteArray NGSDReferenceDataCache::geneSymbol(NGSD& db, int id)
{
	QMutexLocker locker(&mutex_);
	if (!id2gene_initialized_)
	{
		SqlQuery query = db.getQuery();
		query.exec("SELECT id, symbol FROM gene");
		while (query.next()) id2gene_[query.value(0).toInt()] = query.value(1).toByteArray();
		id2gene_initialized_ = true;
	}
	if (!id2gene_.contains(id)) THROW(DatabaseException, "No gene with database ID '" + QString::number(id) + "' in NGSD!");
	return id2gene_.value(id);
}

QByteArray NGSDReferenceDataCache::geneHgncId(NGSD& db, int id)
{
	QMutexLocker locker(&mutex_);
	if (!gene_id_to_hgnc_initialized_)
	{
		SqlQuery query = db.getQuery();
		query.exec("SELECT id, hgnc_id FROM gene");
		while (query.next()) gene_id_to_hgnc_.insert(query.value(0).toInt(), "HGNC:" + query.value(1).toByteArray());
		gene_id_to_hgnc_initialized_ = true;
	}
	if (!gene_id_to_hgnc_.contains(id)) THROW(DatabaseException, "No gene with database ID '" + QString::number(id) + "' in NGSD!");
	return gene_id_to_hgnc_.value(id);
}

int NGSDReferenceDataCache::hgncIdToGeneId(NGSD& db, QByteArray hgnc_id)
{
	QMutexLocker locker(&mutex_);
	if (!hgnc_to_gene_id_initialized_)
	{
		SqlQuery query = db.getQuery();
		query.exec("SELECT hgnc_id, id FROM gene");
		while (query.next()) hgnc_id_to_gene_id_.insert("HGNC:" + query.value(0).toByteArray(), query.value(1).toInt());
		hgnc_to_gene_id_initialized_ = true;
	}
	hgnc_id = hgnc_id.trimmed();
	if (!hgnc_id.startsWith("HGNC:")) hgnc_id.prepend("HGNC:");
	if (!hgnc_id_to_gene_id_.contains(hgnc_id)) THROW(DatabaseException, "No gene with HGNC ID '" + hgnc_id + "' in NGSD!");
	return hgnc_id_to_gene_id_.value(hgnc_id);
}

QByteArray NGSDReferenceDataCache::geneToApproved(NGSD& db, QByteArray gene, bool return_input_when_unconvertable)
{
	QMutexLocker locker(&mutex_);
	gene = gene.trimmed().toUpper();
	if (approvedGeneNames(db).contains(gene)) return gene;
	if (!non_approved_to_approved_gene_names_.contains(gene))
	{
		const int id = geneId(db, gene);
		non_approved_to_approved_gene_names_[gene] = id == -1 ? QByteArray() : geneSymbol(db, id);
	}
	const QByteArray approved = non_approved_to_approved_gene_names_.value(gene);
	return return_input_when_unconvertable && approved.isEmpty() ? gene : approved;
}

QStringList NGSDReferenceDataCache::enumValues(NGSD& db, const QString& table, const QString& column, bool use_cache)
{
	QMutexLocker locker(&mutex_);
	const QString key = table + "." + column;
	if (use_cache && enum_values_.contains(key)) return enum_values_.value(key);
	SqlQuery query = db.getQuery();
	query.exec("DESCRIBE " + table + " " + column);
	if (query.next())
	{
		QString type = query.value(1).toString();
		if (type.startsWith("enum(")) type = type.mid(6, type.length()-8);
		else if (type.startsWith("set(")) type = type.mid(5, type.length()-7);
		else THROW(ProgrammingException, "Could not determine enum values of column '"+column+"' in table '"+table+"'! Column type doesn't start with 'enum' or 'set'. Type: " + type);
		enum_values_[key] = type.split("','");
		return enum_values_.value(key);
	}
	THROW(ProgrammingException, "Could not determine enum values of column '"+column+"' in table '"+table+"'!");
}

const QHash<int, QList<QByteArray>>& NGSDReferenceDataCache::hpoGenes(NGSD& db)
{
	QMutexLocker locker(&mutex_);
	if (!hpo_genes_initialized_)
	{
		SqlQuery query = db.getQuery();
		query.exec("SELECT hpo_term_id, gene FROM hpo_genes");
		while (query.next()) hpo_genes_[query.value(0).toInt()] << query.value(1).toByteArray();
		hpo_genes_initialized_ = true;
	}
	return hpo_genes_;
}

const QHash<int, QList<int>>& NGSDReferenceDataCache::hpoParent(NGSD& db)
{
	QMutexLocker locker(&mutex_);
	if (!hpo_parent_initialized_)
	{
		SqlQuery query = db.getQuery();
		query.exec("SELECT parent, child FROM hpo_parent");
		while (query.next()) hpo_parent_[query.value(0).toInt()] << query.value(1).toInt();
		hpo_parent_initialized_ = true;
	}
	return hpo_parent_;
}

SomaticGeneRole NGSDReferenceDataCache::somaticGeneRole(NGSD& db, const QByteArray& gene, bool throw_on_fail)
{
	QMutexLocker locker(&mutex_);
	initSomaticGeneRoleCache(db);
	const QString approved = geneToApproved(db, gene, true);
	if (!gene_symbol_to_somatic_gene_role_.contains(approved))
	{
		if (throw_on_fail) THROW(DatabaseException, "There is no somatic gene role for gene symbol '" + gene + "' (used approved symbol" + approved + ") in the NGSD.");
		return SomaticGeneRole();
	}
	return gene_symbol_to_somatic_gene_role_.value(approved);
}

void NGSDReferenceDataCache::initSomaticGeneRoleCache(NGSD& db)
{
	QMutexLocker locker(&mutex_);
	if (somatic_gene_roles_initialized_) return;
	SqlQuery query = db.getQuery();
	query.exec("SELECT symbol, gene_role, high_evidence, comment FROM somatic_gene_role");
	while (query.next())
	{
		SomaticGeneRole role;
		role.gene = query.value(0).toByteArray();
		const QString role_string = query.value(1).toString();
		if (role_string=="activating") role.role = SomaticGeneRole::Role::ACTIVATING;
		else if (role_string=="loss_of_function") role.role = SomaticGeneRole::Role::LOSS_OF_FUNCTION;
		else if (role_string=="ambiguous") role.role = SomaticGeneRole::Role::AMBIGUOUS;
		else THROW(DatabaseException, "Unknown gene role '" + role_string + "' in relation 'somatic_gene_role'.");
		role.high_evidence = query.value(2).toBool();
		role.comment = query.value(3).toString();
		gene_symbol_to_somatic_gene_role_.insert(role.gene, role);
	}
	somatic_gene_roles_initialized_ = true;
}

QMap<QString, SomaticGeneRole> NGSDReferenceDataCache::somaticGeneRoles(NGSD& db, bool only_high_evidence)
{
	QMutexLocker locker(&mutex_);
	initSomaticGeneRoleCache(db);
	if (!only_high_evidence) return gene_symbol_to_somatic_gene_role_;
	QMap<QString, SomaticGeneRole> output;
	for (auto it=gene_symbol_to_somatic_gene_role_.cbegin(); it!=gene_symbol_to_somatic_gene_role_.cend(); ++it)
	{
		if (it.value().high_evidence) output.insert(it.key(), it.value());
	}
	return output;
}

void NGSDReferenceDataCache::updateSomaticGeneRole(const SomaticGeneRole& role)
{
	QMutexLocker locker(&mutex_);
	if (somatic_gene_roles_initialized_) gene_symbol_to_somatic_gene_role_.insert(role.gene, role);
}

void NGSDReferenceDataCache::removeSomaticGeneRole(const QByteArray& gene)
{
	QMutexLocker locker(&mutex_);
	if (somatic_gene_roles_initialized_) gene_symbol_to_somatic_gene_role_.remove(gene);
}

void NGSDReferenceDataCache::initPhenotypeCache(NGSD& db)
{
	QMutexLocker locker(&mutex_);
	if (phenotypes_by_id_initialized_ && phenotype_accession_to_id_initialized_) return;
	QHash<int, Phenotype> phenotypes;
	QHash<QByteArray, int> accession_to_id;
	SqlQuery query = db.getQuery();
	query.exec("SELECT id, hpo_id, name FROM hpo_term");
	while (query.next())
	{
		const int id = query.value(0).toInt();
		const QByteArray accession = query.value(1).toByteArray();
		phenotypes[id] = Phenotype(accession, query.value(2).toByteArray());
		accession_to_id[accession] = id;
	}
	phenotypes_by_id_ = std::move(phenotypes);
	phenotypes_accession_to_id_ = std::move(accession_to_id);
	phenotypes_by_id_initialized_ = true;
	phenotype_accession_to_id_initialized_ = true;
}

int NGSDReferenceDataCache::phenotypeIdByAccession(NGSD& db, const QByteArray& accession, bool throw_on_error)
{
	QMutexLocker locker(&mutex_);
	initPhenotypeCache(db);
	if (!phenotypes_accession_to_id_.contains(accession))
	{
		if (throw_on_error) THROW(DatabaseException, "Unknown HPO phenotype accession '" + accession + "'!");
		return -1;
	}
	return phenotypes_accession_to_id_.value(accession);
}

const Phenotype& NGSDReferenceDataCache::phenotype(NGSD& db, int id)
{
	QMutexLocker locker(&mutex_);
	initPhenotypeCache(db);
	if (!phenotypes_by_id_.contains(id)) THROW(DatabaseException, "HPO phenotype with id '" + QString::number(id) + "' not found in NGSD!");
	return phenotypes_by_id_[id];
}

PhenotypeList NGSDReferenceDataCache::phenotypes(NGSD& db)
{
	QMutexLocker locker(&mutex_);
	initPhenotypeCache(db);
	PhenotypeList output;
	for (const Phenotype& phenotype : std::as_const(phenotypes_by_id_)) output << phenotype;
	output.sortByName();
	return output;
}

void NGSDReferenceDataCache::initTranscriptCache(NGSD& db)
{
	QMutexLocker locker(&mutex_);
	if (transcript_cache_initialized_) return;

	//Build into temporary containers so failed database access cannot publish a partial cache.
	TranscriptList transcripts;
	QHash<int, int> id2index;
	QHash<QByteArray, QSet<int>> symbol2indices;
	QHash<QByteArray, int> name2id;

	QSet<QByteArray> preferred_transcripts;
	for (const QString& trans : db.getValues("SELECT DISTINCT name FROM preferred_transcripts"))
	{
		preferred_transcripts.insert(trans.toUtf8());
	}

	QHash<int, QList<QPair<int, int>>> coordinates;
	SqlQuery query = db.getQuery();
	query.setForwardOnly(true);
	query.exec("SELECT transcript_id, start, end FROM gene_exon ORDER BY start, end");
	while(query.next())
	{
		coordinates[query.value(0).toInt()] << qMakePair(query.value(1).toInt(), query.value(2).toInt());
	}

	QHash<QByteArray, int> temporary_name_to_id;
	query.exec("SELECT t.id, g.symbol, t.name, t.source, t.strand, t.chromosome, t.start_coding, t.end_coding, t.biotype, t.is_gencode_basic, t.is_gencode_primary, t.is_ensembl_canonical, t.is_mane_select, t.is_mane_plus_clinical, t.version, g.ensembl_id FROM gene_transcript t, gene g WHERE t.gene_id=g.id");
	while(query.next())
	{
		const int transcript_id = query.value(0).toInt();
		Transcript transcript;
		transcript.setGene(query.value(1).toByteArray());
		transcript.setName(query.value(2).toByteArray());
		transcript.setSource(Transcript::stringToSource(query.value(3).toString()));
		transcript.setStrand(Transcript::stringToStrand(query.value(4).toByteArray()));
		transcript.setBiotype(Transcript::stringToBiotype(query.value(8).toByteArray()));
		transcript.setPreferredTranscript(preferred_transcripts.contains(transcript.name()));
		transcript.setGencodeBasicTranscript(query.value(9).toInt()!=0);
		transcript.setGencodePrimaryTranscript(query.value(10).toInt()!=0);
		transcript.setEnsemblCanonicalTranscript(query.value(11).toInt()!=0);
		transcript.setManeSelectTranscript(query.value(12).toInt()!=0);
		transcript.setManePlusClinicalTranscript(query.value(13).toInt()!=0);
		transcript.setVersion(query.value(14).toInt());
		transcript.setGeneId(query.value(15).toByteArray());

		BedFile regions;
		const Chromosome chromosome = "chr" + query.value(5).toByteArray();
		for (const auto& coordinate : coordinates[transcript_id])
		{
			regions.append(BedLine(chromosome, coordinate.first, coordinate.second));
		}
		int start_coding = query.value(6).isNull() ? 0 : query.value(6).toInt();
		int end_coding = query.value(7).isNull() ? 0 : query.value(7).toInt();
		if (transcript.strand()==Transcript::MINUS) std::swap(start_coding, end_coding);
		transcript.setRegions(regions, start_coding, end_coding);

		transcripts << transcript;
		temporary_name_to_id[transcript.name()] = transcript_id;
	}

	transcripts.sortByPosition();
	for (int i=0; i<transcripts.count(); ++i)
	{
		const Transcript& transcript = transcripts[i];
		const int transcript_id = temporary_name_to_id[transcript.name()];
		id2index[transcript_id] = i;
		name2id[transcript.name()] = transcript_id;
		symbol2indices[transcript.gene()] << i;
	}

	gene_transcripts_ = std::move(transcripts);
	gene_transcripts_id2index_ = std::move(id2index);
	gene_transcripts_symbol2indices_ = std::move(symbol2indices);
	gene_transcripts_name2id_ = std::move(name2id);
	gene_transcripts_index_.createIndex();
	transcript_cache_initialized_ = true;
}

void NGSDReferenceDataCache::initGeneExpressionCache(NGSD& db)
{
	QMutexLocker locker(&mutex_);
	if (gene_expression_cache_initialized_) return;

	QMap<int, QByteArray> id2gene;
	QMap<QByteArray, int> gene2id;
	SqlQuery query = db.getQuery();
	query.exec("SELECT id, symbol FROM expression_gene");
	while(query.next())
	{
		id2gene.insert(query.value(0).toInt(), query.value(1).toByteArray());
		gene2id.insert(query.value(1).toByteArray(), query.value(0).toInt());
	}

	gene_expression_id2gene_ = std::move(id2gene);
	gene_expression_gene2id_ = std::move(gene2id);
	gene_expression_cache_initialized_ = true;
}

GeneSet NGSDReferenceDataCache::genesOverlapping(NGSD& db, const Chromosome& chr, int start, int end, int extend, bool exons_only)
{
	QMutexLocker locker(&mutex_);
	initTranscriptCache(db);
	start -= extend;
	end += extend;
	GeneSet output;
	for (int index : gene_transcripts_index_.matchingIndices(chr, start, end))
	{
		const Transcript& transcript = gene_transcripts_[index];
		if (!exons_only) { output << transcript.gene(); continue; }
		if (output.contains(transcript.gene())) continue;
		const BedFile& regions = transcript.regions();
		for (int i=0; i<regions.count(); ++i)
		{
			if (regions[i].overlapsWith(chr, start, end)) { output << transcript.gene(); break; }
		}
	}
	return output;
}

int NGSDReferenceDataCache::transcriptId(NGSD& db, const QByteArray& name, bool throw_on_error)
{
	QMutexLocker locker(&mutex_);
	initTranscriptCache(db);
	int id = gene_transcripts_name2id_.value(name, -1);
	if (id==-1 && name.contains('.')) id = gene_transcripts_name2id_.value(name.left(name.indexOf('.')), -1);
	if (id==-1 && throw_on_error) THROW(DatabaseException, "No transcript with name '" + name + "' found in NGSD!");
	return id;
}

TranscriptList NGSDReferenceDataCache::transcripts(NGSD& db, int gene_id, Transcript::SOURCE source, bool coding_only)
{
	QMutexLocker locker(&mutex_);
	initTranscriptCache(db);
	TranscriptList output;
	const QByteArray gene = geneSymbol(db, gene_id);
	for (int index : gene_transcripts_symbol2indices_.value(gene))
	{
		const Transcript& transcript = gene_transcripts_[index];
		if (transcript.source()!=source || (coding_only && !transcript.isCoding())) continue;
		output << transcript;
	}
	output.sortByPosition();
	return output;
}

TranscriptList NGSDReferenceDataCache::transcriptsOverlapping(NGSD& db, const Chromosome& chr, int start, int end, int extend, Transcript::SOURCE source)
{
	QMutexLocker locker(&mutex_);
	initTranscriptCache(db);
	TranscriptList output;
	for (int index : gene_transcripts_index_.matchingIndices(chr, start-extend, end+extend))
	{
		if (gene_transcripts_[index].source()==source) output << gene_transcripts_[index];
	}
	return output;
}

const TranscriptList& NGSDReferenceDataCache::transcripts(NGSD& db)
{
	QMutexLocker locker(&mutex_);
	initTranscriptCache(db);
	return gene_transcripts_;
}

const Transcript& NGSDReferenceDataCache::transcript(NGSD& db, int id)
{
	QMutexLocker locker(&mutex_);
	initTranscriptCache(db);
	const int index = gene_transcripts_id2index_.value(id, -1);
	if (index==-1) THROW(DatabaseException, "Could not find transcript with ID '" + QString::number(id) + "' in NGSD!");
	return gene_transcripts_[index];
}

bool NGSDReferenceDataCache::transcriptCacheInitialized()
{
	QMutexLocker locker(&mutex_);
	return transcript_cache_initialized_;
}

QMap<int, QByteArray> NGSDReferenceDataCache::expressionIdToGene(NGSD& db)
{
	QMutexLocker locker(&mutex_);
	initGeneExpressionCache(db);
	return gene_expression_id2gene_;
}

QMap<QByteArray, int> NGSDReferenceDataCache::expressionGeneToId(NGSD& db)
{
	QMutexLocker locker(&mutex_);
	initGeneExpressionCache(db);
	return gene_expression_gene2id_;
}

int NGSDReferenceDataCache::expressionGeneId(NGSD& db, const QByteArray& gene)
{
	QMutexLocker locker(&mutex_);
	initGeneExpressionCache(db);
	if (!gene_expression_gene2id_.contains(gene))
	{
		const int id = db.addGeneSymbolToExpressionTable(gene);
		gene_expression_gene2id_.insert(gene, id);
		gene_expression_id2gene_.insert(id, gene);
	}
	return gene_expression_gene2id_.value(gene);
}

const TableInfo& NGSDReferenceDataCache::tableInfo(NGSD& db, const QString& table, bool use_cache)
{
	QMutexLocker cache_locker(&mutex_);
	QMap<QString, TableInfo>& table_infos = table_infos_;

	//create if necessary
	if (!table_infos.contains(table) || !use_cache)
	{
		//check table exists
		if (!db.tables().contains(table))
		{
			THROW(DatabaseException, "Table '" + table + "' not found in NGSD!");
		}

		TableInfo output;
		output.setTable(table);

		//get PK info
		QSqlIndex index = db.db_->driver()->primaryIndex(table);

		//get FK info
		SqlQuery query_fk = db.getQuery();
		query_fk.exec("SELECT k.COLUMN_NAME, k.REFERENCED_TABLE_NAME, k.REFERENCED_COLUMN_NAME FROM information_schema.TABLE_CONSTRAINTS i LEFT JOIN information_schema.KEY_COLUMN_USAGE k ON i.CONSTRAINT_NAME = k.CONSTRAINT_NAME "
					"WHERE i.CONSTRAINT_TYPE = 'FOREIGN KEY' AND i.TABLE_SCHEMA = DATABASE() AND i.TABLE_NAME='" + table + "'");

		QList<TableFieldInfo> infos;
		SqlQuery query = db.getQuery();
		query.exec("DESCRIBE " + table);
		while(query.next())
		{
			TableFieldInfo info;

			//name
			info.name = query.value(0).toString();

			//index
			info.index = output.fieldCount();

			//type
			QString type = query.value(1).toString().toLower();
			info.is_unsigned = type.contains(" unsigned");
			if (info.is_unsigned)
			{
				type = type.replace(" unsigned", "");
			}
			if(type=="text") info.type = TableFieldInfo::TEXT;
			else if(type=="mediumtext") info.type = TableFieldInfo::TEXT;
			else if(type=="float") info.type = TableFieldInfo::FLOAT;
			else if(type=="date") info.type = TableFieldInfo::DATE;
			else if(type=="datetime") info.type = TableFieldInfo::DATETIME;
			else if(type=="timestamp") info.type = TableFieldInfo::TIMESTAMP;
			else if(type=="tinyint(1)") info.type = TableFieldInfo::BOOL;
			else if(type=="int" || type.startsWith("int(") || type.startsWith("tinyint(")) info.type = TableFieldInfo::INT;
			else if (type=="bigint" || type.startsWith("bigint(")) info.type = TableFieldInfo::LONG;
			else if(type.startsWith("enum("))
			{
				info.type = TableFieldInfo::ENUM;
				info.type_constraints.valid_strings = enumValues(db, table, info.name, use_cache);
			}
			else if (type.startsWith("set("))
			{
				info.type = TableFieldInfo::SET;
				info.type_constraints.valid_strings = enumValues(db, table, info.name, use_cache);
			}
			else if(type.startsWith("varchar("))
			{
				info.type = TableFieldInfo::VARCHAR;
				info.type_constraints.max_length = Helper::toInt(type.mid(8, type.length()-9), "VARCHAR length");

				//password column
				if (table=="user" && info.name=="password")
				{
					info.type = TableFieldInfo::VARCHAR_PASSWORD;
				}

				//special constraints
				if (table=="sample" && info.name=="name") info.type_constraints.regexp = QRegularExpression("^[A-Za-z0-9-]*$");
				if (table=="mid" && info.name=="sequence") info.type_constraints.regexp = QRegularExpression("^[ACGT]*$");
				if (table=="project" && info.name=="name") info.type_constraints.regexp = QRegularExpression("^[A-Za-z0-9_-]*$");
				if (table=="processing_system" && info.name=="name_short") info.type_constraints.regexp = QRegularExpression("^[A-Za-z0-9_\\.-]*$");
				if (table=="processing_system" && info.name=="adapter1_p5") info.type_constraints.regexp = QRegularExpression("^[ACGTN]*$");
				if (table=="processing_system" && info.name=="adapter2_p7") info.type_constraints.regexp = QRegularExpression("^[ACGTN]*$");
				if (table=="processed_sample" && info.name=="lane") info.type_constraints.regexp = QRegularExpression("^[1-8](,[1-8])*$");
				if (table=="user" && info.name=="user_id") info.type_constraints.regexp = QRegularExpression("^[A-Za-z0-9_]+$");
				if (table=="study" && info.name=="name") info.type_constraints.regexp = QRegularExpression("^[A-Za-z0-9_ -\\.]+$");
			}
			else
			{
				THROW(ProgrammingException, "Unhandled SQL field type '" + type + "' in field '" + info.name + "' of table '" + table + "'!");
			}

			//nullable
			info.is_nullable = query.value(2).toString().toLower()=="yes";

			//PK
			info.is_primary_key = index.contains(info.name);

			//unique
			info.is_unique = query.value(3).toString()=="UNI";

			//default value
			info.default_value =  query.value(4).isNull() ? QString() : query.value(4).toString();

			//FK
			query_fk.seek(-1);
			while (query_fk.next())
			{
				if (query_fk.value(0)==info.name)
				{
					info.fk_table = query_fk.value(1).toString();
					info.fk_field = query_fk.value(2).toString();

					//set type
					if (info.type!=TableFieldInfo::FK && info.type!=TableFieldInfo::INT && info.type!=TableFieldInfo::LONG)
					{
						THROW(ProgrammingException, "Found SQL foreign key with non-integer type '" + type + "' in field '" + info.name + "' of table '" + table + "'!");
					}
					info.type = TableFieldInfo::FK;

					//set name for FK
					if (table=="sequencing_run")
					{
						if (info.name=="device_id")
						{
							info.fk_name_sql = "CONCAT(name, ' (', type, ')')";
						}
					}
					else if (table=="project")
					{
						if (info.name=="internal_coordinator_id")
						{
							info.fk_name_sql = "name";
						}
					}
					else if (table=="processing_system")
					{
						if (info.name=="genome_id")
						{
							info.fk_name_sql = "build";
						}
					}
					else if (table=="sample")
					{
						if (info.name=="species_id")
						{
							info.fk_name_sql = "name";
						}
						else if (info.name=="sender_id")
						{
							info.fk_name_sql = "name";
						}
						else if (info.name=="receiver_id")
						{
							info.fk_name_sql = "name";
						}
					}
					else if (table=="processed_sample")
					{
						if (info.name=="sequencing_run_id")
						{
							info.fk_name_sql = "name";
						}
						else if (info.name=="sample_id")
						{
							info.fk_name_sql = "name";
						}
						else if ( info.name=="project_id")
						{
							info.fk_name_sql = "name";
						}
						else if (info.name=="processing_system_id")
						{
							info.fk_name_sql = "CONCAT(name_manufacturer, ' (', name_short, ')')";
						}
						else if (info.name=="operator_id")
						{
							info.fk_name_sql = "name";
						}
						else if (info.name=="normal_id")
						{
							info.fk_name_sql = "(SELECT CONCAT(s.name,'_',LPAD(ps.process_id,2,'0')) FROM sample s, processed_sample ps WHERE ps.id=processed_sample.id AND s.id=ps.sample_id)";
						}
						else if (info.name=="mid1_i7")
						{
							info.fk_name_sql = "CONCAT(name, ' (', sequence, ')')";
						}
						else if (info.name=="mid2_i5")
						{
							info.fk_name_sql = "CONCAT(name, ' (', sequence, ')')";
						}
					}
					else if (table=="variant_publication")
					{
						if (info.name=="sample_id")
						{
							info.fk_name_sql = "name";
						}
						else if (info.name=="user_id")
						{
							info.fk_name_sql = "name";
						}
					}
					else if (table=="preferred_transcripts")
					{
						if (info.name=="added_by")
						{
							info.fk_name_sql = "name";
						}
					}
					else if (table=="study_sample")
					{
						if (info.name=="study_id")
						{
							info.fk_name_sql = "name";
						}
						else if (info.name=="processed_sample_id")
						{
							info.fk_name_sql = "(SELECT CONCAT(s.name,'_',LPAD(ps.process_id,2,'0')) FROM sample s, processed_sample ps WHERE ps.id=processed_sample.id AND s.id=ps.sample_id)";
						}
					}
					else if (table=="somatic_gene_role")
					{
						if(info.name == "gene_id") info.fk_name_sql = "symbol";
					}
					else if (table=="sample_relations")
					{
						if (info.name=="sample1_id")
						{
							info.fk_name_sql = "name";
						}
						if (info.name=="sample2_id")
						{
							info.fk_name_sql = "name";
						}
						if (info.name=="user_id")
						{
							info.fk_name_sql = "name";
						}
					}
					else if (table=="sample_disease_info")
					{
						if (info.name=="sample_id")
						{
							info.fk_name_sql = "name";
						}
						if (info.name=="user_id")
						{
							info.fk_name_sql = "name";
						}
					}
					else if (table=="user_permissions")
					{
						if (info.name=="user_id")
						{
							info.fk_name_sql = "name";
						}
					}
					else if (table=="somatic_pathway_gene")
					{
						if (info.name=="pathway_id")
						{
							info.fk_name_sql = "name";
						}
					}
				}
			}

			//labels
			info.label = info.name;
			info.label.replace('_', ' ');
			if (table=="sequencing_run" && info.name=="fcid") info.label = "flowcell ID";
			if (table=="sequencing_run" && info.name=="device_id") info.label = "device";
			if (table=="project" && info.name=="preserve_fastqs") info.label = "preserve FASTQs";
			if (table=="project" && info.name=="internal_coordinator_id") info.label = "internal coordinator";
			if (table=="processing_system" && info.name=="adapter1_p5") info.label = "adapter read 1";
			if (table=="processing_system" && info.name=="adapter2_p7") info.label = "adapter read 2";
			if (table=="processing_system" && info.name=="genome_id") info.label = "genome";
			if (table=="sample" && info.name=="od_260_280") info.label = "od 260/280";
			if (table=="sample" && info.name=="od_260_230") info.label = "od 260/230";
			if (table=="sample" && info.name=="integrity_number") info.label = "RIN/DIN";
			if (table=="sample" && info.name=="species_id") info.label = "species";
			if (table=="sample" && info.name=="sender_id") info.label = "sender";
			if (table=="sample" && info.name=="receiver_id") info.label = "receiver";
			if (table=="processed_sample" && info.name=="sequencing_run_id") info.label = "sequencing run";
			if (table=="processed_sample" && info.name=="operator_id") info.label = "operator";
			if (table=="processed_sample" && info.name=="processing_system_id") info.label = "processing system";
			if (table=="processed_sample" && info.name=="project_id") info.label = "project";
			if (table=="processed_sample" && info.name=="processing_input") info.label = "processing input [ng]";
			if (table=="processed_sample" && info.name=="molarity") info.label = "molarity [nM]";
			if (table=="processed_sample" && info.name=="normal_id") info.label = "normal sample";
			if (table=="processed_sample" && info.name=="mid1_i7") info.label = "mid1 i7";
			if (table=="processed_sample" && info.name=="mid2_i5") info.label = "mid2 i5";
			if (table=="processed_sample" && info.name=="sample_id") info.label = "sample";
			if (table=="variant_publication" && info.name=="sample_id") info.label = "sample";
			if (table=="variant_publication" && info.name=="user_id") info.label = "published by";
			if (table=="preferred_transcripts" && info.name=="added_by") info.label = "added by";

			//read-only
			if (
				(table=="sample" && info.name=="name") ||
				(table=="processing_system" && info.name=="name_short")
			   )
			{
				info.is_readonly = true;
			}

			//hidden
			if (
				info.is_primary_key ||
				info.type==TableFieldInfo::TIMESTAMP ||
				info.type==TableFieldInfo::DATETIME ||
				(table=="processed_sample" && info.name=="sample_id") ||
				(table=="processed_sample" && info.name=="process_id") ||
				(table=="user" && info.name=="salt")
			   )
			{
				info.is_hidden = true;
			}

			//tooltip
			info.tooltip = db.getValue("SELECT COLUMN_COMMENT FROM INFORMATION_SCHEMA.COLUMNS WHERE TABLE_SCHEMA=database() AND TABLE_NAME='" + table + "' AND COLUMN_NAME='" + info.name + "'").toString().trimmed();
			info.tooltip.replace("<br>", "\n");

			infos.append(info);
		}
		output.setFieldInfo(infos);
		table_infos.insert(table, output);
	}

	return table_infos[table];
}

NGSDUserCache& NGSDUserCache::instance(const QString& database_context)
{
	static QMutex registry_mutex;
	static QHash<QString, QSharedPointer<NGSDUserCache>> caches;
	QMutexLocker locker(&registry_mutex);
	if (!caches.contains(database_context)) caches.insert(database_context, QSharedPointer<NGSDUserCache>(new NGSDUserCache));
	return *caches.value(database_context);
}

QByteArray NGSDUserCache::userRole(NGSD& db, int user_id)
{
	QMutexLocker locker(&roles_mutex_);
	if (!user_role_.contains(user_id))
	{
		user_role_[user_id] = db.getValue("SELECT user_role FROM user WHERE id='" + QString::number(user_id) + "'").toByteArray().toLower();
	}
	return user_role_.value(user_id);
}

bool NGSDUserCache::userCanAccess(NGSD& db, int user_id, int ps_id)
{
	QMutexLocker locker(&access_mutex_);
	if (!user_can_access_.contains(user_id))
	{
		QList<int> processed_sample_ids;
		SqlQuery query = db.getQuery();
		query.exec("SELECT * FROM user_permissions WHERE user_id=" + QString::number(user_id));
		while(query.next())
		{
			const AccessPermission permission = stringToAccessPermission(query.value("permission").toString());
			const QString data = query.value("data").toString();
			switch(permission)
			{
				case AccessPermission::PROJECT:
					processed_sample_ids << db.getValuesInt("SELECT id FROM processed_sample WHERE project_id=" + data);
					break;
				case AccessPermission::PROJECT_TYPE:
					processed_sample_ids << db.getValuesInt("SELECT ps.id FROM processed_sample ps, project p WHERE ps.project_id=p.id AND p.type='" + data + "'");
					break;
				case AccessPermission::SAMPLE:
					processed_sample_ids << db.getValuesInt("SELECT id FROM processed_sample WHERE sample_id=" + data);
					break;
				case AccessPermission::STUDY:
					processed_sample_ids << db.getValuesInt("SELECT processed_sample_id FROM study_sample WHERE study_id=" + data);
					break;
			}
		}
		user_can_access_.insert(user_id, Helper::listToSet(processed_sample_ids));
	}
	return user_can_access_.value(user_id).contains(ps_id);
}

QSet<ActionPermission> NGSDUserCache::actionPermissions(NGSD& db, int user_id)
{
	QMutexLocker locker(&actions_mutex_);
	if (!user_can_perform_actions_.contains(user_id))
	{
		QSet<ActionPermission> permissions;
		SqlQuery query = db.getQuery();
		query.exec("SELECT * FROM user_action_permissions WHERE user_id=" + QString::number(user_id));
		if (query.next())
		{
			if (query.value("change_ngsd_data").toBool()) permissions << ActionPermission::CHANGE_NGSD_DATA;
			if (query.value("perform_variant_search").toBool()) permissions << ActionPermission::PERFORM_VARIANT_SEARCH;
			if (query.value("perform_burden_test").toBool()) permissions << ActionPermission::PERFORM_BURDEN_TEST;
			if (query.value("start_analysis_jobs").toBool()) permissions << ActionPermission::START_ANALYSIS_JOBS;
		}
		user_can_perform_actions_.insert(user_id, permissions);
	}
	return user_can_perform_actions_.value(user_id);
}

void NGSDUserCache::clear()
{
	{
		QMutexLocker locker(&roles_mutex_);
		user_role_.clear();
	}
	{
		QMutexLocker locker(&access_mutex_);
		user_can_access_.clear();
	}
	{
		QMutexLocker locker(&actions_mutex_);
		user_can_perform_actions_.clear();
	}
}
