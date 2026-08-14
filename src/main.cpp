#include "mainwindow.h"
#include <QApplication>
#include <QSettings>
#include <QStyleFactory>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    a.setOrganizationName("QRandomPicker");
    a.setApplicationName("QRandomPicker");

    MainWindow w;
    w.show();

    // 检查默认启动项配置
    QSettings settings;
    int startup = settings.value("startupMode", 0).toInt(); // 0=主界面, 1=小窗, 2=悬浮球
    if (startup == 1) {
        w.onStartMiniWindow();
    } else if (startup == 2) {
        w.onStartFloatingBall();
    }

    return a.exec();
}
