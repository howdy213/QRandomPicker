#include "mainwindow.h"
#include "historydialog.h"
#include "miniwindow.h"
#include "newcopydialog.h"
#include "session.h"
#include "sessionmanager.h"
#include "sessionpickerdialog.h"
#include "tabpage.h"
#include <QAction>
#include <QActionGroup>
#include <QCloseEvent>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QJsonDocument>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMenuBar>
#include <QMenu>
#include <QMessageBox>
#include <QStatusBar>
#include <QTabWidget>
#include <QVBoxLayout>
#include <QWidget>

SessionManager *g_sessionManager = nullptr;

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , m_sessionManager(new SessionManager(this))
    , m_fileWatcher(new QFileSystemWatcher(this)) {
    g_sessionManager = m_sessionManager;
    initUI();
    initMenu();

    // 监听 sessions 文件夹
    QString sessDir = m_sessionManager->sessionsDir();
    QDir().mkpath(sessDir);
    m_fileWatcher->addPath(sessDir);
    connect(m_fileWatcher, &QFileSystemWatcher::directoryChanged, this, &MainWindow::onRefreshSessions);

    // 连接 SessionManager 信号
    connect(m_sessionManager, &SessionManager::sessionCreated, this, &MainWindow::onSessionCreated);
    connect(m_sessionManager, &SessionManager::sessionDeleted, this, &MainWindow::onSessionDeleted);
    connect(m_sessionManager, &SessionManager::sessionChanged, this, &MainWindow::onSessionChanged);

    // 启动时加载所有 session
    m_sessionManager->loadAll();
    for (auto *session : m_sessionManager->allSessions()) {
        createTabPage(session, false);
    }
    reorderTabs();
    if (m_tabWidget->count() > 0)
        m_tabWidget->setCurrentIndex(0);

    updateActionsState();
}

MainWindow::~MainWindow() {
    g_sessionManager = nullptr;
}

void MainWindow::closeEvent(QCloseEvent *event) {
    m_sessionManager->saveAll();
    event->accept();
}

void MainWindow::initUI() {
    setWindowTitle("随机点名");
    setMinimumSize(600, 400);
    resize(1000, 700);

    auto *centralWidget = new QWidget(this);
    auto *centralLayout = new QVBoxLayout(centralWidget);
    centralLayout->setContentsMargins(0, 0, 0, 0);
    centralLayout->setSpacing(0);
    setCentralWidget(centralWidget);

    m_tabWidget = new QTabWidget(this);
    m_tabWidget->setTabsClosable(true);
    m_tabWidget->setDocumentMode(true);
    centralLayout->addWidget(m_tabWidget);

    connect(m_tabWidget, &QTabWidget::tabCloseRequested, this, [this](int index) {
        if (auto *page = qobject_cast<TabPage *>(m_tabWidget->widget(index))) {
            onCloseTab(page);
        }
    });

    connect(m_tabWidget, &QTabWidget::currentChanged, this, &MainWindow::onCurrentTabChanged);

    // 状态栏
    m_lblStatus = new QLabel("就绪", this);
    statusBar()->addWidget(m_lblStatus);
}

