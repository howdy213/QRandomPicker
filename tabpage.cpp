#include "tabpage.h"
#include <QDir>
#include <QFile>
#include <QFont>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMessageBox>
#include <QPushButton>
#include <QRadioButton>
#include <QTextStream>
#include <QVBoxLayout>
#include <algorithm>

TabPage::TabPage(const QString &listName, QWidget *parent)
    : QWidget(parent), m_listName(listName),
    m_namePicker(new NamePicker(this)) {
    initUI();
    loadNameList();
}

TabPage::~TabPage() = default;

void TabPage::initUI() {
    auto *mainLayout = new QHBoxLayout(this);
    mainLayout->setContentsMargins(15, 15, 15, 15);
    mainLayout->setSpacing(15);

    // 左侧面板
    auto *leftWidget = new QWidget(this);
    leftWidget->setFixedWidth(250);
    auto *leftLayout = new QVBoxLayout(leftWidget);
    leftLayout->setSpacing(10);

    m_listNames = new QListWidget(this);
    leftLayout->addWidget(m_listNames);

    m_lblCount = new QLabel("当前人数：0", this);
    m_lblCount->setStyleSheet("font-size: 14px;");
    leftLayout->addWidget(m_lblCount);

    auto *modeWidget = new QWidget(this);
    auto *modeLayout = new QHBoxLayout(modeWidget);
    m_radioRandom = new QRadioButton("随机模式", this);
    m_radioFair = new QRadioButton("公平模式", this);
    m_radioRandom->setChecked(true);
    modeLayout->addWidget(m_radioRandom);
    modeLayout->addWidget(m_radioFair);
    leftLayout->addWidget(modeWidget);

    auto *nameOptWidget = new QWidget(this);
    auto *nameOptLayout = new QHBoxLayout(nameOptWidget);
    m_btnAdd = new QPushButton("添加名字", this);
    m_btnDel = new QPushButton("删除选中", this);
    nameOptLayout->addWidget(m_btnAdd);
    nameOptLayout->addWidget(m_btnDel);
    leftLayout->addWidget(nameOptWidget);

    mainLayout->addWidget(leftWidget);

    // 右侧面板
    auto *rightWidget = new QWidget(this);
    auto *rightLayout = new QVBoxLayout(rightWidget);
    rightLayout->setSpacing(15);

    m_resultBrowser = new QTextBrowser(this);
    m_resultBrowser->setStyleSheet(R"(
        QTextBrowser { color: #000000; border: 2px dashed #ccc; border-radius: 8px; padding: 15px; background-color: white; }
        QTextBrowser::viewport { border: 2px dashed #ccc; border-radius: 8px; }
    )");
    m_resultBrowser->setWordWrapMode(QTextOption::WrapAtWordBoundaryOrAnywhere);
    m_resultBrowser->setAlignment(Qt::AlignLeft | Qt::AlignTop);
    QFont font = m_resultBrowser->font();
    font.setPointSize(FONT_SIZE);
    font.setWordSpacing(WORD_SPACING);
    m_resultBrowser->setFont(font);
    m_resultBrowser->setReadOnly(true);
    m_resultBrowser->setText("等待抽取...");
    m_resultBrowser->setAlignment(Qt::AlignCenter);
    rightLayout->addWidget(m_resultBrowser);

    auto *controlWidget = new QWidget(this);
    auto *controlLayout = new QHBoxLayout(controlWidget);
    controlLayout->setSpacing(10);
    rightLayout->addWidget(controlWidget);

    // 抽取人数控件
    auto *pickCountWidget = new QWidget(this);
    auto *pickCountLayout = new QHBoxLayout(pickCountWidget);
    m_lblPickCount = new QLabel("抽取人数：", this);
    m_lblPickCount->setStyleSheet("font-size: 16px; font-weight: bold;");

    m_btnDecrease = new QPushButton("-", this);
    m_btnDecrease->setFixedSize(40, 40);
    m_btnDecrease->setStyleSheet(R"(
        QPushButton { font-size: 20px; background-color: #ff9800; color: white; border: none; border-radius: 20px; }
        QPushButton:pressed { background-color: #e68900; }
        QPushButton:disabled { background-color: #cccccc; }
    )");

    m_lblPickCountValue = new QLabel("1", this);
    m_lblPickCountValue->setStyleSheet("font-size: 20px; font-weight: bold;");
    m_lblPickCountValue->setMinimumWidth(40);
    m_lblPickCountValue->setAlignment(Qt::AlignCenter);

    m_btnIncrease = new QPushButton("+", this);
    m_btnIncrease->setFixedSize(40, 40);
    m_btnIncrease->setStyleSheet(R"(
        QPushButton { font-size: 20px; background-color: #4CAF50; color: white; border: none; border-radius: 20px; }
        QPushButton:pressed { background-color: #3d8b40; }
        QPushButton:disabled { background-color: #cccccc; }
    )");

    pickCountLayout->addWidget(m_lblPickCount);
    pickCountLayout->addWidget(m_btnDecrease);
    pickCountLayout->addWidget(m_lblPickCountValue);
    pickCountLayout->addWidget(m_btnIncrease);
    pickCountLayout->addStretch();
    controlLayout->addWidget(pickCountWidget);

    m_btnPick = new QPushButton("开始抽取", this);
    m_btnPick->setStyleSheet(R"(
        QPushButton { font-size: 24px; padding: 15px; background-color: #00DDAA; color: white; border: none; border-radius: 8px; }
        QPushButton:pressed { background-color: #1976D2; }
        QPushButton:disabled { background-color: #cccccc; }
    )");
    controlLayout->addWidget(m_btnPick);

    mainLayout->addWidget(rightWidget);

    // 信号连接
    connect(m_btnPick, &QPushButton::clicked, this, &TabPage::onPickClicked);
    connect(m_radioRandom, &QRadioButton::clicked, this, &TabPage::onModeChanged);
    connect(m_radioFair, &QRadioButton::clicked, this, &TabPage::onModeChanged);
    connect(m_btnAdd, &QPushButton::clicked, this, &TabPage::onAddNameClicked);
    connect(m_btnDel, &QPushButton::clicked, this, &TabPage::onDelNameClicked);
    connect(m_btnDecrease, &QPushButton::clicked, this,
            &TabPage::onDecreasePickCount);
    connect(m_btnIncrease, &QPushButton::clicked, this,
            &TabPage::onIncreasePickCount);

    onModeChanged();
    m_btnDecrease->setDisabled(m_pickCount <= 1);
}

QString TabPage::arrangeNames(const QStringList &names) const {
    if (names.isEmpty())
        return "等待抽取...";
    QFont font;
    font.setPointSize(FONT_SIZE);
    QFontMetrics fm(font);
    int avgNameWidth = fm.averageCharWidth() * 9;
    int namesPerRow = m_resultBrowser->width() / avgNameWidth - 1;
    int rows = (names.size() + namesPerRow - 1) / namesPerRow;
    QString gridText;
    for (int i = 0; i < rows; ++i) {
        QString row;
        for (int j = 0; j < namesPerRow; ++j) {
            int idx = i * namesPerRow + j;
            if (idx >= names.size())
                break;
            row += names[idx] + QString(3 * (5 - names[idx].length()), ' ');
        }
        gridText += row.trimmed() + "\n";
    }
    return gridText.trimmed();
}

void TabPage::updateNameListColors() {
    if (m_radioRandom->isChecked()) {
        for (int i = 0; i < m_listNames->count(); ++i)
            m_listNames->item(i)->setForeground(Qt::black);
    } else {
        QVector<int> picked = m_namePicker->pickedIndices();
        for (int i = 0; i < m_listNames->count(); ++i) {
            m_listNames->item(i)->setForeground(picked.contains(i) ? Qt::blue
                                                                   : Qt::black);
        }
    }
}

void TabPage::onDecreasePickCount() {
    if (m_pickCount > 1) {
        --m_pickCount;
        m_lblPickCountValue->setText(QString::number(m_pickCount));
        m_btnDecrease->setDisabled(m_pickCount <= 1);
    }
    m_btnIncrease->setEnabled(true);
}

void TabPage::onIncreasePickCount() {
    int maxCount = m_listNames->count();
    if (m_pickCount < maxCount) {
        ++m_pickCount;
        m_lblPickCountValue->setText(QString::number(m_pickCount));
        m_btnIncrease->setDisabled(m_pickCount >= maxCount);
    }
    m_btnDecrease->setEnabled(true);
}

void TabPage::loadNameList() {
    QString dataPath = QDir::currentPath() + "/data";
    QString filePath = dataPath + "/" + m_listName + ".txt";

    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        m_listNames->clear();
        m_lblCount->setText("当前人数：0");
        m_namePicker->setNameList({});
        m_btnPick->setDisabled(true);
        m_btnIncrease->setDisabled(true);
        return;
    }

    QStringList names;
    QTextStream in(&file);
    while (!in.atEnd()) {
        QString line = in.readLine().trimmed();
        if (!line.isEmpty())
            names.append(line);
    }
    file.close();

    m_listNames->clear();
    m_listNames->addItems(names);
    m_lblCount->setText(QString("当前人数：%1").arg(names.size()));
    m_namePicker->setNameList(names);

    m_pickCount = 1;
    m_lblPickCountValue->setText("1");
    m_btnDecrease->setDisabled(true);
    m_btnIncrease->setDisabled(names.size() <= 1);
    m_btnPick->setDisabled(names.isEmpty());
    updateNameListColors();
}

bool TabPage::saveNameList() {
    QString dataPath = QDir::currentPath() + "/data";
    QDir dir(dataPath);
    if (!dir.exists())
        dir.mkpath(".");

    QString filePath = dataPath + "/" + m_listName + ".txt";
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::warning(this, "错误", "无法保存名单：" + filePath);
        return false;
    }

    QStringList names;
    for (int i = 0; i < m_listNames->count(); ++i)
        names.append(m_listNames->item(i)->text().trimmed());

    QTextStream out(&file);
    for (const QString &name : names)
        out << name << "\n";
    file.close();

    m_lblCount->setText(QString("当前人数：%1").arg(names.size()));
    m_namePicker->setNameList(names);
    m_btnIncrease->setDisabled(names.size() <= m_pickCount);
    m_btnPick->setDisabled(names.isEmpty());
    updateNameListColors();
    return true;
}

void TabPage::addName(const QString &name) {
    if (name.isEmpty())
        return;
    for (int i = 0; i < m_listNames->count(); ++i) {
        if (m_listNames->item(i)->text() == name) {
            QMessageBox::information(this, "提示", "名字已存在！");
            return;
        }
    }
    m_listNames->addItem(name);
    saveNameList();
}

void TabPage::removeSelectedName() {
    QListWidgetItem *selected = m_listNames->currentItem();
    if (!selected) {
        QMessageBox::information(this, "提示", "请先选中要删除的名字！");
        return;
    }
    delete selected;
    saveNameList();
}

QStringList TabPage::pickNames(int count) {
    if (m_namePicker->getNameList().isEmpty())
        return {};
    int maxCount = m_namePicker->getNameList().size();
    count = std::min(count, maxCount);

    if (m_radioRandom->isChecked()) {
        QStringList tempList = m_namePicker->getNameList();
        std::shuffle(tempList.begin(), tempList.end(),
                     m_namePicker->getRandomEngine());
        return tempList.mid(0, count);
    } else {
        QSet<QString> result;
        for (int i = 0; i < count; ++i) {
            QString name = m_namePicker->pickName();
            if (name.isEmpty())
                break;
            result.insert(name);
        }
        return QStringList(result.begin(),result.end());
    }
}

void TabPage::onPickClicked() {
    QStringList picked = pickNames(m_pickCount);
    if (picked.isEmpty()) {
        m_resultBrowser->setText("等待抽取...");
        return;
    }

    QFont font = m_resultBrowser->font();
    font.setPointSize(FONT_SIZE);
    m_resultBrowser->setFont(font);
    m_resultBrowser->setText(arrangeNames(picked));

    updateNameListColors();

    if (m_radioFair->isChecked()) {
        int total = m_listNames->count();
        if (m_namePicker->pickedIndices().size() >= total) {
            m_namePicker->resetFairPick();
            updateNameListColors();
        }
    }
}

void TabPage::onModeChanged() {
    m_namePicker->setPickMode(m_radioRandom->isChecked() ? PickMode::Random
                                                         : PickMode::Fair);
    m_namePicker->resetFairPick();
    updateNameListColors();
}

void TabPage::onAddNameClicked() {
    bool ok;
    QString name = QInputDialog::getText(this, "添加名字", "请输入名字：",
                                         QLineEdit::Normal, "", &ok);
    if (ok && !name.trimmed().isEmpty())
        addName(name.trimmed());
}

void TabPage::onDelNameClicked() { removeSelectedName(); }
void TabPage::onCloseClicked() { emit closeRequested(this); }
