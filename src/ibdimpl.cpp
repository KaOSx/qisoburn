#include "ibdimpl.h"
//
IBDialogImpl::IBDialogImpl( QWidget * parent, Qt::WindowFlags f) 
	: QDialog(parent, f)
{
	setupUi(this);
	
	mediainfo_proc = new QProcess(this);
	growisofs_proc = new QProcess(this);
	settings = new QSettings("mycelo", "qisoburn");
	state = 0;
	
	connect(mediainfo_proc, SIGNAL(readyReadStandardOutput()), this, SLOT(mediainfo_output()));
	connect(mediainfo_proc, SIGNAL(finished(int, QProcess::ExitStatus)), this, SLOT(mediainfo_finished(int, QProcess::ExitStatus)));
	connect(mediainfo_proc, SIGNAL(error(QProcess::ProcessError)), this, SLOT(mediainfo_error(QProcess::ProcessError)));
	
	connect(growisofs_proc, SIGNAL(readyReadStandardOutput()), this, SLOT(growisofs_output()));
	connect(growisofs_proc, SIGNAL(finished(int, QProcess::ExitStatus)), this, SLOT(growisofs_finished(int, QProcess::ExitStatus)));
	connect(growisofs_proc, SIGNAL(error(QProcess::ProcessError)), this, SLOT(growisofs_error(QProcess::ProcessError)));

	clearUI();
}
//
void IBDialogImpl::clearUI()
{
	comboBreak->clear();
	comboBreak->addItem(LAYER_BREAK);
	comboBreak->setEditText(settings->value("layer_break", "").toString());
	comboSpeed->clear();
	comboSpeed->setEditText(settings->value("speed", "").toString());
	checkCompat->setDisabled(false);
	checkCompat->setChecked(settings->value("dvd_compat", "y").toString() == "y");
	checkDelete->setChecked(settings->value("delete_image", "n").toString() == "y");
	labelStatus->clear();
	labelDevice->setText(tr("Burner: %1 \n Media: %1").arg(tr("(click refresh)")));
	lineDevice->setText(settings->value("device", DEVICE).toString());
	plainLog->clear();
	pushBurn->setDisabled(false);
	pushAbort->setDisabled(true);
	pushClose->setDisabled(false);
	progressBar->hide();
	
	last_path = settings->value("last_path", "").toString();
	lineFileName->setText(last_path);

	QPoint pos = settings->value("pos", this->pos()).toPoint();
	QSize size = settings->value("size", this->size()).toSize();
	move(pos);
	resize(size);
}
//
void IBDialogImpl::on_toolBrowse_clicked()
{
	QString fileName;
	fileName = QFileDialog::getOpenFileName(this, tr("Image File"), lineFileName->text().isEmpty() ? QDir::homePath() : lineFileName->text());	
	
	if( !fileName.isEmpty() && QFile::exists(fileName) )
	{
		lineFileName->setText(fileName);
		last_path = fileName.remove(QRegExp("\\/[^/]+$"));
	}
}
//
bool IBDialogImpl::checkDevice()
{
	QRegExp devcheck("(\\w|\\-|\\.|\\/)+");
	
	if( !devcheck.exactMatch(lineDevice->text().simplified()) )
	{
		QMessageBox::warning(this, this->windowTitle(), tr("The device name looks wrong"));
		return( false );
	}
	else return( true );
}
//
bool IBDialogImpl::checkFile()
{
	if( !QFile::exists(lineFileName->text().simplified()) )
	{
		QMessageBox::warning(this, this->windowTitle(), tr("Image file not found"));
		return( false );
	}
	else return( true );
}
//
void IBDialogImpl::on_toolRefresh_clicked()
{
	if( checkDevice() )
	{
		labelStatus->setText(tr("Running '%1'...").arg(MEDIAINFO));
		comboSpeed->setDisabled(true);
		comboBreak->setDisabled(true);
		lineDevice->setDisabled(true);
		lineFileName->setDisabled(true);
		pushBurn->setDisabled(true);
		pushClose->setDisabled(true);
		pushAbort->setDisabled(false);
		checkCompat->setDisabled(true);
		labelDevice->clear();
		toolRefresh->setDisabled(true);
		toolBrowse->setDisabled(true);
		toolSave->setDisabled(true);
		plainLog->appendPlainText(tr("***** %1 BEGIN *****").arg(MEDIAINFO));
	
		mediainfo_data.clear();
		mediainfo_proc->setProcessChannelMode(QProcess::MergedChannels);
		mediainfo_proc->start(MEDIAINFO, QStringList() << lineDevice->text().simplified());
		state = 1;
	}
}
//
void IBDialogImpl::mediainfo_output()
{
	mediainfo_data.append(QString().append(mediainfo_proc->readAllStandardOutput()).split("\n"));
	plainLog->appendPlainText(mediainfo_data.filter(QRegExp(".*\\S.*")).join("\n"));
}
//
void IBDialogImpl::mediainfo_error(QProcess::ProcessError error)
{
	plainLog->appendPlainText(tr("***** %1: process ERROR %2 detected *****").arg(MEDIAINFO).arg(error));
	state = 0;
}
//
void IBDialogImpl::mediainfo_finished(int code, QProcess::ExitStatus status)
{
	QRegExp burner_line("INQUIRY:\\s*(.+)");
	QRegExp media_line("(?:MOUNTED MEDIA|MEDIA BOOK TYPE|MEDIA ID):\\s*(.+)");
	QRegExp speed_line("WRITE SPEED\\s*\\#\\d\\:\\s*((?:[0-9]|[,.])+)[xX].*");
	QString burner_txt;
	QString temp_speed;
	QStringList media_txt;
	
	plainLog->appendPlainText(tr("***** %1 END *****\n").arg(MEDIAINFO));
	pushBurn->setDisabled(false);
	comboSpeed->setDisabled(false);
	comboBreak->setDisabled(false);
	lineDevice->setDisabled(false);
	lineFileName->setDisabled(false);
	toolRefresh->setDisabled(false);
	toolBrowse->setDisabled(false);
	pushAbort->setDisabled(true);
	pushClose->setDisabled(false);
	checkCompat->setDisabled(false);
	toolSave->setDisabled(false);
	
	state = 0;
	temp_speed = comboSpeed->currentText();
	
	if( code >= 0 && status == 0 )
	{
		comboSpeed->clear();

		for( int i = 0; i < mediainfo_data.size(); i++ )
		{
			if( burner_line.exactMatch(mediainfo_data.at(i).simplified().toUpper()) )
			{
				burner_txt = burner_line.capturedTexts().at(1).simplified();
			}
			else if( media_line.exactMatch(mediainfo_data.at(i).simplified().toUpper()) )
			{
				media_txt.append(media_line.capturedTexts().at(1).simplified().split(","));
			}
			else if( speed_line.exactMatch(mediainfo_data.at(i).simplified().toUpper()) )
			{
				comboSpeed->addItem(speed_line.capturedTexts().at(1).simplified());
			}
		}
		
		if( burner_txt.simplified().isEmpty() ) burner_txt = tr("(not available)");
		if( media_txt.filter(QRegExp(".*\\S.*")).isEmpty() ) media_txt.append(tr("(not available)"));
		if( !temp_speed.simplified().isEmpty() ) comboSpeed->setEditText(temp_speed);
		
		labelDevice->setText(tr("Burner: %1 \n Media: %2").arg(burner_txt).arg(media_txt.filter(QRegExp(".*\\S.*")).join(",").simplified().replace(", ", ",")));	
		labelStatus->setText(QString());
	}
	else
	{
		labelStatus->setText(tr("ERROR in '%1', exit code %2").arg(MEDIAINFO).arg(code));
	}
}
//
void IBDialogImpl::on_pushBurn_clicked()
{
	QStringList parameters;
	
	if( checkFile() && checkDevice() )
	{
		if( QMessageBox::question(this, this->windowTitle(), tr("Confirm media burning?"), QMessageBox::Ok|QMessageBox::Cancel, QMessageBox::Cancel) == QMessageBox::Ok )
		{
			labelStatus->setText(tr("Running '%1'...").arg(GROWISOFS));
			comboSpeed->setDisabled(true);
			comboBreak->setDisabled(true);
			lineFileName->setDisabled(true);
			lineDevice->setDisabled(true);
			toolRefresh->setDisabled(true);
			toolBrowse->setDisabled(true);
			toolSave->setDisabled(true);
			pushBurn->setDisabled(true);
			pushAbort->setDisabled(false);
			pushClose->setDisabled(true);
			checkCompat->setDisabled(true);
			progressBar->setValue(0);
			progressBar->show();
			plainLog->appendPlainText(tr("***** %1 BEGIN *****").arg(GROWISOFS));

			last_path = lineFileName->text().simplified().remove(QRegExp("\\/[^/]+$"));

			parameters << "-use-the-force-luke=dao";
			if( checkCompat->isChecked() ) parameters << "-dvd-compat";
			if( !comboSpeed->currentText().simplified().isEmpty() ) parameters << QString("-speed=%1").arg(comboSpeed->currentText().simplified());
			if( !comboBreak->currentText().simplified().isEmpty() ) parameters << QString("-use-the-force-luke=break:%1").arg(comboBreak->currentText().simplified());
			parameters << "-Z";
			parameters << lineDevice->text().simplified() + "=" + lineFileName->text().simplified();

			plainLog->appendPlainText(parameters.join(" "));
			growisofs_proc->setProcessChannelMode(QProcess::MergedChannels);
			growisofs_proc->start(GROWISOFS, parameters);
			state = 3;
		}
	}
}
//
void IBDialogImpl::growisofs_output()
{
	int progress;
	QString growisofs_data;
	QRegExp status_line(lineDevice->text().simplified().toLower() + ":?\\s+(.+)");
	QRegExp progress_line("\\d+/\\d+\\s*\\(\\s*((?:[0-9]|[.,])+)\\%\\s*\\)\\s*@((?:[0-9]|[.,])+)[xX],\\s+\\w+\\s+((?:[0-9]|[:?])+).*");
	
	while( growisofs_proc->canReadLine() )
	{
		growisofs_data = growisofs_proc->readLine().simplified().toLower();
		
		if( !growisofs_data.isEmpty() )
		{
			plainLog->appendPlainText(growisofs_data);
	
			if( status_line.exactMatch(growisofs_data) )
			{
				labelStatus->setText(status_line.capturedTexts().at(1).simplified());
			}
			else if( progress_line.exactMatch(growisofs_data) )
			{
				progress = progress_line.capturedTexts().at(1).simplified().toFloat() + 0.5;
				labelStatus->setText(tr("Burning at %1x, %2 remaining").arg(progress_line.capturedTexts().at(2).simplified()).arg(progress_line.capturedTexts().at(3).simplified()));
				if( progress > 0 && progress <= 100 ) progressBar->setValue(progress);
			}
		}
		QCoreApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
	}
}
//
void IBDialogImpl::growisofs_error(QProcess::ProcessError error)
{
	plainLog->appendPlainText(tr("***** %1: process ERROR %2 detected *****").arg(GROWISOFS).arg(error));
	state = 0;
}
//
void IBDialogImpl::growisofs_finished(int code, QProcess::ExitStatus status)
{
	if( status == QProcess::NormalExit && code == 0 )
	{
		if( checkDelete->isChecked() )
		{
			plainLog->appendPlainText(tr("\nDeleting image file..."));
			QCoreApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
			QFile::remove(lineFileName->text().simplified());
			
			if( !QFile::exists(lineFileName->text().simplified()) )
			{
				plainLog->appendPlainText(tr("Image file \"%1\" deleted successfully\n").arg(lineFileName->text().simplified()));
			}
			else
			{
				plainLog->appendPlainText(tr("Unable to delete image file \"%1\"\n").arg(lineFileName->text().simplified()));
			}
		}
		
		lineFileName->setText(last_path);
	}
	
	comboSpeed->setDisabled(false);
	comboBreak->setDisabled(false);
	lineFileName->setDisabled(false);
	lineDevice->setDisabled(false);
	toolRefresh->setDisabled(false);
	toolBrowse->setDisabled(false);
	toolSave->setDisabled(false);
	pushBurn->setDisabled(false);
	pushAbort->setDisabled(true);
	pushClose->setDisabled(false);
	checkCompat->setDisabled(false);
	progressBar->hide();
	labelStatus->setText(tr("Burning %1, '%2' %3, exit code %4").arg(code == 0 ? tr("finished") : tr("ERROR")).arg(GROWISOFS).arg(status == QProcess::NormalExit ? tr("ended normally") : tr("CRASHED")).arg(code));
	state = 0;
}

