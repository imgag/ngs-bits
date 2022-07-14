#ifndef SOMATICCNVINTERPRETER_H
#define SOMATICCNVINTERPRETER_H

#include "cppNGS_global.h"
#include "CnvList.h"


struct SomaticGeneRole
{
	//role of gene in cancer
	enum class Role
	{
		ACTIVATING,
		LOSS_OF_FUNCTION,
		AMBIGUOUS
	};

	QByteArray gene = ""; //gene symbol
	Role role = Role::AMBIGUOUS;
	bool high_evidence = false; //level of evidence
	QString comment = "";

	///returns gene role as string
	QString roleAsString() const
	{
		if(role == Role::ACTIVATING) return "activating";
		else if(role == Role::LOSS_OF_FUNCTION) return "loss_of_function";
		else return "ambiguous";
	}
};


class CPPNGSSHARED_EXPORT SomaticCnvInterpreter
{
public:
	static bool includeInReport(const CnvList& cnvs_, const CopyNumberVariant& cnv, const SomaticGeneRole& gene_role);

private:
	SomaticCnvInterpreter() = delete;
};

#endif // SOMATICCNVINTERPRETER_H
