#ifndef RTFDOCUMENT_H
#define RTFDOCUMENT_H

#include "cppNGS_global.h"
#include <QByteArray>
#include <QList>
#include <tuple>

typedef QByteArray RtfSourceCode;


/*************************
 * RTF TEXT BASE CLASSES *
 *************************/
///struct describesformat of raw RTF text (e.g. if text format shall changes within a paragraph or you need text outside a paragraph)
class CPPNGSSHARED_EXPORT RtfText
{
public:
	friend class RtfParagraph;

	RtfText()
	{
	}
	RtfText(const QByteArray& content)
	{
		content_ = content;
	}
	RtfText(const QByteArrayList& content)
	{
		setContent(content);
	}

	virtual RtfText& append(const QByteArray& content,bool new_line = false)
	{
		if(new_line) content_.append("\n\\line\n" + content);
		else content_.append(content);
		return *this;
	}

	virtual RtfText& setFontSize(int font_size)
	{
		font_size_ = font_size;
		return *this;
	}

	virtual RtfText& setBold(bool bold)
	{
		bold_ = bold;
		return *this;
	}
	virtual RtfText& setItalic(bool italic)
	{
		italic_ = italic;
		return *this;
	}

	///Highlight text according color number specified in Document header
	virtual RtfText& highlight(int color_number)
	{
		highlight_color_ = color_number;
		return *this;
	}


	void setContent(const QByteArray& content)
	{
		content_ = content;
	}

	void setContent(const QByteArrayList& content)
	{
		content_ = content.join("\n\\line\n");
	}



	///Horizontal alignment c: center, l: left, r: right, j: justified
	virtual RtfText& setHorizontalAlignment(const QByteArray& alignment)
	{
		const QByteArrayList all = {"c","l","r","j"};
		if(!all.contains(alignment)) return *this;
		horizontal_alignment_ = alignment;
		return *this;
	}

	const QByteArray& content() const
	{
		return content_;
	}

	virtual RtfSourceCode RtfCode();
private:
	int font_size_ = 18;
	bool bold_ = false;
	bool italic_ = false;
	QByteArray horizontal_alignment_ = "l";
	//font number as specified in header
	int font_number_ = 0;
	//font color as specified in header
	int font_color_ = 0;
	int highlight_color_ = 0;

	QByteArray content_ = "";
};


///struct describing layout of a text paragraph, all values in twips
class CPPNGSSHARED_EXPORT RtfParagraph : public RtfText
{
public:
	RtfParagraph()
	{
	}
	///Create paragraph using content as text and standard format
	RtfParagraph(const QByteArray& content) : RtfText(content)
	{
	}

	RtfSourceCode RtfCode() override;

	///Set indention,
	RtfParagraph& setIndent(int left,int right,int first_line)
	{
		indent_block_left_ = left;
		indent_block_right_ = right;
		indent_first_line_ = first_line;
		return *this;
	}

	RtfParagraph& highlight(int color_number) override
	{
		this->RtfText::highlight(color_number);
		return *this;
	}

	RtfParagraph& setPartOfACell(bool is_part_of_cell)
	{
		part_of_a_cell_ = is_part_of_cell;
		return *this;
	}

	RtfParagraph& setItalic(bool italic) override
	{
		this->RtfText::setItalic(italic);
		return *this;
	}

	RtfParagraph& setFontSize(int font_size) override
	{
		this->RtfText::setFontSize(font_size);
		return *this;
	}

	RtfParagraph& setBold(bool bold) override
	{
		this->RtfText::setBold(bold);
		return *this;
	}

	RtfParagraph& setSpaceBefore(int space_before)
	{
		space_before_ = space_before;
		return *this;
	}

	RtfParagraph& setSpaceAfter(int space_after)
	{
		space_after_ = space_after;
		return *this;
	}

	RtfParagraph& setHorizontalAlignment(const QByteArray& alignment) override
	{
		this->RtfText::setHorizontalAlignment(alignment);
		return *this;
	}

	RtfParagraph& setLineSpacing(int line_spacing)
	{
		line_spacing_ = line_spacing;
		return *this;
	}

private:
	bool part_of_a_cell_ = false;

	int space_before_ = 30;
	int space_after_ = 30;

	int indent_block_left_ = 30;
	int indent_block_right_ = 30;
	int indent_first_line_ = 0;

	int line_spacing_ = 0;
};

class CPPNGSSHARED_EXPORT RtfDocument
{
public:
	///Constructor initializes class using suggestive values for the document
	RtfDocument();

	void save(const QByteArray& file_name);

	///adds part to Rtf body part;
	void addPart(const RtfSourceCode& part)
	{
		body_parts_ << part;
	}

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

	void newPage()
	{
		body_parts_ << "\n\\page\n";
	}

	///max width that can be used without breaking page
	int maxWidth()
	{
		return width_ -margin_left_-margin_right_;
	}

private:
	//page dimensions
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


	//parts between header and footer
	QList<RtfSourceCode> body_parts_;

	///Returns RTF header according settings of this class
	RtfSourceCode header();
	///Returns footer of RTF code
	RtfSourceCode footer();
};


/**************************************
 * CLASSES FOR RTF CELL/ROW AND TABLE *
 **************************************/
