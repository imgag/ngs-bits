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
	ReCallingWorker(QString bam, QString ref_file, VcfData& data, const QList<VariantDefinition>& var_defs, bool no_genotype_correction, QMutex& mutex, QTextStream& debug, bool& errors);
	void run() override;

protected:
	QString bam_;
	QString ref_file_;
	VcfData& data_;
	const QList<VariantDefinition>& var_defs_;
	bool no_genotype_correction_;

	QStringList log_;
	QMutex &mutex_;
	QTextStream& debug_;
	bool& errors_;

	void writeLog(QString error_message = QString());
};

#endif // RECALLINGWORKER_H
