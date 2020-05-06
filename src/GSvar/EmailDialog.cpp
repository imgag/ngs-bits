#include "EmailDialog.h"
#include <QDesktopServices>
#include <QMessageBox>

EmailDialog::EmailDialog(QWidget* parent, QStringList to, QString subject, QStringList body)
	: QDialog(parent)
	, ui_()
{
	ui_.setupUi(this);
	connect(ui_.send_btn, SIGNAL(clicked(bool)), this, SLOT(sendEmail()));

	//cutate recipient list
	std::for_each(to.begin(), to.end(), [](QString& value){ value = value.toLower(); });
	to.removeDuplicates();
	to.removeAll("");

	//fill GUI
	ui_.to->setText(to.join("; "));
	ui_.subject->setText(subject);
	ui_.body->setPlainText(body.join("\n"));
}

void EmailDialog::sendEmail()
{
	bool result = QDesktopServices::openUrl(QUrl("mailto:" + ui_.to->text() + "?subject=" + ui_.subject->text() + "&body=" + ui_.body->toPlainText()));
	if (result==true)
	{
		accept();
	}
	else
	{
		QMessageBox::warning(this, "Email error", "The email app could not be opened or failed to process the data!");
	}
}
