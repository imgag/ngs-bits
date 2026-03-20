#include "FilterState.h"
#include "NGSD.h"
#include "GlobalServiceProvider.h"
#include "FilterWidgetHelper.h"

FilterState::FilterState()
    : QObject()
{
}

GeneSet FilterState::getRelevantGenes() const
{
    NGSD db;
    GeneSet relevant_genes;
    if(getPhenotypes().count() > 0)
    {
        for (const Phenotype& phenotype : getPhenotypes())
        {
            relevant_genes << db.phenotypeToGenes(db.phenotypeIdByAccession(phenotype.accession()), false);
        }
    }

    if(getTargetRegionInfo().isValid())
    {
        if (relevant_genes.isEmpty())
        {
            relevant_genes = getTargetRegionInfo().genes;
        }
        else
        {
            relevant_genes = getTargetRegionInfo().genes.intersect(relevant_genes);
        }
    }

    return relevant_genes;
}

void FilterState::clearPhenotypeFilter()
{
    bool changed = false;

    if (phenotypes_.count() != 0)
    {
        phenotypes_.clear();
        changed=true;
    }

    if (phenotype_settings_ != PhenotypeSettings())
    {
        phenotype_settings_.revert();
        changed=true;
    }

    phenotype_roi_.clear();

    if (changed) emit filterStateChanged();
}

void FilterState::clearFilters(bool clear_roi)
{
    blockSignals(true);

    blockSignals(false);
}

void FilterState::clearTargetRegionFilter()
{
    target_region_.clear();

    emit filterStateChanged();
    emit targetRegionChanged(target_region_);
}

const QString& FilterState::getFilterName() const
{
    return filter_name_;
}

const FilterCascade& FilterState::getFilterCascade() const
{
    return filter_cascade_;
}

QStringList FilterState::getFilterCascadeText() const
{
    return filter_cascade_.toText();
}

const TargetRegionInfo& FilterState::getTargetRegionInfo() const
{
    return target_region_;
}

const GeneSet& FilterState::getGenes() const
{
    return genes_;
}

const QString& FilterState::getTextFilter() const
{
    return text_filter_;
}

const BedLine& FilterState::getRegionFilter() const
{
    return region_filter_;
}

const PhenotypeList& FilterState::getPhenotypes() const
{
    return phenotypes_;
}

const PhenotypeSettings& FilterState::getPhenotypeSettings() const
{
    return phenotype_settings_;
}

const BedFile& FilterState::getPhenotypeRoi() const
{
    return phenotype_roi_;
}

const ReportConfigFilter& FilterState::getReportConfigFilter() const
{
    return rc_filter_;
}

void FilterState::setFilterName(const QString& filter_name, bool emit_gui_signal)
{
    if (filter_name_ != filter_name)
    {
        filter_name_ = filter_name;
        if (emit_gui_signal) emit filterNameChanged(filter_name_);

    }
}

void FilterState::setFilterCascade(const FilterCascade& filter_cascade, bool emit_gui_signal)
{
    if (filter_cascade != filter_cascade_)
    {
        filter_cascade_ = filter_cascade;
        emit filterStateChanged();
        if (emit_gui_signal) emit filterCascadeChanged(filter_cascade_);

    }
}

void FilterState::setTargetReionInfo(const TargetRegionInfo& target_region, bool emit_gui_signal)
{
    if (target_region_.name != target_region.name || target_region_.regions.baseCount() != target_region.regions.baseCount())
    {
        target_region_ = target_region;
        emit filterStateChanged();
        if (emit_gui_signal) emit targetRegionChanged(target_region_);

    }
}

void FilterState::setTargetRegionInfoByName(const QString& name, bool emit_gui_signal)
{
    if (name != target_region_.name)
    {
        NGSD db;
        FilterWidgetHelper::loadTargetRegionData(target_region_, name);
        emit filterStateChanged();
        if (emit_gui_signal) emit targetRegionChanged(target_region_);
    }
}

void FilterState::setGenes(const GeneSet& genes, bool emit_gui_signal)
{
    if (genes_ != genes)
    {
        genes_ = genes;
        emit filterStateChanged();
        if (emit_gui_signal) emit genesChanged(genes_);

    }
}

void FilterState::setTextFilter(const QString& text_filter, bool emit_gui_signal)
{
    if (text_filter_ != text_filter)
    {
        text_filter_ = text_filter;
        emit filterStateChanged();
        if (emit_gui_signal) emit textFilterChanged(text_filter_);

    }
}

void FilterState::setRegionFilter(const BedLine& region_filter, bool emit_gui_signal)
{
    if (region_filter_.chr() != region_filter.chr() || region_filter_.start() != region_filter.start() || region_filter_.end() != region_filter.end())
    {
        region_filter_ = region_filter;
        emit filterStateChanged();
        if (emit_gui_signal) emit regionFilterChanged(region_filter_);

    }
}

void FilterState::setPhenotypes(const PhenotypeList& phenotypes, bool emit_gui_signal)
{
    if (phenotypes_ != phenotypes)
    {
        phenotypes_ = phenotypes;
        updatePhenotypeRoi();
        emit filterStateChanged();
        if (emit_gui_signal) emit phenotypesChanged(phenotypes_);

    }
}

void FilterState::setPhenotypeSettings(const PhenotypeSettings& phenotype_settings, bool emit_gui_signal)
{
    if (phenotype_settings_ != phenotype_settings)
    {
        phenotype_settings_ = phenotype_settings;
        updatePhenotypeRoi();
        emit filterStateChanged();
        if (emit_gui_signal) emit phenotypeSettingsChanged(phenotype_settings_);

    }
}

void FilterState::setReportConfigFilter(const ReportConfigFilter& rc_filter, bool emit_gui_signal)
{
    if (rc_filter_ != rc_filter)
    {
        rc_filter_ = rc_filter;
        emit filterStateChanged();
        if (emit_gui_signal) emit reportConfigFilterChanged(rc_filter_);

    }
}

void FilterState::updatePhenotypeRoi()
{
    //convert phenotypes to genes
    NGSD db;
    GeneSet pheno_genes;
    int i = 0;
    foreach (const Phenotype& pheno, phenotypes_)
    {
        GeneSet genes = db.phenotypeToGenesbySourceAndEvidence(db.phenotypeIdByAccession(pheno.accession()), phenotype_settings_.sources, phenotype_settings_.evidence_levels, true, false);

        if (phenotype_settings_.mode==PhenotypeCombimnationMode::MERGE || (phenotype_settings_.mode==PhenotypeCombimnationMode::INTERSECT && i==0))
        {
            pheno_genes << genes;
        }
        else
        {
            pheno_genes = pheno_genes.intersect(genes);
        }
        ++i;
    }

    //convert genes to ROI (using a cache to speed up repeating queries)
    phenotype_roi_.clear();
    for (const QByteArray& gene: std::as_const(pheno_genes))
    {
        phenotype_roi_.add(GlobalServiceProvider::geneToRegions(gene, db));
    }
    phenotype_roi_.merge();
}
