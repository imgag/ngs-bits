#ifndef BAMTRACKDATA_H
#define BAMTRACKDATA_H

#include "SharedData.h"
#include "BamReader.h"
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

	BamAlignmentWrapper(BamAlignment aln)
		: id(AlignmentKey::makeKey(aln))
	{
		storeAlignmentInfo(aln);
	}

	BamAlignmentWrapper(BamAlignment&& aln)
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

	bool operator==(const BamAlignmentWrapper& other) const
	{
		return id == other.id;
	}

	int start(bool soft_clip = false) const { return (soft_clip) ? st_with_soft_clip_ : st_; }

	int end(bool soft_clip = false) const { return (soft_clip) ? en_with_soft_clip_ :  en_; }

	bool isReverseStrand() const {return is_reverse_;}

	int mateStart() const {return mate_start_;}
	int mateChrId() const {return mat_chr_id_;}

	bool isMateMapped() const {return mate_is_mapped_;}

	QString name() const {return name_;}

private:
	int st_;
	int en_;
	int st_with_soft_clip_;
	int en_with_soft_clip_;
	bool is_reverse_;
	bool mate_is_mapped_;
	int mat_chr_id_ = -1;
	int mate_start_;
	QString name_;

	void storeAlignmentInfo(const BamAlignment& al)
	{
		st_ = al.start();
		en_ = al.end();
		st_with_soft_clip_ = st_;
		en_with_soft_clip_ = en_;
		is_reverse_ = al.isReverseStrand();
		mate_is_mapped_ = !al.isMateUnmapped();
		mate_start_ = al.mateStart();
		mat_chr_id_ = al.mateChrosomeID();
		name_ = al.name();
	}
};

inline size_t qHash(const BamAlignmentWrapper& key, size_t seed = 0)
{
	return qHash(key.id, seed);
}

/*this is used for connecting BamAlignemnt and BamCoverage tracks*/
class CPPVISUALSHARED_EXPORT BamTrackData : public QObject
{
	Q_OBJECT
public:
	BamTrackData(QString file_path)
		: file_path_(file_path)
	{
		connect(SharedData::instance(), SIGNAL(regionChanged()), this, SLOT(updateRegion()));
	}

	const QVector<BamAlignmentWrapper>& getAlignments()
	{
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
		bam_reader_ = reader;
		loaded_region_.setEnd(-1); // invalidate loaded_region
		alignments_.clear();
		loaded_ids_.clear();

		alignments_.squeeze();
		loaded_ids_.squeeze();

		updateRegion();
	}

	void reload();

signals:
	// void onDataChange();
	void onDataUpdate();
	void onFullLoad();

public slots:
	void updateRegion();

private:
	QString file_path_;
	QSharedPointer<BamReader> bam_reader_;
	QVector<BamAlignmentWrapper> alignments_;
	QVector<BamAlignmentWrapper> dummy_alignments_;
	Sequence ref_seq_;
	int ref_start_;
	QSet<AlignmentKey> loaded_ids_;

	void updateData();

	bool is_loading_;
	bool return_empty_ = true;

	//loaded region
	BedLine loaded_region_;

	void fullLoad(const BedLine& region);

	void fetchRegion(const BedLine& region, QVector<BamAlignmentWrapper>* dest = nullptr);

	void pruneAlignments(int keep_start, int keep_end);
};

#endif // BAMTRACKDATA_H
