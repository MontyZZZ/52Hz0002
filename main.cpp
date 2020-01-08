#include "DownloadWidget.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    DownloadWidget w;
    w.show();
    return a.exec();
}
