#ifndef NEWCOPYDIALOG_H
#define NEWCOPYDIALOG_H

#include <QDialog>
#include <QStringList>

class QLineEdit;
class QComboBox;
class QLabel;

class NewCopyDialog : public QDialog
{
    Q_OBJECT
public:
    explicit NewCopyDialog(bool isNew, const QStringList &existLists, QWidget *parent = nullptr);

    QString getListName() const;
    QString getSourceListName() const;

private:
    void initUI();

    bool m_isNew;
    QLineEdit *m_editName;
    QComboBox *m_comboSource;
};

#endif // NEWCOPYDIALOG_H
