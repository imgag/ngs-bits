#ifndef BIGWIGREADER_H
#define BIGWIGREADER_H

#include "cppNGS_global.h"
#include "VersatileFile.h"
#include <QString>
#include <QVector>
#include <QDataStream>


// Helper structs to save the parsed data from the big wig file:
// Header holds the high level data about the file.
struct BigWigHeader
{
	quint32 magic_number; // 0x888FFC26 for bigWig (byte swaped?)
	quint16 version;
	quint16 zoom_levels;
	quint64 chromosome_tree_offset;
	quint64 full_data_offset;
	quint64 full_index_offset;
	quint16 field_count;
	quint16 defined_field_count;
	quint64 auto_sql_offset;
	quint64 total_summary_offset;
	quint32 uncompress_buf_size;
	quint64 reserved;
};

// General summary provided by the file
struct Summary
{
	quint64 bases_covered;
	double  min_val;
	double max_val;
	double sum_data;
	double sum_squares;
};

/*Internaly used structs*/
// Currently only zoom level headers are parsed but the zoomlevel data isn't used and isn't supported.
struct ZoomLevel
{
	quint32 reduction_level;
	quint32 reserved;
	quint64 data_offset;
	quint64 index_offset;
};

struct ChromosomeHeader
{
	quint32 magic;
	quint32 children_per_block;
	quint32 key_size; // significant bytes in key (min prefix to distinguish chromosomes)
	quint32 val_size;

	quint64 item_count; // num of chromosomes/contigs
	quint64 reserved;
};

struct ChromosomeItem
{
	QByteArray key;
	quint32 chrom_id;
	quint32 chrom_size;
};

struct IndexRTreeNode
{
	quint8 isLeaf;
	quint16 count;
	QVector<quint32> chr_idx_start;
	QVector<quint32> chr_idx_end;
	QVector<quint32> base_start;
	QVector<quint32> base_end;
	QVector<quint64> data_offset; // offset to the children for non leafs for leafs the offset to the data.
	QVector<quint64> size; // leaves only: size of data
	QVector<IndexRTreeNode> children; // twigs only
};

struct IndexRTree
{
	quint32 block_size;
	quint64 num_items;
	quint32 chr_idx_start;
	quint32 chr_idx_end;
	quint32 base_start;
	quint32 base_end;
	quint64 end_file_offset;
	quint32 num_items_per_leaf;
	quint64 root_offset;
	IndexRTreeNode root;
};

struct DataHeader
{
	quint32 chrom_id;
	quint32 start;
	quint32 end;
	quint32 step;
	quint32 span;
	quint8 type;
	quint16 num_items;

};

struct OverlappingBlock
{
	quint64 offset;
	quint64 size;
    quint32 start;
    quint32 end;

    static bool lessThan(const OverlappingBlock& b1, const OverlappingBlock& b2)
    {
        return b1.start < b2.start;
    }
};

struct OverlappingInterval
{
	OverlappingInterval(quint32 start, quint32 end, float value):
		start(start)
	  , end(end)
	  , value(value)
	{
	}
	quint32 start;
	quint32 end;
	float value;
};

// Buffer for the intervals that were recently read from file to reduce decompression calls.
struct IntervalBuffer
{
	quint32 chr_id;
	quint32 start;
	quint32 end;
	QList<OverlappingInterval> intervals;

	// returns true if the given region is covered by the intervals in the buffer
	bool contains(quint32 pos_chr_id, quint32 pos_start, quint32 pos_end)
	{
		if (intervals.size() == 0) return false;
		if (pos_chr_id != chr_id) return false;
		if (pos_start >= end || pos_end < start) return false;

		return true;
	}

	/**
	 * @brief add the interval to the buffer and adjusts the end of the buffer accordingly
	 * @note expects the appended intervals to be in increasing order and without gaps
	 */
	void append(OverlappingInterval interval)
	{
		if (intervals.length() == 0)
		{
			start = interval.start;
			end = interval.end;
		}
		else
		{
			if (interval.start < end )
			{
				THROW(ProgrammingException, "Intervals need to be inserted in increasing order")
			}
			end = interval.end;
		}

		intervals.append(interval);
	}

