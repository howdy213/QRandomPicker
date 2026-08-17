#include "pickpopup.h"
#include <QApplication>
#include <QFont>
#include <QFontMetrics>
#include <QMouseEvent>
#include <QScreen>
#include <QVBoxLayout>

static constexpr int POPUP_FONT_SIZE = 20;
static constexpr int POPUP_PADDING = 32;   // 四边内边距（每边16，总共32）
static constexpr int CELL_HPAD = 12;      // 单元格左右 padding
static constexpr int CELL_VPAD = 8;       // 单元格上下 padding
static constexpr int MAX_WIDTH = 800;     // 弹窗最大宽度

PickPopup::PickPopup(QWidget *parent)
    : QDialog(parent) {
    setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::Tool);
    setModal(false);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(16, 16, 16, 16);

    m_label = new QLabel(this);
    QFont font;
    font.setPointSize(POPUP_FONT_SIZE);
    font.setBold(true);
    m_label->setFont(font);
    m_label->setAlignment(Qt::AlignCenter);
    m_label->setTextFormat(Qt::RichText);
    layout->addWidget(m_label);

    m_autoCloseTimer.setSingleShot(true);
    m_autoCloseTimer.setInterval(4000);
    connect(&m_autoCloseTimer, &QTimer::timeout, this, &QDialog::accept);
}

PickPopup::~PickPopup() = default;

void PickPopup::showNames(const QStringList &names) {
    m_clickCount = 0;
    buildAndSize(names);
    m_autoCloseTimer.start();

    auto *screen = QApplication::primaryScreen();
    if (screen) {
        QRect screenGeometry = screen->availableGeometry();
        move(screenGeometry.center() - rect().center());
    }
    show();
    raise();
    activateWindow();
}

void PickPopup::buildAndSize(const QStringList &names) {
    if (names.isEmpty()) {
        m_label->setText("<div style='color:#999;'>无结果</div>");
        return;
    }

    QFont font;
    font.setPointSize(POPUP_FONT_SIZE);
    font.setBold(true);
    QFontMetrics fm(font);

    // 每个名字的测量宽度
    QVector<int> nameWidths;
    nameWidths.reserve(names.size());
    int maxNameWidth = 0;
    for (const auto &name : names) {
        int w = fm.horizontalAdvance(name);
        nameWidths.append(w);
        if (w > maxNameWidth) maxNameWidth = w;
    }

    // 单元格宽度 = 名字最大宽度 + 水平 padding
    int cellWidth = maxNameWidth + CELL_HPAD * 2;

    // 最大允许宽度内决定列数
    int usableWidth = MAX_WIDTH - POPUP_PADDING;
    int cols = std::max(1, usableWidth / cellWidth);
    if (cols > names.size()) cols = names.size();

    int rows = (names.size() + cols - 1) / cols;
    int totalWidth = cols * cellWidth + POPUP_PADDING;
    int cellHeight = fm.height() + CELL_VPAD * 2;
    int totalHeight = rows * cellHeight + POPUP_PADDING;

    // 构建 HTML 表格
    QString html = QString("<table cellspacing='0' cellpadding='0' "
                           "style='border-collapse:collapse;'>");
    for (int i = 0; i < names.size(); i += cols) {
        html += "<tr>";
        for (int j = 0; j < cols; ++j) {
            int idx = i + j;
            if (idx < names.size()) {
                html += QString("<td align='center' width='%1' style='padding:%2px %3px;'>%4</td>")
                .arg(cellWidth)
                    .arg(CELL_VPAD).arg(CELL_HPAD)
                    .arg(names[idx].toHtmlEscaped());
            } else {
                html += QString("<td width='%1'>&nbsp;</td>").arg(cellWidth);
            }
        }
        html += "</tr>";
    }
    html += "</table>";
    m_label->setText(html);

    // 自适应窗口大小
    setFixedSize(totalWidth, totalHeight);
}

void PickPopup::mousePressEvent(QMouseEvent *event) {
    if (event->button() == Qt::LeftButton) {
        m_clickCount++;
        if (m_clickCount >= 3) {
            m_autoCloseTimer.stop();
            accept();
            return;
        }
    }
    QDialog::mousePressEvent(event);
}
