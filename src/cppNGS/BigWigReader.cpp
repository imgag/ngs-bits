#include "BigWigReader.h"
#include "Helper.h"
//#include "bigWig.h"
#include "Exceptions.h"
#include <QDebug>
#include <QDataStream>
#include <iostream>
#include <zlib.h>
#include <math.h>


BigWigReader::BigWigReader(const QString& bigWigFilepath)
{
	file_path_ = bigWigFilepath;

	fp_ = Helper::openVersatileFileForReading(bigWigFilepath, false);
	fp_->open(QIODevice::ReadOnly); // necessary?!? if not there shifts through bits
	parseInfo();
	parseChrom();
	parseIndexTree();
}

float BigWigReader::readValue(const QByteArray& chr, int position, int offset)
{
	QList<OverlappingInterval> intervals = readValues(chr, position+offset, position+1, offset);

	std::cout << "Length intervals:" << intervals.length();
	for (int i=0; i<intervals.length(); i++)
	{
		std::cout << "interval " << i << "\t" << intervals[i].start << "-" << intervals[i].end << ": " << intervals[i].value << std::endl;
	}

	if (intervals.length() == 1)
	{
		return intervals[0].value;
	}
	else if (intervals.length() == 0)
	{
		return -5000; //TODO!!!!replace with sensible default value
	}

	THROW(FileParseException, "Found multiple Overlapping Intervals for a single position? - " + chr + ":" + QString::number(position))

}

QList<OverlappingInterval> BigWigReader::readValues(const QByteArray& region, int offset)
{
	QList<QByteArray> parts1 = region.split(':');
	if (parts1.length() != 2) THROW(ArgumentException, "Given region is not formatted correctly: Expected 'chr:start-end'\n Given:" + QString(region));

	QList<QByteArray> parts2 = parts1[1].split('-');
	if (parts2.length() != 2) THROW(ArgumentException, "Given region is not formatted correctly: Expected 'chr:start-end'\n Given:" + QString(region));

	std::cout << "parsed region:" << parts1[0].toStdString() << ":" << parts2[0].toInt() << "-" << parts2[1].toInt() << std::endl;

	return readValues(parts1[0], parts2[0].toInt(), parts2[1].toInt(), offset);
}

QList<OverlappingInterval> BigWigReader::readValues(const QByteArray& chr, quint32 start, quint32 end, int offset)
{
	quint32 chr_id = getChrId(chr);

	QList<OverlappingInterval> intervals;
	if (chr_id == (quint32) -1)
	{
		std::cout << "not found:"  << chr.toStdString() << "\tid:"<< chr_id << std::endl;
		THROW(ArgumentException, "Couldn't find given chromosome in file.")
		return intervals;
	}

	QList<OverlappingBlock> blocks = getOverlappingBlocks(chr_id, start+offset, end+offset);

	if (blocks.length() == 0)
	{
		std::cout << "Didn't find any overlapping blocks" << std::endl;
		return intervals;
	}

	intervals = extractOverlappingIntervals(blocks, chr_id, start+offset, end+offset);
	return intervals;
}

