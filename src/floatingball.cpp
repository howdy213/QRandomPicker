#include "floatingball.h"
#include "pickpopup.h"
#include "session.h"
#include <QApplication>
#include <QEvent>
#include <QMouseEvent>
#include <QScreen>
#include <QSettings>
#include <QVBoxLayout>

static constexpr int DRAG_THRESHOLD = 5; // 拖动判定阈值（像素）

// 按钮尺寸
static constexpr int PICK_BTN_SIZE = 48;   // 抽取按钮：正方形
static constexpr int RET_BTN_W = 48;      // 返回按钮宽度
static constexpr int RET_BTN_H = 28;      // 返回按钮高度
static constexpr int MARGIN = 4;           // 外边距
static constexpr int GAP = 8;             // 按钮间隙

FloatingBall::FloatingBall(Session *session, QWidget *parent)
    : QWidget(parent), m_session(session) {
    setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::Tool);
    setAttribute(Qt::WA_TranslucentBackground, true);

    int totalW = PICK_BTN_SIZE + MARGIN * 2;
    int totalH = PICK_BTN_SIZE + RET_BTN_H + GAP + MARGIN * 2;
    setFixedSize(totalW, totalH);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(MARGIN, MARGIN, MARGIN, MARGIN);
    layout->setSpacing(GAP);

    m_btnPick = new QPushButton("抽取", this);
    m_btnPick->setFixedSize(PICK_BTN_SIZE, PICK_BTN_SIZE);
    m_btnPick->setStyleSheet(
        "QPushButton {"
        "  border-radius: 6px;"
        "  background-color: #4CAF50;"
        "  color: white;"
        "  font-size: 13px;"
        "  font-weight: bold;"
        "  border: 2px solid #45a049;"
        "}"
        "QPushButton:hover { background-color: #45a049; }"
        "QPushButton:pressed { background-color: #3d8b40; }");

    m_btnReturn = new QPushButton("返回", this);
    m_btnReturn->setFixedSize(RET_BTN_W, RET_BTN_H);
    m_btnReturn->setStyleSheet(
        "QPushButton {"
        "  border-radius: 4px;"
        "  background-color: #2196F3;"
        "  color: white;"
        "  font-size: 11px;"
        "  border: 2px solid #1976D2;"
        "}"
        "QPushButton:hover { background-color: #1976D2; }"
        "QPushButton:pressed { background-color: #1565C0; }");

    layout->addWidget(m_btnPick);
    layout->addWidget(m_btnReturn, 0, Qt::AlignHCenter);

    connect(m_btnPick, &QPushButton::clicked, this, &FloatingBall::onPickClicked);
    connect(m_btnReturn, &QPushButton::clicked, this, &FloatingBall::onReturnClicked);

    // 给按钮安装事件过滤器，这样按钮区域也能触发拖动
    m_btnPick->installEventFilter(this);
    m_btnReturn->installEventFilter(this);

    // 加载上次位置
    loadPosition();
}

FloatingBall::~FloatingBall() = default;

void FloatingBall::setSession(Session *session) {
    m_session = session;
}

void FloatingBall::onPickClicked() {
    if (m_moved) return; // 拖动后不触发点击
    if (!m_session) return;
    QStringList picked = m_session->pickNames(m_session->pickCount());
    if (picked.isEmpty()) return;

    if (m_popup) {
        m_popup->close();
        m_popup.clear();
    }
    auto *popup = new PickPopup(nullptr);
    popup->setAttribute(Qt::WA_DeleteOnClose);
    popup->showNames(picked);
    m_popup = popup;
}

void FloatingBall::onReturnClicked() {
    if (m_moved) return; // 拖动后不触发点击
    emit returnToMain();
    close();
}

void FloatingBall::clampToScreen(QPoint &pos) const {
    auto *screen = QApplication::primaryScreen();
    if (!screen) return;
    QRect avail = screen->availableGeometry();
    int w = width();
    int h = height();
    // 确保整个窗口在可用区域内（不被任务栏遮挡，不超出屏幕）
    if (pos.x() < avail.left()) pos.setX(avail.left());
    if (pos.y() < avail.top()) pos.setY(avail.top());
    if (pos.x() + w > avail.right() + 1) pos.setX(avail.right() + 1 - w);
    if (pos.y() + h > avail.bottom() + 1) pos.setY(avail.bottom() + 1 - h);
}

