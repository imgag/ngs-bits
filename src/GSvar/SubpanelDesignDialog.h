#ifndef SUBPANELDESIGNDIALOG_H
#define SUBPANELDESIGNDIALOG_H

#include <QCompleter>
#include "BedFile.h"
#include "GeneSet.h"
#include "ui_SubpanelDesignDialog.h"

class SubpanelDesignDialog
	: public QDialog
{
	Q_OBJECT

public:
	SubpanelDesignDialog(QWidget *parent = 0);

	///Sets the gene list
	void setGenes(const GeneSet& genes_);

	///Returns the last created sub-panel name (or an empty string if not subpanel was designed).
	QString lastCreatedSubPanel();

protected slots:
	void checkAndCreatePanel();
	void storePanel();
	void disableStoreButton();
	void importFromExistingSubpanel();

protected:
	QString getName(bool with_suffix) const;

	void createSubpanelCompleter();

	void clearMessages();
	void addMessage(QString text, bool is_error, bool update_gui);
	bool errorMessagesPresent();

private:
	Ui::SubpanelDesignDialog ui_;
	struct Message
	{
		QString text;
		bool is_error;
	};
	QStringList subpanel_names_;
	QList<Message> messages_;
	QCompleter* completer_;
	GeneSet genes_;
	BedFile regions_;
	QString last_created_subpanel_;

};

#endif // SUBPANELDESIGNDIALOG_H