void BigWigReader::parseInfo()
{
	//Header
	QByteArray header_bytes = fp_->read(64);
	QDataStream header_stream(header_bytes);
	quint32 magic;
	header_stream >> magic;

	if (magic == 0x888FFC26)
	{
		byte_order_ = QDataStream::BigEndian;
	}
	else if (magic ==  0x26FC8F88)
	{
		byte_order_ = QDataStream::LittleEndian;
	}
	else
	{
		THROW(FileParseException, "Magic number of file doesn't belong to BigWig.")
	}
	header_stream.setByteOrder(byte_order_);

	header_.magic_number = magic;
	header_stream >> header_.version;
	header_stream >> header_.zoom_levels;
	header_stream >> header_.chromosome_tree_offset;
	header_stream >> header_.full_data_offset;
	header_stream >> header_.full_index_offset;
	header_stream >> header_.field_count;
	header_stream >> header_.defined_field_count;
	header_stream >> header_.auto_sql_offset; // not used in BigWig files
	header_stream >> header_.total_summary_offset;
	header_stream >> header_.uncompress_buf_size;
	header_stream >> header_.reserved;

	if ( ! header_stream.atEnd())
	{
		THROW(FileParseException, "Datastream not at the end after parsing Header.")
	}

	//ZoomHeaders:
	zoom_levels_.clear();
	for (auto i=0; i<header_.zoom_levels; i++)
	{
		QByteArray zoom_header_bytes = fp_->read(24);
		QDataStream zoom_level_stream(zoom_header_bytes);
		zoom_level_stream.setByteOrder(byte_order_);

		ZoomLevel z;
		zoom_level_stream >> z.reduction_level;
		zoom_level_stream >> z.reserved;
		zoom_level_stream >> z.data_offset;
		zoom_level_stream >> z.index_offset;

		zoom_levels_.append(z);
	}

	if ( ! header_stream.atEnd())
	{
		THROW(FileParseException, "Datastream not at the end after parsing zoom headers.")
	}

	//Summary:
	fp_->seek(header_.total_summary_offset);
	QByteArray summary_bytes = fp_->read(40);
	QDataStream summary_stream(summary_bytes);
	summary_stream.setByteOrder(byte_order_);

	summary_stream >> summary_.bases_covered;
	summary_stream >> summary_.min_val;
	summary_stream >> summary_.max_val;
	summary_stream >> summary_.sum_data;
	summary_stream >> summary_.sum_squares;

	if ( ! header_stream.atEnd())
	{
		THROW(FileParseException, "Datastream not at the end after parsing summary.")
	}
}

void BigWigReader::parseChrom()
{
	fp_->seek(header_.chromosome_tree_offset);
	QByteArray chr_tree_header_bytes = fp_->read(32);
	QDataStream ds(chr_tree_header_bytes);
	ds.setByteOrder(byte_order_);

	//header
	ds >> chr_header.magic;
	ds >> chr_header.children_per_block;
	ds >> chr_header.key_size;
	ds >> chr_header.val_size;
	ds >> chr_header.item_count;
	ds >> chr_header.reserved;

	parseChromBlock(chr_header.key_size);
}

void BigWigReader::parseChromBlock(quint32 key_size)
{
	QByteArray block_bytes = fp_->read(4);
	QDataStream ds(block_bytes);
	ds.setByteOrder(byte_order_);

	quint8 is_leaf, padding;
	quint16 num_items;

	ds >> is_leaf;
	ds >> padding;
	ds >> num_items;

	if (is_leaf == 1)
	{
		parseChromLeaf(num_items, key_size);
	}
	else
	{
		parseChromNonLeaf(num_items, key_size);
	}
}

void BigWigReader::parseChromLeaf(quint16 num_items, quint32 key_size)
{
	// key, key_size, bytes
	// chromId, 4 , uint
	// chromSize, 4, uint

	for (int i=0; i<num_items; i++)
	{
		ChromosomeItem chr;
		QByteArray bytes = fp_->read(key_size + 8);
		chr.key = bytes.mid(0, key_size);
		chr.key = chr.key.trimmed();
		QDataStream ds(bytes.mid(key_size, bytes.length()));
		ds.setByteOrder(byte_order_);

		ds >> chr.chrom_id;
		ds >> chr.chrom_size;

		chr_list.append(chr);
	}
}

void BigWigReader::parseChromNonLeaf(quint16 num_items, quint32 key_size)
{
	// key, key_size, bytes
	// childOffset, 8, uint

	quint64 currentOffset = fp_->pos()+key_size;
	for (int i=0; i<num_items; i++)
	{
		fp_->seek(currentOffset);
		QByteArray bytes = fp_->read(8);
		//QByteArray key = bytes.mid(0, key_size);
		QDataStream ds(bytes);
		ds.setByteOrder(byte_order_);

		quint64 offset;
		ds >> offset;

		fp_->seek(offset);
		parseChromBlock(key_size);
		currentOffset += key_size + 8;
	}
}

