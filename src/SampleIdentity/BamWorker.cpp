#include "BamWorker.h"
#include "BamReader.h"

BamWorker::BamWorker(const BamWorkerArgs& args)
	:args_(args)
{
}

void BamWorker::run()
{
	if (!QFile::exists(args_.bam))
	{

		emit outputMessage("##skipped " + args_.bam + ": file does not exist\n");
		return;
	};

	try
	{
		BamReader reader(args_.bam, args_.ref);
		reader.skipQualities();
		reader.skipTags();

		AfData tmp;
		tmp.resize(args_.snps.count());
		for (int i =0; i < args_.snps.count(); ++i)
		{
			const Chromosome& chr = args_.snps[i].chr();
			int pos = args_.snps[i].start();
			char ref = args_.snps[i].ref()[0];
			int ref_c =0;
			char alt = args_.snps[i].alt()[0][0];
			int alt_c =0;

			reader.setRegion(chr, pos, pos);

			BamAlignment al;
			while (reader.getNextAlignment(al))
			{
				if (al.isSecondaryAlignment() || al.isSupplementaryAlignment() || al.isDuplicate()) continue;

				QPair<char, int> base = al.extractBaseByCIGAR(pos);
				if (base.first == ref) ++ref_c;
				if (base.first == alt) ++alt_c;
			}

			if (ref_c+alt_c < args_.min_depth)
			{
				tmp[i] = -1;
				if (args_.debug) emit debugMessage(" low coverage for " + args_.snps[i].toString() + " in " + args_.bam + "\n");
			}
			else
			{
				tmp[i] = std::round(100.0*alt_c/(ref_c + alt_c));
			}
		}
		args_.output_af = tmp;

		if (args_.time) emit bamDone();
	}
	catch (Exception& e)
	{
		emit outputMessage("##skipped " + args_.bam + " because of error: " + e.message().replace("\n", " ") + "\n");
	}
}
