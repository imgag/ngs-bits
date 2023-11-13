#include "FileLocationProviderLocal.h"
#include "NGSD.h"
#include <QDir>

FileLocationProviderLocal::FileLocationProviderLocal(QString gsvar_file, const SampleHeaderInfo& header_info, AnalysisType analysis_type)
  : gsvar_file_(gsvar_file)
  , header_info_(header_info)
  , analysis_type_(analysis_type)
{
	if (gsvar_file_.isEmpty()) THROW(ArgumentException, "GSvar filename has not been specified!");
	if (header_info_.isEmpty()) THROW(ArgumentException, "Header information has not been specified!");
}

bool FileLocationProviderLocal::isLocal() const
{
	return true;
}

FileLocation FileLocationProviderLocal::getAnalysisVcf() const
{
	QString name = QFileInfo(gsvar_file_).baseName();
	QString file = gsvar_file_.left(gsvar_file_.length()-6) + "_var_annotated.vcf.gz";

	return FileLocation{name, PathType::VCF, file, QFile::exists(file)};
}

FileLocation FileLocationProviderLocal::getAnalysisSvFile() const
{
	QString name = QFileInfo(gsvar_file_).baseName();
	QString file = gsvar_file_.left(gsvar_file_.length()-6) + "_var_structural_variants.bedpe";
	if(!QFile::exists(file))
	{
		//Fallback to support old manta file name:
		file = gsvar_file_.left(gsvar_file_.length()-6) + "_manta_var_structural.bedpe";
	}

	return FileLocation{name, PathType::STRUCTURAL_VARIANTS, file, QFile::exists(file)};
}

FileLocation FileLocationProviderLocal::getAnalysisCnvFile() const
{
	QString name = QFileInfo(gsvar_file_).baseName();
	QString base = gsvar_file_.left(gsvar_file_.length()-6);

	if (analysis_type_==SOMATIC_SINGLESAMPLE || analysis_type_==SOMATIC_PAIR)
	{
		QString file = base	+ "_clincnv.tsv";
		return FileLocation{name, PathType::COPY_NUMBER_CALLS, file, QFile::exists(file)};
	}
	else
	{
		QString file = base	+ "_cnvs_clincnv.tsv";
		return FileLocation{name, PathType::COPY_NUMBER_CALLS, file, QFile::exists(file)};
	}
}

FileLocation FileLocationProviderLocal::getAnalysisMosaicCnvFile() const
{

	QString name = QFileInfo(gsvar_file_).baseName();
	QString file = gsvar_file_.left(gsvar_file_.length()-6) + "_mosaic_cnvs.tsv";

	return FileLocation{name, PathType::COPY_NUMBER_CALLS_MOSAIC, file, QFile::exists(file)};
}

FileLocation FileLocationProviderLocal::getAnalysisUpdFile() const
{
	if (analysis_type_!=GERMLINE_TRIO) return FileLocation();

	QString name = QFileInfo(gsvar_file_).baseName();
	QString file = gsvar_file_.left(gsvar_file_.length()-6) + "_upd.tsv";

	return FileLocation(name, PathType::UPD, file, QFile::exists(file));
}

FileLocation FileLocationProviderLocal::getRepeatExpansionImage(QString locus) const
{
	QString name = QFileInfo(gsvar_file_).baseName();
	QString file = getAnalysisPath() + QDir::separator() + "repeat_expansions" + QDir::separator() + name  + "_repeats_expansionhunter_" + locus + ".svg";
	return FileLocation(name, PathType::REPEAT_EXPANSION_IMAGE, file, QFile::exists(file));
}

FileLocationList FileLocationProviderLocal::getQcFiles() const
{
	QString name = QFileInfo(gsvar_file_).baseName();

	FileLocationList output;

	QStringList qc_files = Helper::findFiles(getAnalysisPath(), "*.qcML", false);
	foreach(const QString& qc_file, qc_files)
	{
		FileLocation file = FileLocation{name, PathType::QC, qc_file, true};
		addToList(file, output);
	}

	return output;
}

