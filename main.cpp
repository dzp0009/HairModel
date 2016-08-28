#include "HairLayers.h"
#include <QtGui/QApplication>

int main(int argc, char *argv[])
{
	QApplication a(argc, argv);
	HairLayers w;
	w.show();
	return a.exec();
}
