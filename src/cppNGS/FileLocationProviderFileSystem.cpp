#include "FileLocationProviderFileSystem.h"

FileLocationProviderFileSystem::FileLocationProviderFileSystem(QString gsvar_file, const SampleHeaderInfo header_info)
  : gsvar_file_(gsvar_file)
  , header_info_(header_info)
{
}

QList<FileLocation> FileLocationProviderFileSystem::getBamFiles()
{
	qDebug("FileLocationProviderFileSystem.getBamFiles() has been invoked");
	QList<FileLocation> output;

	if (gsvar_file_ == nullptr)
	{
		THROW(Exception, "File name has not been specified")
		return output;
	}

	if (header_info_.empty())
	{
		THROW(Exception, "Header information has not been specified");
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
