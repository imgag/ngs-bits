#include "FileLocationProviderLocal.h"
#include <QDir>

FileLocationProviderLocal::FileLocationProviderLocal(QString gsvar_file, const SampleHeaderInfo& header_info, AnalysisType analysis_type)
  : gsvar_file_(gsvar_file)
  , header_info_(header_info)
  , analysis_type_(analysis_type)
{
	if (gsvar_file_.isEmpty()) THROW(ArgumentException, "GSvar filename has not been specified!");
	if (header_info_.isEmpty()) THROW(ArgumentException, "Header information has not been specified!");
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
	QString file = gsvar_file_.left(gsvar_file_.length()-6) + "_manta_var_structural.bedpe";

	return FileLocation{name, PathType::STRUCTURAL_VARIANTS, file, QFile::exists(file)};
}

FileLocation FileLocationProviderLocal::getAnalysisCnvFile() const
{
	QString name = QFileInfo(gsvar_file_).baseName();
	QString base = gsvar_file_.left(gsvar_file_.length()-6);

	if (analysis_type_==SOMATIC_SINGLESAMPLE || analysis_type_==SOMATIC_PAIR)
	{
		QString file = base	+ "_clincnv.tsv";
		return FileLocation{name, PathType::STRUCTURAL_VARIANTS, file, QFile::exists(file)};
	}
	else
	{
		QString file = base	+ "_cnvs_clincnv.tsv";
		return FileLocation{name, PathType::STRUCTURAL_VARIANTS, file, QFile::exists(file)};
	}
}

FileLocation FileLocationProviderLocal::getAnalysisUpdFile() const
{
	FileLocation output;

	if (analysis_type_!=GERMLINE_TRIO) return FileLocation();

	{
		QString name = QFileInfo(gsvar_file_).baseName();
		QString file = gsvar_file_.left(gsvar_file_.length()-6) + "_manta_var_structural.bedpe";

		return FileLocation{name, PathType::STRUCTURAL_VARIANTS, file, QFile::exists(file)};

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
		addToList(file, output, return_if_missing);
	}

	return output;
}

FileLocationList FileLocationProviderLocal::getCnvCoverageFiles(bool return_if_missing) const
{
	FileLocationList output;

	//special handling of tumor-normal pair: add pair coverage data
	if (analysis_type_==SOMATIC_PAIR)
	{
		//tumor-normal SEG file
		QString pair = QFileInfo(gsvar_file_).baseName();
		FileLocation con_seg = FileLocation{pair, PathType::COPY_NUMBER_RAW_DATA, gsvar_file_.left(gsvar_file_.length()-6) + "_cov.seg", false};
		addToList(con_seg, output, return_if_missing);
	}

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

	//tumor-normal SEG file
	if (analysis_type_==SOMATIC_PAIR)
	{
		QString pair = QFileInfo(gsvar_file_).baseName();
		FileLocation con_seg = FileLocation{pair, PathType::LOWCOV_BED, gsvar_file_.left(gsvar_file_.length()-6) + "_stat_lowcov.bed", false};
		addToList(con_seg, output, return_if_missing);
	}

	foreach(const KeyValuePair& loc, getBaseLocations())
	{
		QString folder = loc.value.left(loc.value.length()-loc.key.length());

		QStringList beds = Helper::findFiles(folder, "*_stat_lowcov.bed", false); //TODO to return missing files, we would have to get rid of the processing system name inside the filename...
		foreach(const QString& bed, beds)
		{
			FileLocation file = FileLocation{loc.key, PathType::LOWCOV_BED, bed, true};
			addToList(file, output, return_if_missing);
		}
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

QString FileLocationProviderLocal::processedSampleName() const
{
	QStringList output;
	switch(analysis_type_)
	{
		case SOMATIC_SINGLESAMPLE:
		case GERMLINE_SINGLESAMPLE:
			foreach(const SampleInfo& entry, header_info_)
			{
				output << entry.id;
			}
			break;
		case GERMLINE_TRIO:
		case GERMLINE_MULTISAMPLE:
			foreach(const SampleInfo& entry, header_info_)
			{
				if (entry.isAffected())
				{
					output << entry.id;
				}
			}
			break;
		case SOMATIC_PAIR:
			foreach(const SampleInfo& entry, header_info_)
			{
				if (entry.isTumor())
				{
					output << entry.id;
				}
			}
			break;
	}

	if (output.count()!=1) THROW(ProgrammingException, "Could not determine single processed sample name from sample header of GSvar file!");

	return output[0];
}

QList<KeyValuePair> FileLocationProviderLocal::getBaseLocations() const
{
	QList<KeyValuePair> output;

	if (analysis_type_==GERMLINE_SINGLESAMPLE || analysis_type_==SOMATIC_SINGLESAMPLE)
	{
		QString id = header_info_.begin()->id;
		output << KeyValuePair(id, getAnalysisPath() + "/" + id);
	}
	else if (analysis_type_==GERMLINE_TRIO || analysis_type_==GERMLINE_MULTISAMPLE || analysis_type_==SOMATIC_PAIR)
	{
		QString project_folder = getProjectPath();

		foreach(const SampleInfo& info, header_info_)
		{
			output << KeyValuePair(info.id, project_folder + "/Sample_" + info.id + "/" + info.id);
		}
	}

	return output;
}
