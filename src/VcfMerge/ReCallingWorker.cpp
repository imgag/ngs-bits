#include "ReCallingWorker.h"
#include "Exceptions.h"
#include "BamReader.h"
#include <QMutexLocker>

ReCallingWorker::ReCallingWorker(QString bam, QString ref_file, VcfData& data, const QList<VariantDefinition>& var_defs, bool no_genotype_correction, QMutex& mutex, QTextStream& debug, bool& errors)
	: QRunnable()
	, bam_(bam)
	, ref_file_(ref_file)
	, data_(data)
	, var_defs_(var_defs)
	, no_genotype_correction_(no_genotype_correction)
	, mutex_(mutex)
	, debug_(debug)
	, errors_(errors)
{
}

// single chunks are processed
void ReCallingWorker::run()
{
	try
	{
		log_ << ("re-calling of variants for sample " + data_.sample);

		//init
		int c_added = 0;
		int c_added_snv = 0;
		int c_added_indel = 0;
		QElapsedTimer timer;
		timer.start();

		BamReader reader(bam_, ref_file_);
		for (const VariantDefinition& var: std::as_const(var_defs_))
		{
			//skip called variants
			if (data_.tag_to_format.contains(var.tag)) continue;

			Pileup pileup = reader.getPileup(var.chr, var.pos, (var.is_snv ? -1 : 1), -1, false, -1);
			int depth = pileup.depth(false);
			QByteArray gt = "0/0";
			QByteArray dp = QByteArray::number(depth);
			QByteArray af = ".";
			QByteArray ct = ".";

			//determine AF and adapt GT if reasonable
			if (var.is_snv)
			{
				double freq = pileup.frequency(var.ref[0], var.alt[0]);
				if (BasicStatistics::isValidFloat(freq))
				{
					af = QByteArray::number(freq, 'f', 3);
					if (!no_genotype_correction_)
					{
						if (depth>=10 || pileup.countOf(var.alt[0])>3)
						{
							if (freq>0.9) gt = "1/1";
							else if (freq>0.1) gt = "0/1";
						}
					}
				}
			}
			else if (var.ref.size()==1) //insertion
			{
				//count insertion with the same seqence
				QByteArray expected = "+" +var.alt.mid(1);
				int count = 0;
				foreach(const Sequence& seq, pileup.indels())
				{
					if (seq==expected) ++count;
				}

				//determine af
				double freq = (double) count / (double) depth;
				if (BasicStatistics::isValidFloat(freq))
				{
					af = QByteArray::number(freq, 'f', 3);
					if (!no_genotype_correction_)
					{
						if (depth>=10 || count>3)
						{
							if (freq>0.9) gt = "1/1";
							else if (freq>0.1) gt = "0/1";
						}
					}
				}
			}
			else if (var.alt.size()==1) //deletion
			{
				//count deletions of the right size
				QByteArray expected = "-" +QByteArray::number(var.ref.size()-1);
				int count = 0;
				foreach(const Sequence& seq, pileup.indels())
				{
					if (seq==expected) ++count;
				}

				//determine af
				double freq = (double) count / (double) depth;
				if (BasicStatistics::isValidFloat(freq))
				{
					if (depth>0) af = QByteArray::number(freq, 'f', 3);
					if (!no_genotype_correction_)
					{
						if (depth>=10 || count>3)
						{
							if (freq>0.9) gt = "1/1";
							else if (freq>0.1) gt = "0/1";
						}
					}
				}
			}

			//flag added variants
			if (gt!="0/0")
			{
				++c_added;
				if (var.is_snv) ++c_added_snv;
				else ++c_added_indel;
				ct = "RC";
			}

			data_.tag_to_format[var.tag] = FormatData{gt, dp, af, ".", ".", ct};
		}
		log_ << ("  updated GT of variants: " + QString::number(c_added) + " (SNVs: " + QString::number(c_added_snv) + " INDELs: " + QString::number(c_added_indel) + ")");
		log_ << ("  time re-calling (single thread): " + Helper::elapsedTime(timer.restart()));

		writeLog();
	}
	catch(Exception& e)
	{
		writeLog(e.message());
	}
}

void ReCallingWorker::writeLog(QString error_message)
{
	QMutexLocker locker(&mutex_);

	for(const QString& line: std::as_const(log_))
	{
		debug_ << line << "\n";
	}

	if (!error_message.isEmpty())
	{
		debug_ << "  Error in re-calling worker: " << error_message << "\n";
		errors_ = true;
	}

	debug_ << Qt::endl;
}

