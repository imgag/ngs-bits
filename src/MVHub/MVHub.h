#ifndef MVHUB_H
#define MVHUB_H

#include <QMainWindow>
#include "ui_MVHub.h"

///Main window class
class MVHub
		: public QMainWindow
{
	Q_OBJECT
	
public:
	///Constructor
	MVHub(QWidget* parent = 0);

public slots:
	void test_apiConsent();
	void test_apiPseudo();
	void test_apiReCapCaseManagement();

private:
	Ui::MVHub ui_;
	void clearOutput(QObject* sender);
};

#endif // MVHUB_H
