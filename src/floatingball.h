#ifndef FLOATINGBALL_H
#define FLOATINGBALL_H

#include <QWidget>
#include <QPushButton>
#include <QPointer>

class Session;
class PickPopup;

class FloatingBall : public QWidget {
    Q_OBJECT
public:
    explicit FloatingBall(Session *session, QWidget *parent = nullptr);
    ~FloatingBall() override;

    void setSession(Session *session);

protected:
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void closeEvent(QCloseEvent *event) override;
    bool eventFilter(QObject *obj, QEvent *event) override;

signals:
    void returnToMain();

private slots:
    void onPickClicked();
    void onReturnClicked();

private:
    void clampToScreen(QPoint &pos) const;
    void savePosition() const;
    void loadPosition();

    Session *m_session;
    QPushButton *m_btnPick;
    QPushButton *m_btnReturn;
    QPointer<PickPopup> m_popup;

    // 拖动
    QPoint m_dragStart;
    bool m_dragging = false;
    bool m_moved = false;
    QPoint m_pressGlobalPos;
    QWidget *m_pressWidget = nullptr;
};

#endif // FLOATINGBALL_H
