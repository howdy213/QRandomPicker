#ifndef MINIWINDOW_H
#define MINIWINDOW_H

#include <QDialog>
#include <QFrame>
#include <QLabel>
#include <QPushButton>
#include <QTextBrowser>
#include <QStringList>

class Session;

class MiniWindow : public QDialog {
    Q_OBJECT
public:
    explicit MiniWindow(Session *session, QWidget *parent = nullptr);
    ~MiniWindow() override;

protected:
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void closeEvent(QCloseEvent *event) override;
    bool eventFilter(QObject *obj, QEvent *event) override;

private slots:
    void onBackToNormal();
    void onToggleAlwaysOnTop();
    void onMinimize();

private:
    void initUI();
    void createTitleBar();
    void syncFromSession();
    void updateDisplay();
    [[nodiscard]] QString buildResultHtml(const QStringList &names) const;
    void displayResult(const QStringList &names);

    Session *m_session;
    bool m_alwaysOnTop = false;

    // 自绘标题栏
    QFrame *m_titleBar;
    QLabel *m_lblTitle;
    QPushButton *m_btnBack;
    QPushButton *m_btnTop;
    QPushButton *m_btnMin;
    QPushButton *m_btnClose;

    // 内容区
    QTextBrowser *m_resultBrowser;
    QStringList m_lastPicked;
    QPushButton *m_btnPick;
    QLabel *m_lblCountValue;
    QPushButton *m_btnInc;
    QPushButton *m_btnDec;

    // 拖动
    QPoint m_dragStart;
    bool m_dragging = false;
};

#endif // MINIWINDOW_H