void FileLocationProviderLocal::addToList(const FileLocation& loc, FileLocationList& list, bool add_if_missing)
{
	bool exists = QFile::exists(loc.filename);
	if (exists)
	{
		list << loc;
		list.last().exists = true;
	}
	else if(add_if_missing)
	{
		list << loc;
		list.last().exists = false;
	}
}

FileLocationList FileLocationProviderLocal::getBamFiles(bool return_if_missing) const
{
	FileLocationList output;

	foreach(const KeyValuePair& loc, getBaseLocations())
	{
		FileLocation file = FileLocation{loc.key, PathType::BAM, loc.value + ".bam", false};
        if (QFile::exists(loc.value + ".cram")) file.filename = loc.value + ".cram";
		addToList(file, output, return_if_missing);
	}

	return output;
}

FileLocationList FileLocationProviderLocal::getViralBamFiles(bool return_if_missing) const
{
	FileLocationList output;

	foreach(const KeyValuePair& loc, getBaseLocations())
	{
		FileLocation file = FileLocation{loc.key, PathType::VIRAL_BAM, loc.value + "_viral.bam", false};
		addToList(file, output, return_if_missing);
	}

	return output;
}

FileLocationList FileLocationProviderLocal::getCnvCoverageFiles(bool return_if_missing) const
{
	FileLocationList output;

	foreach(const KeyValuePair& loc, getBaseLocations())
	{
		FileLocation file = FileLocation{loc.key, PathType::COPY_NUMBER_RAW_DATA, loc.value + "_cnvs_clincnv.seg", false};
		addToList(file, output, return_if_missing);
	}

	return output;
}

FileLocationList FileLocationProviderLocal::getBafFiles(bool return_if_missing) const
{
	FileLocationList output;

	foreach(const KeyValuePair& loc, getBaseLocations())
	{
		FileLocation file = FileLocation{loc.key, PathType::BAF, loc.value + "_bafs.igv", false};
		addToList(file, output, return_if_missing);
	}

	if (analysis_type_==SOMATIC_PAIR)
	{
		QString name = QFileInfo(gsvar_file_).baseName() + " (somatic)";
		QString file = gsvar_file_.left(gsvar_file_.length()-6) + "_bafs.igv";
		addToList( FileLocation{name, PathType::BAF, file, QFile::exists(file)}, output, return_if_missing);
	}


	return output;
}

FileLocationList FileLocationProviderLocal::getMantaEvidenceFiles(bool return_if_missing) const
{
	FileLocationList output;

	foreach(const KeyValuePair& loc, getBaseLocations())
	{
		QString folder = loc.value.left(loc.value.length()-loc.key.length());

		FileLocation file = FileLocation{loc.key, PathType::MANTA_EVIDENCE, folder + "manta_evid/" + loc.key + "_manta_evidence.bam", false};
		addToList(file, output, return_if_missing);
	}

	return output;
}

FileLocationList FileLocationProviderLocal::getCircosPlotFiles(bool return_if_missing) const
{
	FileLocationList output;

	foreach(const KeyValuePair& loc, getBaseLocations())
	{
		FileLocation file = FileLocation{loc.key, PathType::CIRCOS_PLOT, loc.value + "_circos.png", false};
		addToList(file, output, return_if_missing);
	}

	return output;
}

FileLocationList FileLocationProviderLocal::getExpressionFiles(bool return_if_missing) const
{
	FileLocationList output;
	foreach(const KeyValuePair& loc, getBaseLocations())
	{
		FileLocation file = FileLocation{loc.key, PathType::EXPRESSION, loc.value + "_expr.tsv", false};
		addToList(file, output, return_if_missing);
	}

	return output;
}

FileLocationList FileLocationProviderLocal::getExonExpressionFiles(bool return_if_missing) const
{
	FileLocationList output;
	foreach(const KeyValuePair& loc, getBaseLocations())
	{
		FileLocation file = FileLocation{loc.key, PathType::EXPRESSION, loc.value + "_expr_exon.tsv", false};
		addToList(file, output, return_if_missing);
	}

	return output;
}

