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

#include "FileLocationProvider.h"
#include "DatabaseService.h"



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

    //return the valid filter entries for the small variant list
    QStringList getValidFilterEntries() const;
    //returns
    QString germlineReportSample();

    //return the list of het hit genes
    GeneSet getHetHitGenes(const FilterResult& filter_result) const;

    //status functions showing state/type of current loaded data
    bool isValid();
    bool isLocal();

    bool existCnvs();
    bool existSvs();
    bool existRes();
    bool existFusions();

    bool existMosaicCnvs();

    bool variantListModified();

    bool germlineReportSupported(bool require_ngsd = true, QString* error=nullptr) const;
    bool somaticReportSupported() const;
    bool tumoronlyReportSupported() const;
    bool rnaReportSupported() const;



    //Getters:
    const QString& getFilename() const;
    AnalysisType getAnalysisType() const;
    QString getAnalysisName() const;

    const VariantList& getSmallVariantList() const;
    const CnvList& getCnvList() const;
    CnvList getPrefilteredCnvList(int max_number, double& min_ll) const;
    const BedpeFile& getSvList() const;
    const RepeatLocusList& getReList() const;
    const VariantList& getControlTissueSmallVariants() const;
    const ArribaFile& getFusionList() const;
    QStringList getMosaicCnvs() const;

    QSharedPointer<ReportConfiguration> getGermlineReportConfig();
    QSharedPointer<SomaticReportConfiguration> getSomaticReportConfig();
    QSharedPointer<RnaReportConfiguration> getRnaReportConfig();

    const ReportSettings& getGermlineReportSettings() const;
    const SomaticReportSettings& getSomaticReportSettings() const;



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

private:
    ///Load germline report configuration
    void loadGermlineReportConfig();
    ///Load somatic report configuration
    void loadSomaticReportConfig();
    ///Load RNA report configuration
    void loadRnaReportConfig(QString rna_ps_id);

    ///Determines normal sample name from filename_, return "" otherwise (tumor-normal pairs)
    QString normalSampleName();

    //general data access:
    QSharedPointer<DatabaseService> db_service_;
    NGSD db_;

    //analysis:
    QString filename_;
    QStringList processed_sample_names_;
    AnalysisType analysis_type_;

    //variants
    VariantList variants_;
    CnvList cnvs_;
    BedpeFile svs_;
    RepeatLocusList repeat_expansions_;
    VariantList somatic_control_tissue_variants_;
    ArribaFile fusions_;

    //Report data:
    ReportSettings germline_report_settings_;
    SomaticReportSettings somatic_report_settings_;
    QSharedPointer<RnaReportConfiguration> rna_report_config_;

    //changes:
    struct VariantListChange
    {
        Variant variant;
        QString column;
        QString text;
    };
    QList<VariantListChange> variants_changed_;

};

#endif // ANALYSISDATACONTROLLER_H
