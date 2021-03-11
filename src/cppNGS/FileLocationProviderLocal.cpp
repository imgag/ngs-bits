#include "FileLocationProviderLocal.h"

FileLocationProviderLocal::FileLocationProviderLocal(QString gsvar_file, const SampleHeaderInfo header_info, const AnalysisType analysis_type)
  : gsvar_file_(gsvar_file)
  , header_info_(header_info)
  , analysis_type_(analysis_type)
{
}

void FileLocationProviderLocal::addToList(const FileLocation& loc, FileLocationList& list)
{
	list << loc;
	list.last().is_found = QFile::exists(loc.filename);
}

FileLocationList FileLocationProviderLocal::getBamFiles()
{
	FileLocationList output;

	if (gsvar_file_ == nullptr)
	{
		THROW(ArgumentException, "File name has not been specified")
		return output;
	}

	if (header_info_.empty())
	{
		THROW(ArgumentException, "Header information has not been specified");
		return output;
	}

	QString sample_folder = QFileInfo(gsvar_file_).absolutePath();
	QString project_folder = QFileInfo(sample_folder).absolutePath();

	foreach(const SampleInfo& info, header_info_)
	{
		if (analysis_type_ == GERMLINE_SINGLESAMPLE)
		{
			FileLocation single_bam = FileLocation{info.id, PathType::BAM, sample_folder + "/" + info.id + ".bam", false};
			addToList(single_bam, output);
		}
		else
		{
			FileLocation multi_bam = FileLocation{info.id, PathType::BAM, project_folder + "/Sample_" + info.id + "/" + info.id + ".bam", false};			
			addToList(multi_bam, output);
		}
	}
	return output;
}

FileLocationList FileLocationProviderLocal::getSegFilesCnv()
{
	FileLocationList output;

	if (analysis_type_==SOMATIC_PAIR)
	{
		//tumor-normal SEG file
		QString pair = QFileInfo(gsvar_file_).baseName();

		FileLocation cnvs_seg = FileLocation{pair + " (copy number)", PathType::COPY_NUMBER_RAW_DATA, gsvar_file_.left(gsvar_file_.length()-6) + "_cnvs.seg", false};		
		addToList(cnvs_seg, output);

		FileLocation con_seg = FileLocation{pair + " (coverage)", PathType::COPY_NUMBER_RAW_DATA, gsvar_file_.left(gsvar_file_.length()-6) + "_cov.seg", false};		
		addToList(con_seg, output);

		//germline SEG file
		QString basename = QFileInfo(gsvar_file_).baseName().left(gsvar_file_.length()-6);
		if (basename.contains("-"))
		{
			QString tumor_ps_name = basename.split("-")[1];
			QString pair_folder = QFileInfo(gsvar_file_).absolutePath();
			QString project_folder = QFileInfo(pair_folder).absolutePath();
			output << FileLocation{tumor_ps_name, PathType::COPY_NUMBER_RAW_DATA, project_folder + "/Sample_" + tumor_ps_name + "/" + tumor_ps_name + "_cnvs.seg", false};
		}
	}
	else
	{
		QList<FileLocation> tmp = getBamFiles();
		foreach(const FileLocation& file, tmp)
		{
			QString base_name = file.filename.left(file.filename.length()-4);
			FileLocation cnvs_clincnv_seg = FileLocation{file.id, PathType::COPY_NUMBER_RAW_DATA, base_name + "_cnvs_clincnv.seg", false};
			addToList(cnvs_clincnv_seg, output);
			if (!output.last().is_found)
			{
				output.removeLast();
				FileLocation cnvs_seg = FileLocation{file.id, PathType::COPY_NUMBER_RAW_DATA, base_name + "_cnvs.seg", false};
				addToList(cnvs_seg, output);
			}
		}
	}

	return output;
}

FileLocationList FileLocationProviderLocal::getIgvFilesBaf()
{
	FileLocationList output;
	if (analysis_type_==SOMATIC_PAIR)
	{
		FileLocation bafs_igv = FileLocation{QFileInfo(gsvar_file_).baseName(), PathType::BAF, gsvar_file_.left(gsvar_file_.length()-6) + "_bafs.igv", false};		
		addToList(bafs_igv, output);
	}
	else
	{
		QList<FileLocation> tmp = getBamFiles();
		foreach(const FileLocation& file, tmp)
		{
			FileLocation bafs_igv = FileLocation{file.id, PathType::BAF, file.filename.left(file.filename.length()-4) + "_bafs.igv", false};			
			addToList(bafs_igv, output);
		}
	}

	return output;
}

