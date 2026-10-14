#ifndef BAMTRACKDATA_H
#define BAMTRACKDATA_H

#include "SharedData.h"
#include "BamReader.h"
#include "ParameterList.h"
#include "cppVISUAL_global.h"

#include <QObject>
#include <QString>

// Alignment key for BamAlignment for making it hashable
struct CPPVISUALSHARED_EXPORT AlignmentKey
{
	uint64_t a;
	uint64_t b;

	bool operator==(const AlignmentKey& other) const
	{
		return a == other.a && b == other.b;
	}

	static AlignmentKey makeKey(const BamAlignment& al);
};

inline size_t qHash(const AlignmentKey& k, size_t seed = 0)
{
	return qHashMulti(seed, k.a, k.b);
}

// wrapper around BamAlignment, used for storing the alignment data in a paint friendly way
struct CPPVISUALSHARED_EXPORT BamAlignmentWrapper
{
	enum Event // Cigar Op
	{
		MATCH, // CMATCH, CEQUAL, CDIFF
		INSERTION, // CINS
		DELETION, // CDEL
		SOFT_CLIP, // C_SOFT_CLIP
	};

	struct EventData // event data from cigar
	{
		Event event;
		int length;
		int genome_pos; //genome start position
		QByteArray bases; // all the bases of the event, empty in case of deletion
		QVector<int> qualities; // all qualities of the event, empty in case of deletion
	};

	struct MismatchInfo
	{
		int genomic_pos;
		char base; // base at genomic pos
		int quality; // quality at genomic pos
	};

	AlignmentKey id; // for hashing
	// uncomment if features needed in future
	// BamAlignment alignment; // not stored currently because of data duplication
	QVector<MismatchInfo> mismatches; // cached mismatches
	QVector<EventData> events;
	Chromosome mate_chr; //needs to be set on creation, cannot be taken as argument because bam_reader_->chromosome might fail

	BamAlignmentWrapper(const BamAlignment& aln)
		: id(AlignmentKey::makeKey(aln))
	{
		storeAlignmentInfo(aln);
	}

	void storeCigarData(const BamAlignment& alignment, const Sequence& ref_seq, int ref_start);

	const QVector<MismatchInfo>& getMismatches() const
	{
		return mismatches;
	}

	const QVector<EventData>& getEvents() const
	{
		return events;
	}

	int start() const { return st_; }

	int end() const { return en_; }

	int startWithSoftClip() const { return st_with_soft_clip_; }

	int endWithSoftClip() const { return en_with_soft_clip_; }

	bool isReverseStrand() const { return is_reverse_; }
	bool isPaired() const { return is_paired_; }
	// Orientation of the left and right mates: FR, RF, FF, RR.
	int pairOrientation() const { return pair_orientation_; }

	int mateStart() const { return mate_start_; }

	bool isMateMapped() const { return mate_is_mapped_; }

	QString name() const { return name_; }

	bool isValid() const { return (st_ >= 0 && en_ >= 0); }

	int insertSize() const { return insert_size_; }
	// Diploid haplotype from HP: 1, 2, or 0 for unassigned.
	int phase() const { return phase_; }
	bool isSecondaryAlignment() const { return is_secondary_; }
	int mappingQuality() const { return mapping_quality_; }

private:
	int st_ = -1;
	int en_ = -1;
	int st_with_soft_clip_;
	int en_with_soft_clip_;
	bool is_reverse_;
	bool is_paired_;
	int pair_orientation_;
	bool mate_is_mapped_;
	int mate_start_;
	int insert_size_ = -1;
	int phase_ = 0;
	bool is_secondary_ = false;
	int mapping_quality_ = 0;
	QString name_;

