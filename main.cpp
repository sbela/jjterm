#include "dialog.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    a.setStyle(QStyleFactory::create("Fusion"));
    a.setWindowIcon(QIcon(":/terminal32.png"));
    Dialog w;
    w.setWindowFlags(Qt::Window);
    w.show();

    return a.exec();
}
