#ifndef SESSION_H
#define SESSION_H

#include <QDateTime>
#include <QJsonObject>
#include <QObject>
#include <QStringList>
#include <QVector>
#include <random>

enum class PickMode {
    Random, // 随机模式（可重复）
    Fair    // 公平模式（不重复，抽完重置）
};

enum class ViewMode {
    Simple,   // 简洁模式
    Normal,   // 正常模式
    Advanced  // 高级模式
};

// 单次抽取记录
struct PickRecord {
    QDateTime timestamp;
    QStringList picked;
    PickMode mode;
    int count = 0;

    QJsonObject toJson() const;
    static PickRecord fromJson(const QJsonObject &obj);
};

class Session : public QObject {
    Q_OBJECT
public:
    explicit Session(QObject *parent = nullptr);
    Session(const QString &id, QObject *parent = nullptr);

    // --- Getters ---
    QString id() const { return m_id; }
    QString name() const { return m_name; }
    QStringList names() const { return m_names; }
    PickMode pickMode() const { return m_pickMode; }
    QVector<int> pickedIndices() const { return m_pickedIndices; }
    int pickCount() const { return m_pickCount; }
    ViewMode viewMode() const { return m_viewMode; }
    QList<PickRecord> history() const { return m_history; }
    QDateTime createdAt() const { return m_createdAt; }
    QDateTime updatedAt() const { return m_updatedAt; }
    bool pinned() const { return m_pinned; }

    // --- Setters ---
    void setName(const QString &name);
    void setNames(const QStringList &names);
    void setPickMode(PickMode mode);
    void setPickCount(int count);
    void setViewMode(ViewMode mode);
    void setPinned(bool pinned);
    void clearHistory();

    // --- 抽取逻辑 ---
    QStringList pickNames(int count);
    void resetFairPick();
    int remainingFairCount() const;

    // --- 序列化 ---
    QJsonObject toJson() const;
    static Session *fromJson(const QJsonObject &obj, QObject *parent = nullptr);

    // 用磁盘上的新数据替换当前session的内部状态（保留指针，避免信号重连）
    void reloadFrom(const Session &other);

    // 随机引擎（供 shuffle 使用）
    std::mt19937 &randomEngine() { return m_randomEngine; }

signals:
    void changed();               // 任意状态变更
    void nameChanged(const QString &name);
    void namesChanged();
    void pickModeChanged(PickMode mode);
    void viewModeChanged(ViewMode mode);
    void pickPerformed(const QStringList &picked);
    void pickCountChanged(int count);
    void pinnedChanged(bool pinned);

private:
    void touch(); // 更新 updatedAt

    QString m_id;
    QString m_name;
    QStringList m_names;
    PickMode m_pickMode = PickMode::Random;
    QVector<int> m_pickedIndices;
    int m_pickCount = 1;
    ViewMode m_viewMode = ViewMode::Normal;
    QList<PickRecord> m_history;
    QDateTime m_createdAt;
    QDateTime m_updatedAt;
    bool m_pinned = false;
    std::mt19937 m_randomEngine;
};

#endif // SESSION_H
