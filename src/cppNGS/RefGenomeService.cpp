#include "RefGenomeService.h"
#include "Settings.h"
#include "Exceptions.h"

RefGenomeService::RefGenomeService()
{
}

RefGenomeService::~RefGenomeService()
{
}

void RefGenomeService::setReferenceGenome(QString filename)
{
	instance().ref_genome_file_ = filename.trimmed();
}

const QString& RefGenomeService::getReferenceGenome()
{
	//fallback to settings INI if unset
    if (instance().ref_genome_file_.isEmpty())
    {
		QString genome_from_settings = Settings::string("reference_genome", true).trimmed();
		if (!genome_from_settings.isEmpty())
		{
			instance().ref_genome_file_ = genome_from_settings;
		}
	}

	//error if not set
	if (instance().ref_genome_file_.isEmpty())
	{
        THROW(ProgrammingException, "Reference genome file name requested but not set!");
    }

    return instance().ref_genome_file_;
}

RefGenomeService& RefGenomeService::instance()
{
    static RefGenomeService instance;
    return instance;
}