void BigWigReader::parseIndexTree()
{
	fp_->seek(header_.full_index_offset);
	QByteArray index_header_bytes = fp_->read(48);
	QDataStream index_header_stream(index_header_bytes);
	index_header_stream.setByteOrder(byte_order_);

	quint32 magic, padding;
	index_header_stream >> magic;

	if (magic != 0x2468ACE0)
	{
		THROW(FileParseException, "Magic number of index not what expected!")
	}

	index_header_stream >> index_tree_.block_size;
	index_header_stream >> index_tree_.num_items;
	index_header_stream >> index_tree_.chr_idx_start;
	index_header_stream >> index_tree_.base_start;
	index_header_stream >> index_tree_.chr_idx_end;
	index_header_stream >> index_tree_.base_end;
	index_header_stream >> index_tree_.end_file_offset;
	index_header_stream >> index_tree_.num_items_per_leaf;
	// four bytes padding
	index_header_stream >> padding;
	index_tree_.root_offset = header_.full_index_offset + 48; // current pos

	index_tree_.root = parseIndexTreeNode(index_tree_.root_offset);
}

IndexRTreeNode BigWigReader::parseIndexTreeNode(quint64 offset)
{
	fp_->seek(offset);
	QByteArray node_header_bytes = fp_->read(4);
	QDataStream node_header_stream(node_header_bytes);
	node_header_stream.setByteOrder(byte_order_);

	quint8 padding;
	IndexRTreeNode node;
	node_header_stream >> node.isLeaf;
	node_header_stream >> padding;
	node_header_stream >> node.count;

	node.chr_idx_start = QVector<quint32>(node.count);
	node.base_start = QVector<quint32>(node.count);
	node.chr_idx_end = QVector<quint32>(node.count);
	node.base_end = QVector<quint32>(node.count);
	node.data_offset = QVector<quint64>(node.count);


	if (node.isLeaf)
	{
		node.size = QVector<quint64>(node.count);
		QByteArray leaf_items_bytes = fp_->read(node.count * 48);
		QDataStream leaf_items_stream(leaf_items_bytes);
		leaf_items_stream.setByteOrder(byte_order_);

		for (int i=0; i<node.count; i++)
		{
			quint32 cis, bs, cie, be;
			quint64 data_offset, data_size;
			leaf_items_stream >> cis >> bs >> cie >> be >> data_offset >> data_size;

			node.chr_idx_start[i] = cis;
			node.base_start[i] = bs;
			node.chr_idx_end[i] = cie;
			node.base_end[i] = be;
			node.data_offset[i] = data_offset;
			node.size[i] = data_size;
		}
	}
	else
	{
		node.children = QVector<IndexRTreeNode>(node.count);
		QByteArray twig_items_bytes = fp_->read(node.count * 40);
		QDataStream twig_items_stream(twig_items_bytes);
		twig_items_stream.setByteOrder(byte_order_);

		for (int i=0; i<node.count; i++)
		{
			quint32 cis, bs, cie, be;
			quint64 data_offset;
			twig_items_stream >> cis >> bs >> cie >> be >> data_offset;

			node.chr_idx_start[i] = cis;
			node.base_start[i] = bs;
			node.chr_idx_end[i] = cie;
			node.base_end[i] = be;
			node.data_offset[i] = data_offset;
			node.children[i] = parseIndexTreeNode(data_offset);
		}
	}
	return node;
}

QList<OverlappingBlock> BigWigReader::getOverlappingBlocks(quint32 chr_id, quint32 start, quint32 end)
{
	QList<OverlappingBlock> result;

	if (chr_id == (quint32) -1) return result; // Throw error for non existent contig?

	if (index_tree_.root.isLeaf)
	{
		result = overlapsLeaf(index_tree_.root, chr_id, start, end);
	}
	else
	{
		result = overlapsTwig(index_tree_.root, chr_id, start, end);
	}
	std::cout << std::endl;
	return result;
}

