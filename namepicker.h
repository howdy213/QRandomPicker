#ifndef NAMEPICKER_H
#define NAMEPICKER_H

#include <QObject>
#include <QStringList>
#include <QVector>
#include <random>

enum class PickMode {
    Random,  // 随机模式（可重复）
    Fair     // 公平模式（不重复，抽完重置）
};

class NamePicker : public QObject {
    Q_OBJECT
public:
    explicit NamePicker(QObject *parent = nullptr);

    void setNameList(const QStringList &list) { m_nameList = list; }
    [[nodiscard]] QStringList getNameList() const { return m_nameList; }
    void setPickMode(PickMode mode) { m_mode = mode; }
    QString pickName();
    void resetFairPick() { m_pickedIndices.clear(); }
    QVector<int> pickedIndices(){return m_pickedIndices;};
    [[nodiscard]] std::mt19937& getRandomEngine() { return m_randomEngine; }

private:
    QStringList m_nameList;
    PickMode m_mode = PickMode::Random;
    QVector<int> m_pickedIndices;
    std::mt19937 m_randomEngine;
};

#endif // NAMEPICKER_H
