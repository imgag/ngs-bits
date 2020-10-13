#include "FileLocationProviderFileSystem.h"

FileLocationProviderFileSystem::FileLocationProviderFileSystem(QString gsvar_file, const SampleHeaderInfo header_info, const AnalysisType analysis_type)
  : gsvar_file_(gsvar_file)
  , header_info_(header_info)
  , analysis_type_(analysis_type)
{
}

QList<FileLocation> FileLocationProviderFileSystem::getBamFiles()
{
	qDebug("FileLocationProviderFileSystem.getBamFiles() has been invoked");
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
		bool found = false;
		QString bam_from_sample = sample_folder + "/" + info.id + ".bam";
		QString bam_from_project = project_folder + "/Sample_" + info.id + "/" + info.id + ".bam";

		if (QFile::exists(bam_from_sample))
		{
			found = true;
			output << FileLocation{info.id, PathType::BAM, bam_from_sample};
		}
		else if (QFile::exists(bam_from_project))
		{
			found = true;
			output << FileLocation{info.id, PathType::BAM, bam_from_project};
		}

		if (!found)
		{
			THROW(Exception, "Could not find BAM file at one of the default locations:"+bam_from_sample+", "+bam_from_project);
			output.clear();
			return output;
		}
	}

	return output;
}

QList<FileLocation> FileLocationProviderFileSystem::getSegFilesCnv()
{
	QList<FileLocation> output;

	if (analysis_type_==SOMATIC_PAIR)
	{
		//tumor-normal SEG file
		QString segfile = gsvar_file_.left(gsvar_file_.length()-6) + "_cnvs.seg";
		QString pair = QFileInfo(gsvar_file_).baseName();
		output << FileLocation{pair + " (copy number)", PathType::CNV_ESTIMATES , segfile};

		QString covfile = gsvar_file_.left(gsvar_file_.length()-6) + "_cov.seg";
		output << FileLocation{pair + " (coverage)", PathType::CNV_ESTIMATES,covfile};

		//germline SEG file
		QString basename = QFileInfo(gsvar_file_).baseName().left(gsvar_file_.length()-6);
		if (basename.contains("-"))
		{
			QString tumor_ps_name = basename.split("-")[1];
			QString pair_folder = QFileInfo(gsvar_file_).absolutePath();
			QString project_folder = QFileInfo(pair_folder).absolutePath();
			segfile = project_folder + "/Sample_" + tumor_ps_name + "/" + tumor_ps_name + "_cnvs.seg";
			output << FileLocation{tumor_ps_name, PathType::CNV_ESTIMATES , segfile};
		}
	}
	else
	{
		QList<FileLocation> tmp = getBamFiles();
		foreach(const FileLocation& file, tmp)
		{
			QString base_name = file.filename.left(file.filename.length()-4);
			QString segfile = base_name + "_cnvs_clincnv.seg";
			if (QFile::exists(segfile))
			{
				output << FileLocation{file.id, PathType::CNV_ESTIMATES , segfile};
			}
			else
			{
				segfile = base_name + "_cnvs.seg";
				if (QFile::exists(segfile))
				{
					output << FileLocation{file.id, PathType::CNV_ESTIMATES , segfile};
				}
			}
		}
	}

	return output;
}

QList<FileLocation> FileLocationProviderFileSystem::getIgvFilesBaf()
{
	QList<FileLocation> output;

	if (analysis_type_==SOMATIC_PAIR)
	{
		QString segfile = gsvar_file_.left(gsvar_file_.length()-6) + "_bafs.igv";
		QString pair = QFileInfo(gsvar_file_).baseName();
		output << FileLocation{pair, PathType::BAF , segfile};
	}
	else
	{
		QList<FileLocation> tmp = getBamFiles();
		foreach(const FileLocation& file, tmp)
		{
			QString segfile = file.filename.left(file.filename.length()-4) + "_bafs.igv";
			if (QFile::exists(segfile))
			{
				output << FileLocation{file.id, PathType::BAF , segfile};
			}
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

		// check if evidence file exists
		if (!QFile::exists(evidence_bam_file)) continue;

		FileLocation evidence_file;
		evidence_file.filename = evidence_bam_file;
		evidence_file.type = PathType::BAM;
		evidence_file.id = QFileInfo(evidence_bam_file).baseName();
		evidence_files.append(evidence_file);
	}
	return evidence_files;
}