FileLocationList FileLocationProviderLocal::getVcfFiles(bool return_if_missing) const
{
	FileLocationList output;

	foreach(const KeyValuePair& loc, getBaseLocations())
	{
		FileLocation file = FileLocation{loc.key, PathType::VCF, loc.value + "_var_annotated.vcf.gz", false};
		addToList(file, output, return_if_missing);
	}

	return output;
}

FileLocationList FileLocationProviderLocal::getRepeatExpansionFiles(bool return_if_missing) const
{
	FileLocationList output;

	foreach(const KeyValuePair& loc, getBaseLocations())
	{
		FileLocation file = FileLocation{loc.key, PathType::REPEAT_EXPANSIONS, loc.value + "_repeats_expansionhunter.vcf", false};
		addToList(file, output, return_if_missing);
	}

	return output;
}

FileLocationList FileLocationProviderLocal::getPrsFiles(bool return_if_missing) const
{
	FileLocationList output;

	foreach(const KeyValuePair& loc, getBaseLocations())
	{
		FileLocation file = FileLocation{loc.key, PathType::PRS, loc.value + "_prs.tsv", false};
		addToList(file, output, return_if_missing);
	}

	return output;
}

FileLocationList FileLocationProviderLocal::getLowCoverageFiles(bool return_if_missing) const
{
	FileLocationList output;

	foreach(const KeyValuePair& loc, getBaseLocations())
	{
		QString folder = loc.value.left(loc.value.length()-loc.key.length());

		QStringList beds = Helper::findFiles(folder, "*_lowcov.bed", false);
		foreach(const QString& bed, beds)
		{
			FileLocation file = FileLocation{loc.key, PathType::LOWCOV_BED, bed, true};
			addToList(file, output, return_if_missing);
		}
	}

	return output;
}

FileLocationList FileLocationProviderLocal::getSomaticLowCoverageFiles(bool return_if_missing) const
{
	if (analysis_type_!=SOMATIC_SINGLESAMPLE && analysis_type_!=SOMATIC_PAIR) THROW(ProgrammingException, "Invalid call of getSomaticLowCoverageFile() on variant list type " + analysisTypeToString(analysis_type_) + "!");

	FileLocationList output;

	QString name = QFileInfo(gsvar_file_).baseName();
	QString folder = QFileInfo(gsvar_file_).absoluteDir().absolutePath();
	QStringList beds = Helper::findFiles(folder, "*_lowcov.bed", false);

	foreach(QString bed_file, beds)
	{
		FileLocation file = FileLocation(name, PathType::LOWCOV_BED, bed_file, QFile::exists(bed_file));
		addToList(file, output, return_if_missing);
	}

	return output;
}


FileLocationList FileLocationProviderLocal::getCopyNumberCallFiles(bool return_if_missing) const
{
	FileLocationList output;

	foreach(const KeyValuePair& loc, getBaseLocations())
	{
		FileLocation file = FileLocation{loc.key, PathType::COPY_NUMBER_CALLS, loc.value + "_cnvs_clincnv.tsv", false};
		addToList(file, output, return_if_missing);
	}
	return output;
}

FileLocationList FileLocationProviderLocal::getRohFiles(bool return_if_missing) const
{
	FileLocationList output;

	foreach(const KeyValuePair& loc, getBaseLocations())
	{
		FileLocation file = FileLocation{loc.key, PathType::ROH, loc.value + "_rohs.tsv", false};
		addToList(file, output, return_if_missing);
	}

	return output;
}

FileLocation FileLocationProviderLocal::getSomaticCnvCoverageFile() const
{
	if (analysis_type_!=SOMATIC_SINGLESAMPLE && analysis_type_!=SOMATIC_PAIR) THROW(ProgrammingException, "Invalid call of getSomaticCnvCoverageFile() on variant list type " + analysisTypeToString(analysis_type_) + "!");

	QString name = QFileInfo(gsvar_file_).baseName() + " (coverage)";
	QString file = gsvar_file_.left(gsvar_file_.length()-6) + "_cov.seg";

	return FileLocation{name, PathType::COPY_NUMBER_RAW_DATA, file, QFile::exists(file)};
}

