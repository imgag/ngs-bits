#include "BamWorker.h"
#include "BamReader.h"

BamWorker::BamWorker(const BamWorkerArgs& args)
	:bam_(args.bam), output_af_(args.output_af),
	is_valid_(args.is_valid), snps_(args.snps), infile_(args.infile),
	out_stream_(args.out_stream), out_stream_mtx_(args.out_stream_mtx),
	debug_stream_(args.debug_stream), debug_stream_mtx_(args.debug_stream_mtx),
	timer_(args.timer), bams_done_(args.bams_done), bams_done_mtx_(args.bams_done_mtx),
	min_depth_(args.min_depth), debug_(args.debug), time_(args.time){}


void BamWorker::run()
{
	if (!QFile::exists(bam_)) {
		QMutexLocker lock(&out_stream_mtx_);
		out_stream_ << "##skipped " << bam_ << ": file does not exist\n";
		is_valid_ = false;
		return;
	};

	try{
		BamReader reader(bam_, infile_);
		reader.skipQualities();
		reader.skipTags();

		for (int i =0; i < snps_.count(); ++i){
			const Chromosome& chr = snps_[i].chr();
			int pos = snps_[i].start();
			char ref = snps_[i].ref()[0];
			int ref_c =0;
			char alt = snps_[i].alt()[0][0];
			int alt_c =0;

			reader.setRegion(chr, pos, pos);

			BamAlignment al;
			while (reader.getNextAlignment(al)){
				if (al.isSecondaryAlignment() || al.isSupplementaryAlignment() || al.isDuplicate() || al.isUnmapped()) continue;

				QPair<char, int> base = al.extractBaseByCIGAR(pos);
				if (base.first == ref) ++ref_c;
				if (base.first == alt) ++alt_c;
			}

			if (ref_c+alt_c < min_depth_){
				output_af_[i] = -1;
				QMutexLocker lock(&debug_stream_mtx_);
				if (debug_)
					debug_stream_ << " low coverage for " << snps_[i].toString() << " in " << bam_ << Qt::endl;
			} else {
				output_af_[i] = std::round(100.0*alt_c/(ref_c + alt_c));
			}
		}
		is_valid_ = true;

		if (time_){
			QMutexLocker lock(&bams_done_mtx_); //this mutex is for both the timer and bams_done
			bams_done_++;
			if (bams_done_%100 == 0){
				QMutexLocker lock(&debug_stream_mtx_);
				debug_stream_ << "##Determining SNPs for 100 BAM/CRAM files took " << Helper::elapsedTime(timer_.restart()) << Qt::endl;
			}
		}
	} catch (Exception& e){
		QMutexLocker lock(&out_stream_mtx_);
		out_stream_ << "##skipped " << bam_ << " because of error: " << e.message().replace("\n", " ") << Qt::endl;
		is_valid_ = false;
	}
}
