#include "BigWigReader.h"
#include "Helper.h"
//#include "bigWig.h"
#include "Exceptions.h"
#include <QDebug>
#include <QDataStream>
#include <iostream>


BigWigReader::BigWigReader(const QString& bigWigFilepath)
{
	fp_ = Helper::openVersatileFileForReading(bigWigFilepath, false);
	fp_->open(QIODevice::ReadOnly);
	parseInfo();
	parseChrom();
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
	} else if (magic ==  0x26FC8F88) {
		byte_order_ = QDataStream::LittleEndian;
	} else {
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
	} else {
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

	for (int i=0; i<num_items; i++)
	{
		QByteArray bytes = fp_->read(key_size + 8);
		QByteArray key = bytes.mid(0, key_size);
		QDataStream ds(bytes.mid(key_size, bytes.length()));
		ds.setByteOrder(byte_order_);

		quint64 offset;
		ds >> offset;

		fp_->seek(offset);
		parseChromBlock(key_size);
	}
}

void BigWigReader::printHeader()
{
	std::cout << "Header: \n";
	std::cout << "Version:\t" << header_.version << "\n";
	std::cout << "Zoom levels:\t" << header_.zoom_levels << "\n";
	std::cout << "ctOffset:\t0x" << std::hex << header_.chromosome_tree_offset <<" \n";
	std::cout << "dataOffset:\t0x" << std::hex << header_.full_data_offset << "\n";
	std::cout << "indexOffset:\t0x" << std::hex << header_.full_index_offset << "\n";
	std::cout << "autoSqlOffset:\t0x" << std::hex << header_.auto_sql_offset << "\n";
	std::cout << "SummaryOffset:\t0x" << std::hex << header_.total_summary_offset << "\n";
	std::cout << "BufSize:\t" << QString::number(header_.uncompress_buf_size).toStdString() << "\n" << std::endl;
}

void BigWigReader::printSummary()
{
	std::cout << "Summary:\n";
	std::cout << "Bases covered:\t" << QString::number(summary_.bases_covered).toStdString() << "\n";
	std::cout << "min Value:\t" << summary_.min_val << "\n";
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
		std::cout << "reduction level:  \t" << QString::number(zoom_levels_[i].reduction_level).toStdString() << "\n";
		std::cout << "data offset: \t0x" << zoom_levels_[i].data_offset << "\n";
		std::cout << "index offset:\t0x" << zoom_levels_[i].index_offset << "\n" << std::endl;

	}
}

void BigWigReader::printChromHeader()
{
	std::cout << "Chrom Header:\n";
	std::cout << "magic: \t0x" << chr_header.magic << "\n";
	std::cout << "children per block: \t" << QString::number(chr_header.children_per_block).toStdString() << "\n";
	std::cout << "key size: \t" << QString::number(chr_header.key_size).toStdString() << "\n";
	std::cout << "val size: \t" << QString::number(chr_header.val_size).toStdString() << "\n";
	std::cout << "item count:\t" << QString::number(chr_header.item_count).toStdString() << "\n" << std::endl;
}

void BigWigReader::printChromosomes()
{
	std::cout << "Chromosomes:\n";
	foreach (ChromosomeItem chr, chr_list)
	{
		std::cout << "chr:\t" <<  chr.key.toStdString() << "\tid:\t" << chr.chrom_id << "\tsize:\t" << chr.chrom_size << "\n";
	}
	std::cout << std::endl;
}

float BigWigReader::readValue(QString chr, int position, int offset)
{
	return 1.0f;
}

BigWigReader::~BigWigReader()
{
	fp_->close();
}

