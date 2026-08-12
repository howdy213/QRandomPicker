#ifndef HISTORYDIALOG_H
#define HISTORYDIALOG_H

#include "session.h"
#include <QDialog>

class QListWidget;
class QLabel;

class HistoryDialog : public QDialog {
    Q_OBJECT
public:
    explicit HistoryDialog(const QList<PickRecord> &history, QWidget *parent = nullptr);
    bool clearRequested() const { return m_clearRequested; }

private slots:
    void onClearClicked();

private:
    QListWidget *m_list;
    QLabel *m_lblCount;
    bool m_clearRequested = false;
};

#endif // HISTORYDIALOG_H
