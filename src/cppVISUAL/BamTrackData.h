#ifndef BAMTRACKDATA_H
#define BAMTRACKDATA_H

#include "SharedData.h"
#include "BamReader.h"

#include <QObject>
#include <QString>

struct AlignmentKey
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

struct BamAlignmentWrapper
{
	enum Event // Cigar Op
	{
		MATCH, // CMATCH, CEQUAL, CDIFF
		INSERTION, // CINS
		DELETION // CDEL
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
	BamAlignment alignment;
	QVector<MismatchInfo> mismatches; // cached mismatches
	QVector<EventData> events;

	BamAlignmentWrapper(BamAlignment aln)
		: id(AlignmentKey::makeKey(aln)), alignment(aln)
	{
	}

	BamAlignmentWrapper(BamAlignment&& aln)
		: id(AlignmentKey::makeKey(aln)), alignment(std::move(aln))
	{
	}

	void storeCigarData(const Sequence& ref_seq, int ref_start);

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
};

inline size_t qHash(const BamAlignmentWrapper& key, size_t seed = 0)
{
	return qHash(key.id, seed);
}

/*this is used for connecting BamAlignemnt and BamCoverage tracks*/
class BamTrackData : public QObject
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

	void fetchRegion(const BedLine& region);

	void pruneAlignments(int keep_start, int keep_end);
};

#endif // BAMTRACKDATA_H
