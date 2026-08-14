#ifndef PICKPOPUP_H
#define PICKPOPUP_H

#include <QDialog>
#include <QLabel>
#include <QStringList>
#include <QTimer>

class PickPopup : public QDialog {
    Q_OBJECT
public:
    explicit PickPopup(QWidget *parent = nullptr);
    ~PickPopup() override;

    void showNames(const QStringList &names);

protected:
    void mousePressEvent(QMouseEvent *event) override;

private:
    void buildAndSize(const QStringList &names);

    QLabel *m_label;
    int m_clickCount = 0;
    QTimer m_autoCloseTimer;
};

#endif // PICKPOPUP_H
