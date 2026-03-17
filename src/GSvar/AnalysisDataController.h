#ifndef ANALYSISDATACONTROLLER_H
#define ANALYSISDATACONTROLLER_H

#include <QObject>

#include "VariantList.h"
#include "ArribaFile.h"
#include "CnvList.h"
#include "BedpeFile.h"
#include "RepeatLocusList.h"

#include "ReportSettings.h"
#include "SomaticReportSettings.h"
#include "RnaReportConfiguration.h"
#include "TumorOnlyReportWorker.h"

#include "NGSD.h"

#include "Histogram.h"
#include "VariantScores.h"

//Filter settings for report configuration
enum class ReportConfigFilter
{
    NONE,
    NO_RC,
    HAS_RC
};

//filter state of variant lists
struct FilterState
{
    QStringList filter_cascade_text;
    TargetRegionInfo target_region;
    GeneSet genes;
    QString text_filter;
    BedLine region_filter;
    PhenotypeList phenotypes;
    PhenotypeSettings phenotype_setting;
    BedFile phenotype_roi;
    ReportConfigFilter rc_filter;

    FilterResult passing_variants;
};

class AnalysisDataController
    : public QObject
{
    Q_OBJECT
public:
    AnalysisDataController();

    void clear();

    QStringList loadFile(QString filename="");
    QStringList loadSample(QString processed_sample);
    QStringList availableAnalysis(QString processed_sample);

    QList<QPair<Log::LogLevel, QString>> checkVariantList();
    QList<QPair<Log::LogLevel, QString>> checkProcessedSamplesInNGSD();

    //Filtering
    const FilterState& getSmallVariantsFilterState() const;
    const FilterResult& applySmallVariantFilter(const FilterCascade& filter_cascade, TargetRegionInfo target_region, GeneSet genes, QString text_filter, QString region_filter_text, PhenotypeList phenotypes, PhenotypeSettings pheno_settings, ReportConfigFilter rc_filter, bool debug_time);
    //const FilterResult& applyCnvFilter(const FilterCascade& filter_cascade, bool debug_time);
    //const FilterResult& applySvFilter(const FilterCascade& filter_cascade, bool debug_time);
    //const FilterResult& applyReFilter(const FilterCascade& filter_cascade, bool debug_time);
    //const FilterResult& applyFusionFilter(const FilterCascade& filter_cascade, bool debug_time);

    ///change the annotation of variants in the small variant list:
    void changeSmallVariantList(int variant_idx, QByteArray column, QByteArray new_text, bool throw_if_not_found=true);
    ///batch version to update the annotation of a single column for multiple variants - sends only one changed SIGNAL
    void changeSmallVariantListBatch(QList<int> variant_indices, QByteArray column, QByteArrayList new_text, bool throw_if_not_found=true);
    ///updates the variant comment in the NGSD and the GSVAR file if column available
    void setSmallVariantComment(int variant_idx, QByteArray new_comment);
    ///reannotate the VICC interpretation from NGSD to the GSvar file
    void reannotateSomaticVariantInterpretation();
    ///annotate the Variant ranking scores to the GSvar file - doesn't get stored when the file is saved!
    void annotateVariantRankingScores(VariantScores::Result variant_scores, bool add_explanations);

    /// return the list of het hit genes
    GeneSet getHetHitGenes() const;
    /// calculate the mendelian errors in a trio analysis
    double calcMendelianErrorRate(int& used, int& errors) const;

    BedFile genesToRegions(GeneSet genes);

    /// get
    Histogram afHistogram(bool filtered) const;
    Histogram cnHistogram(QString sample_id, Chromosome chr, int start, int end) const;
    Histogram bafHistogram(QString sample_id, Chromosome chr, int start, int end) const;

    void exportGSvar(QString file_name, bool as_vcf=false);
    void exportHerediCareVCF(QString herediCare_id, QString file_name);

    ///return the valid filter entries for the small variant list
    QStringList getValidFilterEntries() const;
    ///
    QList<KeyValuePair> inheritanceByGene(int variant_idx);

    //status functions showing state/type of current loaded data
    bool isValid() const;
    bool isLocal() const;
    bool isRnaSet() const;

    bool existBam(QString ps_name="") const;
    bool existCnvs() const;
    bool existSvs() const;
    bool existRes() const;
    bool existFusions() const;

    bool existMosaicCnvs() const;
    bool existCircos() const;
    bool existPrs() const;
    bool existRohs() const;
    bool existUpds() const;
    bool existPathogenicWt() const;
    bool existMethylation() const;
    bool existVirusDetection() const;
    bool existCnvSegmentation(QString sample_id="") const;
    bool existBaf(QString sample_id="") const;

    bool existRnaSplicing(int rna_id=-1) const;
    bool existRnaFusions(int rna_id=-1) const;
    bool existRnaExpressionGenes(int rna_id=-1) const;
    bool existRnaExpressionExons(int rna_id=-1) const;

    bool variantListModified() const;

    bool germlineReportSupported(bool require_ngsd = true, QString* error=nullptr) const;
    bool somaticReportSupported() const;
    bool tumoronlyReportSupported() const;
    bool rnaReportSupported() const;

    //Setter:
    void setRnaSampleId(int rna_id);

    //Getters:
    const QString& getFilename() const;
    AnalysisType getAnalysisType() const;
    QString getAnalysisName() const;

    QString getMainSampleName() const;
    ///Determines normal sample name from filename_, throws otherwise (tumor-normal pairs)
    QString getNormalSampleName() const;
    QStringList getSampleNames() const;
    QString germlineReportSample() const;

    QStringList getBamFile(QString ps_name="") const;
    const VariantList& getSmallVariantList() const;
    const CnvList& getCnvList() const;
    CnvList getPrefilteredCnvList(int max_number, double& min_ll) const;
    const BedpeFile& getSvList() const;
    const RepeatLocusList& getReList() const;
    const VariantList& getControlTissueSmallVariants() const;
    const ArribaFile& getFusionList() const;
    QStringList getMosaicCnvs() const;
    FileLocation getMethylation() const;
    FileLocation getPrsFile() const;
    FileLocation getPathogenicWildtype() const;
    QStringList getUpdFile() const;
    FileLocation getRohFile() const;
    FileLocation getCircosFile() const;
    FileLocation getVirusDetectionFile() const;

    int getActiveRnaPsId() const;
    QString getActiveRnaPsName() const;
    QSet<int> getRelatedRnaSampleIds() const;
    QSet<int> getRelatedRnaProcessedSampleIds() const;
    FileLocation getRnaSplicing(int rna_id=-1) const;
    FileLocation getRnaFusions(int rna_id=-1) const;
    FileLocation getRnaExpressionGenes(int rna_id=-1) const;
    FileLocation getRnaExpressionExons(int rna_id=-1) const;

    QSet<int> getRelatedCfdnaSampleIds() const;

    QSharedPointer<ReportConfiguration> getGermlineReportConfig();
    QSharedPointer<SomaticReportConfiguration> getSomaticReportConfig();
    QSharedPointer<RnaReportConfiguration> getRnaReportConfig();

    const ReportSettings& getGermlineReportSettings() const;
    const SomaticReportSettings& getSomaticReportSettings() const;

    ///returns the variant validation entry for a small variant - if it doesnt exist returns empty object
    VariantValidation getSmallVariantValidationEntry(int variant_idx, QString ps_name="");
    ///stores the variant validation entry as given after a validation check
    void storeVariantValidation(const VariantValidation& var_val);


    EvaluationSheetData getEvaluationSheetData();
    void finalizeGermlineReportConfig(int user_id);

    TumorOnlyReportWorkerConfig getTumorOnlyReportWorkerConfig();

public slots:
    ///Store changes in the GSvar file
    void storeSmallVariantList();
    ///Store report configuration
    void storeGermlineReportConfig();
    ///Store somatic report configuration
    void storeSomaticReportConfig();
    ///Store RNA report configuration
    void storeRnaReportConfig();



signals:
    void dataCleared();
    void dataLoaded();

    void thrownError(QString title, QString text);
    void thrownWarning(QString title, QString text);
    void thrownInfo(QString title, QString text);

    void smallVariantsChanged();
    void loadedSmallVariantFilters(QString filter_name, FilterCascade filters, QString traget_region_name);

private:
    //changes:
    struct VariantListChange
    {
        Variant variant;
        QString column;
        QString text;
    };


    ///Load germline report configuration
    void loadGermlineReportConfig();
    ///Load somatic report configuration
    void loadSomaticReportConfig();
    ///Load RNA report configuration
    void loadRnaReportConfig(QString rna_ps_id);

    ///returns rna_id if != -1 else returns active_rna_ps_id if it is unequal -1. Throws if both are -1.
    int getValidRna(int rna_id) const;

    //analysis:
    QString filename_;
    QStringList processed_sample_names_;
    AnalysisType analysis_type_;
    int active_rna_ps_id_;

    //variants
    VariantList variants_;
    FilterState variants_filter_state_;
    QList<VariantListChange> variants_changed_;
    CnvList cnvs_;
    FilterState cnvs_filter_state_;
    BedpeFile svs_;
    FilterState svs_filter_state_;
    RepeatLocusList repeat_expansions_;
    FilterState repeat_expansions_filter_state_;
    VariantList somatic_control_tissue_variants_;
    ArribaFile fusions_;
    FilterState fusions_filter_state_;

    //Report data:
    ReportSettings germline_report_settings_;
    SomaticReportSettings somatic_report_settings_;
    QSharedPointer<RnaReportConfiguration> rna_report_config_;
};

#endif // ANALYSISDATACONTROLLER_H
