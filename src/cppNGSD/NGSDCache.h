#ifndef NGSDCACHE_H
#define NGSDCACHE_H

#include "NGSD.h"
#include "ChromosomalIndex.h"
#include <QMutex>
#include <QRecursiveMutex>

///Database-derived NGSD data that remains unchanged during normal operation.
class CPPNGSDSHARED_EXPORT NGSDReferenceDataCache
{
public:
	static NGSDReferenceDataCache& instance(const QString& database_context);

private:
	friend class NGSD;

	void clear();
	const QSet<int>& sameSamples(NGSD& db, int sample_id, SameSampleMode mode);
	const QSet<int>& relatedSamples(NGSD& db, int sample_id);
	const TableInfo& tableInfo(NGSD& db, const QString& table, bool use_cache);
	const GeneSet& approvedGeneNames(NGSD& db);
	int geneId(NGSD& db, const QByteArray& gene);
	QByteArray geneSymbol(NGSD& db, int id);
	QByteArray geneHgncId(NGSD& db, int id);
	int hgncIdToGeneId(NGSD& db, QByteArray hgnc_id);
	QByteArray geneToApproved(NGSD& db, QByteArray gene, bool return_input_when_unconvertable);
	QStringList enumValues(NGSD& db, const QString& table, const QString& column, bool use_cache);
	const QHash<int, QList<QByteArray>>& hpoGenes(NGSD& db);
	const QHash<int, QList<int>>& hpoParent(NGSD& db);
	SomaticGeneRole somaticGeneRole(NGSD& db, const QByteArray& gene, bool throw_on_fail);
	QMap<QString, SomaticGeneRole> somaticGeneRoles(NGSD& db, bool only_high_evidence);
	void updateSomaticGeneRole(const SomaticGeneRole& role);
	void removeSomaticGeneRole(const QByteArray& gene);
	int phenotypeIdByAccession(NGSD& db, const QByteArray& accession, bool throw_on_error);
	const Phenotype& phenotype(NGSD& db, int id);
	PhenotypeList phenotypes(NGSD& db);
	GeneSet genesOverlapping(NGSD& db, const Chromosome& chr, int start, int end, int extend, bool exons_only);
	int transcriptId(NGSD& db, const QByteArray& name, bool throw_on_error);
	TranscriptList transcripts(NGSD& db, int gene_id, Transcript::SOURCE source, bool coding_only);
	TranscriptList transcriptsOverlapping(NGSD& db, const Chromosome& chr, int start, int end, int extend, Transcript::SOURCE source);
	const TranscriptList& transcripts(NGSD& db);
	const Transcript& transcript(NGSD& db, int id);
	bool transcriptCacheInitialized();
	QMap<int, QByteArray> expressionIdToGene(NGSD& db);
	QMap<QByteArray, int> expressionGeneToId(NGSD& db);
	int expressionGeneId(NGSD& db, const QByteArray& gene);

	NGSDReferenceDataCache();
	NGSDReferenceDataCache(const NGSDReferenceDataCache&) = delete;
	NGSDReferenceDataCache& operator=(const NGSDReferenceDataCache&) = delete;
	void initTranscriptCache(NGSD& db);
	void initGeneExpressionCache(NGSD& db);
	void initSomaticGeneRoleCache(NGSD& db);
	void initPhenotypeCache(NGSD& db);

	QRecursiveMutex mutex_;
	QMap<QString, TableInfo> table_infos_;
	QHash<int, QSet<int>> same_samples_;
	QHash<int, QSet<int>> same_patients_;
	QHash<int, QSet<int>> related_samples_;
	bool same_samples_initialized_ = false;
	bool same_patients_initialized_ = false;
	bool related_samples_initialized_ = false;
	GeneSet approved_gene_names_;
	bool approved_gene_names_initialized_ = false;
	QHash<QByteArray, int> gene2id_;
	bool gene2id_initialized_ = false;
	QHash<int, QByteArray> id2gene_;
	bool id2gene_initialized_ = false;
	QMap<QString, QStringList> enum_values_;
	QMap<QByteArray, QByteArray> non_approved_to_approved_gene_names_;
	QHash<int, Phenotype> phenotypes_by_id_;
	bool phenotypes_by_id_initialized_ = false;
	QHash<QByteArray, int> phenotypes_accession_to_id_;
	bool phenotype_accession_to_id_initialized_ = false;
	QHash<int, QList<QByteArray>> hpo_genes_;
	bool hpo_genes_initialized_ = false;
	QHash<int, QList<int>> hpo_parent_;
	bool hpo_parent_initialized_ = false;
	QMap<QString, SomaticGeneRole> gene_symbol_to_somatic_gene_role_;
	bool somatic_gene_roles_initialized_ = false;
	QMap<int, QByteArray> gene_id_to_hgnc_;
	bool gene_id_to_hgnc_initialized_ = false;
	QMap<QByteArray, int> hgnc_id_to_gene_id_;
	bool hgnc_to_gene_id_initialized_ = false;

	TranscriptList gene_transcripts_;
	ChromosomalIndex<TranscriptList> gene_transcripts_index_;
	QHash<int, int> gene_transcripts_id2index_;
	QHash<QByteArray, QSet<int>> gene_transcripts_symbol2indices_;
	QHash<QByteArray, int> gene_transcripts_name2id_;
	bool transcript_cache_initialized_ = false;

	QMap<int, QByteArray> gene_expression_id2gene_;
	QMap<QByteArray, int> gene_expression_gene2id_;
	bool gene_expression_cache_initialized_ = false;
};

///User-specific NGSD data that can be invalidated during normal operation.
class CPPNGSDSHARED_EXPORT NGSDUserCache
{
public:
	static NGSDUserCache& instance(const QString& database_context);

private:
	friend class NGSD;

	QByteArray userRole(NGSD& db, int user_id);
	bool userCanAccess(NGSD& db, int user_id, int ps_id);
	QSet<ActionPermission> actionPermissions(NGSD& db, int user_id);
	void clear();

	NGSDUserCache() = default;
	NGSDUserCache(const NGSDUserCache&) = delete;
	NGSDUserCache& operator=(const NGSDUserCache&) = delete;

	QMutex roles_mutex_;
	QMutex access_mutex_;
	QMutex actions_mutex_;
	QMap<int, QByteArray> user_role_;
	QMap<int, QSet<int>> user_can_access_;
	QMap<int, QSet<ActionPermission>> user_can_perform_actions_;
};

#endif // NGSDCACHE_H
