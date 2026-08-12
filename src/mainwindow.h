#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QFileSystemWatcher>
#include "session.h"

class SessionManager;
class Session;
class TabPage;
class MiniWindow;
class QTabWidget;
class QMenuBar;
class QMenu;
class QAction;
class QActionGroup;
class QLabel;

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

protected:
    void closeEvent(QCloseEvent *event) override;

private slots:
    // 文件菜单
    void onNewSession();
    void onCopySession();
    void onOpenSession();
    void onOpenAllSessions();
    void onSaveCurrentSession();
    void onSaveAllSessions();
    void onCloseCurrentSession();
    void onCloseAllSessions();
    void onDeleteSession();
    void onRefreshSessions();
    // 会话菜单
    void onEditNames();
    void onTogglePickMode();
    void onResetFairMode();
    void onShowHistory();      // 打开历史记录窗口
    void onTogglePinned();
    // 视图菜单
    void onSwitchViewMode();
    void onAlwaysOnTopToggled(bool checked);
    // 内部
    void onCloseTab(TabPage *page);
    void onSessionCreated(Session *session);
    void onSessionDeleted(const QString &id);
    void onSessionChanged(Session *session);
    void onCurrentTabChanged(int index);
    void onTabPinnedChanged(TabPage *page);
    void onTabPageStatusMessage(const QString &msg);

private:
    void initUI();
    void initMenu();
    TabPage *createTabPage(Session *session, bool autoReorder = true);
    TabPage *findTabPageBySession(Session *session);
    TabPage *findTabPageById(const QString &id);
    TabPage *currentTabPage() const;
    void updateViewModeActions(ViewMode mode);
    void adjustWindowSize(ViewMode mode);
    void reorderTabs();
    void updateTabTitle(TabPage *page);
    void updateActionsState();
    void showStatusMessage(const QString &msg, int timeout = 3000);
    void showMiniWindow(Session *session);

    SessionManager *m_sessionManager;
    QFileSystemWatcher *m_fileWatcher;
    QTabWidget *m_tabWidget;
    QLabel *m_lblStatus;

    // 文件菜单
    QAction *m_actNew;
    QAction *m_actCopy;
    QAction *m_actOpen;
    QAction *m_actOpenAll;
    QAction *m_actSaveCurrent;
    QAction *m_actSaveAll;
    QAction *m_actCloseCurrent;
    QAction *m_actCloseAll;
    QAction *m_actDelete;
    QAction *m_actRefresh;

    // 会话菜单
    QAction *m_actEditNames;
    QAction *m_actTogglePickMode;
    QAction *m_actResetFairMode;
    QAction *m_actShowHistory;
    QAction *m_actTogglePinned;

    // 视图菜单
    QActionGroup *m_viewModeGroup;
    QAction *m_actViewSimple;
    QAction *m_actViewNormal;
    QAction *m_actViewAdvanced;
    QAction *m_actViewMini;
    QAction *m_actAlwaysOnTop;
};

#endif // MAINWINDOW_H
