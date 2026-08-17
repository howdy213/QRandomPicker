#include "miniwindow.h"
#include "session.h"
#include <QEvent>
#include <QFont>
#include <QFontMetrics>
#include <QHBoxLayout>
#include <QMouseEvent>
#include <QResizeEvent>
#include <QVBoxLayout>

static constexpr int MINI_FONT_SIZE = 16;

MiniWindow::MiniWindow(Session *session, QWidget *parent)
    : QDialog(parent), m_session(session) {
    setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint | Qt::CustomizeWindowHint);
    setWindowTitle("小窗模式");
    resize(480, 320);
    setMinimumSize(320, 220);

    initUI();
    syncFromSession();

    if (m_session) {
        connect(m_session, &Session::pickPerformed, this, [this]() { updateDisplay(); });
        connect(m_session, &Session::namesChanged, this, [this]() { updateDisplay(); });
        connect(m_session, &Session::pickCountChanged, this, [this]() { updateDisplay(); });
    }
}

MiniWindow::~MiniWindow() = default;

void MiniWindow::initUI() {
    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(0);

    createTitleBar();
    root->addWidget(m_titleBar);

    // 内容区
    auto *content = new QWidget(this);
    auto *contentLayout = new QVBoxLayout(content);
    contentLayout->setContentsMargins(12, 12, 12, 8);
    contentLayout->setSpacing(10);

    m_resultBrowser = new QTextBrowser(content);
    m_resultBrowser->setOpenExternalLinks(false);
    QFont resultFont;
    resultFont.setPointSize(MINI_FONT_SIZE);
    m_resultBrowser->setFont(resultFont);
    m_resultBrowser->setReadOnly(true);
    m_resultBrowser->setHtml("<div style='color:#999; text-align:center; margin-top:40px;'>等待抽取...</div>");
    m_resultBrowser->installEventFilter(this);
    contentLayout->addWidget(m_resultBrowser, 1);

    // 控制行：- 人数 + 开始抽取
    auto *ctrlRow = new QHBoxLayout();
    ctrlRow->setSpacing(8);

    m_btnDec = new QPushButton("-", content);
    m_btnDec->setFixedSize(32, 32);
    ctrlRow->addWidget(m_btnDec);

    m_lblCountValue = new QLabel("1", content);
    m_lblCountValue->setAlignment(Qt::AlignCenter);
    m_lblCountValue->setMinimumWidth(40);
    ctrlRow->addWidget(m_lblCountValue);

    m_btnInc = new QPushButton("+", content);
    m_btnInc->setFixedSize(32, 32);
    ctrlRow->addWidget(m_btnInc);

    ctrlRow->addSpacing(12);

    m_btnPick = new QPushButton("开始抽取", content);
    m_btnPick->setFixedHeight(32);
    ctrlRow->addWidget(m_btnPick, 1);

    contentLayout->addLayout(ctrlRow);
    root->addWidget(content, 1);

    connect(m_btnPick, &QPushButton::clicked, this, [this]() {
        if (!m_session) return;
        m_session->pickNames(m_session->pickCount());
    });
    connect(m_btnInc, &QPushButton::clicked, this, [this]() {
        if (!m_session) return;
        int c = m_session->pickCount();
        if (c < m_session->names().size())
            m_session->setPickCount(c + 1);
    });
    connect(m_btnDec, &QPushButton::clicked, this, [this]() {
        if (!m_session) return;
        int c = m_session->pickCount();
        if (c > 1)
            m_session->setPickCount(c - 1);
    });
}

void MiniWindow::createTitleBar() {
    m_titleBar = new QFrame(this);
    m_titleBar->setFixedHeight(36);

    auto *layout = new QHBoxLayout(m_titleBar);
    layout->setContentsMargins(8, 0, 4, 0);
    layout->setSpacing(2);

    m_btnBack = new QPushButton("←", m_titleBar);
    m_btnBack->setFixedSize(28, 28);
    m_btnBack->setToolTip("返回一般模式");
    layout->addWidget(m_btnBack);

    m_lblTitle = new QLabel("小窗模式", m_titleBar);
    m_lblTitle->setStyleSheet("font-weight: bold;");
    layout->addWidget(m_lblTitle, 1);

    m_btnTop = new QPushButton("↑", m_titleBar);
    m_btnTop->setFixedSize(28, 28);
    m_btnTop->setToolTip("置顶");
    m_btnTop->setCheckable(true);
    layout->addWidget(m_btnTop);

    m_btnMin = new QPushButton("-", m_titleBar);
    m_btnMin->setFixedSize(28, 28);
    m_btnMin->setToolTip("最小化");
    layout->addWidget(m_btnMin);

    m_btnClose = new QPushButton("✕", m_titleBar);
    m_btnClose->setFixedSize(28, 28);
    m_btnClose->setToolTip("关闭");
    layout->addWidget(m_btnClose);

    connect(m_btnBack, &QPushButton::clicked, this, &MiniWindow::onBackToNormal);
    connect(m_btnTop, &QPushButton::toggled, this, &MiniWindow::onToggleAlwaysOnTop);
    connect(m_btnMin, &QPushButton::clicked, this, &MiniWindow::onMinimize);
    connect(m_btnClose, &QPushButton::clicked, this, &QDialog::reject);
}

