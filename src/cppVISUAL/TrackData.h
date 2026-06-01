#ifndef TRACKDATA_H
#define TRACKDATA_H

#include "SharedData.h"
#include "BamReader.h"

#include <QObject>
#include <QString>

struct TrackData
{
	QString file_path;
	QString name;

	TrackData(QString file_path, QString name)
		: file_path(file_path), name(name)
	{
	}
};

struct BedTrackData : public TrackData
{
	QSharedPointer<BedFile> bed_file;
};

struct AlignmentKey
{
	uint64_t a;
	uint64_t b;

	bool operator==(const AlignmentKey& other) const
	{
		return a == other.a && b == other.b;
	}
};

inline size_t qHash(const AlignmentKey& k, size_t seed = 0)
{
	return qHashMulti(seed, k.a, k.b);
}

static AlignmentKey makeKey(const BamAlignment& al)
{
	QByteArray cigar = al.cigarDataAsString();

	uint64_t h1 = qHash(al.name());
	h1 ^= ((uint64_t)al.start() << 1);
	h1 ^= ((uint64_t)al.isReverseStrand() << 32);

	uint64_t h2 = qHashBits(cigar.data(), cigar.size());

	return {h1, h2};
}

struct BamAlignmentWrapper
{
	struct VariantInfo
	{
		int genomic_pos;
		char base;
		int quality;
	};

	AlignmentKey id; // for hashing
	BamAlignment alignment;
	QVector<VariantInfo> variants;

	BamAlignmentWrapper(BamAlignment aln)
		: id(makeKey(aln)), alignment(aln)
	{
	}

	BamAlignmentWrapper(BamAlignment&& aln)
		: id(makeKey(aln)), alignment(std::move(aln))
	{
	}

	void storeVariants(const Sequence& ref_seq, int ref_start)
	{
		variants.clear();

		alignment.forEachAlignedBase(
			[&](int genome_pos, char base, int qual, int)
			{
				int idx = genome_pos - ref_start;

				if (idx < 0 || idx >= ref_seq.length())
					return;

				char ref = ref_seq[idx] | 32;
				base |= 32;

				if (base != ref)
				{
					variants.push_back({
						genome_pos,
						base,
						(false) ? qual : 41
					});
				}
			});
	}

	const QVector<VariantInfo>& getVariants() const
	{
		return variants;
	}

	bool operator==(const BamAlignmentWrapper& other) const
	{
		return id == other.id;
	}

	QString makeId(const BamAlignment& al)
	{
		return QString("%1_%2_%3_%4")
			.arg(al.name())
			.arg(al.start())
			.arg(al.isReverseStrand())
			.arg(al.cigarDataAsString());
	}
};

inline size_t qHash(const BamAlignmentWrapper& key, size_t seed = 0)
{
	return qHash(key.id, seed);
}

/*this is used for connected BamAlignemnt and BamCoverage tracks*/
class BamTrackData : public QObject, public TrackData
{
	Q_OBJECT
public:
	BamTrackData(QString file_path, QString name)
		: TrackData(file_path, name)
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

#endif // TRACKDATA_H
