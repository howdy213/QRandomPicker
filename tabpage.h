#ifndef TABPAGE_H
#define TABPAGE_H

#include "namepicker.h"
#include <QLabel>
#include <QListWidget>
#include <QPushButton>
#include <QRadioButton>
#include <QTextBrowser>

class TabPage : public QWidget {
    Q_OBJECT
public:
    explicit TabPage(const QString &listName, QWidget *parent = nullptr);
    ~TabPage() override;

    [[nodiscard]] QString listName() const { return m_listName; }
    void loadNameList();
    bool saveNameList();
    void addName(const QString &name);
    void removeSelectedName();
    void updateNameListColors();
signals:
    void closeRequested(TabPage *);

private slots:
    void onPickClicked();
    void onModeChanged();
    void onAddNameClicked();
    void onDelNameClicked();
    void onCloseClicked();
    void onDecreasePickCount();
    void onIncreasePickCount();

private:
    void initUI();
    [[nodiscard]] QString arrangeNames(const QStringList &names) const;
    [[nodiscard]] QStringList pickNames(int count);

    QString m_listName;
    NamePicker *m_namePicker;

    static constexpr int RESULT_WIDTH = 680;
    static constexpr int RESULT_HEIGHT = 480;
    static constexpr int FONT_SIZE = 22;
    static constexpr int LINE_SPACING = 8;
    static constexpr int WORD_SPACING = 4;

    QTextBrowser *m_resultBrowser;
    QListWidget *m_listNames;
    QLabel *m_lblCount;
    QRadioButton *m_radioRandom;
    QRadioButton *m_radioFair;
    QPushButton *m_btnPick;
    QPushButton *m_btnAdd;
    QPushButton *m_btnDel;
    QLabel *m_lblPickCount;
    QPushButton *m_btnDecrease;
    QPushButton *m_btnIncrease;
    QLabel *m_lblPickCountValue;
    int m_pickCount = 1;
};

#endif // TABPAGE_H