class CPPNGSSHARED_EXPORT RtfTableCell
{
	friend class RtfTableRow;

public:
	const RtfParagraph& format() const
	{
		return paragraph_;
	}

	RtfParagraph& format()
	{
		return paragraph_;
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

	void setBorderColor(int color_number)
	{
		border_color_ = color_number;
	}


	int width()
	{
		return width_;
	}

	///Method can be used to add special control words to the header of a cell. Do not include "\" into statement.
	void setHeaderControlWord(const QByteArray& statement)
	{
		control_word_ = statement;
	}

	const QByteArray controlWord()
	{
		return control_word_;
	}


private:
	///private constructor, only to be accessed by friend class
	RtfTableCell(int width, const RtfParagraph &text_format = RtfParagraph());

	///returns RTF code formatting the cell, can only be called from friend classes
	QByteArray writeCell();

	RtfParagraph paragraph_;

	//Distance between left and right margin in twips
	int width_ = 1000;

	//Cell Border width
	int border_left_ = 0;
	int border_right_ = 0;
	int border_top_ = 0;
	int border_bottom_ = 0;

	int border_color_ = 0;

	//Cell border style
	QByteArray border_type_ = "brdrs";

	QByteArray control_word_ = "";

	//Background color as specified in color table
	int background_color_ = 0;
};

class CPPNGSSHARED_EXPORT RtfTableRow
{
public:
	///Default constructor creates empty instance, cells can be added later
	RtfTableRow();

	///Initialize Row containing one cell
	RtfTableRow(QByteArray cell_content, int width, const RtfParagraph& format = RtfParagraph());

	///Constructor creates a row, content and cell widths according parameter lists
	RtfTableRow(const QList<QByteArray>& cell_contents, const QList<int>& cell_widths,const RtfParagraph& format = RtfParagraph());

	///Add cell using predefined cell format settings
	void addCell(int width, const RtfParagraph& paragraph);
	///Add cell using standard paragraph format
	void addCell(int width, const QByteArray& content);

	void addCell(int width, const QByteArray &content, const RtfParagraph& par_format);
	///Add cellusing predefined cell format settings, each element of cell_contents will be separated by RTF new line "\line"
	void addCell(const QByteArrayList& cell_contents, int width, const RtfParagraph& par_format = RtfParagraph());

	RtfTableRow& setHeader()
	{
		for(auto& cell : cells_)
		{
			cell.setHeaderControlWord("trhdr");
		}
		return *this;
	}

	///sets consistent border to all cells
	RtfTableRow& setBorders(int width, const QByteArray& type="brdrs");
	RtfTableRow& setBorders(int width, const QByteArray &type,int color);
	///sets border color for all cells
	RtfTableRow& setBorderColor(int border_color)
	{
		for(auto& cell : cells_)
		{
			cell.setBorderColor(border_color);
		}
		return *this;
	}

	RtfTableRow& setPadding(int cell_padding)
	{
		padding_ = cell_padding;
		return *this;
	}

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

	///sets Background color for each cell in row, color number must be specified in RtfDocument header
	RtfTableRow& setBackgroundColor(int color);

private:
	QList<RtfTableCell> cells_;

	//Spacing between two adjacent cells in twips
	int table_row_gap_half = 180;
	//Spacing to left margin of page in twips
	int tr_left_ = 0;

	//cell padding in twips
	int padding_ = 28;

	///write RTF code of the row header
	RtfSourceCode writeRowHeader();
};


class CPPNGSSHARED_EXPORT RtfTable
{
public:
	RtfTable();

	///Constructor initializes table using unique format
	RtfTable(const QList< QList<QByteArray> >& contents, const QList< QList<int> >& widths, const RtfParagraph& format = RtfParagraph());

	void addRow(const RtfTableRow& row)
	{
		rows_.append(row);
	}

	void prependRow(const RtfTableRow& row)
	{
		rows_.prepend(row);
	}

	void insertRow(int idx, const RtfTableRow& row)
	{
		rows_.insert(idx, row);
	}

	void removeRow(int row)
	{
		rows_.removeAt(row);
	}

	RtfSourceCode RtfCode();

	const RtfTableRow& operator[](int index) const
	{
		return rows_[index];
	}
	RtfTableRow& operator[](int index)
	{
		return rows_[index];
	}

	const RtfTableRow& first() const
	{
		return rows_.first();
	}

	RtfTableRow& first()
	{
		return rows_.first();
	}

	const RtfTableRow& last() const
	{
		return rows_.last();
	}
	RtfTableRow& last()
	{
		return rows_.last();
	}

	int count()
	{
		return rows_.count();
	}

	bool isEmpty()
	{
		if(rows_.isEmpty()) return true;
		return false;
	}

	///sort table ascending by column index i_col
	void sortByCol(int i_col);

	///sorts table ascending by columns, starting with first column index in indices
	void sortbyCols(const QList<int>& indices);

	///swaps position of two rows with indices i_row_a and i_row_b
	void swapRows(int i_row_a, int i_row_b);

	///sets border for all table cells
	RtfTable& setUniqueBorder(int border,const QByteArray& border_type = "brdrs", int border_color = 0);

	RtfTable& setUniqueFontSize(int font_size);

private:
	QList<RtfTableRow> rows_;
};

#endif // RTFDOCUMENT_H