void FloatingBall::savePosition() const {
    QSettings settings;
    settings.setValue("floatingBallPos", pos());
}

void FloatingBall::loadPosition() {
    QSettings settings;
    QPoint pos = settings.value("floatingBallPos", QPoint(-1, -1)).toPoint();
    if (pos.x() < 0 || pos.y() < 0) {
        // 默认位置：屏幕右侧中间偏上
        auto *screen = QApplication::primaryScreen();
        if (screen) {
            QRect g = screen->availableGeometry();
            pos = QPoint(g.right() - width() - 10, g.top() + 100);
        }
    }
    clampToScreen(pos);
    move(pos);
}

bool FloatingBall::eventFilter(QObject *obj, QEvent *event) {
    if (obj == m_btnPick || obj == m_btnReturn) {
        if (event->type() == QEvent::MouseButtonPress) {
            auto *me = static_cast<QMouseEvent *>(event);
            if (me->button() == Qt::LeftButton) {
                m_pressWidget = qobject_cast<QWidget *>(obj);
                QPoint globalPos = me->globalPosition().toPoint();
                m_pressGlobalPos = globalPos;
                m_dragStart = globalPos - frameGeometry().topLeft();
                m_dragging = true;
                m_moved = false;
            }
        } else if (event->type() == QEvent::MouseMove) {
            auto *me = static_cast<QMouseEvent *>(event);
            if (m_dragging && (me->buttons() & Qt::LeftButton)) {
                QPoint globalPos = me->globalPosition().toPoint();
                if (!m_moved) {
                    int dx = globalPos.x() - m_pressGlobalPos.x();
                    int dy = globalPos.y() - m_pressGlobalPos.y();
                    if (dx * dx + dy * dy > DRAG_THRESHOLD * DRAG_THRESHOLD) {
                        m_moved = true;
                    }
                }
                if (m_moved) {
                    QPoint newPos = globalPos - m_dragStart;
                    clampToScreen(newPos);
                    move(newPos);
                    return true;
                }
            }
        } else if (event->type() == QEvent::MouseButtonRelease) {
            auto *me = static_cast<QMouseEvent *>(event);
            if (me->button() == Qt::LeftButton) {
                if (m_moved && m_pressWidget) {
                    // 是拖动，消费掉这个 Release，不让它触发按钮 clicked
                    savePosition();
                    m_dragging = false;
                    m_moved = false;
                    m_pressWidget = nullptr;
                    return true;
                }
                m_dragging = false;
                m_moved = false;
                m_pressWidget = nullptr;
            }
        }
    }
    return QWidget::eventFilter(obj, event);
}

void FloatingBall::mousePressEvent(QMouseEvent *event) {
    if (event->button() == Qt::LeftButton) {
        m_pressGlobalPos = event->globalPosition().toPoint();
        m_dragStart = m_pressGlobalPos - frameGeometry().topLeft();
        m_dragging = true;
        m_moved = false;
    }
    QWidget::mousePressEvent(event);
}

void FloatingBall::mouseMoveEvent(QMouseEvent *event) {
    if (m_dragging && (event->buttons() & Qt::LeftButton)) {
        QPoint globalPos = event->globalPosition().toPoint();
        if (!m_moved) {
            int dx = globalPos.x() - m_pressGlobalPos.x();
            int dy = globalPos.y() - m_pressGlobalPos.y();
            if (dx * dx + dy * dy > DRAG_THRESHOLD * DRAG_THRESHOLD) {
                m_moved = true;
            }
        }
        if (m_moved) {
            QPoint newPos = globalPos - m_dragStart;
            clampToScreen(newPos);
            move(newPos);
        }
    }
    QWidget::mouseMoveEvent(event);
}

void FloatingBall::mouseReleaseEvent(QMouseEvent *event) {
    if (m_moved)
        savePosition();
    m_dragging = false;
    m_moved = false;
    QWidget::mouseReleaseEvent(event);
}

void FloatingBall::closeEvent(QCloseEvent *event) {
    savePosition();
    if (m_popup) {
        m_popup->close();
        m_popup.clear();
    }
    QWidget::closeEvent(event);
}
