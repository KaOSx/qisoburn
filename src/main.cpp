#include <QApplication>
#include "ibdimpl.h"
//
int main(int argc, char ** argv)
{
	QApplication app( argc, argv );
	IBDialogImpl win;
	win.show(); 
	app.connect( &app, SIGNAL( lastWindowClosed() ), &app, SLOT( quit() ) );
	return app.exec();
}