	// returns the intervals in the buffer that overlap with the given region
	QList<OverlappingInterval> get(quint32 pos_chr_id, quint32 pos_start, quint32 pos_end)
	{
		QList<OverlappingInterval> res = QList<OverlappingInterval>();
		if (chr_id != pos_chr_id)
		{
			return res;
		}

		foreach (const OverlappingInterval i, intervals)
		{
			if (pos_start >= i.end ||  pos_end <= i.start) continue;
			res.append(i);
		}
		return res;
	}

	//clears the data within the buffer
	void clear()
	{
		chr_id = start = end = 0;
		intervals.clear();
	}
};

// Reader for BigWig files:
class CPPNGSSHARED_EXPORT BigWigReader
{

public:
	// Constructor
	BigWigReader(const QString& bigWigFilepath, float default_value=-50);
	// Destructor
	~BigWigReader();

	// Sets the default_value to a new value default.
	void setDefault(float new_default);
	// Returns weather the current default value is outside of the value range of the bw file
	bool defaultValid();
	// Getters:
	float defaultValue();
	BigWigHeader header();
	Summary summary();

	bool isLittleEndian();

	// reports if a given chromosome name is contained in the file.
	bool containsChromosome(const QByteArray& chr);

	/**
	 * @brief Reads the bigWig value for the given position of the genome.
	 * @param offset Offset for regions as libBigWig uses zero-based genome indexing -> 0 - length-1
	 * @return The value specified in the file or when the given position is not covered in the file returns the default_value.
	 */
	float readValue(const QByteArray& chr, int position, int offset=-1);

	/**
	 * @brief Reads the bigWig values for the given region of the genome.
	 * @param offset Offset for regions as libBigWig uses zero-based genome indexing -> 0 - length-1
	 * @return A QVector containing a value for each position requested: values specified in the file or when the given position is not covered in the file the default_value.
	 */
	QVector<float> readValues(const QByteArray& chr, quint32 start, quint32 end, int offset=-1);

	// Convenience function to call readValues with an unparsed region of type (chrNAME:start-end)
	QVector<float> readValues(const QByteArray& region, int offset=-1);

	// Function that reproduces the phylop annotation as VEP would write it.
	float reproduceVepPhylopAnnotation(const QByteArray& chr, int start, int end, const QString& ref, const QString& alt);

	// Print functions for convenience while testing
	void printHeader();
	void printSummary();
	void printZoomLevels();
	void printChromHeader();
	void printChromosomes();
	void printIndexTree();
	void printIndexTreeNode(const IndexRTreeNode& node, int level);

private:
	// searches the indextree for blocks containing requested data
	QList<OverlappingBlock> getOverlappingBlocks(quint32 chr_id, quint32 start, quint32 end);
	QList<OverlappingBlock> overlapsTwig(const IndexRTreeNode& node, quint32 chr_id, quint32 start, quint32 end);
	QList<OverlappingBlock> overlapsLeaf(const IndexRTreeNode& node, quint32 chr_id, quint32 start, quint32 end);

	// if needed decompress blocks and return the Intervals that overlap the requested region
	QList<OverlappingInterval> extractOverlappingIntervals(const QList<OverlappingBlock>& blocks, quint32 chr_id, quint32 start, quint32 end);

	// Parse functions parse the corresponding part of the binary file (need to be called in the right order to set necessary member variables)
	void parseInfo();
	void parseChrom();
	void parseChromBlock(quint32 key_size);
	void parseChromLeaf(quint16 num_items, quint32 key_size);
	void parseChromNonLeaf(quint16 num_items, quint32 key_size);
	void parseIndexTree();
	IndexRTreeNode parseIndexTreeNode(quint64 offset);


	const QString file_path_;
	float default_value_;
	BigWigHeader header_;
	Summary summary_;
	QList<ZoomLevel> zoom_levels_;
	ChromosomeHeader chr_header;
	IndexRTree index_tree_;
	QHash<QString, ChromosomeItem> chromosomes;
	QSharedPointer<VersatileFile> fp_;
	QDataStream::ByteOrder byte_order_;
	IntervalBuffer buffer_;

};

#endif // BIGWIGREADER_H