void MainWindow::initMenu() {
    QMenuBar *menuBar = new QMenuBar(this);
    setMenuBar(menuBar);

    // === 文件菜单 ===
    QMenu *menuFile = new QMenu("文件", this);
    menuBar->addMenu(menuFile);

    m_actNew = new QAction("新建会话", this);
    m_actNew->setShortcut(QKeySequence("Ctrl+N"));
    menuFile->addAction(m_actNew);
    connect(m_actNew, &QAction::triggered, this, &MainWindow::onNewSession);

    m_actCopy = new QAction("复制会话", this);
    m_actCopy->setShortcut(QKeySequence("Ctrl+Shift+C"));
    menuFile->addAction(m_actCopy);
    connect(m_actCopy, &QAction::triggered, this, &MainWindow::onCopySession);

    menuFile->addSeparator();

    m_actOpen = new QAction("打开会话...", this);
    m_actOpen->setShortcut(QKeySequence("Ctrl+O"));
    menuFile->addAction(m_actOpen);
    connect(m_actOpen, &QAction::triggered, this, &MainWindow::onOpenSession);

    m_actOpenAll = new QAction("打开所有会话", this);
    m_actOpenAll->setShortcut(QKeySequence("Ctrl+Shift+O"));
    menuFile->addAction(m_actOpenAll);
    connect(m_actOpenAll, &QAction::triggered, this, &MainWindow::onOpenAllSessions);

    menuFile->addSeparator();

    m_actSaveCurrent = new QAction("保存当前会话", this);
    m_actSaveCurrent->setShortcut(QKeySequence("Ctrl+S"));
    menuFile->addAction(m_actSaveCurrent);
    connect(m_actSaveCurrent, &QAction::triggered, this, &MainWindow::onSaveCurrentSession);

    m_actSaveAll = new QAction("保存所有会话", this);
    m_actSaveAll->setShortcut(QKeySequence("Ctrl+Shift+S"));
    menuFile->addAction(m_actSaveAll);
    connect(m_actSaveAll, &QAction::triggered, this, &MainWindow::onSaveAllSessions);

    menuFile->addSeparator();

    m_actCloseCurrent = new QAction("关闭当前会话", this);
    m_actCloseCurrent->setShortcut(QKeySequence("Ctrl+W"));
    menuFile->addAction(m_actCloseCurrent);
    connect(m_actCloseCurrent, &QAction::triggered, this, &MainWindow::onCloseCurrentSession);

    m_actCloseAll = new QAction("关闭所有会话", this);
    m_actCloseAll->setShortcut(QKeySequence("Ctrl+Shift+W"));
    menuFile->addAction(m_actCloseAll);
    connect(m_actCloseAll, &QAction::triggered, this, &MainWindow::onCloseAllSessions);

    menuFile->addSeparator();

    m_actDelete = new QAction("删除会话...", this);
    menuFile->addAction(m_actDelete);
    connect(m_actDelete, &QAction::triggered, this, &MainWindow::onDeleteSession);

    menuFile->addSeparator();

    m_actRefresh = new QAction("刷新会话列表", this);
    m_actRefresh->setShortcut(QKeySequence("F5"));
    menuFile->addAction(m_actRefresh);
    connect(m_actRefresh, &QAction::triggered, this, &MainWindow::onRefreshSessions);

    menuFile->addSeparator();

    QAction *actExit = new QAction("退出", this);
    actExit->setShortcut(QKeySequence("Ctrl+Q"));
    menuFile->addAction(actExit);
    connect(actExit, &QAction::triggered, this, &QMainWindow::close);

    // === 会话菜单 ===
    QMenu *menuSession = new QMenu("会话", this);
    menuBar->addMenu(menuSession);

    m_actEditNames = new QAction("编辑名单...", this);
    m_actEditNames->setShortcut(QKeySequence("Ctrl+E"));
    menuSession->addAction(m_actEditNames);
    connect(m_actEditNames, &QAction::triggered, this, &MainWindow::onEditNames);

    menuSession->addSeparator();

    m_actTogglePickMode = new QAction("切换抽取模式", this);
    m_actTogglePickMode->setShortcut(QKeySequence("Ctrl+M"));
    menuSession->addAction(m_actTogglePickMode);
    connect(m_actTogglePickMode, &QAction::triggered, this, &MainWindow::onTogglePickMode);

    m_actResetFairMode = new QAction("重置抽取记录", this);
    menuSession->addAction(m_actResetFairMode);
    connect(m_actResetFairMode, &QAction::triggered, this, &MainWindow::onResetFairMode);

    m_actShowHistory = new QAction("历史记录...", this);
    m_actShowHistory->setShortcut(QKeySequence("Ctrl+H"));
    menuSession->addAction(m_actShowHistory);
    connect(m_actShowHistory, &QAction::triggered, this, &MainWindow::onShowHistory);

    menuSession->addSeparator();

    m_actTogglePinned = new QAction("置顶此会话", this);
    m_actTogglePinned->setCheckable(true);
    menuSession->addAction(m_actTogglePinned);
    connect(m_actTogglePinned, &QAction::triggered, this, &MainWindow::onTogglePinned);

    // === 视图菜单 ===
    QMenu *menuView = new QMenu("视图", this);
    menuBar->addMenu(menuView);

    m_viewModeGroup = new QActionGroup(this);
    m_viewModeGroup->setExclusive(true);

    m_actViewSimple = new QAction("简洁模式", this);
    m_actViewSimple->setShortcut(QKeySequence("Ctrl+1"));
    m_actViewSimple->setCheckable(true);
    m_actViewSimple->setActionGroup(m_viewModeGroup);
    menuView->addAction(m_actViewSimple);
    connect(m_actViewSimple, &QAction::triggered, this, &MainWindow::onSwitchViewMode);

    m_actViewNormal = new QAction("正常模式", this);
    m_actViewNormal->setShortcut(QKeySequence("Ctrl+2"));
    m_actViewNormal->setCheckable(true);
    m_actViewNormal->setActionGroup(m_viewModeGroup);
    m_actViewNormal->setChecked(true);
    menuView->addAction(m_actViewNormal);
    connect(m_actViewNormal, &QAction::triggered, this, &MainWindow::onSwitchViewMode);

    m_actViewAdvanced = new QAction("高级模式", this);
    m_actViewAdvanced->setShortcut(QKeySequence("Ctrl+3"));
    m_actViewAdvanced->setCheckable(true);
    m_actViewAdvanced->setActionGroup(m_viewModeGroup);
    menuView->addAction(m_actViewAdvanced);
    connect(m_actViewAdvanced, &QAction::triggered, this, &MainWindow::onSwitchViewMode);

    menuView->addSeparator();

    m_actViewMini = new QAction("小窗模式", this);
    m_actViewMini->setShortcut(QKeySequence("Ctrl+4"));
    menuView->addAction(m_actViewMini);
    connect(m_actViewMini, &QAction::triggered, this, [this]() {
        auto *page = currentTabPage();
        if (page && page->session()) {
            // 打开小窗对话框
            showMiniWindow(page->session());
        }
    });

    menuView->addSeparator();

    m_actAlwaysOnTop = new QAction("窗口始终置顶", this);
    m_actAlwaysOnTop->setCheckable(true);
    menuView->addAction(m_actAlwaysOnTop);
    connect(m_actAlwaysOnTop, &QAction::toggled, this, &MainWindow::onAlwaysOnTopToggled);
}

