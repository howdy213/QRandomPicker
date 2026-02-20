#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QFileSystemWatcher>

class QTabWidget;
class QMenuBar;
class QMenu;
class QAction;

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

private slots:
    void onNewList();
    void onCopyList();
    void onRefreshLists();
    void onCloseTab(class TabPage *page);

private:
    void initUI();
    void initMenu();
    void createTabPage(const QString &listName);
    void extracted(QStringList &files, QStringList &lists);
    QStringList getExistLists();
    bool copyList(const QString &source, const QString &dest);

    // 声明顺序：m_dataPath → m_fileWatcher → m_tabWidget
    QString m_dataPath;
    QFileSystemWatcher *m_fileWatcher;
    QTabWidget *m_tabWidget;
};

#endif // MAINWINDOW_H
