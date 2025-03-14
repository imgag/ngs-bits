#include "BigWigReader.h"
#include "Exceptions.h"
#include <QDataStream>
#include <iostream>
#include <zlib.h>
#include <Log.h>
#include <QRegularExpression>
#include "Chromosome.h"


BigWigReader::BigWigReader(const QString& bigWigFilepath)
	: file_path_(bigWigFilepath)
	, default_value_(0)
	, default_value_is_set_(false)
    , fp_(new VersatileFile(bigWigFilepath))
{
	//init
    if (!fp_->open(QFile::ReadOnly))
	{
		THROW(FileAccessException, "Could not open file for reading: '" + bigWigFilepath + "'!");
	}
	buffer_.clear();

	parseInfo();
	parseChrom();
	parseIndexTree();
}

BigWigReader::~BigWigReader()
{
}

void BigWigReader::setDefaultValue(double default_value)
{
	if (summary_.min_val <= default_value_ && default_value_ <= summary_.max_val)
	{
		// warn user that the default value cannot be distinguished from a real value
		Log::warn(QString("The default value of the BigWigReader is within min and maxValue of the file! It can't be distinguished from a real value!\n %1 - min: %2 max: %3 default: %4")
				  .arg(file_path_, QString::number(summary_.min_val), QString::number(summary_.max_val), QString::number(default_value)));
	}

	default_value_ = default_value;
	default_value_is_set_ = true;
}


BigWigReader::Header BigWigReader::header() const
{
	return header_;
}

BigWigReader::Summary BigWigReader::summary() const
{
	return summary_;
}

double BigWigReader::defaultValue() const
{
	return default_value_;
}

bool BigWigReader::defaulValueIsSet() const
{
	return default_value_is_set_;
}

bool BigWigReader::isLittleEndian() const
{
	return byte_order_ == QDataStream::LittleEndian;
}

bool BigWigReader::containsChromosome(const QByteArray& chr) const
{
	return chromosomes_.contains(chr);
}

float BigWigReader::readValue(const QByteArray& chr, int position, int offset)
{
	QVector<float> values = readValues(chr, position, position+1, offset);
	if (values.size() == 1)
	{
		return values[0];
	}
	else if (values.size() == 0)
	{
		return default_value_;
	}

	THROW(FileParseException, "Found multiple Overlapping Intervals for a single position? - chr " + chr + ": " + QString::number(position))

}

QVector<float> BigWigReader::readValues(const QByteArray& region, int offset)
{
	QList<QByteArray> parts1 = region.split(':');
	if (parts1.length() != 2) THROW(ArgumentException, "Given region is not formatted correctly: Expected 'chr:start-end'\n Given:" + QString(region));

	QList<QByteArray> parts2 = parts1[1].split('-');
	if (parts2.length() != 2) THROW(ArgumentException, "Given region is not formatted correctly: Expected 'chr:start-end'\n Given:" + QString(region));

	return readValues(parts1[0], parts2[0].toInt(), parts2[1].toInt(), offset);
}

QVector<float> BigWigReader::readValues(const QByteArray& chr, quint32 start, quint32 end, int offset)
{
	if (! default_value_is_set_)
	{
		THROW(ProgrammingException, "The default value has to be set before the readValue functions can be used!")
	}

    QList<OverlappingInterval> intervals = getOverlappingIntervals(chr, start, end, offset);

	// split long intervals into single values:
	QVector<float> result = QVector<float>(end-start, default_value_);
	foreach (const OverlappingInterval& interval, intervals)
	{
		if (interval.end-interval.start == 1) // covers a single position if it is overlapping it has to be in vector
		{
			result[interval.start-(start+offset)] = interval.value;
		}
		else
		{
			int idx;
			for(quint32 i=interval.start; i<interval.end; i++)
			{
				idx = i-start;
				if(idx>= 0 && idx < (int) (end-start))
				{
					result[idx] = interval.value;
				}
			}
		}
	}

	return result;
}

QList<BigWigReader::OverlappingInterval> BigWigReader::getOverlappingIntervals(const QByteArray& chr, quint32 start, quint32 end, int offset)
{
    quint32 chr_id;
    if (containsChromosome(chr))
    {
		chr_id = chromosomes_[chr].chrom_id;
    }
    else
    {
		THROW(ArgumentException, "Couldn't find given chromosome in file: " + chr)
    }
    QList<OverlappingInterval> intervals;

    // try to find it in the buffer:
    if ( ! buffer_.contains(chr_id, start, end))
    {
        QList<OverlappingBlock> blocks = getOverlappingBlocks(chr_id, start+offset, end+offset);

        if (blocks.length() == 0)
        {
            return QList<OverlappingInterval>();
        }

        intervals = extractOverlappingIntervals(blocks, chr_id, start+offset, end+offset);
    }
    else
    {
        intervals = buffer_.get(chr_id, start+offset, end+offset);
    }

    return intervals;
}

QList<BigWigReader::OverlappingBlock> BigWigReader::getOverlappingBlocks(quint32 chr_id, quint32 start, quint32 end)
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
    std::sort(result.begin(), result.end(), OverlappingBlock::lessThan);
	return result;
}

QList<BigWigReader::OverlappingBlock> BigWigReader::overlapsTwig(const IndexRTreeNode& node, quint32 chr_id, quint32 start, quint32 end)
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
			continue;
		}
		// merge lists
		blocks.append(node_blocks);
	}
	return blocks;
}

