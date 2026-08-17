#ifndef HISTORYDIALOG_H
#define HISTORYDIALOG_H

#include "session.h"
#include <QDialog>

class QListWidget;
class QLabel;
class QPushButton;

class HistoryDialog : public QDialog {
    Q_OBJECT
public:
    explicit HistoryDialog(const QList<PickRecord> &history, QWidget *parent = nullptr);
    bool clearRequested() const { return m_clearRequested; }
    QStringList selectedItems() const;

private slots:
    void onClearClicked();
    void onCopyClicked();

private:
    QListWidget *m_list;
    QLabel *m_lblCount;
    QPushButton *m_btnCopy;
    bool m_clearRequested = false;
};

#endif // HISTORYDIALOG_H