// ============== 工具函数 ==============

TabPage *MainWindow::currentTabPage() const {
    return qobject_cast<TabPage *>(m_tabWidget->currentWidget());
}

TabPage *MainWindow::findTabPageBySession(Session *session) {
    for (int i = 0; i < m_tabWidget->count(); ++i) {
        if (auto *page = qobject_cast<TabPage *>(m_tabWidget->widget(i))) {
            if (page->session() == session)
                return page;
        }
    }
    return nullptr;
}

TabPage *MainWindow::findTabPageById(const QString &id) {
    for (int i = 0; i < m_tabWidget->count(); ++i) {
        if (auto *page = qobject_cast<TabPage *>(m_tabWidget->widget(i))) {
            if (page->session() && page->session()->id() == id)
                return page;
        }
    }
    return nullptr;
}

TabPage *MainWindow::createTabPage(Session *session, bool autoReorder) {
    if (!session)
        return nullptr;
    if (auto *existing = findTabPageBySession(session)) {
        m_tabWidget->setCurrentWidget(existing);
        if (autoReorder)
            reorderTabs();
        return existing;
    }

    auto *newPage = new TabPage(session, this);
    m_tabWidget->addTab(newPage, session->name());
    updateTabTitle(newPage);
    m_tabWidget->setCurrentWidget(newPage);

    connect(newPage, &TabPage::closeRequested, this, &MainWindow::onCloseTab);
    connect(newPage, &TabPage::viewModeChanged, this, [this](ViewMode mode) {
        updateViewModeActions(mode);
        adjustWindowSize(mode);
    });
    connect(newPage, &TabPage::pinnedChanged, this, &MainWindow::onTabPinnedChanged);
    connect(newPage, &TabPage::statusMessage, this, &MainWindow::onTabPageStatusMessage);

    updateActionsState();

    if (autoReorder)
        reorderTabs();
    return newPage;
}

