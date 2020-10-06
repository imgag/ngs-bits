#include "FileLocationProviderNGSD.h"
#include "LoginManager.h"
#include "NGSD.h"

FileLocationProviderNGSD::FileLocationProviderNGSD(QString procssed_sample_id)
{
	procssed_sample_id_ = procssed_sample_id;
}

QList<FileLocation> FileLocationProviderNGSD::getBamFiles()
{
	qDebug("FileLocationProviderNGSD.getBamFiles() has been invoked");
	QList<FileLocation> output;

	if (procssed_sample_id_ == nullptr)
	{
		THROW(Exception, "Processed sample id has not been specified");
		return output;
	}

	bool found = false;
	QString bam_file = "";

	if (LoginManager::active())
	{
		NGSD db;
		QString ps_id = db.processedSampleId(procssed_sample_id_, false);
		if (ps_id!="")
		{
			bam_file = db.processedSamplePath(ps_id, PathType::BAM);
			if (QFile::exists(bam_file))
			{
				found = true;
				output << FileLocation{procssed_sample_id_, PathType::BAM, bam_file};
			}
		}
	}

	if (!found)
	{
		THROW(Exception, "Could not find BAM file at the default location:"+bam_file);
		output.clear();
		return output;
	}

	return output;
}