FileLocationList FileLocationProviderLocal::getMantaEvidenceFiles()
{
	FileLocationList evidence_files;
	// search at location of all available BAM files
	QList<FileLocation> bam_files = getBamFiles();
	foreach (FileLocation bam_file, bam_files)
	{
		QString evidence_bam_file = getEvidenceFile(bam_file.filename);
		FileLocation evidence_bam = FileLocation{QFileInfo(evidence_bam_file).baseName(), PathType::BAM, evidence_bam_file, false};		
		addToList(evidence_bam, evidence_files);
	}
	return evidence_files;
}

QString FileLocationProviderLocal::getEvidenceFile(QString bam_file)
{
	if (!bam_file.endsWith(".bam", Qt::CaseInsensitive))
	{
		THROW(ArgumentException, "Invalid BAM file path \"" + bam_file + "\"!");
	}
	QFileInfo bam_file_info(bam_file);
	QDir evidence_dir(bam_file_info.absolutePath() + "/manta_evid/");
	QString ps_name = bam_file_info.fileName().left(bam_file_info.fileName().length() - 4);
	return evidence_dir.absoluteFilePath(ps_name + "_manta_evidence.bam");
}

FileLocationList FileLocationProviderLocal::getAnalysisLogFiles()
{
	return FileLocationList{};
}

FileLocationList FileLocationProviderLocal::getCircosPlotFiles()
{
	QStringList files = Helper::findFiles(getAnalysisPath(), "*_circos.png", false);
	FileLocationList output = mapFoundFilesToFileLocation(files, PathType::CIRCOS_PLOT);

	return output;
}

FileLocationList FileLocationProviderLocal::getVcfGzFiles()
{
	QStringList files = Helper::findFiles(getAnalysisPath(), "*_var_annotated.vcf.gz", false);
	FileLocationList output = mapFoundFilesToFileLocation(files, PathType::VCF_GZ);

	return output;
}

FileLocationList FileLocationProviderLocal::getExpansionhunterVcfFiles()
{
	QStringList files = Helper::findFiles(getAnalysisPath(), processedSampleName() + "_repeats_expansionhunter.vcf", false);
	FileLocationList output = mapFoundFilesToFileLocation(files, PathType::REPEATS_EXPANSION_HUNTER_VCF);

	return output;
}

FileLocationList FileLocationProviderLocal::getPrsTsvFiles()
{
	QStringList files = Helper::findFiles(getAnalysisPath(), processedSampleName() + "_prs.tsv", false);
	FileLocationList output = mapFoundFilesToFileLocation(files, PathType::PRS_TSV);

	return output;
}

FileLocationList FileLocationProviderLocal::getClincnvTsvFiles()
{
	QStringList files = Helper::findFiles(getAnalysisPath(), "*_clincnv.tsv", false);
	FileLocationList output = mapFoundFilesToFileLocation(files, PathType::CLINCNV_TSV);

	return output;
}

FileLocationList FileLocationProviderLocal::getLowcovBedFiles()
{
	QStringList files = Helper::findFiles(getAnalysisPath(), "*_lowcov.bed", false);
	FileLocationList output = mapFoundFilesToFileLocation(files, PathType::LOWCOV_BED);

	return output;
}

FileLocationList FileLocationProviderLocal::getStatLowcovBedFiles()
{
	FileLocationList output {};

	if (analysis_type_==SOMATIC_PAIR)
	{
		//search in analysis folder
		QStringList beds = Helper::findFiles(getAnalysisPath(), "*_stat_lowcov.bed", false);
		foreach(const QString& bed, beds)
		{
			FileLocation file;
			file.id = QFileInfo(bed).fileName().replace("_stat_lowcov.bed", "") + " (low-coverage regions)";
			file.type = PathType::COPY_NUMBER_CALLS;
			file.filename = bed;
			output.append(file);
		}
	}
	else
	{
		// search in sample folders containing BAM files
		foreach (const FileLocation& bam_file, getBamFiles())
		{
			QString folder = QFileInfo(bam_file.filename).absolutePath();
			QStringList beds = Helper::findFiles(folder, "*_lowcov.bed", false);

			foreach(const QString& bed, beds)
			{
				FileLocation file;
				file.id = bam_file.id + " (low-coverage regions)";
				file.type = PathType::COPY_NUMBER_CALLS;
				file.filename = bed;
				output.append(file);
			}
		}
	}

	return output;
}

