#include "FileLocationProviderNGSD.h"
#include "LoginManager.h"
#include "NGSD.h"

QList<IgvFile> FileLocationProviderNGSD::getBamFilesInNGSD()
{
	QList<IgvFile> output;

	if (getVariants().count() == 0)
	{
//		throw out_of_range("Variant list is empty");
		return output;
	}

	SampleHeaderInfo data = getVariants().getSampleHeader();
	foreach(const SampleInfo& info, data)
	{
		bool found = false;
		QString bam_file = "";

		if (LoginManager::active())
		{
			NGSD db;
			QString ps_id = db.processedSampleId(info.id, false);
			if (ps_id!="")
			{
				bam_file = db.processedSamplePath(ps_id, NGSD::BAM);
				if (QFile::exists(bam_file))
				{
					found = true;
					output << IgvFile{info.id, "BAM" , bam_file};
				}
			}
		}

		if (!found)
		{
//			throw exception("Could not find BAM file at the default location:"+bam_file);
			output.clear();
			return output;
		}
	}

	return output;
}
