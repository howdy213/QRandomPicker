#include "tabpage.h"
#include "nameeditor.h"
#include "sessionmanager.h"
#include <QButtonGroup>
#include <QDateTime>
#include <QEvent>
#include <QFont>
#include <QFontMetrics>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMessageBox>
#include <QPushButton>
#include <QRadioButton>
#include <QResizeEvent>
#include <QTextBrowser>
#include <QTextOption>
#include <QTimer>
#include <QVBoxLayout>

extern SessionManager *g_sessionManager;

TabPage::TabPage(Session *session, QWidget *parent)
    : QWidget(parent), m_session(session) {
    initUI();
    loadFromSession();
    applyViewMode(m_session ? m_session->viewMode() : ViewMode::Normal);

    if (m_session) {
        connect(m_session, &Session::namesChanged, this, [this]() {
            loadFromSession();
        });
        connect(m_session, &Session::pickPerformed, this, [this]() {
            updateNameListColors();
            updateHistoryPanel();
            updateStatsPanel();
        });
        connect(m_session, &Session::pickModeChanged, this, [this](PickMode mode) {
            m_btnToggleMode->setText(mode == PickMode::Random ? "随机" : "公平");
            updateNameListColors();
            updateCountLabel();
            updateStatsPanel();
        });
        connect(m_session, &Session::viewModeChanged, this, [this](ViewMode mode) {
            applyViewMode(mode);
        });
    }
}

TabPage::~TabPage() = default;

