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

struct BamAlignmentWrapper
{
	struct VariantInfo
	{
		int genomic_pos;
		char base;
		int quality;
	};

	QString id; // for hashing
	BamAlignment alignment;
	QVector<VariantInfo> variants;

	BamAlignmentWrapper(BamAlignment aln)
		: id(makeId(aln)), alignment(aln)
	{
	}

	BamAlignmentWrapper(BamAlignment&& aln)
		: id(makeId(aln)), alignment(std::move(aln))
	{
	}

	void storeVariants(const Sequence& ref_seq, int ref_start)
	{
		variants.clear();
		// const auto& region = SharedData::region();
		for (int pos = alignment.start(); pos < alignment.end(); ++pos)
		{
			// int idx = pos - region.start();
			// if (idx < 0 || idx >= region.length()) continue;
			int idx = pos - ref_start;
			if (idx < 0 || idx >= ref_seq.length())
			{
				// qDebug() << "Bug: invalid idx encountered in store Variants: " << idx << Qt::endl;;
				continue;
			}

			char ref_base = ref_seq[idx] | 32;
			auto [base, qual] = alignment.extractBaseByCIGAR(pos);
			base |= 32;
			if (base != ref_base && base != '-')
			{
				variants << VariantInfo{pos, base, qual};
			}
		}
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
		updateRegion();
	}

	void reload();

signals:
	void onDataUpdate();

public slots:
	void updateRegion();

private:
	QSharedPointer<BamReader> bam_reader_;
	QVector<BamAlignmentWrapper> alignments_;
	QVector<BamAlignmentWrapper> dummy_alignments_;
	Sequence ref_seq_;
	int ref_start_;

	void updateData();

	bool is_loading_;
	bool return_empty_ = true;

	//loaded region
	BedLine loaded_region_;

	void fullLoad(const BedLine& region);

	void fetchRegion(const BedLine& region, int ref_start);

	void pruneAlignments(int keep_start, int keep_end);

	static constexpr int PADDING = 1000;
};

#endif // TRACKDATA_H
