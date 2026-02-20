#include "mainwindow.h"
#include "tabpage.h"
#include "newcopydialog.h"
#include <QTabWidget>
#include <QMenuBar>
#include <QMenu>
#include <QAction>
#include <QVBoxLayout>
#include <QDir>
#include <QFile>
#include <QMessageBox>
#include <QFileInfo>
#include <QKeySequence>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , m_dataPath(QDir::currentPath() + "/data")
    , m_fileWatcher(new QFileSystemWatcher(this))
{
    initUI();
    initMenu();

    // 初始化data文件夹
    QDir dir(m_dataPath);
    if (!dir.exists()) dir.mkpath(".");

    // 监听data文件夹变化
    m_fileWatcher->addPath(m_dataPath);
    connect(m_fileWatcher, &QFileSystemWatcher::directoryChanged, this, &MainWindow::onRefreshLists);

    // 启动时加载所有名单
    onRefreshLists();
    QStringList existLists = getExistLists();
    for (const QString &listName : existLists) {
        createTabPage(listName);
    }
}

MainWindow::~MainWindow() = default;

void MainWindow::initUI()
{
    setWindowTitle("随机点名");
    setMinimumSize(1000, 700);

    QWidget *centralWidget = new QWidget(this);
    QVBoxLayout *centralLayout = new QVBoxLayout(centralWidget);
    centralLayout->setContentsMargins(0, 0, 0, 0);
    setCentralWidget(centralWidget);

    m_tabWidget = new QTabWidget(this);
    m_tabWidget->setTabsClosable(true);
    centralLayout->addWidget(m_tabWidget);

    // Tab关闭信号
    connect(m_tabWidget, &QTabWidget::tabCloseRequested, this, [this](int index) {
        QWidget *page = m_tabWidget->widget(index);
        if (auto tabPage = qobject_cast<TabPage*>(page)) {
            onCloseTab(tabPage);
        }
    });
}

void MainWindow::initMenu()
{
    QMenuBar *menuBar = new QMenuBar(this);
    setMenuBar(menuBar);

    QMenu *menuFile = new QMenu("文件", this);
    menuBar->addMenu(menuFile);

    QAction *actNew = new QAction("新建名单", this);
    actNew->setShortcut(QKeySequence("Ctrl+N"));
    menuFile->addAction(actNew);
    connect(actNew, &QAction::triggered, this, &MainWindow::onNewList);

    QAction *actCopy = new QAction("复制名单", this);
    actCopy->setShortcut(QKeySequence("Ctrl+C"));
    menuFile->addAction(actCopy);
    connect(actCopy, &QAction::triggered, this, &MainWindow::onCopyList);

    QAction *actRefresh = new QAction("刷新名单", this);
    actRefresh->setShortcut(QKeySequence("F5"));
    menuFile->addAction(actRefresh);
    connect(actRefresh, &QAction::triggered, this, &MainWindow::onRefreshLists);
}

void MainWindow::createTabPage(const QString &listName)
{
    // 检查是否已存在
    for (int i = 0; i < m_tabWidget->count(); ++i) {
        if (auto page = qobject_cast<TabPage*>(m_tabWidget->widget(i))) {
            if (page->listName() == listName) {
                m_tabWidget->setCurrentIndex(i);
                return;
            }
        }
    }

    TabPage *newPage = new TabPage(listName, this);
    m_tabWidget->addTab(newPage, listName);
    m_tabWidget->setCurrentWidget(newPage);

    connect(newPage, &TabPage::closeRequested, this, &MainWindow::onCloseTab);
}

QStringList MainWindow::getExistLists()
{
    QDir dir(m_dataPath);
    QStringList filters;
    filters << "*.txt";
    dir.setNameFilters(filters);
    QStringList files = dir.entryList(QDir::Files | QDir::NoDotAndDotDot);

    QStringList lists;
    for (const QString &file : std::as_const( files)) {
        lists.append(file.left(file.lastIndexOf(".")));
    }
    return lists;
}

bool MainWindow::copyList(const QString &source, const QString &dest)
{
    QString srcPath = m_dataPath + "/" + source + ".txt";
    QString destPath = m_dataPath + "/" + dest + ".txt";

    if (QFile::exists(destPath)) {
        QMessageBox::warning(this, "错误", "目标名单已存在！");
        return false;
    }

    return QFile::copy(srcPath, destPath);
}

void MainWindow::onNewList()
{
    NewCopyDialog dlg(true, getExistLists(), this);
    if (dlg.exec() == QDialog::Accepted) {
        QString listName = dlg.getListName();
        QString filePath = m_dataPath + "/" + listName + ".txt";
        QFile file(filePath);
        if (file.open(QIODevice::WriteOnly)) file.close();
        createTabPage(listName);
    }
}

void MainWindow::onCopyList()
{
    QStringList existLists = getExistLists();
    if (existLists.isEmpty()) {
        QMessageBox::warning(this, "提示", "暂无可复制的名单！");
        return;
    }

    NewCopyDialog dlg(false, existLists, this);
    if (dlg.exec() == QDialog::Accepted) {
        QString newName = dlg.getListName();
        QString sourceName = dlg.getSourceListName();
        if (copyList(sourceName, newName)) {
            createTabPage(newName);
        }
    }
}

void MainWindow::onRefreshLists()
{
    QStringList latestLists = getExistLists();
    QStringList openedLists;
    for (int i = 0; i < m_tabWidget->count(); ++i) {
        if (auto page = qobject_cast<TabPage*>(m_tabWidget->widget(i))) {
            openedLists.append(page->listName());
        }
    }

    // 处理已删除的名单
    QStringList deletedLists;
    for (const QString &list : openedLists) {
        if (!latestLists.contains(list)) {
            deletedLists.append(list);
        }
    }
    if (!deletedLists.isEmpty()) {
        QString msg = "检测到以下名单文件已被删除，是否关闭对应的Tab页？\n";
        msg += deletedLists.join("\n");
        int ret = QMessageBox::question(this, "刷新提示", msg,
                                        QMessageBox::Yes | QMessageBox::No);
        if (ret == QMessageBox::Yes) {
            for (int i = m_tabWidget->count() - 1; i >= 0; --i) {
                if (auto page = qobject_cast<TabPage*>(m_tabWidget->widget(i))) {
                    if (deletedLists.contains(page->listName())) {
                        m_tabWidget->removeTab(i);
                        page->deleteLater();
                    }
                }
            }
        }
    }

    // 打开未打开的名单
    for (const QString &listName : std::as_const(latestLists)) {
        if (!openedLists.contains(listName)) {
            createTabPage(listName);
        }
    }

    // 刷新已打开的名单
    for (int i = 0; i < m_tabWidget->count(); ++i) {
        if (auto page = qobject_cast<TabPage*>(m_tabWidget->widget(i))) {
            page->loadNameList();
        }
    }

    // QMessageBox::information(this, "刷新完成",
    //                          QString("名单列表已同步！\n- 共加载 %1 个名单\n- 已刷新所有Tab页数据")
    //                              .arg(latestLists.size()));
}

void MainWindow::onCloseTab(TabPage *page)
{
    int index = m_tabWidget->indexOf(page);
    if (index >= 0) {
        m_tabWidget->removeTab(index);
        page->deleteLater();
    }
}
