#include "RtfDocument.h"


RtfDocument::RtfDocument()
	: width_(11905)
	, height_(15840)
	, margin_top_(1134)
	, margin_bottom_(1134)
	, margin_left_(1134)
	, margin_right_(1134)
	, fonts_({"Arial"})
	, default_font_size_(18)
{
}


RtfSourceCode RtfDocument::header()
{
	QByteArrayList output;
	output << "{\\rtf\\ansi";
	output << "\\deff0";

	//Create list of fonts
	//default font
	QByteArray font_table = "{\\fonttbl{\\f0 " + fonts_[0] + ";";
	//additional fonts
	if(fonts_.count() > 1)
	{
		for(int i=1;i<fonts_.count();++i)
		{
			font_table.append("\\f" + QByteArray::number(i+1) + " " + fonts_[i] + ";");
		}
	}
	font_table.append("}}");
	output << font_table;

	//page size
	output << "\\paperw" + QByteArray::number(width_);
	output << "\\paperh" + QByteArray::number(height_);

	//margins
	output << "\\margl" + QByteArray::number(margin_left_);
	output << "\\margr" + QByteArray::number(margin_right_);
	output << "\\margt" + QByteArray::number(margin_top_);
	output << "\\margb" + QByteArray::number(margin_bottom_);


	//colors used in document
	if(!colors_.isEmpty())
	{
		QByteArray tmp_out_color_table = "{\\colortbl;";
		foreach(auto rgb_value,colors_)
		{
			int red,green,blue;
			std::tie(red,green,blue) = rgb_value;
			tmp_out_color_table.append("\\red" + QByteArray::number(red) + "\\green" +  QByteArray::number(green) + "\\blue" +  QByteArray::number(blue) +";");
		}
		tmp_out_color_table.append("}");
		output << tmp_out_color_table;
	}

	//Set document language to German, specifiy default font size, turn on widow control and auto hyphenation
	output << "\\deflang1031\\plain\\fs" + QByteArray::number(default_font_size_)  + "\\widowctrl\\hyphauto";

	return output.join("\n");
}

RtfSourceCode RtfDocument::footer()
{
	return "\n}";
}

RtfSourceCode RtfDocument::escapeUmlauts(const QByteArray &text)
{
	QByteArray output = text;
	output.replace("ß","\\u223;");
	output.replace("ä","\\u228;");
	output.replace("ö","\\u246;");
	output.replace("ü","\\u252;");
	output.replace("Ä","\\u196;");
	output.replace("Ö","\\u214;");
	output.replace("Ü","\\u220;");
	return output;
}

RtfSourceCode RtfDocument::text(const QByteArray &content, const RtfTextFormat &format)
{
	QByteArrayList output;

	output << "{";

	output << "\\fs" + QByteArray::number(format.font_size);
	if(format.font_number > 0) output << "\\f" + QByteArray::number(format.font_number);
	if(format.font_color > 0) output << "\\cf" + QByteArray::number(format.font_color);

	if(format.bold) output << "\\b";
	if(format.italic) output << "\\i";
	if(format.highlight_color != 0) output << "\\highlight" + QByteArray::number(format.highlight_color);

	output << "\\q" + format.horizontal_alignment;

	output << RtfDocument::escapeUmlauts(content);

	output << "}";

	return output.join("\n");
}

RtfSourceCode RtfDocument::paragraph(const QByteArrayList &content, const RtfParagraphFormat &format, bool in_cell)
{
	return RtfDocument::paragraph(content.join("\\line\n"),format,in_cell);
}

RtfSourceCode RtfDocument::paragraph(const QByteArray& content, const RtfParagraphFormat &format, bool in_cell)
{
	QByteArrayList output;

	output << "\\pard";

	output << "\\fs" + QByteArray::number(format.font_size);
	if(format.font_number != 0) output << "\\f" + QByteArray::number(format.font_number);

	output << "\\q" + format.horizontal_alignment;

	//Spacing
	if(format.space_after != 0) output << "\\sa" + QByteArray::number(format.space_after);
	if(format.space_before != 0) output << "\\sb" + QByteArray::number(format.space_before);

	//Indenting
	if(format.intent_block_left != 0) output << "\\li" + QByteArray::number(format.intent_block_left);
	if(format.intent_block_right != 0) output << "\\ri" + QByteArray::number(format.intent_block_right);
	if(format.intent_first_line != 0) output << "\\fi" + QByteArray::number(format.intent_first_line);

	//Character format
	if(format.italic) output << "\\i";
	if(format.bold) output << "\\b";

	if(format.font_color != 0) output << "\\cf" + QByteArray::number(format.font_color);

	if(!content.isEmpty()) output << RtfDocument::text(content,format);
	else output << " ";

	//Skip paragraph end line break if paragraph is part of a table cell
	if(!in_cell) output << "\\par\n";

	return output.join("\n");
}

double RtfDocument::cm2twip(double input_cm)
{
	return 566.929133858264*input_cm;
}