void MainWindow::reorderTabs() {
    QList<TabPage *> pinned;
    QList<TabPage *> unpinned;
    for (int i = 0; i < m_tabWidget->count(); ++i) {
        if (auto *page = qobject_cast<TabPage *>(m_tabWidget->widget(i))) {
            if (page->session() && page->session()->pinned())
                pinned.append(page);
            else
                unpinned.append(page);
        }
    }
    QList<TabPage *> ordered = pinned + unpinned;
    TabPage *currentPage = currentTabPage();

    for (int targetIdx = 0; targetIdx < ordered.size(); ++targetIdx) {
        TabPage *page = ordered[targetIdx];
        int curIdx = m_tabWidget->indexOf(page);
        if (curIdx != targetIdx) {
            QString text = m_tabWidget->tabText(curIdx);
            m_tabWidget->removeTab(curIdx);
            m_tabWidget->insertTab(targetIdx, page, text);
        }
        updateTabTitle(page);
    }

    if (currentPage) {
        int idx = m_tabWidget->indexOf(currentPage);
        if (idx >= 0)
            m_tabWidget->setCurrentIndex(idx);
    }
}

void MainWindow::updateTabTitle(TabPage *page) {
    if (!page || !page->session())
        return;
    int idx = m_tabWidget->indexOf(page);
    if (idx < 0)
        return;
    QString title = page->session()->name();
    if (page->session()->pinned())
        title = "📌 " + title;
    m_tabWidget->setTabText(idx, title);
}

void MainWindow::updateViewModeActions(ViewMode mode) {
    switch (mode) {
    case ViewMode::Simple:
        m_actViewSimple->setChecked(true);
        break;
    case ViewMode::Normal:
        m_actViewNormal->setChecked(true);
        break;
    case ViewMode::Advanced:
        m_actViewAdvanced->setChecked(true);
        break;
    }
}

void MainWindow::adjustWindowSize(ViewMode mode) {
    switch (mode) {
    case ViewMode::Simple:
        setMinimumSize(350, 250);
        if (width() > 500)
            resize(450, 350);
        break;
    case ViewMode::Normal:
        setMinimumSize(600, 400);
        if (width() < 600 || height() < 400)
            resize(1000, 700);
        break;
    case ViewMode::Advanced:
        setMinimumSize(800, 600);
        if (height() < 600)
            resize(1000, 800);
        break;
    }
}

void MainWindow::updateActionsState() {
    bool hasTab = (m_tabWidget->count() > 0);
    bool hasSession = (currentTabPage() != nullptr);

    m_actCopy->setEnabled(hasTab);
    m_actOpenAll->setEnabled(true);
    m_actSaveCurrent->setEnabled(hasSession);
    m_actSaveAll->setEnabled(hasTab);
    m_actCloseCurrent->setEnabled(hasSession);
    m_actCloseAll->setEnabled(hasTab);
    m_actDelete->setEnabled(hasTab);
    m_actRefresh->setEnabled(true);

    m_actEditNames->setEnabled(hasSession);
    m_actTogglePickMode->setEnabled(hasSession);
    m_actResetFairMode->setEnabled(hasSession);
    m_actShowHistory->setEnabled(hasSession);
    m_actTogglePinned->setEnabled(hasSession);

    if (hasSession) {
        Session *s = currentTabPage()->session();
        if (s) {
            m_actTogglePinned->setChecked(s->pinned());
        }
    }
}

void MainWindow::showStatusMessage(const QString &msg, int timeout) {
    m_lblStatus->setText(msg);
    if (timeout > 0) {
        statusBar()->showMessage(msg, timeout);
    }
}

// ============== 文件菜单 slots ==============

void MainWindow::onNewSession() {
    NewCopyDialog dlg(true, m_sessionManager->allSessionNames(), this);
    if (dlg.exec() != QDialog::Accepted)
        return;
    QString name = dlg.getListName();
    if (name.isEmpty()) {
        showStatusMessage("会话名称不能为空");
        return;
    }
    if (m_sessionManager->allSessionNames().contains(name)) {
        QMessageBox::warning(this, "错误", "会话名称已存在！");
        return;
    }
    Session *session = m_sessionManager->createSession(name);
    createTabPage(session, true);
    showStatusMessage(QString("已创建会话：%1").arg(name));
}

