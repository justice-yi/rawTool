/* main.cpp - 应用入口（对应原 CWinApp InitInstance）*/
#include <QApplication>
#include <QTimer>
#include <QMetaObject>
#include "MainWindow.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName("ImageTool-Qt");
    app.setOrganizationName("ImageTool-Qt");

    MainWindow win;
    win.resize(960, 720);
    win.showMaximized();

    /* 命令行: ImageToolQt.exe [--analysis] file
     * --analysis: 打开文件后自动弹出直方图/统计对话框（自动化测试用）*/
    QString file;
    bool autoAnalysis = false;
    for (int i = 1; i < argc; i++) {
        QString a = QString::fromLocal8Bit(argv[i]);
        if (a == "--analysis")
            autoAnalysis = true;
        else
            file = a;
    }
    if (!file.isEmpty()) {
        const QString lower = file.toLower();
        const bool isRaw = lower.endsWith(".raw") || lower.endsWith(".byr") ||
                           lower.endsWith(".dat") || lower.endsWith(".sraw");
        const bool ok = isRaw ? win.loadRawFile(file) : win.loadImageFile(file);
        if (ok && autoAnalysis)
            QTimer::singleShot(300, &win, [&win]() {
                QMetaObject::invokeMethod(&win, "showAnalysis");
            });
    }
    return app.exec();
}
