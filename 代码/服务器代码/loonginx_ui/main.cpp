#include "widget.h"
#include <QApplication>
#include "backstorg.h"
int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    Widget w;
    w.show();
//    backstorg back;
//    back.show();
    return a.exec();
}