void MainWindow::onCopySession() {
    QStringList existNames = m_sessionManager->allSessionNames();
    if (existNames.isEmpty()) {
        QMessageBox::information(this, "提示", "暂无可复制的会话！");
        return;
    }
    NewCopyDialog dlg(false, existNames, this);
    if (dlg.exec() != QDialog::Accepted)
        return;
    QString newName = dlg.getListName();
    QString sourceName = dlg.getSourceListName();
    if (newName.isEmpty())
        return;
    if (existNames.contains(newName)) {
        QMessageBox::warning(this, "错误", "目标会话名称已存在！");
        return;
    }
    for (auto *s : m_sessionManager->allSessions()) {
        if (s->name() == sourceName) {
            Session *newSession = m_sessionManager->copySession(s->id(), newName);
            createTabPage(newSession, true);
            showStatusMessage(QString("已复制会话：%1").arg(newName));
            break;
        }
    }
}

void MainWindow::onOpenSession() {
    // 先刷新一下，确保磁盘上的 session 都已加载到 manager
    m_sessionManager->refresh();

    // 收集磁盘上存在但未在 Tab 中打开的 session
    QStringList availableNames;
    QList<Session*> availableSessions;
    for (auto *s : m_sessionManager->allSessions()) {
        if (!findTabPageById(s->id())) {
            availableNames << s->name();
            availableSessions << s;
        }
    }

    if (availableNames.isEmpty()) {
        QMessageBox::information(this, "提示", "没有可打开的会话（所有会话都已打开或无会话存在）");
        return;
    }

    SessionPickerDialog dlg(availableNames, this);
    if (dlg.exec() != QDialog::Accepted)
        return;

    QStringList selected = dlg.selectedNames();
    if (selected.isEmpty())
        return;

    int openedCount = 0;
    for (const QString &name : std::as_const(selected)) {
        for (Session *s : std::as_const(availableSessions)) {
            if (s->name() == name) {
                createTabPage(s, false);
                ++openedCount;
                break;
            }
        }
    }
    reorderTabs();
    showStatusMessage(QString("已打开 %1 个会话").arg(openedCount));
}

void MainWindow::onOpenAllSessions() {
    // 调用 refresh 加载磁盘新增的
    auto result = m_sessionManager->refresh();
    for (auto *session : std::as_const(result.added)) {
        createTabPage(session, false);
    }
    for (auto *session : std::as_const(result.modified)) {
        if (auto *page = findTabPageBySession(session))
            page->loadFromSession();
    }
    reorderTabs();
    showStatusMessage(QString("已加载所有会话：%1 个").arg(m_tabWidget->count()));
}

void MainWindow::onSaveCurrentSession() {
    auto *page = currentTabPage();
    if (!page || !page->session())
        return;
    if (m_sessionManager->saveSession(page->session())) {
        showStatusMessage(QString("会话 \"%1\" 已保存").arg(page->session()->name()));
    } else {
        showStatusMessage("保存失败");
    }
}

void MainWindow::onSaveAllSessions() {
    m_sessionManager->saveAll();
    showStatusMessage(QString("已保存所有会话（%1 个）").arg(m_tabWidget->count()));
}

void MainWindow::onCloseCurrentSession() {
    auto *page = currentTabPage();
    if (!page)
        return;
    onCloseTab(page);
}

void MainWindow::onCloseAllSessions() {
    if (m_tabWidget->count() == 0)
        return;

    int ret = QMessageBox::question(this, "关闭所有会话",
                                     QString("是否保存并关闭所有 %1 个会话？\nYes 保存并关闭，No 不保存关闭，Cancel 取消")
                                         .arg(m_tabWidget->count()),
                                     QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel);
    if (ret == QMessageBox::Cancel)
        return;

    if (ret == QMessageBox::Yes) {
        m_sessionManager->saveAll();
    }

    while (m_tabWidget->count() > 0) {
        QWidget *page = m_tabWidget->widget(0);
        m_tabWidget->removeTab(0);
        page->deleteLater();
    }
    showStatusMessage("已关闭所有会话");
    updateActionsState();
}

