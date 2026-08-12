#include "newcopydialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QComboBox>
#include <QDialogButtonBox>

NewCopyDialog::NewCopyDialog(bool isNew, const QStringList &existLists, QWidget *parent)
    : QDialog(parent)
    , m_isNew(isNew)
{
    initUI();
    setWindowTitle(m_isNew ? "新建名单" : "复制名单");
    setFixedSize(400, m_isNew ? 150 : 200);

    if (!m_isNew) {
        m_comboSource->addItems(existLists);
    }
}

void NewCopyDialog::initUI()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(20, 20, 20, 20);
    mainLayout->setSpacing(15);

    QWidget *nameWidget = new QWidget(this);
    QHBoxLayout *nameLayout = new QHBoxLayout(nameWidget);
    QLabel *lblName = new QLabel("名单名称：", this);
    m_editName = new QLineEdit(this);
    m_editName->setPlaceholderText("请输入名单名称");
    nameLayout->addWidget(lblName);
    nameLayout->addWidget(m_editName);
    mainLayout->addWidget(nameWidget);

    m_comboSource = new QComboBox(this);
    QWidget *sourceWidget = new QWidget(this);
    QHBoxLayout *sourceLayout = new QHBoxLayout(sourceWidget);
    QLabel *lblSource = new QLabel("复制源：", this);
    sourceLayout->addWidget(lblSource);
    sourceLayout->addWidget(m_comboSource);
    if (m_isNew) {
        sourceWidget->hide();
    } else {
        mainLayout->addWidget(sourceWidget);
    }

    QDialogButtonBox *btnBox = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel,
        Qt::Horizontal, this
        );
    mainLayout->addWidget(btnBox);

    connect(btnBox, &QDialogButtonBox::accepted, this, [this]() {
        if (m_editName->text().trimmed().isEmpty()) {
            m_editName->setFocus();
            return;
        }
        accept();
    });
    connect(btnBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
}

QString NewCopyDialog::getListName() const
{
    return m_editName->text().trimmed();
}

QString NewCopyDialog::getSourceListName() const
{
    return m_comboSource->currentText();
}
