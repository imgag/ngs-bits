#ifndef SOMATICCNVINTERPRETER_H
#define SOMATICCNVINTERPRETER_H

#include "cppNGS_global.h"
#include <QString>

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

	///Returns a string representaion of the gene role.
	QString asString() const
	{
		if(role == Role::ACTIVATING) return "activating";
		else if(role == Role::LOSS_OF_FUNCTION) return "loss_of_function";
		else return "ambiguous";
	}

	///Returns if the gene role is valid.
	bool isValid() const
	{
		return !gene.isEmpty();
	}
};


class CPPNGSSHARED_EXPORT SomaticCnvInterpreter
{
public:
	static bool includeInReport(const int copy_number, const QByteArray& cnv_type, const SomaticGeneRole& gene_role);

private:
	SomaticCnvInterpreter() = delete;
};

#endif // SOMATICCNVINTERPRETER_H