void TabPage::initUI() {
    auto *outerLayout = new QVBoxLayout(this);
    outerLayout->setContentsMargins(0, 0, 0, 0);
    outerLayout->setSpacing(0);

    auto *mainLayout = new QHBoxLayout();
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);
    outerLayout->addLayout(mainLayout);

    // 左面板
    m_leftPanel = new QWidget(this);
    m_leftPanel->setFixedWidth(147);
    auto *leftLayout = new QVBoxLayout(m_leftPanel);
    leftLayout->setContentsMargins(0, 0, 0, 0);
    leftLayout->setSpacing(0);

    m_listNames = new QListWidget(this);
    leftLayout->addWidget(m_listNames);

    auto *leftBottomBar = new QWidget(this);
    leftBottomBar->setFixedHeight(44);
    auto *leftBottomLayout = new QVBoxLayout(leftBottomBar);
    leftBottomLayout->setContentsMargins(8, 6, 8, 6);
    leftBottomLayout->setSpacing(2);

    auto *topRow = new QHBoxLayout();
    topRow->setContentsMargins(0, 0, 0, 0);
    m_lblCount = new QLabel("0/0", this);
    topRow->addWidget(m_lblCount);
    topRow->addStretch();

    m_btnToggleMode = new QPushButton("随机", this);
    m_btnToggleMode->setFixedSize(40, 22);
    topRow->addWidget(m_btnToggleMode);

    leftBottomLayout->addLayout(topRow);

    leftLayout->addWidget(leftBottomBar);
    mainLayout->addWidget(m_leftPanel);

    // 分隔线
    m_vLine = new QFrame(this);
    m_vLine->setFrameShape(QFrame::NoFrame);
    m_vLine->setFixedWidth(1);
    m_vLine->setStyleSheet("background-color: #cccccc;");
    mainLayout->addWidget(m_vLine);

    // 右面板
    m_rightPanel = new QWidget(this);
    auto *rightLayout = new QVBoxLayout(m_rightPanel);
    rightLayout->setContentsMargins(0, 0, 0, 0);
    rightLayout->setSpacing(0);

    m_resultBrowser = new QTextBrowser(this);
    m_resultBrowser->setOpenExternalLinks(false);
    QFont font = m_resultBrowser->font();
    font.setPointSize(FONT_SIZE);
    m_resultBrowser->setFont(font);
    m_resultBrowser->setReadOnly(true);
    m_resultBrowser->setHtml("<div style='color:#999; text-align:center; margin-top:60px;'>等待抽取...</div>");
    m_resultBrowser->installEventFilter(this);
    rightLayout->addWidget(m_resultBrowser);

    auto *controlWidget = new QWidget(this);
    controlWidget->setFixedHeight(44);
    auto *controlLayout = new QHBoxLayout(controlWidget);
    controlLayout->setContentsMargins(8, 6, 8, 6);
    controlLayout->setSpacing(6);
    rightLayout->addWidget(controlWidget);

    auto *pickCountWidget = new QWidget(this);
    auto *pickCountLayout = new QHBoxLayout(pickCountWidget);
    pickCountLayout->setContentsMargins(0, 0, 0, 0);
    pickCountLayout->setSpacing(4);
    m_lblPickCount = new QLabel("抽取人数：", this);

    m_btnDecrease = new QPushButton("-", this);
    m_btnDecrease->setFixedSize(32, 32);

    m_btnBigDecrease = new QPushButton("--", this);
    m_btnBigDecrease->setFixedSize(32, 32);

    m_lblPickCountValue = new QLabel("1", this);
    m_lblPickCountValue->setMinimumWidth(32);
    m_lblPickCountValue->setAlignment(Qt::AlignCenter);

    m_btnBigIncrease = new QPushButton("++", this);
    m_btnBigIncrease->setFixedSize(32, 32);

    m_btnIncrease = new QPushButton("+", this);
    m_btnIncrease->setFixedSize(32, 32);

    pickCountLayout->addWidget(m_lblPickCount);
    pickCountLayout->addWidget(m_btnBigDecrease);
    pickCountLayout->addWidget(m_btnDecrease);
    pickCountLayout->addWidget(m_lblPickCountValue);
    pickCountLayout->addWidget(m_btnIncrease);
    pickCountLayout->addWidget(m_btnBigIncrease);
    controlLayout->addWidget(pickCountWidget);

    m_btnPick = new QPushButton("开始抽取", this);
    m_btnPick->setFixedHeight(32);
    controlLayout->addWidget(m_btnPick, 1);

    mainLayout->addWidget(m_rightPanel, 1);

    // 底部面板（高级模式）
    m_bottomPanel = new QWidget(this);
    m_bottomPanel->setContentsMargins(0, 0, 0, 0);
    m_bottomPanel->setFixedHeight(160);
    auto *bottomLayout = new QHBoxLayout(m_bottomPanel);
    bottomLayout->setContentsMargins(0, 0, 0, 0);
    bottomLayout->setSpacing(0);

    auto *historyWidget = new QWidget(this);
    auto *historyLayout = new QVBoxLayout(historyWidget);
    historyLayout->setContentsMargins(8, 6, 8, 6);
    historyLayout->setSpacing(4);

    auto *historyHeader = new QHBoxLayout();
    auto *lblHistory = new QLabel("抽取历史", this);
    historyHeader->addWidget(lblHistory);
    historyHeader->addStretch();
    m_btnClearHistory = new QPushButton("清空", this);
    historyHeader->addWidget(m_btnClearHistory);
    historyLayout->addLayout(historyHeader);

    m_historyList = new QListWidget(this);
    historyLayout->addWidget(m_historyList);
    bottomLayout->addWidget(historyWidget, 3);

    auto *statsWidget = new QWidget(this);
    statsWidget->setFixedWidth(220);
    auto *statsLayout = new QVBoxLayout(statsWidget);
    statsLayout->setContentsMargins(10, 8, 10, 8);
    statsLayout->setSpacing(4);

    auto *statsTitle = new QLabel("统计信息", this);
    statsLayout->addWidget(statsTitle);

    m_lblTotalNames = new QLabel("总人数：0", this);
    statsLayout->addWidget(m_lblTotalNames);

    m_lblTotalPicks = new QLabel("累计抽取：0 次", this);
    statsLayout->addWidget(m_lblTotalPicks);

    m_lblFairRemaining = new QLabel("公平模式剩余：-", this);
    statsLayout->addWidget(m_lblFairRemaining);

    statsLayout->addStretch();
    bottomLayout->addWidget(statsWidget, 1);
    outerLayout->addWidget(m_bottomPanel);

    connect(m_btnPick, &QPushButton::clicked, this, &TabPage::onPickClicked);
    connect(m_btnToggleMode, &QPushButton::clicked, this, &TabPage::onToggleModeClicked);
    connect(m_btnDecrease, &QPushButton::clicked, this, &TabPage::onDecreasePickCount);
    connect(m_btnBigDecrease, &QPushButton::clicked, this, &TabPage::onBigDecreasePickCount);
    connect(m_btnIncrease, &QPushButton::clicked, this, &TabPage::onIncreasePickCount);
    connect(m_btnBigIncrease, &QPushButton::clicked, this, &TabPage::onBigIncreasePickCount);
    connect(m_btnClearHistory, &QPushButton::clicked, this, &TabPage::onClearHistoryClicked);
}

