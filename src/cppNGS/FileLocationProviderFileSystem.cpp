#include "FileLocationProviderFileSystem.h"

FileLocationProviderFileSystem::FileLocationProviderFileSystem(QString gsvar_file, const SampleHeaderInfo header_info, const AnalysisType analysis_type)
  : gsvar_file_(gsvar_file)
  , header_info_(header_info)
  , analysis_type_(analysis_type)
{
}

QList<FileLocation> FileLocationProviderFileSystem::getBamFiles()
{
	QList<FileLocation> output;

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
			output << FileLocation{info.id, PathType::BAM, sample_folder + "/" + info.id + ".bam", false};
		}
		else
		{
			output << FileLocation{info.id, PathType::BAM, project_folder + "/Sample_" + info.id + "/" + info.id + ".bam", false};
		}
	}

	if (output.length() == 0)
	{
		THROW(Exception, "Could not find BAM file at one of the default locations:"+sample_folder+", "+project_folder);
	}
	return output;
}

QList<FileLocation> FileLocationProviderFileSystem::getSegFilesCnv()
{
	QList<FileLocation> output;

	if (analysis_type_==SOMATIC_PAIR)
	{
		//tumor-normal SEG file
		QString pair = QFileInfo(gsvar_file_).baseName();
		output << FileLocation{pair + " (copy number)", PathType::CNV_ESTIMATES, gsvar_file_.left(gsvar_file_.length()-6) + "_cnvs.seg", false};
		output << FileLocation{pair + " (coverage)", PathType::CNV_ESTIMATES, gsvar_file_.left(gsvar_file_.length()-6) + "_cov.seg", false};

		//germline SEG file
		QString basename = QFileInfo(gsvar_file_).baseName().left(gsvar_file_.length()-6);
		if (basename.contains("-"))
		{
			QString tumor_ps_name = basename.split("-")[1];
			QString pair_folder = QFileInfo(gsvar_file_).absolutePath();
			QString project_folder = QFileInfo(pair_folder).absolutePath();
			output << FileLocation{tumor_ps_name, PathType::CNV_ESTIMATES, project_folder + "/Sample_" + tumor_ps_name + "/" + tumor_ps_name + "_cnvs.seg", false};
		}
	}
	else
	{
		QList<FileLocation> tmp = getBamFiles();
		foreach(const FileLocation& file, tmp)
		{
			QString base_name = file.filename.left(file.filename.length()-4);
			output << FileLocation{file.id, PathType::CNV_ESTIMATES, base_name + "_cnvs_clincnv.seg", false};
			output << FileLocation{file.id, PathType::CNV_ESTIMATES, base_name + "_cnvs.seg", false};
		}
	}

	return output;
}

QList<FileLocation> FileLocationProviderFileSystem::getIgvFilesBaf()
{
	QList<FileLocation> output;
	if (analysis_type_==SOMATIC_PAIR)
	{
		output << FileLocation{QFileInfo(gsvar_file_).baseName(), PathType::BAF, gsvar_file_.left(gsvar_file_.length()-6) + "_bafs.igv", false};
	}
	else
	{
		QList<FileLocation> tmp = getBamFiles();
		foreach(const FileLocation& file, tmp)
		{
			output << FileLocation{file.id, PathType::BAF, file.filename.left(file.filename.length()-4) + "_bafs.igv", false};
		}
	}

	return output;
}

QList<FileLocation> FileLocationProviderFileSystem::getMantaEvidenceFiles()
{
	QList<FileLocation> evidence_files;
	// search at location of all available BAM files
	QList<FileLocation> bam_files = getBamFiles();
	foreach (FileLocation bam_file, bam_files)
	{
		QString evidence_bam_file = FileLocationHelper::getEvidenceFile(bam_file.filename);
		evidence_files << FileLocation{QFileInfo(evidence_bam_file).baseName(), PathType::BAM, evidence_bam_file, false};
	}
	return evidence_files;
}