	void storeAlignmentInfo(const BamAlignment& al)
	{
		st_ = al.start();
		en_ = al.end();
		st_with_soft_clip_ = st_;
		en_with_soft_clip_ = en_;
		is_reverse_ = al.isReverseStrand();
		is_paired_ = al.isPaired();
		mate_is_mapped_ = is_paired_ && !al.isMateUnmapped();
		mate_start_ = al.mateStart();
		bool left_reverse = is_reverse_;
		bool right_reverse = al.isMateReverseStrand();
		if (mate_start_ < st_ || (mate_start_ == st_ && al.isRead2())) std::swap(left_reverse, right_reverse);
		pair_orientation_ = left_reverse == right_reverse ? (left_reverse ? 3 : 2) : (left_reverse ? 1 : 0);
		name_ = al.name();
		insert_size_ = al.insertSize();
		const int hp = al.tagi("HP");
		phase_ = hp == 1 || hp == 2 ? hp : 0;
		is_secondary_ = al.isSecondaryAlignment();
		mapping_quality_ = al.mappingQuality();
	}
};

struct InsertSizeStats
{
	int insert_size_min = 0;
	int insert_size_max = 1000;
};

/*this is used for connecting BamAlignemnt and BamCoverage tracks*/
class CPPVISUALSHARED_EXPORT BamTrackData : public QObject
{
	Q_OBJECT
public:
	BamTrackData(QString file_path)
		: file_path_(file_path)
		, visibility_range_(SharedData::settings().bam_max_region_len)
	{
		connect(SharedData::instance(), SIGNAL(regionChanged()), this, SLOT(updateRegion()));
		connect(SharedData::instance(), &SharedData::settingsChanged, this, [this]()
		{
			computeInsertSizeStats();
			updateRegion();
		});
	}

	const QVector<BamAlignmentWrapper>& getAlignments()
	{
		// returns empty alignments when region is bigger than max size
		// so that downstream tasks don't accidentally process new data
		return (return_empty_) ? dummy_alignments_ : alignments_;
	}

	const Sequence& getReferenceSeq()
	{
		return ref_seq_;
	}

	int getRefSeqStart() const
	{
		return ref_start_;
	}

	void setBamReader(QSharedPointer<BamReader> reader)
	{
		const bool paired_end = reader->checkIfPairedEnd();
		bam_reader_ = reader;
		paired_end_ = paired_end;
		loaded_region_.setEnd(-1); // invalidate loaded_region
		alignments_.clear();
		loaded_ids_.clear();

		alignments_.squeeze();
		loaded_ids_.squeeze();

		updateRegion();
	}

	void computeInsertSizeStats();
	bool isPairedEnd() const { return paired_end_; }

	InsertSizeStats getInsertSizeStats() { return insert_size_stats_; };

	void reload();
	// Keeps visibility parameters of all tracks using this data in sync.
	void bindVisibilityRange(ParameterList& parameters);
	void setVisibilityRange(int range);
	// Synchronizes read filters between alignment and coverage tracks for this file.
	void bindReadFilters(ParameterList& parameters);

signals:
	// void onDataChange();
	void onDataUpdate();
	void onFullLoad();
	void visibilityRangeChanged(int range);
	void readFilterChanged(const QByteArray& name, const QVariant& value);

public slots:
	void updateRegion();

private:
	QString file_path_;
	int visibility_range_;
	QHash<QByteArray, QVariant> read_filters_{{"show soft-clipped bases", false}, {"show secondary alignments", true}, {"min MQ", 0}};
	bool syncing_read_filters_ = false;
	QSharedPointer<BamReader> bam_reader_;
	bool paired_end_ = false;
	QVector<BamAlignmentWrapper> alignments_;
	QVector<BamAlignmentWrapper> dummy_alignments_;
	Sequence ref_seq_;
	int ref_start_;
	QSet<AlignmentKey> loaded_ids_;

	bool invalid_mate_warning_shown_ = false;

	bool is_loading_ = false;
	bool return_empty_ = true;
	InsertSizeStats insert_size_stats_;

	//loaded region
	BedLine loaded_region_;

	void fullLoad(const BedLine& region);

	void fetchRegion(const BedLine& region, QVector<BamAlignmentWrapper>* dest = nullptr);

	void pruneAlignments(int keep_start, int keep_end);
};

#endif // BAMTRACKDATA_H