void MainWindow::onDeleteSession() {
    QStringList names = m_sessionManager->allSessionNames();
    if (names.isEmpty()) {
        QMessageBox::information(this, "提示", "没有可删除的会话");
        return;
    }

    bool ok;
    QString name = QInputDialog::getItem(this, "删除会话", "选择要删除的会话：",
                                          names, 0, false, &ok);
    if (!ok || name.isEmpty())
        return;

    int ret = QMessageBox::question(this, "确认删除",
                                     QString("确定要删除会话 \"%1\" 吗？\n此操作会同时删除磁盘上的文件，且不可恢复！").arg(name),
                                     QMessageBox::Yes | QMessageBox::No);
    if (ret != QMessageBox::Yes)
        return;

    // 找到对应 id
    QString targetId;
    for (auto *s : m_sessionManager->allSessions()) {
        if (s->name() == name) {
            targetId = s->id();
            break;
        }
    }
    if (targetId.isEmpty())
        return;

    // 先移除 Tab
    if (auto *page = findTabPageById(targetId)) {
        int idx = m_tabWidget->indexOf(page);
        if (idx >= 0) {
            m_tabWidget->removeTab(idx);
            page->deleteLater();
        }
    }

    // 从 SessionManager 删除
    m_sessionManager->deleteSession(targetId);
    showStatusMessage(QString("已删除会话：%1").arg(name));
    updateActionsState();
}

void MainWindow::onRefreshSessions() {
    auto result = m_sessionManager->refresh();

    for (auto *session : std::as_const(result.added)) {
        createTabPage(session, false);
    }

    if (!result.deletedIds.isEmpty()) {
        QStringList names;
        for (const QString &id : std::as_const(result.deletedIds)) {
            if (auto *page = findTabPageById(id)) {
                if (page->session())
                    names.append(page->session()->name());
            }
        }
        if (!names.isEmpty()) {
            int ret = QMessageBox::question(this, "检测到会话文件被删除",
                                             QString("以下会话在磁盘上已被删除：\n  • %1\n\n"
                                                     "是否同时关闭对应的标签页？").arg(names.join("\n  • ")),
                                             QMessageBox::Yes | QMessageBox::No);
            if (ret == QMessageBox::Yes) {
                for (const QString &id : std::as_const(result.deletedIds)) {
                    if (auto *page = findTabPageById(id)) {
                        int idx = m_tabWidget->indexOf(page);
                        if (idx >= 0) {
                            m_tabWidget->removeTab(idx);
                            page->deleteLater();
                        }
                    }
                    m_sessionManager->deleteSession(id);
                }
            }
        }
    }

    for (auto *session : std::as_const(result.modified)) {
        if (auto *page = findTabPageBySession(session))
            page->loadFromSession();
    }

    reorderTabs();
    updateActionsState();

    QString msg;
    if (!result.added.isEmpty() || !result.deletedIds.isEmpty() || !result.modified.isEmpty()) {
        msg = QString("刷新完成：新增 %1，删除 %2，更新 %3")
                  .arg(result.added.size()).arg(result.deletedIds.size()).arg(result.modified.size());
    } else {
        msg = "已是最新，无变化";
    }
    showStatusMessage(msg);
}

// ============== 会话菜单 slots ==============

void MainWindow::onEditNames() {
    auto *page = currentTabPage();
    if (!page)
        return;
    page->editNames();
}

void MainWindow::onTogglePickMode() {
    auto *page = currentTabPage();
    if (!page || !page->session())
        return;
    Session *s = page->session();
    s->setPickMode(s->pickMode() == PickMode::Random ? PickMode::Fair : PickMode::Random);
    showStatusMessage(QString("已切换为：%1 模式")
                          .arg(s->pickMode() == PickMode::Random ? "随机" : "公平"));
}

void MainWindow::onResetFairMode() {
    auto *page = currentTabPage();
    if (!page || !page->session())
        return;
    page->session()->resetFairPick();
    showStatusMessage("公平模式已重置");
}

void MainWindow::onShowHistory() {
    auto *page = currentTabPage();
    if (!page || !page->session())
        return;
    Session *s = page->session();
    HistoryDialog dlg(s->history(), this);
    if (dlg.exec() == QDialog::Accepted && dlg.clearRequested()) {
        s->clearHistory();
        // 通知 TabPage 刷新其底部历史显示
        page->loadFromSession();
        showStatusMessage("抽取历史已清空");
    }
}