QList<OverlappingBlock> BigWigReader::overlapsTwig(const IndexRTreeNode& node, quint32 chr_id, quint32 start, quint32 end)
{
	QList<OverlappingBlock> blocks;
	for (quint16 i=0; i<node.count; i++)
	{
		if (chr_id < node.chr_idx_start[i]) break;
		if (chr_id > node.chr_idx_end[i]) continue;

		if (node.chr_idx_start[i] != node.chr_idx_end[i]) // child spans contigs
		{
			if (chr_id == node.chr_idx_start[i])
			{
				if (node.base_start[i] >= end) continue;
			}
			else if(chr_id == node.chr_idx_end[i])
			{
				if (node.base_end[i] <= start) continue;
			}
		}
		// a block that overlaps:
		QList<OverlappingBlock> node_blocks;
		if (node.children[i].isLeaf)
		{
			node_blocks = overlapsLeaf(node.children[i], chr_id, start, end);
		}
		else
		{
			node_blocks = overlapsTwig(node.children[i], chr_id, start, end);
		}

		if (node_blocks.length() == 0)
		{
			continue; // TODO problematic?
		}
		// merge lists
		blocks.append(node_blocks);
	}
	return blocks;
}



QList<OverlappingBlock> BigWigReader::overlapsLeaf(const IndexRTreeNode& node, quint32 chr_id, quint32 start, quint32 end)
{
	QList<OverlappingBlock> blocks;
	std::cout << "\nleaf called. Node count:" << node.count << "\n";
	for (quint16 i=0; i<node.count; i++)
	{
		std::cout << "On item " << i << std::endl;
		if (chr_id < node.chr_idx_start[i])
		{
			std::cout << "chr id smaller than of leaf.\n";
			break;
		}
		if (chr_id > node.chr_idx_end[i])
		{
			std::cout << "chr id bigger than the last chr id of leaf.\n";
			continue;
		}

		if (node.chr_idx_start[i] != node.chr_idx_end[i]) // child spans contigs
		{
			if (chr_id == node.chr_idx_start[i])
			{
				if (node.base_start[i] >= end)
				{
					std::cout << "Leaf spans contigs and block bases end before the region.\n";
					continue;
				}
			}
			else if(chr_id == node.chr_idx_end[i])
			{
				if (node.base_end[i] <= start)
				{
					std::cout << "Leaf spans contigs and block bases start after the region\n";
					continue;
				}
			}
		}
		else
		{
			if ((node.base_start[i] >= end) || node.base_end[i] <= start)
			{

				std::cout << "Blocks has right contig but the wrong part of the bases.\n";
				std::cout << "Block: start:\t" << node.base_start[i] << "\tend:\t" << node.base_end[i] << "\n";
				std::cout << "Searched was start:\t" << start << "\tend:\t" << end << "\n";
				continue;
			}
		}
		// overlap found:

		OverlappingBlock newBlock;
		newBlock.offset = node.data_offset[i];
		newBlock.size = node.size[i];
		blocks.append(newBlock);
		std::cout << "APPENDED BLOCK!" << std::endl;

	}

	return blocks;
}

