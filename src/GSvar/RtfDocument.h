#ifndef RTFDOCUMENT_H
#define RTFDOCUMENT_H

#include <QByteArray>
#include <QList>
#include <tuple>

typedef QByteArray RtfSourceCode;


/****************
 * RTF SETTINGS *
 ****************/
///struct describesformat of raw RTF text (e.g. if text format shall changes within a paragraph or you need text outside a paragraph)
struct RtfTextFormat
{
	RtfTextFormat()
	{
	}

	int font_size = 18;
	bool bold = false;
	bool italic = false;
	QByteArray horizontal_alignment = "l";
	//font number as specified in header
	int font_number = 0;
	//font color as specified in header
	int font_color = 0;
	int highlight_color = 0;
};


///struct describing layout of a text paragraph, all values in twips
struct RtfParagraphFormat : RtfTextFormat
{
	RtfParagraphFormat()
	{
	}

	int space_before = 0;
	int space_after = 0;

	int intent_block_left = 0;
	int intent_block_right = 0;
	int intent_first_line = 0;

};



class RtfDocument
{
public:
	///Constructor initializes class using suggestive values for the document
	RtfDocument();

	///Returns RTF header according settings of this class
	RtfSourceCode header();
	///Returns footer of RTF code
	RtfSourceCode footer();

	///Returns formatted text outside \par\pard construct
	static RtfSourceCode text(const QByteArray& content,const RtfTextFormat& format);

	///Return a paragraph formatted according format
	static RtfSourceCode paragraph(const QByteArray& content = "",const RtfParagraphFormat& format = RtfParagraphFormat(),bool in_cell = false);
	///Return a paragraph, each element of content will be its own line
	static RtfSourceCode paragraph(const QByteArrayList& content,const RtfParagraphFormat& format = RtfParagraphFormat(),bool in_cell = false);
	///Converts German special characters to unicode notation.
	static RtfSourceCode escapeUmlauts(const QByteArray& text);
	///Converts centimeters into RTF twip format
	static double cm2twip(double input_cm);

	///Adds RGB color to document color table
	void addColor(int red,int green,int blue)
	{
		colors_ << std::make_tuple(red,green,blue);
	}
	///Adds font into document header
	void addFont(const QByteArray& font)
	{
		fonts_ << font;
	}

	void setDefaultFont(const QByteArray& default_font)
	{
		fonts_[0] = default_font;
	}

	void setDefaultFontSize(int default_fs)
	{
		default_font_size_ = default_fs;
	}

	///sets page width dimension (in twips)
	void setWidth(int width)
	{
		width_ = width;
	}
	///sets page height dimension (in twips)
	void setHeight(int height)
	{
		height_ = height;
	}

	///sets page margins (in twips)
	void setMargins(int left, int top, int right, int bottom);

private:
	int width_;
	int height_;

	//page borders
	int margin_top_;
	int margin_bottom_;
	int margin_left_;
	int margin_right_;

	//fonts used in document, first font is default document font
	QByteArrayList fonts_;
	//colors to be defined in header
	QList<std::tuple<int,int,int>> colors_;

	int default_font_size_;
};


/**************************************
 * CLASSES FOR RTF CELL/ROW AND TABLE *
 **************************************/
class RtfTableCell
{
	friend class RtfTableRow;

public:
	void setContent(const QByteArray& content)
	{
		content_ = content;
	}

	const RtfParagraphFormat& format() const
	{
		return par_format_;
	}

	RtfParagraphFormat& format()
	{
		return par_format_;
	}


	void setWidth(int width)
	{
		width_ = width;
	}

	///set border of each cell
	void setBorder(int left,int top,int right,int bottom, const QByteArray& type =  "brdrs");

	///sets background color to number specified in document header
	void setBackgroundColor(int color)
	{
		background_color_ = color;
	}

	int width()
	{
		return width_;
	}

	///Method can be used to add special control words to the header of a cell. Do not include "\" into statement.
	void addHeaderControlWord(const QByteArray& statement)
	{
		control_words_ << statement;
	}

	const QList<QByteArray>& controlWords() const
	{
		return control_words_;
	}

private:
	///private constructor, only to be accessed by friend class
	RtfTableCell(const QByteArray &content, int width, const RtfParagraphFormat &text_format = RtfParagraphFormat());

	///returns RTF code formatting the cell, can only be called from friend classes
	QByteArray writeCell();

	QByteArray content_;
	RtfParagraphFormat par_format_;

	//Distance between left and right margin in twips
	int width_ = 1000;

	//Cell Border width
	int border_left_ = 0;
	int border_right_ = 0;
	int border_top_ = 0;
	int border_bottom_ = 0;
	//Cell border style
	QByteArray border_type_ = "brdrs";

	//Add additional style elements, placed directly in front of "\cellx" in row header
	QList<QByteArray> control_words_;

	//Background color as specified in color table
	int background_color_ = 0;
};

class RtfTableRow
{
public:
	///Default constructor creates empty instance, cells can be added later
	RtfTableRow();

	RtfTableRow(const QByteArray& cell_content, int width, const RtfParagraphFormat& format = RtfParagraphFormat());

	///Constructor creates a row, content and cell widths according parameter lists
	RtfTableRow(const QList<QByteArray>& cell_contents, const QList<int>& cell_widths,const RtfParagraphFormat& format = RtfParagraphFormat());

	///Add cell using predefined cell format settings
	void addCell(const QByteArray& cell_content, int width, const RtfParagraphFormat& par_format = RtfParagraphFormat());
	///Add cellusing predefined cell format settings, each element of cell_contents will be separated by RTF new line "\line"
	void addCell(const QByteArrayList& cell_contents, int width, const RtfParagraphFormat& par_format = RtfParagraphFormat());

	///sets consistent border to all cells
	void setBorders(int width, const QByteArray& type="brdrs");
	///Write RTF code of a whole row
	RtfSourceCode writeRow();

	///Read access to cells
	const RtfTableCell& operator[](int index) const
	{
		return cells_[index];
	}
	///Read/write access to cells
	RtfTableCell& operator[](int index)
	{
		return cells_[index];
	}

	const RtfTableCell& last() const
	{
		return cells_.last();
	}

	RtfTableCell& last()
	{
		return cells_.last();
	}

	int count()
	{
		return cells_.count();
	}

private:
	QList<RtfTableCell> cells_;

	//Spacing between two adjacent cells in twips
	int table_row_gap_half = 180;
	//Spacing to left margin of page in twips
	int tr_left_ = 0;

	///write RTF code of the row header
	RtfSourceCode writeRowHeader();
};


class RtfTable
{
public:
	RtfTable();

	///Constructor initializes table using unique format
	RtfTable(const QList< QList<QByteArray> >& contents, const QList< QList<int> >& widths, const RtfParagraphFormat& format = RtfParagraphFormat());

	void addRow(const RtfTableRow& row)
	{
		rows_.append(row);
	}

	RtfSourceCode writeTable();

	const RtfTableRow& operator[](int index) const
	{
		return rows_[index];
	}
	RtfTableRow& operator[](int index)
	{
		return rows_[index];
	}

	int count()
	{
		return rows_.count();
	}

	///sets format for all table cells
	void setUniqueFormat(const RtfParagraphFormat& format);
	///sets border for all table cells
	void setUniqueBorder(int border,const QByteArray& border_type = "brdrs");


private:
	QList<RtfTableRow> rows_;


};

#endif // RTFDOCUMENT_H
