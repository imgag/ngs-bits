#ifndef CHROMOSOMECOLORS_H
#define CHROMOSOMECOLORS_H

#include "cppVISUAL_global.h"
#include <QHash>
#include <QColor>


struct CPPVISUALSHARED_EXPORT ChromosomeColors
{
public:
	static QColor getColor(QString chr)
	{
		if (instance()->colorMap.contains(chr)) return instance()->colorMap[chr];
		return QColor(0, 0, 0);
	}

private:
	QHash<QString, QColor> colorMap;

	static ChromosomeColors* instance()
	{
		static ChromosomeColors s;
		return &s;
	}

	ChromosomeColors(){
		colorMap["chrX"] =  QColor(204, 153, 0);
		colorMap["chrY"] =  QColor(153, 204, 0);
		colorMap["chrUn"] = QColor(150, 150, 150);
		colorMap["chr1"] =  QColor(80, 80, 255);
		colorMap["I"] =  QColor(139, 155, 187);
		colorMap["chr2"] =  QColor(206, 61, 50);
		colorMap["II"] =  QColor(206, 61, 50);
		colorMap["chr2a"] =  QColor(206, 61, 5);
		colorMap["chr2b"] =  QColor(206, 61, 50);
		colorMap["chr3"] =  QColor(116, 155, 88);
		colorMap["III"] =  QColor(116, 155, 88);
		colorMap["chr4"] =  QColor(240, 230, 133);
		colorMap["IV"] =  QColor(240, 230, 133);
		colorMap["chr5"] =  QColor(70, 105, 131);
		colorMap["chr6"] =  QColor(186, 99, 56);
		colorMap["chr7"] =  QColor(93, 177, 221);
		colorMap["chr8"] =  QColor(128, 34, 104);
		colorMap["chr9"] =  QColor(107, 215, 107);
		colorMap["chr10"] =  QColor(213, 149, 167);
		colorMap["chr11"] =  QColor(146, 72, 34);
		colorMap["chr12"] =  QColor(131, 123, 141);
		colorMap["chr13"] =  QColor(199, 81, 39);
		colorMap["chr14"] =  QColor(213, 143, 92);
		colorMap["chr15"] =  QColor(122, 101, 165);
		colorMap["chr16"] =  QColor(228, 175, 105);
		colorMap["chr17"] =  QColor(59, 27, 83);
		colorMap["chr18"] =  QColor(205, 222, 183);
		colorMap["chr19"] =  QColor(97, 42, 121);
		colorMap["chr20"] =  QColor(174, 31, 99);
		colorMap["chr21"] =  QColor(231, 199, 111);
		colorMap["chr22"] =  QColor(90, 101, 94);
		colorMap["chr23"] =  QColor(204, 153, 0);
		colorMap["chr24"] =  QColor(153, 204, 0);
		colorMap["chr25"] =  QColor(51, 204, 0);
		colorMap["chr26"] =  QColor(0, 204, 51);
		colorMap["chr27"] =  QColor(0, 204, 153);
		colorMap["chr28"] =  QColor(0, 153, 204);
		colorMap["chr29"] =  QColor(10, 71, 255);
		colorMap["chr30"] =  QColor(71, 117, 255);
		colorMap["chr31"] =  QColor(255, 194, 10);
		colorMap["chr32"] =  QColor(255, 209, 71);
		colorMap["chr33"] =  QColor(153, 0, 51);
		colorMap["chr34"] =  QColor(153, 26, 0);
		colorMap["chr35"] =  QColor(153, 102, 0);
		colorMap["chr36"] =  QColor(128, 153, 0);
		colorMap["chr37"] =  QColor(51, 153, 0);
		colorMap["chr38"] =  QColor(0, 153, 26);
		colorMap["chr39"] =  QColor(0, 153, 102);
		colorMap["chr40"] =  QColor(0, 128, 153);
		colorMap["chr41"] =  QColor(0, 51, 153);
		colorMap["chr42"] =  QColor(26, 0, 153);
		colorMap["chr43"] =  QColor(102, 0, 153);
		colorMap["chr44"] =  QColor(153, 0, 128);
		colorMap["chr45"] =  QColor(214, 0, 71);
		colorMap["chr46"] =  QColor(255, 20, 99);
		colorMap["chr47"] =  QColor(0, 214, 143);
		colorMap["chr48"] =  QColor(20, 255, 177);
	}
};

#endif // CHROMOSOMECOLORS_H
