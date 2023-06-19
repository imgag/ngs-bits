#include "RefGenomeService.h"

RefGenomeService::RefGenomeService()
{
}

RefGenomeService::~RefGenomeService()
{
}

void RefGenomeService::setReferenceGenome(QString filename)
{
    instance().ref_genome_file_ = filename;
}

const QString& RefGenomeService::getReferenceGenome()
{
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
