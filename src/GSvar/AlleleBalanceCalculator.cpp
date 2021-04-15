#include "AlleleBalanceCalculator.h"
#include "BasicStatistics.h"
#include "Exceptions.h"
#include <cmath>

AlleleBalanceCalculator::AlleleBalanceCalculator(QWidget* parent)
	: QWidget(parent)
	, ui_()
{
	ui_.setupUi(this);
	connect(ui_.depth, SIGNAL(valueChanged(int)), this, SLOT(updateResult()));
	connect(ui_.allele_count, SIGNAL(valueChanged(int)), this, SLOT(updateResult()));
}

void AlleleBalanceCalculator::updateResult()
{
	ui_.result->clear();

	try
	{
		BasicStatistics::precalculateFactorials();

		//init
		int depth = ui_.depth->value();
		int allele_count = ui_.allele_count->value();
		if (allele_count>0.5*depth) allele_count = depth - allele_count; //the code below assumes we are on the left side of the distribution...
		double p = 0.5;

		//take care of factorial overflow
		while(!BasicStatistics::isValidFloat(BasicStatistics::factorial(depth)))
		{
			depth /= 2;
			allele_count /=2;
		}

		//calculate probability
		double output = 0.0;
		for (int i=0; i<=allele_count; ++i)
		{
			double q = std::pow(1.0-p, depth-i) * std::pow(p, i) * BasicStatistics::factorial(depth) / BasicStatistics::factorial(i) / BasicStatistics::factorial(depth-i);
			output += q;
		}
		output = std::min(output*2.0, 1.0); //two times because we need to consider both sides of the distribution.

		ui_.result->setText(QString::number(100.0*output, 'f', 4) + "%");
	}
	catch(Exception& e)
	{
		ui_.result->setText("Error: " + e.message());
	}
}