QList<OverlappingInterval> BigWigReader::extractOverlappingIntervals(const QList<OverlappingBlock>& blocks, quint32 chr_id, quint32 start, quint32 end)
{
	std::cout << "extracting intervals" << std::endl;
	QList<OverlappingInterval> result;

	// TODO Test if buffer would be too big -> split decompression into multiple steps
	quint32 decompress_buffer_size = header_.uncompress_buf_size;
	char out[decompress_buffer_size];
	QByteArray decompressed_block;

	foreach (const OverlappingBlock &b, blocks)
	{
		if (decompress_buffer_size > 0) // if data is compressed -> decompress it
		{
			std::cout << "starting decompression" << std::endl;
			fp_->seek(b.offset);
			QByteArray compressed_block = fp_->read(b.size);
			std::cout << "compressed block length: " << compressed_block.length() << std::endl;

			//set zlib vars
			z_stream infstream;
			infstream.zalloc = Z_NULL;
			infstream.zfree = Z_NULL;
			infstream.opaque = Z_NULL;
			// setup "compressed_block.data()" as the input and "out" as the uncompressed output
			infstream.avail_in = b.size; // size of input
			infstream.next_in = (Bytef *)compressed_block.data(); // input char array
			infstream.avail_out = decompress_buffer_size; // size of output
			infstream.next_out = (Bytef *)out; // output char array

			inflateInit(&infstream);
			int ret = inflate(&infstream, Z_FINISH);

			if (ret != Z_STREAM_END)
			{
				std::cout << "Inflation didn't work es planned - error int" << ret << std::endl;
				THROW(FileParseException, "Couldn't decompress a Data block. Too little buffer space?")
			}
			inflateEnd(&infstream);

			decompressed_block.append(out, infstream.avail_out);
			std::cout << "finished decompression. avail_out: " << infstream.avail_out << std::endl;
			std::cout << "finished decompression. length decompressed block: " << decompressed_block.length() << std::endl;
		}
		else // data is not compressed -> just read it
		{
			fp_->seek(b.offset);
			decompressed_block = fp_->read(b.size);
		}

		// parse decompressed block
		QDataStream ds(decompressed_block);
		ds.setByteOrder(byte_order_);
		ds.setFloatingPointPrecision(QDataStream::SinglePrecision);

		// parse header
		DataHeader data_header;
		ds >> data_header.chrom_id >> data_header.start >> data_header.end;
		ds >> data_header.step >> data_header.span >> data_header.type;
		quint8 padding;
		ds >> padding >> data_header.num_items;

		if (data_header.chrom_id != chr_id) continue;

		std::cout << "data header - chrom id:\t" << (uint) data_header.chrom_id << std::endl;
		std::cout << "data header - chrom start:\t" << (uint) data_header.start << std::endl;
		std::cout << "data header - chrom end:\t" << (uint) data_header.end << std::endl;
		std::cout << "data header - step:\t" << (uint) data_header.step << std::endl;
		std::cout << "data header - type:\t" << (uint) data_header.type << std::endl;
		std::cout << "data header - span:\t" << (uint) data_header.span << std::endl;
		std::cout << "data header - count:\t" << (uint) data_header.num_items << std::endl;

		quint32 interval_start, interval_end;
		float interval_value;

		if (data_header.type == 3) {
			interval_start = data_header.start - data_header.step; // minus step as it is added below before evaluating.
		}

		// parse items
		for (quint16 i=0; i<data_header.num_items; i++)
		{
			if(ds.atEnd())
			{
				std::cout << "datasteam shorter than expected!!!\n";
			}
			switch (data_header.type)
			{
				case 1:
					ds >> interval_start >> interval_end >> interval_value;
					break;
				case 2:
					ds >> interval_start >> interval_value;
					interval_end = interval_start + data_header.span;
					break;
				case 3:
					interval_start += data_header.step;
					interval_end = interval_start + data_header.span;
					ds >> interval_value;
					break;
				default:
					THROW(FileParseException, "Unknown type while parsing a data block.")
					break;
			}
			std::cout << interval_start << "-" << interval_end << "\tvalue:" << interval_value << "\n";
			if (start >= interval_end ||  end < interval_start) continue; // doesn't overlap

			OverlappingInterval interval; // TODO Constructor?
			interval.start = interval_start;
			interval.end = interval_end;
			interval.value = interval_value;
			result.append(interval);
		}
	}
	return result;
}

quint32 BigWigReader::getChrId(const QByteArray& chr)
{
	for (int i=0; i<chr_list.length(); i++)
	{
		if (QString(chr_list[i].key) == QString(chr)) return chr_list[i].chrom_id;
	}
	return -1; // maxValue of quint32
}

void BigWigReader::printHeader()
{
	std::cout << "Header: \n";
	std::cout << "Version:\t" << header_.version << "\n";
	std::cout << "Zoom levels:\t" << header_.zoom_levels << "\n";
	std::cout << "ctOffset:\t0x" << std::hex << header_.chromosome_tree_offset <<" \n";
	std::cout << "dataOffset:\t0x" << header_.full_data_offset << "\n";
	std::cout << "indexOffset:\t0x" << header_.full_index_offset << "\n";
	std::cout << "autoSqlOffset:\t0x" << header_.auto_sql_offset << "\n";
	std::cout << "SummaryOffset:\t0x" << header_.total_summary_offset << "\n";
	std::cout << "BufSize:\t" << std::dec << header_.uncompress_buf_size << "\n" << std::endl;

}

