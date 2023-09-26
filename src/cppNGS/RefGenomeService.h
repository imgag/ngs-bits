#ifndef REFGENOMESERVICE_H
#define REFGENOMESERVICE_H

#include "cppNGS_global.h"
#include "Exceptions.h"
#include <QString>

//Singleton that defines the reference genome used in the application (e.g. for reading CRAM files)
class CPPNGSSHARED_EXPORT RefGenomeService
{
public:
	//returns the reference genome FASTA file. Default is the 'reference_genome' entry from the settings INI file.
	static const QString& getReferenceGenome();
	//sets the reference genome FASTA file. Setting the reference genome manually should be necessary only in special cases.
	static void setReferenceGenome(QString filename);

protected:
    RefGenomeService();
    ~RefGenomeService();
    static RefGenomeService& instance();

private:
    QString ref_genome_file_;
};



#endif // REFGENOMESERVICE_H
