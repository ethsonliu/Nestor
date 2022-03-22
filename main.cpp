#include "main_window.h"

#include <QtSingleApplication>

int main(int argc, char *argv[])
{
    // log file
    plog::init(plog::debug, "./log.txt", 1024 * 1024 * 25, 1);

    LOG_INFO << APP_NAME << " " << APP_VERSION;

    // check for single running instance
    QtSingleApplication a(argc, argv);
    if (a.isRunning())
    {
        LOG_WARNING << "Another app is running, please close it firstly.";

        return 0;
    }

    MainWindow w;
    w.setStyleSheet("QWidget{font-family: 'Microsoft YaHei UI'; font-size: 13px;}"
                    "QGroupBox{font-size: 15px;}");
    w.show();

    return a.exec();
}
