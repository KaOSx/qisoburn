#ifndef IBDIMPL_H
#define IBDIMPL_H
//

#include <QDialog>
#include <QFileDialog>
#include <QDir>
#include <QRegExp>
#include <QProcess>
#include <QMessageBox>
#include <QFile>
#include <QCoreApplication>
#include <QEventLoop>
#include <QSettings>
#include "ui_ibdialog.h"
//
const QString GROWISOFS = "growisofs";
const QString MEDIAINFO = "dvd+rw-mediainfo";
const QString LAYER_BREAK = "1913760";
const QString DEVICE = "/dev/sr0";
//
class IBDialogImpl : public QDialog, public Ui::IBDialog
{
Q_OBJECT
public:
	IBDialogImpl( QWidget * parent = 0, Qt::WindowFlags f = 0 );
protected:
	void closeEvent(QCloseEvent *event);
private:
	QSettings *settings;
	QStringList mediainfo_data;
	QProcess *mediainfo_proc;
	QProcess *growisofs_proc;
	QString last_path;
	void clearUI();
	bool checkDevice();
	bool checkFile();
	int state;
private slots:
	void on_toolSave_clicked();
	void on_pushAbort_clicked();
	void on_toolRefresh_clicked();
	void on_pushBurn_clicked();
	void on_toolBrowse_clicked();
	void mediainfo_output();
	void mediainfo_finished(int code, QProcess::ExitStatus status);
	void mediainfo_error(QProcess::ProcessError error);
	void growisofs_output();
	void growisofs_finished(int code, QProcess::ExitStatus status);
	void growisofs_error(QProcess::ProcessError error);
};
#endif