void RtfDocument::setMargins(int left, int top, int right, int bottom)
{
	margin_top_ = top;
	margin_bottom_ = bottom;
	margin_right_ = right;
	margin_left_ = left;
}

RtfTableCell::RtfTableCell(const QByteArray &content, int width, const RtfParagraphFormat &text_format)
{
	content_ = content;
	par_format_ = text_format;
	width_ = width;
}

void RtfTableCell::setBorder(int left,int top,int right,int bottom, const QByteArray& type)
{
	border_left_ = left;
	border_right_ = right;
	border_bottom_ = bottom;
	border_top_ = top;
	border_type_ = type;
}

void RtfTableRow::addCell(const QByteArray& cell_content, int width, const RtfParagraphFormat& par_format)
{
	cells_ << RtfTableCell(cell_content, width, par_format);
}
void RtfTableRow::addCell(const QByteArrayList& cell_contents,int width, const RtfParagraphFormat& par_format)
{
	addCell(cell_contents.join("\\line\n"),width,par_format);
}

void RtfTableRow::setBorders(int width, const QByteArray& type)
{
	for(auto &cell : cells_)
	{
		cell.setBorder(width,width,width,width,type);
	}
}

QByteArray RtfTableCell::writeCell()
{
	QByteArray content = "{";
	content.append(RtfDocument::paragraph("\\intbl " + content_,par_format_,true));
	content.append("\\cell");
	content.append("}");
	return content;
}


RtfTableRow::RtfTableRow()
{
}

RtfTableRow::RtfTableRow(const QByteArray& cell_content, int width,const RtfParagraphFormat& format)
{
	addCell(cell_content,width,format);

}

RtfTableRow::RtfTableRow(const QList<QByteArray>& cell_contents, const QList<int>& cell_widths, const RtfParagraphFormat& format)
{
	if(cell_contents.count() != cell_widths.count()) //Create empty instance if no does not match
	{
		RtfTableRow();
		return;
	}

	for(int i=0;i<cell_contents.count();++i)
	{
		addCell(cell_contents.at(i),cell_widths.at(i),format);
	}

}

RtfSourceCode RtfTableRow::writeRowHeader()
{
	QByteArray output = "\\trowd\\trgraph" + QByteArray::number(table_row_gap_half) + (tr_left_ > 0 ? "\\trleft" + QByteArray::number(tr_left_) : "");

	//Position of the rightmost extreme of a single cell
	int right_cell_offset = tr_left_;
	foreach(auto cell, cells_) //Cell specification
	{
		right_cell_offset += cell.width();

		//cell border
		if(cell.border_top_ != 0) output.append("\\clbrdrt\\brdrw" + QByteArray::number(cell.border_top_) + "\\" + cell.border_type_);
		if(cell.border_bottom_ != 0) output.append("\\clbrdrb\\brdrw" + QByteArray::number(cell.border_bottom_) + "\\" + cell.border_type_);
		if(cell.border_left_ != 0) output.append("\\clbrdrl\\brdrw" + QByteArray::number(cell.border_left_) + "\\" + cell.border_type_);
		if(cell.border_right_ != 0) output.append("\\clbrdrr\\brdrw" + QByteArray::number(cell.border_right_) + "\\" + cell.border_type_);

		if(cell.background_color_ != 0) output.append("\\clcbpat") + QByteArray::number(cell.background_color_);

		//Add specific control words
		if(cell.controlWords().count() > 0)
		{
			for(auto control_word : cell.controlWords())
			{
				output.append("\\" + control_word);
			}
		}

		//cell width
		output.append("\\cellx" + QByteArray::number(right_cell_offset));
	}

	output.append("\n");
	return output;
}

RtfSourceCode RtfTableRow::writeRow()
{
	QByteArrayList output;

	output << writeRowHeader();

	foreach(RtfTableCell cell,cells_)
	{
		output << cell.writeCell();
	}

	output << "\\row";

	return output.join("\n");
}

RtfTable::RtfTable()
{

}

RtfTable::RtfTable(const QList< QList<QByteArray> >& contents, const QList< QList<int> >& widths, const RtfParagraphFormat& format)
{
	for(int i=0;i<contents.count();++i)
	{
		rows_.append(RtfTableRow(contents[i],widths[i],format));
	}
}

RtfSourceCode RtfTable::writeTable()
{
	QByteArrayList output;
	for(int i=0;i<rows_.count();++i)
	{
		output << rows_[i].writeRow();
	}

	return output.join("\n");
}

void RtfTable::setUniqueFormat(const RtfParagraphFormat &format)
{
	for(int i=0;i<count();++i)
	{
		for(int j=0;j<rows_[i].count();++j)
		{
			rows_[i][j].format() = format;
		}
	}
}

void RtfTable::setUniqueBorder(int border, const QByteArray &border_type)
{
	for(int i=0;i<rows_.count();++i)
	{
		rows_[i].setBorders(border,border_type);
	}
}

