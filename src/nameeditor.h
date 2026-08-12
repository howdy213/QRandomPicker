#ifndef NAMEEDITOR_H
#define NAMEEDITOR_H

#include <QDialog>
#include <QStringList>

class QTextEdit;
class QLabel;

class NameListEditor : public QDialog {
    Q_OBJECT
public:
    explicit NameListEditor(const QStringList &names, QWidget *parent = nullptr);
    QStringList names() const;

private slots:
    void onImportClicked();
    void onClearClicked();
    void onTextChanged();

private:
    QTextEdit *m_editor;
    QLabel *m_lblCount;
};

#endif // NAMEEDITOR_H
