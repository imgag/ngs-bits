#include "FileLocationProviderFileSystem.h"

FileLocationProviderFileSystem::FileLocationProviderFileSystem(QString gsvar_file, const SampleHeaderInfo header_info, const AnalysisType analysis_type)
  : gsvar_file_(gsvar_file)
  , header_info_(header_info)
  , analysis_type_(analysis_type)
{
}

void FileLocationProviderFileSystem::setIsFoundFlag(FileLocation& file)
{
	file.is_found = false;
	if (QFile::exists(file.filename))
	{
		file.is_found = true;
	}
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
			FileLocation single_bam = FileLocation{info.id, PathType::BAM, sample_folder + "/" + info.id + ".bam", false};
			setIsFoundFlag(single_bam);
			output << single_bam;
		}
		else
		{
			FileLocation multi_bam = FileLocation{info.id, PathType::BAM, project_folder + "/Sample_" + info.id + "/" + info.id + ".bam", false};
			setIsFoundFlag(multi_bam);
			output << multi_bam;
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
		QString pair = QFileInfo(gsvar_file_).baseName();

		FileLocation cnvs_seg = FileLocation{pair + " (copy number)", PathType::CNV_ESTIMATES, gsvar_file_.left(gsvar_file_.length()-6) + "_cnvs.seg", false};
		setIsFoundFlag(cnvs_seg);
		output << cnvs_seg;

		FileLocation con_seg = FileLocation{pair + " (coverage)", PathType::CNV_ESTIMATES, gsvar_file_.left(gsvar_file_.length()-6) + "_cov.seg", false};
		setIsFoundFlag(con_seg);
		output << con_seg;

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
			FileLocation cnvs_clincnv_seg = FileLocation{file.id, PathType::CNV_ESTIMATES, base_name + "_cnvs_clincnv.seg", false};
			setIsFoundFlag(cnvs_clincnv_seg);
			if (cnvs_clincnv_seg.is_found)
			{
				output << cnvs_clincnv_seg;
			}
			else
			{
				FileLocation cnvs_seg = FileLocation{file.id, PathType::CNV_ESTIMATES, base_name + "_cnvs.seg", false};
				setIsFoundFlag(cnvs_seg);
				output << cnvs_seg;
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
		FileLocation bafs_igv = FileLocation{QFileInfo(gsvar_file_).baseName(), PathType::BAF, gsvar_file_.left(gsvar_file_.length()-6) + "_bafs.igv", false};
		setIsFoundFlag(bafs_igv);
		output << bafs_igv;
	}
	else
	{
		QList<FileLocation> tmp = getBamFiles();
		foreach(const FileLocation& file, tmp)
		{
			FileLocation bafs_igv = FileLocation{file.id, PathType::BAF, file.filename.left(file.filename.length()-4) + "_bafs.igv", false};
			setIsFoundFlag(bafs_igv);
			output << bafs_igv;
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
		FileLocation evidence_bam = FileLocation{QFileInfo(evidence_bam_file).baseName(), PathType::BAM, evidence_bam_file, false};
		setIsFoundFlag(evidence_bam);
		evidence_files << evidence_bam;
	}
	return evidence_files;
}
