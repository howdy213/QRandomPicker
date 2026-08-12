#ifndef SESSIONPICKERDIALOG_H
#define SESSIONPICKERDIALOG_H

#include <QDialog>
#include <QStringList>

class QListWidget;
class QLabel;

class SessionPickerDialog : public QDialog {
    Q_OBJECT
public:
    explicit SessionPickerDialog(const QStringList &availableNames, QWidget *parent = nullptr);
    QStringList selectedNames() const;

private slots:
    void onSelectionChanged();
    void onSelectAll();
    void onInvert();

private:
    QListWidget *m_list;
    QLabel *m_lblCount;
};

#endif // SESSIONPICKERDIALOG_H