QList<BigWigReader::OverlappingBlock> BigWigReader::overlapsLeaf(const IndexRTreeNode& node, quint32 chr_id, quint32 start, quint32 end)
{
	QList<OverlappingBlock> blocks;
	for (quint16 i=0; i<node.count; i++)
	{
		if (chr_id < node.chr_idx_start[i])
		{
			break;
		}
		if (chr_id > node.chr_idx_end[i])
		{
			continue;
		}

		if (node.chr_idx_start[i] != node.chr_idx_end[i]) // child spans contigs
		{
			if (chr_id == node.chr_idx_start[i])
			{
				if (node.base_start[i] >= end)
				{
					continue;
				}
			}
			else if(chr_id == node.chr_idx_end[i])
			{
				if (node.base_end[i] <= start)
				{
					continue;
				}
			}
		}
		else
		{
			if ((node.base_start[i] >= end) || node.base_end[i] <= start)
			{
				continue;
			}
		}
		// overlap found:

		OverlappingBlock newBlock;
		newBlock.offset = node.data_offset[i];
		newBlock.size = node.size[i];
        newBlock.start = node.base_start[i];
        newBlock.end = node.base_end[i];

		blocks.append(newBlock);
	}

	return blocks;
}

QList<BigWigReader::OverlappingInterval> BigWigReader::extractOverlappingIntervals(const QList<OverlappingBlock>& blocks, quint32 chr_id, quint32 start, quint32 end)
{
	QList<OverlappingInterval> result;
	buffer_.clear();
	buffer_.chr_id = chr_id;

	// Test if buffer would be too big -> split decompression into multiple steps but probably never needed
	quint32 decompress_buffer_size = header_.uncompress_buf_size;
	char out[decompress_buffer_size];
	QByteArray decompressed_block;

	foreach (const OverlappingBlock &b, blocks)
	{
		decompressed_block = "";
		if (decompress_buffer_size > 0) // if data is compressed -> decompress it
		{
            fp_->seek(b.offset);
            QByteArray compressed_block = fp_->read(b.size);

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
				THROW(FileParseException, "Couldn't decompress a Data block. Too little buffer space?")
			}
			inflateEnd(&infstream);

			decompressed_block.append(out, infstream.avail_out);
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

		quint32 interval_start, interval_end;
		float interval_value;
		
		if (data_header.type == 3)
		{
			interval_start = data_header.start - data_header.step; // minus step as it is added below before evaluating.
		}

		// parse items
		for (quint16 i=0; i<data_header.num_items; i++)
		{
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
			OverlappingInterval interval(interval_start, interval_end, interval_value);
			buffer_.append(interval);
			if (start >= interval_end ||  end <= interval_start) continue; // doesn't overlap

			result.append(interval);
		}
	}
	return result;
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
        QString k =bytes.mid(0, key_size); // shorter than max length keys end with zero bytes that don't get trimmed normally

        trimNonNumericFromEnd(k);
        chr.key = Chromosome(k.toUtf8().trimmed()).strNormalized(true);

		QDataStream ds(bytes.mid(key_size, bytes.length()));
		ds.setByteOrder(byte_order_);

		ds >> chr.chrom_id;
		ds >> chr.chrom_size;

		chromosomes_.insert(chr.key, chr);
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

BigWigReader::IndexRTreeNode BigWigReader::parseIndexTreeNode(quint64 offset)
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

void BigWigReader::printHeader() const
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

void BigWigReader::printSummary() const
{
	std::cout << "Summary:\n";
	std::cout << "Bases covered:\t" << summary_.bases_covered << "\n";
	std::cout << "min Value:\t"  << summary_.min_val << "\n";
	std::cout << "max Value:\t" << summary_.max_val<< "\n";
	std::cout << "sum Data:\t" << summary_.sum_data<< "\n";
	std::cout << "sum Squares:\t" << summary_.sum_squares << "\n" << std::endl;
}

void BigWigReader::printZoomLevels() const
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

void BigWigReader::printChromHeader() const
{
	std::cout << "Chrom Header:\n";
	std::cout << "magic: \t0x" << chr_header.magic << "\n";
	std::cout << "children per block: \t" << chr_header.children_per_block << "\n";
	std::cout << "key size: \t" << chr_header.key_size << "\n";
	std::cout << "val size: \t" << chr_header.val_size << "\n";
	std::cout << "item count:\t" << chr_header.item_count << "\n" << std::endl;
}

void BigWigReader::printChromosomes() const
{
	std::cout << "Chromosomes: #" << chromosomes_.keys().length() <<"\n";
	foreach (const QByteArray &chr_key, chromosomes_.keys())
	{
		std::cout << "chr: " <<  chromosomes_[chr_key].key.toStdString() << "\tid: " << QByteArray::number(chromosomes_[chr_key].chrom_id).toStdString() << "\tsize: " << QByteArray::number(chromosomes_[chr_key].chrom_size).toStdString() << "\tkey: " <<"'" << chr_key.toStdString() << "'" << "\n";
	}
	std::cout << std::endl;
}

void BigWigReader::printIndexTree() const
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

void BigWigReader::printIndexTreeNode(const BigWigReader::IndexRTreeNode& node, int level) const
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

void BigWigReader::trimNonNumericFromEnd(QString& data)
{
    QRegularExpression regex("[^0-9]+$");
    QRegularExpressionMatch match = regex.match(data);

    if (match.hasMatch()) {
        int start = match.capturedStart();
        int length = match.capturedLength();
        data.remove(start, length);
    }
}