FileLocation FileLocationProviderLocal::getSomaticCnvCallFile() const
{
	if (analysis_type_!=SOMATIC_SINGLESAMPLE && analysis_type_!=SOMATIC_PAIR) THROW(ProgrammingException, "Invalid call of getSomaticCnvCallFile() on variant list type " + analysisTypeToString(analysis_type_) + "!");

	QString name = QFileInfo(gsvar_file_).baseName() + " (copy number)";
	QString file = gsvar_file_.left(gsvar_file_.length()-6) + "_cnvs.seg";

	return FileLocation{name, PathType::CNV_RAW_DATA_CALL_REGIONS, file, QFile::exists(file)};
}

FileLocation FileLocationProviderLocal::getSomaticLowCoverageFile() const
{
	if (analysis_type_!=SOMATIC_SINGLESAMPLE && analysis_type_!=SOMATIC_PAIR) THROW(ProgrammingException, "Invalid call of getSomaticLowCoverageFile() on variant list type " + analysisTypeToString(analysis_type_) + "!");

	QString name = QFileInfo(gsvar_file_).baseName();
	QString file = gsvar_file_.left(gsvar_file_.length()-6) + "_stat_lowcov.bed";

	return FileLocation{name, PathType::LOWCOV_BED, file, QFile::exists(file)};
}

FileLocation FileLocationProviderLocal::getSomaticMsiFile() const
{
	if (analysis_type_!=SOMATIC_SINGLESAMPLE && analysis_type_!=SOMATIC_PAIR) THROW(ProgrammingException, "Invalid call of getSomaticMsiFile() on variant list type " + analysisTypeToString(analysis_type_) + "!");

	QString name = QFileInfo(gsvar_file_).baseName();
	QString file = gsvar_file_.left(gsvar_file_.length()-6) + "_msi.tsv";

	return FileLocation{name, PathType::MSI, file, QFile::exists(file)};
}

FileLocation FileLocationProviderLocal::getSomaticIgvScreenshotFile() const
{
	if (analysis_type_ != SOMATIC_SINGLESAMPLE && analysis_type_ != SOMATIC_PAIR) THROW(ProgrammingException, "Invalid call of getSomaticIgvScreenshotFile() on variant list type " + analysisTypeToString(analysis_type_) + "!");
	QString name = QFileInfo(gsvar_file_).baseName();	
	QString file = gsvar_file_.left(gsvar_file_.length()-6) + "_igv_screenshot.png";

	return FileLocation{name, PathType::IGV_SCREENSHOT, file, QFile::exists(file)};
}

FileLocation FileLocationProviderLocal::getSomaticCfdnaCandidateFile() const
{
	if (analysis_type_ != SOMATIC_SINGLESAMPLE && analysis_type_ != SOMATIC_PAIR) THROW(ProgrammingException, "Invalid call of getSomaticCfdnaCandidateFile() on variant list type " + analysisTypeToString(analysis_type_) + "!");
	QString name = QFileInfo(gsvar_file_).baseName();
	QString file = gsvar_file_.left(gsvar_file_.length()-6) + "_cfDNA_candidates" + QDir::separator() + "monitoring.vcf";

	return FileLocation{name, PathType::CFDNA_CANDIDATES, file, QFile::exists(file)};
}

FileLocation FileLocationProviderLocal::getSignatureSbsFile() const
{
	if (analysis_type_ != SOMATIC_SINGLESAMPLE && analysis_type_ != SOMATIC_PAIR) THROW(ProgrammingException, "Invalid call of getSomaticCfdnaCandidateFile() on variant list type " + analysisTypeToString(analysis_type_) + "!");
	QString name = QFileInfo(gsvar_file_).baseName();
	QString file = QFileInfo(gsvar_file_).dir().absolutePath() + QDir::separator() + "snv_signatures" + QDir::separator() + "De_Novo_map_to_COSMIC_SBS96.csv";

	return FileLocation{name, PathType::SIGNATURE_SBS, file, QFile::exists(file)};
}

