#ifndef TABPAGE_H
#define TABPAGE_H

#include "session.h"
#include <QCheckBox>
#include <QFrame>
#include <QLabel>
#include <QListWidget>
#include <QPushButton>
#include <QRadioButton>
#include <QTextBrowser>
#include <QWidget>

class Session;

class TabPage : public QWidget {
    Q_OBJECT
public:
    explicit TabPage(Session *session, QWidget *parent = nullptr);
    ~TabPage() override;

    [[nodiscard]] Session *session() const { return m_session; }
    [[nodiscard]] QString listName() const { return m_session ? m_session->name() : QString(); }
    void loadFromSession();
    void setViewMode(ViewMode mode);
    void editNames();   // 打开名单编辑窗口
    void saveSession(); // 保存当前 session

signals:
    void closeRequested(TabPage *);
    void viewModeChanged(ViewMode mode);
    void pinnedChanged(TabPage *page);
    void requestSaveSession(TabPage *page);
    void statusMessage(const QString &msg);

private slots:
    void onPickClicked();
    void onToggleModeClicked();
    void onDecreasePickCount();
    void onIncreasePickCount();
    void onBigDecreasePickCount();
    void onBigIncreasePickCount();
    void onClearHistoryClicked();
    void onPinnedToggled(bool checked);

private:
    void initUI();
    void applyViewMode(ViewMode mode);
    void updateNameListColors();
    void updateHistoryPanel();
    void updateStatsPanel();
    void updateControlsState();
    void updateViewModeButtons();
    [[nodiscard]] QString buildResultHtml(const QStringList &names) const;
    void displayResult(const QStringList &names);
    void syncNamesToSession();

protected:
    bool eventFilter(QObject *obj, QEvent *event) override;

private:
    Session *m_session;
    QStringList m_lastPicked;

    static constexpr int RESULT_WIDTH = 680;
    static constexpr int RESULT_HEIGHT = 480;
    static constexpr int FONT_SIZE = 22;

    // 左面板（名单显示）
    QWidget *m_leftPanel;
    QTextBrowser *m_resultBrowser;
    QListWidget *m_listNames;
    QLabel *m_lblCount;
    QPushButton *m_btnToggleMode;

    // 左右面板之间的分隔线
    QFrame *m_vLine;

    // 右面板（控制）
    QWidget *m_rightPanel;
    QLabel *m_lblPickCount;
    QPushButton *m_btnDecrease;
    QPushButton *m_btnBigDecrease;
    QPushButton *m_btnIncrease;
    QPushButton *m_btnBigIncrease;
    QLabel *m_lblPickCountValue;
    QPushButton *m_btnPick;

    // 高级模式底部
    QWidget *m_bottomPanel;
    QListWidget *m_historyList;
    QLabel *m_lblTotalPicks;
    QLabel *m_lblFairRemaining;
    QLabel *m_lblTotalNames;
    QPushButton *m_btnClearHistory;
};

#endif // TABPAGE_H
