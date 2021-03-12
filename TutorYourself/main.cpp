#include <QApplication>
#include "tutoryourself.h"

int main(int argc, char *argv[])
{
	QApplication a(argc, argv);
	TutorYourself *tutorYourself = new TutorYourself;
	tutorYourself->show();

	return a.exec();
}