void TabPage::loadFromSession() {
    if (!m_session)
        return;

    m_listNames->clear();
    m_listNames->addItems(m_session->names());
    updateCountLabel();

    m_btnToggleMode->setText(m_session->pickMode() == PickMode::Random ? "随机" : "公平");

    m_lblPickCountValue->setText(QString::number(m_session->pickCount()));

    updateNameListColors();
    updateHistoryPanel();
    updateStatsPanel();
    updateControlsState();
}

void TabPage::applyViewMode(ViewMode mode) {
    switch (mode) {
    case ViewMode::Simple:
        m_leftPanel->hide();
        m_bottomPanel->hide();
        m_rightPanel->show();
        break;
    case ViewMode::Normal:
        m_leftPanel->show();
        m_bottomPanel->hide();
        m_rightPanel->show();
        break;
    case ViewMode::Advanced:
        m_leftPanel->show();
        m_bottomPanel->show();
        m_rightPanel->show();
        break;
    }
    emit viewModeChanged(mode);
}

void TabPage::setViewMode(ViewMode mode) {
    if (m_session)
        m_session->setViewMode(mode);
    else
        applyViewMode(mode);
}

void TabPage::editNames() {
    if (!m_session)
        return;
    NameListEditor dlg(m_session->names(), this);
    if (dlg.exec() == QDialog::Accepted) {
        QStringList newNames = dlg.names();
        m_session->setNames(newNames);
        loadFromSession();
        emit statusMessage(QString("名单已更新：%1 人").arg(newNames.size()));
    }
}

void TabPage::saveSession() {
    if (!m_session || !g_sessionManager)
        return;
    if (g_sessionManager->saveSession(m_session)) {
        emit statusMessage(QString("会话 \"%1\" 已保存").arg(m_session->name()));
    } else {
        emit statusMessage(QString("保存失败").arg(m_session->name()));
    }
}

void TabPage::updateControlsState() {
    if (!m_session)
        return;
    int count = m_listNames->count();
    int pickCount = m_session->pickCount();
    m_btnDecrease->setDisabled(pickCount <= 1);
    m_btnBigDecrease->setDisabled(pickCount <= 1);
    m_btnIncrease->setDisabled(pickCount >= count);
    m_btnBigIncrease->setDisabled(pickCount >= count);
    m_btnPick->setDisabled(count == 0);
}

void TabPage::updateCountLabel() {
    if (!m_session)
        return;
    int total = m_session->names().size();
    if (m_session->pickMode() == PickMode::Fair) {
        int remaining = m_session->remainingFairCount();
        m_lblCount->setText(QString("公平剩余 %1/%2").arg(remaining).arg(total));
    } else {
        m_lblCount->setText(QString("%1/%2").arg(total).arg(total));
    }
}

void TabPage::updateNameListColors() {
    if (!m_session)
        return;

    if (m_session->pickMode() == PickMode::Random) {
        for (int i = 0; i < m_listNames->count(); ++i)
            m_listNames->item(i)->setForeground(Qt::black);
    } else {
        QVector<int> picked = m_session->pickedIndices();
        for (int i = 0; i < m_listNames->count(); ++i) {
            m_listNames->item(i)->setForeground(picked.contains(i) ? Qt::blue : Qt::black);
        }
    }
}

void TabPage::updateHistoryPanel() {
    if (!m_session)
        return;
    m_historyList->clear();
    for (const auto &rec : m_session->history()) {
        QString timeStr = rec.timestamp.toString("MM-dd hh:mm:ss");
        QString modeStr = rec.mode == PickMode::Random ? "随机" : "公平";
        QString namesStr = rec.picked.join(", ");
        QString entry = QString("[%1] %2 %3人: %4")
                            .arg(timeStr)
                            .arg(modeStr)
                            .arg(rec.count)
                            .arg(namesStr);
        m_historyList->addItem(entry);
    }
}

void TabPage::updateStatsPanel() {
    if (!m_session)
        return;
    m_lblTotalNames->setText(QString("总人数：%1").arg(m_session->names().size()));
    m_lblTotalPicks->setText(QString("累计抽取：%1 次").arg(m_session->history().size()));
    if (m_session->pickMode() == PickMode::Fair) {
        m_lblFairRemaining->setText(QString("公平模式剩余：%1 人").arg(m_session->remainingFairCount()));
    } else {
        m_lblFairRemaining->setText("公平模式剩余：-");
    }
}