FileLocation FileLocationProviderLocal::getSignatureIdFile() const
{
	if (analysis_type_ != SOMATIC_SINGLESAMPLE && analysis_type_ != SOMATIC_PAIR) THROW(ProgrammingException, "Invalid call of getSomaticCfdnaCandidateFile() on variant list type " + analysisTypeToString(analysis_type_) + "!");
	QString name = QFileInfo(gsvar_file_).baseName();
	QString file = QFileInfo(gsvar_file_).dir().absolutePath() + QDir::separator() + "snv_signatures" + QDir::separator() + "De_Novo_map_to_COSMIC_ID83.csv";

	return FileLocation{name, PathType::SIGNATURE_ID, file, QFile::exists(file)};
}

FileLocation FileLocationProviderLocal::getSignatureDbsFile() const
{
	if (analysis_type_ != SOMATIC_SINGLESAMPLE && analysis_type_ != SOMATIC_PAIR) THROW(ProgrammingException, "Invalid call of getSomaticCfdnaCandidateFile() on variant list type " + analysisTypeToString(analysis_type_) + "!");
	QString name = QFileInfo(gsvar_file_).baseName();
	QString file = QFileInfo(gsvar_file_).dir().absolutePath() + QDir::separator() + "snv_signatures" + QDir::separator() + "De_Novo_map_to_COSMIC_DBS78.csv";

	return FileLocation{name, PathType::SIGNATURE_DBS, file, QFile::exists(file)};
}

FileLocation FileLocationProviderLocal::getSignatureCnvFile() const
{
	if (analysis_type_ != SOMATIC_SINGLESAMPLE && analysis_type_ != SOMATIC_PAIR) THROW(ProgrammingException, "Invalid call of getSomaticCfdnaCandidateFile() on variant list type " + analysisTypeToString(analysis_type_) + "!");
	QString name = QFileInfo(gsvar_file_).baseName();
	QString file = QFileInfo(gsvar_file_).dir().absolutePath() + QDir::separator() + "cnv_signatures" + QDir::separator() + "De_Novo_map_to_COSMIC_CNV48.csv";

	return FileLocation{name, PathType::CFDNA_CANDIDATES, file, QFile::exists(file)};
}



QString FileLocationProviderLocal::getAnalysisPath() const
{
	return QFileInfo(gsvar_file_).absolutePath();
}

QString FileLocationProviderLocal::getProjectPath() const
{
	QDir directory = QFileInfo(gsvar_file_).dir();
	directory.cdUp();
	return directory.absolutePath();
}

QList<KeyValuePair> FileLocationProviderLocal::getBaseLocations() const
{
    QList<KeyValuePair> output;

    if (analysis_type_==GERMLINE_SINGLESAMPLE || analysis_type_==SOMATIC_SINGLESAMPLE || analysis_type_==CFDNA)
    {
        QString id = header_info_.begin()->name;
        output << KeyValuePair(id, getAnalysisPath() + "/" + id);
    }
    else if (analysis_type_==GERMLINE_TRIO || analysis_type_==GERMLINE_MULTISAMPLE || analysis_type_==SOMATIC_PAIR)
    {
        QString project_folder = getProjectPath();

        foreach(const SampleInfo& info, header_info_)
        {
            if (Settings::boolean("NGSD_enabled", true))
            {
                try
                {
                    QString id = NGSD().processedSampleId(info.name, false);
                    QString sample_path = NGSD().processedSamplePath(id, PathType::SAMPLE_FOLDER);
                    output << KeyValuePair(info.name, sample_path + info.name);
                    continue;
                }
                catch (...)
                {
                    // We fall back to the standard behaviour, if the sample cannot be found
                }

            }

            output << KeyValuePair(info.name, project_folder + "/Sample_" + info.name + "/" + info.name);
        }
    }
    else
    {
        THROW(ProgrammingException, "Cannot handle unknown analysis type");
    }

    return output;
}
