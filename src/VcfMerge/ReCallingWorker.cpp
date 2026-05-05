#include "ReCallingWorker.h"
#include "Exceptions.h"
#include "BamReader.h"
#include <QMutexLocker>

ReCallingWorker::ReCallingWorker(const Chromosome &chr, QString bam, QString ref_file, VcfData& data, const QList<VariantDefinition>& var_defs, bool no_genotype_correction, OutputData& out_data)
	: QRunnable()
	, chr_(chr)
	, bam_(bam)
	, ref_file_(ref_file)
	, data_(data)
	, var_defs_(var_defs)
	, no_genotype_correction_(no_genotype_correction)
	, out_data_(out_data)
{
}

// single chunks are processed
void ReCallingWorker::run()
{
	QElapsedTimer timer;
	timer.start();
	try
	{
		//init
		int c_added_snv = 0;
		int c_added_indel = 0;

		//get tags of called variants (own scope for QMutexLocker)
		QSet<QByteArray> called_vars;
		{
			QMutexLocker locker(data_.tag_to_format_mutex);
			called_vars = Helper::listToSet(data_.tag_to_format.keys());
		}

		//re-call
		QHash<QByteArray, FormatData> tmp; //we used a tmp variable so that we need to lock only once
		BamReader reader(bam_, ref_file_);
		for (const VariantDefinition& var: std::as_const(var_defs_))
		{
			//skip wrong chromosomes
			if (var.chr!=chr_) continue;

			//skip called variants
			if (called_vars.contains(var.tag)) continue;

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
				if (var.is_snv) ++c_added_snv;
				else ++c_added_indel;
				ct = "RC";
			}

			tmp[var.tag] = FormatData{gt, dp, af, ".", ".", ct};
		}

		//insert variants (own scope for QMutexLocker)
		{
			QMutexLocker locker(data_.tag_to_format_mutex);
			data_.tag_to_format.insert(tmp);
		}

		writeLog(timer.restart(), "updated GT of " + QString::number(c_added_snv) + " SNVs and " + QString::number(c_added_indel) + " INDELs");
	}
	catch(Exception& e)
	{
		writeLog(timer.restart(), "", e.message());
	}
}

void ReCallingWorker::writeLog(qint64 time, QString log, QString error_message)
{
	QMutexLocker locker(out_data_.mutex);

	out_data_.stream << "re-calling of variants for " + data_.sample + "/" + chr_.str() + " - time: " + Helper::elapsedTime(time);
	if (!log.isEmpty()) out_data_.stream << " - " << log;
	if (!error_message.isEmpty())
	{
		out_data_.stream << " - error: " << error_message;
		out_data_.error_occurred = true;
	}

	out_data_.stream << Qt::endl;
}

