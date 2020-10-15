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
//		bool found = false;
//		QString bam_from_sample = sample_folder + "/" + info.id + ".bam";
//		QString bam_from_project = project_folder + "/Sample_" + info.id + "/" + info.id + ".bam";


		if (analysis_type_ == GERMLINE_SINGLESAMPLE)
		{

//		if (QFile::exists(bam_from_sample))
//		{
//			found = true;
//			output << FileLocation{info.id, PathType::BAM, bam_from_sample};
			output << FileLocation{info.id, PathType::BAM, sample_folder + "/" + info.id + ".bam", false};
		}
		else
		{
//		}
//		else if (QFile::exists(bam_from_project))
//		{
//			found = true;
//			output << FileLocation{info.id, PathType::BAM, bam_from_project};
			output << FileLocation{info.id, PathType::BAM, project_folder + "/Sample_" + info.id + "/" + info.id + ".bam", false};
//		}
		}

//
	}

	if (output.length() == 0)
	{
		THROW(Exception, "Could not find BAM file at one of the default locations:"+sample_folder+", "+project_folder);
//		output.clear();
//		return output;
	}
	return output;
}

QList<FileLocation> FileLocationProviderFileSystem::getSegFilesCnv()
{
	QList<FileLocation> output;

	if (analysis_type_==SOMATIC_PAIR)
	{
		//tumor-normal SEG file
//		QString segfile = gsvar_file_.left(gsvar_file_.length()-6) + "_cnvs.seg";
		QString pair = QFileInfo(gsvar_file_).baseName();
		output << FileLocation{pair + " (copy number)", PathType::CNV_ESTIMATES, gsvar_file_.left(gsvar_file_.length()-6) + "_cnvs.seg", false};




//		QString covfile = gsvar_file_.left(gsvar_file_.length()-6) + "_cov.seg";
		output << FileLocation{pair + " (coverage)", PathType::CNV_ESTIMATES, gsvar_file_.left(gsvar_file_.length()-6) + "_cov.seg", false};

		//germline SEG file
		QString basename = QFileInfo(gsvar_file_).baseName().left(gsvar_file_.length()-6);
		if (basename.contains("-"))
		{
			QString tumor_ps_name = basename.split("-")[1];
			QString pair_folder = QFileInfo(gsvar_file_).absolutePath();
			QString project_folder = QFileInfo(pair_folder).absolutePath();
//			segfile = project_folder + "/Sample_" + tumor_ps_name + "/" + tumor_ps_name + "_cnvs.seg";
//			output << FileLocation{tumor_ps_name, PathType::CNV_ESTIMATES, segfile, false};
			output << FileLocation{tumor_ps_name, PathType::CNV_ESTIMATES, project_folder + "/Sample_" + tumor_ps_name + "/" + tumor_ps_name + "_cnvs.seg", false};
		}
	}
	else
	{
		QList<FileLocation> tmp = getBamFiles();
		foreach(const FileLocation& file, tmp)
		{
			QString base_name = file.filename.left(file.filename.length()-4);
//			QString segfile = base_name + "_cnvs_clincnv.seg";
			appendIfUnique(output, FileLocation{file.id, PathType::CNV_ESTIMATES, base_name + "_cnvs_clincnv.seg", false});
//			if (QFile::exists(segfile))
//			{
//				output << FileLocation{file.id, PathType::CNV_ESTIMATES, segfile, false};
//			}
//			else
//			{
//				segfile = base_name + "_cnvs.seg";
				appendIfUnique(output, FileLocation{file.id, PathType::CNV_ESTIMATES, base_name + "_cnvs.seg", false});
//				if (QFile::exists(segfile))
//				{
//					output << FileLocation{file.id, PathType::CNV_ESTIMATES, segfile, false};
//				}
//			}
		}
	}

	return output;
}

QList<FileLocation> FileLocationProviderFileSystem::getIgvFilesBaf()
{
	QList<FileLocation> output;

	if (analysis_type_==SOMATIC_PAIR)
	{
//		QString seg_file_name = gsvar_file_.left(gsvar_file_.length()-6) + "_bafs.igv";
//		QString pair = QFileInfo(gsvar_file_).baseName();
//		FileLocation seg_file = FileLocation{pair, PathType::BAF, seg_file_name, false};
//		if (!output.contains(seg_file)) output << FileLocation{pair, PathType::BAF, seg_file_name, false};
		output << FileLocation{QFileInfo(gsvar_file_).baseName(), PathType::BAF, gsvar_file_.left(gsvar_file_.length()-6) + "_bafs.igv", false};
	}
	else
	{
		QList<FileLocation> tmp = getBamFiles();
		foreach(const FileLocation& file, tmp)
		{
//			QString seg_file_name = file.filename.left(file.filename.length()-4) + "_bafs.igv";
//			FileLocation seg_file = FileLocation{file.id, PathType::BAF, seg_file_name, false};
			appendIfUnique(output, FileLocation{file.id, PathType::BAF, file.filename.left(file.filename.length()-4) + "_bafs.igv", false});
//			if (output.contains(seg_file)) continue;
//			if (QFile::exists(segfile))
//			{
//				output << FileLocation{file.id, PathType::BAF, seg_file_name, false};
//			}
		}
	}

	return output;
}

QList<FileLocation> FileLocationProviderFileSystem::getMantaEvidenceFiles()
{
	QList<FileLocation> evidence_files;

	// search at location of all available BAM files
	QList<FileLocation> bam_files = getBamFiles();

//	qDebug() << "FILES=";
	foreach (FileLocation bam_file, bam_files)
	{
		QString evidence_bam_file = FileLocationHelper::getEvidenceFile(bam_file.filename);

		// check if evidence file exists
//		if (!QFile::exists(evidence_bam_file)) continue;

//		qDebug() << evidence_bam_file;

//		FileLocation evidence_file;
//		evidence_file.filename = evidence_bam_file;
//		evidence_file.type = PathType::BAM;
//		evidence_file.id = QFileInfo(evidence_bam_file).baseName();

		appendIfUnique(evidence_files, FileLocation{QFileInfo(evidence_bam_file).baseName(), PathType::BAM, evidence_bam_file, false});
//		if (evidence_files.contains(evidence_file)) continue;
//		evidence_files.append(evidence_file);
	}
	return evidence_files;
}

void FileLocationProviderFileSystem::appendIfUnique(QList<FileLocation>& list, const FileLocation item)
{
	if (!list.contains(item))
	{
		list.append(item);
	}
}
