#ifndef RECALLINGWORKER_H
#define RECALLINGWORKER_H

#include <QRunnable>
#include <QMutex>
#include <QTextStream>
#include "Auxilary.h"

class ReCallingWorker
		: public QRunnable
{

public:
	ReCallingWorker(const Chromosome& chr, QString bam, QString ref_file, VcfData& data, const QList<VariantDefinition>& var_defs, bool no_genotype_correction, OutputData& out_data);
	void run() override;

protected:
	Chromosome chr_;
	QString bam_;
	QString ref_file_;
	VcfData& data_;
	const QList<VariantDefinition>& var_defs_;
	bool no_genotype_correction_;
	OutputData& out_data_;

	void writeLog(qint64 time, QString log, QString error_message = QString());
};

#endif // RECALLINGWORKER_H