void MainWindow::onTogglePinned() {
    auto *page = currentTabPage();
    if (!page || !page->session())
        return;
    bool newPinned = !page->session()->pinned();
    page->session()->setPinned(newPinned);
    updateTabTitle(page);
    reorderTabs();
    if (g_sessionManager)
        g_sessionManager->saveSession(page->session());
    showStatusMessage(QString("会话%1置顶").arg(newPinned ? "已" : "已取消"));
}

// ============== 视图菜单 slots ==============

void MainWindow::onSwitchViewMode() {
    auto *page = currentTabPage();
    if (!page)
        return;

    ViewMode mode = ViewMode::Normal;
    if (m_actViewSimple->isChecked())
        mode = ViewMode::Simple;
    else if (m_actViewAdvanced->isChecked())
        mode = ViewMode::Advanced;

    page->setViewMode(mode);
    adjustWindowSize(mode);
}

void MainWindow::onAlwaysOnTopToggled(bool checked) {
    Qt::WindowFlags flags = windowFlags();
    if (checked)
        flags |= Qt::WindowStaysOnTopHint;
    else
        flags &= ~Qt::WindowStaysOnTopHint;
    setWindowFlags(flags);
    show();
}

// ============== 内部 slots ==============

void MainWindow::onCloseTab(TabPage *page) {
    if (!page)
        return;
    auto *session = page->session();
    int ret = QMessageBox::question(this, "关闭会话",
                                     QString("是否保存并关闭会话 \"%1\"？\nYes 保存并关闭，No 仅关闭标签页，Cancel 取消")
                                         .arg(session ? session->name() : ""),
                                     QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel);
    if (ret == QMessageBox::Cancel)
        return;
    if (ret == QMessageBox::Yes && session)
        m_sessionManager->saveSession(session);

    int index = m_tabWidget->indexOf(page);
    if (index >= 0) {
        m_tabWidget->removeTab(index);
        page->deleteLater();
    }
    updateActionsState();
}

void MainWindow::onTabPinnedChanged(TabPage *page) {
    reorderTabs();
    if (page)
        updateTabTitle(page);
    if (page && page->session() && g_sessionManager) {
        g_sessionManager->saveSession(page->session());
    }
    updateActionsState();
}

void MainWindow::onSessionCreated(Session *session) {
    if (!findTabPageBySession(session)) {
        createTabPage(session);
    }
}

void MainWindow::onSessionDeleted(const QString &id) {
    for (int i = m_tabWidget->count() - 1; i >= 0; --i) {
        if (auto *page = qobject_cast<TabPage *>(m_tabWidget->widget(i))) {
            if (page->session() && page->session()->id() == id) {
                m_tabWidget->removeTab(i);
                page->deleteLater();
            }
        }
    }
    updateActionsState();
}

void MainWindow::onSessionChanged(Session *session) {
    if (!session)
        return;
    for (int i = 0; i < m_tabWidget->count(); ++i) {
        if (auto *page = qobject_cast<TabPage *>(m_tabWidget->widget(i))) {
            if (page->session() == session) {
                updateTabTitle(page);
                break;
            }
        }
    }
}

void MainWindow::onCurrentTabChanged(int index) {
    if (index < 0) {
        updateActionsState();
        return;
    }
    auto *page = qobject_cast<TabPage *>(m_tabWidget->widget(index));
    if (!page || !page->session())
        return;
    ViewMode mode = page->session()->viewMode();
    updateViewModeActions(mode);
    adjustWindowSize(mode);
    updateActionsState();
}

void MainWindow::onTabPageStatusMessage(const QString &msg) {
    showStatusMessage(msg);
}

void MainWindow::showMiniWindow(Session *session) {
    if (!session) return;
    hide();
    auto *dlg = new MiniWindow(session);
    dlg->setAttribute(Qt::WA_DeleteOnClose);
    dlg->setParent(nullptr);
    dlg->show();
    connect(dlg, &QDialog::finished, this, [this](int) {
        show();
        raise();
        activateWindow();
    });
}
