#ifndef REFGENOMESERVICE_H
#define REFGENOMESERVICE_H

#include "cppNGS_global.h"
#include "Exceptions.h"
#include <QString>

class CPPNGSSHARED_EXPORT RefGenomeService
{
public:
    static void setReferenceGenome(QString filename);
    static const QString& getReferenceGenome();

protected:
    RefGenomeService();
    ~RefGenomeService();
    static RefGenomeService& instance();

private:
    QString ref_genome_file_;
};



#endif // REFGENOMESERVICE_H