void IBDialogImpl::on_pushAbort_clicked()
{
	if( QMessageBox::question(this, this->windowTitle(), tr("Really abort current operation?"), QMessageBox::Yes|QMessageBox::No, QMessageBox::No) == QMessageBox::Yes )
	{
		switch( state )
		{
			case 1:
				labelStatus->setText(tr("Killing '%1'").arg(MEDIAINFO));
				mediainfo_proc->kill();
				pushAbort->setDisabled(true);
				state = 0;
				break;
			case 2:
				labelStatus->setText(tr("Killing '%1'").arg(GROWISOFS));
				growisofs_proc->kill();
				pushAbort->setDisabled(true);
				state = 0;
				break;
			case 3:
				labelStatus->setText(tr("Asking '%1' to terminate").arg(GROWISOFS));
				growisofs_proc->terminate();
				state = 2;
				break;
		}
	}
}
//
void IBDialogImpl::closeEvent(QCloseEvent *event)
{
	if( state == 3 ) event->ignore(); else event->accept();
}
//
void IBDialogImpl::on_toolSave_clicked()
{
	settings->setValue("pos", this->pos());
	settings->setValue("size", this->size());
	settings->setValue("device", lineDevice->text().simplified());
	settings->setValue("layer_break", comboBreak->currentText().simplified());
	settings->setValue("speed", comboSpeed->currentText().simplified());
	settings->setValue("dvd_compat", checkCompat->isChecked() ? "y" : "n");
	settings->setValue("delete_image", checkDelete->isChecked() ? "y" : "n");
	settings->setValue("last_path", last_path);
	labelStatus->setText(tr("GUI settings have been saved"));
}
//