QString TabPage::buildResultHtml(const QStringList &names) const {
    if (names.isEmpty())
        return "<div style='color:#999; text-align:center; margin-top:60px;'>等待抽取...</div>";

    QFont font;
    font.setPointSize(FONT_SIZE);
    QFontMetrics fm(font);

    // 测量所有名字实际宽度，取最大值
    int maxNameWidth = 0;
    for (const auto &name : names)
        maxNameWidth = std::max(maxNameWidth, fm.horizontalAdvance(name));

    // 每列宽度 = 最长名字 + 少量间距
    int colWidth = maxNameWidth + 12;
    int browserWidth = m_resultBrowser->viewport()->width();
    if (browserWidth < 100) browserWidth = 100;
    int cols = std::max(1, browserWidth / colWidth);

    // 构建 HTML 表格
    QString html = QString("<table width='100%%' cellspacing='0' cellpadding='2' "
                           "style='font-size:%1pt; margin:0 auto; border-collapse:collapse;'>")
                       .arg(FONT_SIZE);
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

void TabPage::displayResult(const QStringList &names) {
    m_lastPicked = names;
    m_resultBrowser->setHtml(buildResultHtml(names));
}

bool TabPage::eventFilter(QObject *obj, QEvent *event) {
    if (obj == m_resultBrowser && event->type() == QEvent::Resize) {
        if (!m_lastPicked.isEmpty())
            m_resultBrowser->setHtml(buildResultHtml(m_lastPicked));
    }
    return QWidget::eventFilter(obj, event);
}

void TabPage::syncNamesToSession() {
    if (!m_session)
        return;
    QStringList names;
    for (int i = 0; i < m_listNames->count(); ++i)
        names.append(m_listNames->item(i)->text().trimmed());
    m_session->setNames(names);
    updateCountLabel();
    updateNameListColors();
    updateControlsState();
    updateStatsPanel();
}

void TabPage::onDecreasePickCount() {
    if (!m_session)
        return;
    int count = m_session->pickCount();
    if (count > 1) {
        m_session->setPickCount(count - 1);
        m_lblPickCountValue->setText(QString::number(m_session->pickCount()));
        updateControlsState();
    }
}

void TabPage::onIncreasePickCount() {
    if (!m_session)
        return;
    int maxCount = m_listNames->count();
    int count = m_session->pickCount();
    if (count < maxCount) {
        m_session->setPickCount(count + 1);
        m_lblPickCountValue->setText(QString::number(m_session->pickCount()));
        updateControlsState();
    }
}

void TabPage::onBigDecreasePickCount() {
    if (!m_session)
        return;
    int count = m_session->pickCount();
    int newCount = std::max(1, count - 10);
    if (newCount != count) {
        m_session->setPickCount(newCount);
        m_lblPickCountValue->setText(QString::number(m_session->pickCount()));
        updateControlsState();
    }
}

void TabPage::onBigIncreasePickCount() {
    if (!m_session)
        return;
    int maxCount = m_listNames->count();
    int count = m_session->pickCount();
    int newCount = std::min(maxCount, count + 10);
    if (newCount != count) {
        m_session->setPickCount(newCount);
        m_lblPickCountValue->setText(QString::number(m_session->pickCount()));
        updateControlsState();
    }
}

void TabPage::onPickClicked() {
    if (!m_session)
        return;
    QStringList picked = m_session->pickNames(m_session->pickCount());
    if (picked.isEmpty()) {
        m_lastPicked.clear();
        m_resultBrowser->setHtml("<div style='color:#999; text-align:center; margin-top:60px;'>等待抽取...</div>");
        return;
    }

    displayResult(picked);

    updateNameListColors();
    updateCountLabel();
    updateHistoryPanel();
    updateStatsPanel();
    updateControlsState();
}

void TabPage::onToggleModeClicked() {
    if (!m_session)
        return;
    auto newMode = m_session->pickMode() == PickMode::Random ? PickMode::Fair : PickMode::Random;
    m_session->setPickMode(newMode);
    m_btnToggleMode->setText(newMode == PickMode::Random ? "随机" : "公平");
    updateNameListColors();
    updateCountLabel();
    updateStatsPanel();
}

void TabPage::onClearHistoryClicked() {
    if (!m_session)
        return;
    m_session->clearHistory();
    updateHistoryPanel();
    updateStatsPanel();
}

void TabPage::onPinnedToggled(bool checked) {
    if (!m_session)
        return;
    m_session->setPinned(checked);
    emit pinnedChanged(this);
}

void TabPage::updateViewModeButtons() {
    // 已无工具栏，空实现
}