void MiniWindow::syncFromSession() {
    if (!m_session) return;
    m_lblTitle->setText(m_session->name() + " - 小窗");
    updateDisplay();
}

void MiniWindow::updateDisplay() {
    if (!m_session) return;
    m_lblCountValue->setText(QString::number(m_session->pickCount()));
    const auto &history = m_session->history();
    if (history.isEmpty()) {
        m_lastPicked.clear();
        m_resultBrowser->setHtml("<div style='color:#999; text-align:center; margin-top:40px;'>等待抽取...</div>");
    } else {
        const auto last = history.first();
        displayResult(last.picked);
    }
    int total = m_session->names().size();
    m_btnDec->setEnabled(m_session->pickCount() > 1);
    m_btnInc->setEnabled(m_session->pickCount() < total);
    m_btnPick->setEnabled(total > 0);
}

void MiniWindow::mousePressEvent(QMouseEvent *event) {
    if (event->button() == Qt::LeftButton && m_titleBar->rect().contains(event->pos())) {
        m_dragStart = event->globalPosition().toPoint() - frameGeometry().topLeft();
        m_dragging = true;
    }
    QDialog::mousePressEvent(event);
}

void MiniWindow::mouseMoveEvent(QMouseEvent *event) {
    if (m_dragging && (event->buttons() & Qt::LeftButton)) {
        move(event->globalPosition().toPoint() - m_dragStart);
    }
    QDialog::mouseMoveEvent(event);
}

void MiniWindow::mouseReleaseEvent(QMouseEvent *event) {
    m_dragging = false;
    QDialog::mouseReleaseEvent(event);
}

void MiniWindow::closeEvent(QCloseEvent *event) {
    QDialog::closeEvent(event);
}

void MiniWindow::onBackToNormal() {
    // 通知主窗口返回正常模式
    if (parentWidget()) {
        // 主窗口会处理恢复视图
        parentWidget()->show();
        parentWidget()->raise();
        parentWidget()->activateWindow();
    }
    reject();
}

void MiniWindow::onToggleAlwaysOnTop() {
    m_alwaysOnTop = m_btnTop->isChecked();
    if (m_alwaysOnTop)
        setWindowFlags(windowFlags() | Qt::WindowStaysOnTopHint);
    else
        setWindowFlags(windowFlags() & ~Qt::WindowStaysOnTopHint);
    show();
}

void MiniWindow::onMinimize() {
    showMinimized();
}

QString MiniWindow::buildResultHtml(const QStringList &names) const {
    if (names.isEmpty())
        return "<div style='color:#999; text-align:center; margin-top:40px;'>等待抽取...</div>";

    QFont font;
    font.setPointSize(MINI_FONT_SIZE);
    QFontMetrics fm(font);

    int maxNameWidth = 0;
    for (const auto &name : names)
        maxNameWidth = std::max(maxNameWidth, fm.horizontalAdvance(name));

    int colWidth = maxNameWidth + 12;
    int browserWidth = m_resultBrowser->viewport()->width();
    if (browserWidth < 100) browserWidth = 100;
    int cols = std::max(1, browserWidth / colWidth);

    QString html = QString("<table width='100%%' cellspacing='0' cellpadding='2' "
                           "style='font-size:%1pt; margin:0 auto; border-collapse:collapse;'>")
                       .arg(MINI_FONT_SIZE);
    for (int i = 0; i < names.size(); i += cols) {
        html += "<tr>";
        for (int j = 0; j < cols; ++j) {
            int idx = i + j;
            if (idx < names.size()) {
                html += QString("<td align='center' style='padding:2px 4px;'>%1</td>")
                .arg(names[idx].toHtmlEscaped());
            } else {
                html += "<td></td>";
            }
        }
        html += "</tr>";
    }
    html += "</table>";
    return html;
}

void MiniWindow::displayResult(const QStringList &names) {
    m_lastPicked = names;
    m_resultBrowser->setHtml(buildResultHtml(names));
}

bool MiniWindow::eventFilter(QObject *obj, QEvent *event) {
    if (obj == m_resultBrowser && event->type() == QEvent::Resize) {
        if (!m_lastPicked.isEmpty())
            m_resultBrowser->setHtml(buildResultHtml(m_lastPicked));
    }
    return QDialog::eventFilter(obj, event);
}