FileLocationList FileLocationProviderLocal::getCnvsClincnvSegFiles()
{
	QStringList files = Helper::findFiles(getAnalysisPath(), "*_cnvs_clincnv.seg", false);
	FileLocationList output = mapFoundFilesToFileLocation(files, PathType::CNVS_CLINCNV_SEG);

	return output;
}

FileLocationList FileLocationProviderLocal::getCnvsClincnvTsvFiles()
{
	QStringList files = Helper::findFiles(getAnalysisPath(), "*_cnvs_clincnv.tsv", false);
	FileLocationList output = mapFoundFilesToFileLocation(files, PathType::CNVS_CLINCNV_TSV);

	return output;
}

FileLocationList FileLocationProviderLocal::getCnvsSegFiles()
{
	QStringList files = Helper::findFiles(getAnalysisPath(), "*_cnvs.seg", false);
	FileLocationList output = mapFoundFilesToFileLocation(files, PathType::CNVS_SEG);

	return output;
}

FileLocationList FileLocationProviderLocal::getCnvsTsvFiles()
{
	QStringList files = Helper::findFiles(getAnalysisPath(), "*_cnvs.tsv", false);
	FileLocationList output = mapFoundFilesToFileLocation(files, PathType::CNVS_TSV);

	return output;
}

FileLocationList FileLocationProviderLocal::getRohsTsvFiles()
{
	QString filename = gsvar_file_;
	if (analysis_type_==GERMLINE_TRIO)
	{
		QString child = header_info_.infoByStatus(true).column_name;
		filename = QFileInfo(gsvar_file_).absolutePath() + "/Sample_" + child + "/" + child + ".GSvar";
	}
	QString folder = QFileInfo(filename).absolutePath();
	QStringList files = Helper::findFiles(folder, "*_rohs.tsv", false);
	FileLocationList output = mapFoundFilesToFileLocation(files, PathType::ROHS_TSV);

	return output;
}

QString FileLocationProviderLocal::getAnalysisPath()
{
	return QFileInfo(gsvar_file_).absolutePath();
}

QString FileLocationProviderLocal::getProjectPath()
{
	QDir directory = QFileInfo(gsvar_file_).dir();
	directory.cdUp();
	return directory.absolutePath();
}

QString FileLocationProviderLocal::getRohFileAbsolutePath()
{
	QString filename = gsvar_file_;
	if (analysis_type_==GERMLINE_TRIO)
	{
		QString child = header_info_.infoByStatus(true).column_name;
		filename = getAnalysisPath() + "/Sample_" + child + "/" + child + ".GSvar";
	}
	return QFileInfo(filename).absolutePath();
}

QString FileLocationProviderLocal::processedSampleName()
{
	switch(analysis_type_)
	{
		case GERMLINE_SINGLESAMPLE:
			return QFileInfo(gsvar_file_).baseName();
			break;
		case GERMLINE_TRIO: //return index (child)
		return header_info_.infoByStatus(true).column_name;
			break;
		case GERMLINE_MULTISAMPLE: //return affected if there is exactly one affected
		try
		{
			SampleInfo info = header_info_.infoByStatus(true);
			return info.column_name;
		}
		catch(...) {} //Nothing to do here
			break;
		case SOMATIC_SINGLESAMPLE:
			break;
		case SOMATIC_PAIR:
			return QFileInfo(gsvar_file_).baseName().split("-")[0];
			break;
	}

	return "";
}

FileLocationList FileLocationProviderLocal::mapFoundFilesToFileLocation(QStringList& files, PathType type)
{
	FileLocationList output {};
	for (int i = 0; i < files.count(); i++)
	{
		FileLocation current_location;
		current_location.id = "";
		current_location.type = type;
		current_location.filename = files[i];
		current_location.is_found = true;

		output.append(current_location);
	}
	return output;
}
