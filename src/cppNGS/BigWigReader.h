#ifndef BIGWIGREADER_H
#define BIGWIGREADER_H

#include "cppNGS_global.h"
#include "VersatileFile.h"
#include <QString>
#include <QVector>
#include <QDataStream>

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

struct ZoomLevel
{
	quint32 reduction_level;
	quint32 reserved;
	quint64 data_offset;
	quint64 index_offset;
};

struct Summary
{
	quint64 bases_covered;
	double  min_val;
	double max_val;
	double sum_data;
	double sum_squares;
};

struct ChromosomeHeader
{
	quint32 magic;
	quint32 children_per_block;
	quint32 key_size; // significant bytes in key (min prefix to distinguish chromosomes)
	quint32 val_size; // currently 8

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
	bool isLeaf;
	quint16 num_children;
	QVector<qint32> chr_idx_start;
	QVector<qint32> chr_idx_end;
	QVector<qint32> base_start;
	QVector<qint32> base_end;
	QVector<qint64> data_offset; // offset to the children for non leafs for leafs the offset to the data.
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


class CPPNGSSHARED_EXPORT BigWigReader
{

public:
	BigWigReader(const QString& bigWigFilepath);
	~BigWigReader();

	// read the bigWig value for a position of the genome. Offset for regions as libBigWig uses zero-based genome indexing -> 0 - length-1
	float readValue(QString chr, int position, int offset=-1);
	void printHeader();
	void printSummary();
	void printZoomLevels();
	void printChromHeader();
	void printChromosomes();

	BigWigHeader header()
	{
		return header_;
	}

	Summary summary()
	{
		return summary_;
	}


	QString file_path;

private:
	void parseInfo();
	void parseChrom();
	void parseChromBlock(quint32 key_size);
	void parseChromLeaf(quint16 num_items, quint32 key_size);
	void parseChromNonLeaf(quint16 num_items, quint32 key_size);

	BigWigHeader header_;
	Summary summary_;
	QList<ZoomLevel> zoom_levels_;
	ChromosomeHeader chr_header;
	QHash<QByteArray, quint32> chromosomes;
	QList<ChromosomeItem> chr_list;
	QSharedPointer<VersatileFile> fp_;
	QDataStream::ByteOrder byte_order_;

};

#endif // BIGWIGREADER_H
