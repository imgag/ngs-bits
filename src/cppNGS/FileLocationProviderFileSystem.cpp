#include "FileLocationProviderFileSystem.h"

QList<IgvFile> FileLocationProviderFileSystem::getBamFilesInFileSystem()
{
	QList<IgvFile> output;

	if (getFilename().length() == 0)
	{
		THROW(Exception, "File name list is empty")
		return output;
	}

	if (getVariants().count() == 0)
	{
		THROW(Exception, "Variant list is empty");
		return output;
	}

	QString sample_folder = QFileInfo(getFilename()).absolutePath();
	QString project_folder = QFileInfo(sample_folder).absolutePath();

	SampleHeaderInfo data = getVariants().getSampleHeader();
	foreach(const SampleInfo& info, data)
	{
		bool found = false;
		QString bam_from_sample = sample_folder + "/" + info.id + ".bam";
		QString bam_from_project = project_folder + "/Sample_" + info.id + "/" + info.id + ".bam";

		if (QFile::exists(bam_from_sample))
		{
			found = true;
			output << IgvFile{info.id, "BAM" , bam_from_sample};
		}
		else if (QFile::exists(bam_from_project))
		{
			found = true;
			output << IgvFile{info.id, "BAM" , bam_from_project};
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