void BigWigReader::printSummary()
{
	std::cout << "Summary:\n";
	std::cout << "Bases covered:\t" << summary_.bases_covered << "\n";
	std::cout << "min Value:\t"  << summary_.min_val << "\n";
	std::cout << "max Value:\t" << summary_.max_val<< "\n";
	std::cout << "sum Data:\t" << summary_.sum_data<< "\n";
	std::cout << "sum Squares:\t" << summary_.sum_squares << "\n" << std::endl;
}

void BigWigReader::printZoomLevels()
{
	std::cout << "Zoom levles:\n";
	for (int i=0; i<zoom_levels_.length(); i++)
	{
		std::cout << "Zoom level number:\t" << i << "\n";
		std::cout << "reduction level:  \t" << zoom_levels_[i].reduction_level << "\n";
		std::cout << "data offset: \t0x" << std::hex << zoom_levels_[i].data_offset << "\n";
		std::cout << "index offset:\t0x" << std::hex << zoom_levels_[i].index_offset << "\n" << std::endl;
		std::cout << std::dec;

	}
}

void BigWigReader::printChromHeader()
{
	std::cout << "Chrom Header:\n";
	std::cout << "magic: \t0x" << chr_header.magic << "\n";
	std::cout << "children per block: \t" << chr_header.children_per_block << "\n";
	std::cout << "key size: \t" << chr_header.key_size << "\n";
	std::cout << "val size: \t" << chr_header.val_size << "\n";
	std::cout << "item count:\t" << chr_header.item_count << "\n" << std::endl;
}

void BigWigReader::printChromosomes()
{
	std::cout << "Chromosomes: #" << chr_list.length() <<"\n";
	foreach (const ChromosomeItem &chr, chr_list)
	{
		std::cout << "chr: " <<  chr.key.toStdString() << " id: " << QString::number(chr.chrom_id).toStdString() << " size: " << QString::number(chr.chrom_size).toStdString() << "\n";
	}
	std::cout << std::endl;
}

void BigWigReader::printIndexTree()
{
	std::cout << "IndexTree:\n";
	std::cout << "ChrIdxStart:\t" << QString::number(index_tree_.chr_idx_start).toStdString() << "\n";
	std::cout << "Base start: \t" << QString::number(index_tree_.base_start).toStdString() << "\n";
	std::cout << "ChrIdxEnd:  \t" << QString::number(index_tree_.chr_idx_end).toStdString() << "\n";
	std::cout << "Base end:   \t" << QString::number(index_tree_.base_end).toStdString() << "\n";
	std::cout << "Index Size: \t" << QString::number(index_tree_.end_file_offset).toStdString() << "\n";

	std::cout << "Nodes:\n";
	std::cout << "level\tchrIdxStart\tBaseStart\tChrIdxEnd\tBaseEnd\tchild\tsize\n";
	printIndexTreeNode(index_tree_.root, 0);
	std::cout << std::endl;
}

void BigWigReader::printIndexTreeNode(const IndexRTreeNode& node, int level)
{
	for (quint32 i=0;  i<node.count; i++)
	{
		if (node.isLeaf)
		{
			std::cout << level << "\t";
			std::cout << node.chr_idx_start[i] << "\t";
			std::cout << node.base_start[i] << "\t";
			std::cout << node.chr_idx_end[i] << "\t";
			std::cout << node.base_end[i] << "\t";
			std::cout << "0x" << std::hex << node.data_offset[i] << "\t";
			std::cout << std::dec << node.size[i] << "\t";
			std::cout << std::endl;
		}
		else
		{
			std::cout << level << "\t";
			std::cout << node.chr_idx_start[i] << "\t";
			std::cout << node.base_start[i] << "\t";
			std::cout << node.chr_idx_end[i] << "\t";
			std::cout << node.base_end[i] << "\t";
			std::cout << "0x" << std::hex << node.data_offset[i] << "\t";
			std::cout << std::dec << std::endl;
			//printIndexTreeNode(node.children[i], level+1);
		}
	}



}

BigWigReader::~BigWigReader()
{
	fp_->close();
}
