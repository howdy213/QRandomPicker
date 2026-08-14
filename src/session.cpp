#include "session.h"
#include <QJsonArray>
#include <QUuid>
#include <algorithm>
#include <chrono>

// ==================== PickRecord ====================

QJsonObject PickRecord::toJson() const {
    QJsonObject obj;
    obj["timestamp"] = timestamp.toString(Qt::ISODateWithMs);
    QJsonArray arr;
    for (const QString &n : picked)
        arr.append(n);
    obj["picked"] = arr;
    obj["mode"] = static_cast<int>(mode);
    obj["count"] = count;
    return obj;
}

PickRecord PickRecord::fromJson(const QJsonObject &obj) {
    PickRecord rec;
    rec.timestamp = QDateTime::fromString(obj["timestamp"].toString(), Qt::ISODateWithMs);
    rec.mode = static_cast<PickMode>(obj["mode"].toInt());
    rec.count = obj["count"].toInt();
    QJsonArray arr = obj["picked"].toArray();
    for (const auto &v : arr)
        rec.picked.append(v.toString());
    return rec;
}

// ==================== Session ====================

Session::Session(QObject *parent)
    : Session(QUuid::createUuid().toString(QUuid::WithoutBraces), parent) {
}

Session::Session(const QString &id, QObject *parent)
    : QObject(parent), m_id(id) {
    m_createdAt = QDateTime::currentDateTime();
    m_updatedAt = m_createdAt;
    auto seed = std::chrono::steady_clock::now().time_since_epoch().count();
    m_randomEngine.seed(static_cast<unsigned int>(seed));
}

void Session::touch() {
    m_updatedAt = QDateTime::currentDateTime();
    emit changed();
}

void Session::setName(const QString &name) {
    if (m_name == name)
        return;
    m_name = name;
    emit nameChanged(name);
    touch();
}

void Session::setNames(const QStringList &names) {
    m_names = names;
    // 名单变更后，已抽取索引可能越界，清理
    for (int i = m_pickedIndices.size() - 1; i >= 0; --i) {
        if (m_pickedIndices[i] >= m_names.size())
            m_pickedIndices.removeAt(i);
    }
    emit namesChanged();
    touch();
}

void Session::setPickMode(PickMode mode) {
    if (m_pickMode == mode)
        return;
    m_pickMode = mode;
    // 切换模式时保留公平模式记录，不清除 m_pickedIndices
    emit pickModeChanged(mode);
    touch();
}

void Session::setPickCount(int count) {
    if (m_pickCount == count)
        return;
    m_pickCount = count;
    touch();
    emit pickCountChanged(count);
}

void Session::setViewMode(ViewMode mode) {
    if (m_viewMode == mode)
        return;
    m_viewMode = mode;
    emit viewModeChanged(mode);
    touch();
}

void Session::setPinned(bool pinned) {
    if (m_pinned == pinned)
        return;
    m_pinned = pinned;
    emit pinnedChanged(pinned);
    touch();
}

void Session::clearHistory() {
    m_history.clear();
    touch();
}

void Session::resetFairPick() {
    m_pickedIndices.clear();
    touch();
}

int Session::remainingFairCount() const {
    if (m_pickMode != PickMode::Fair)
        return -1;
    return m_names.size() - m_pickedIndices.size();
}

QStringList Session::pickNames(int count) {
    if (m_names.isEmpty())
        return {};

    int maxCount = m_names.size();
    count = std::min(count, maxCount);

    QStringList result;

    if (m_pickMode == PickMode::Random) {
        QStringList tempList = m_names;
        std::shuffle(tempList.begin(), tempList.end(), m_randomEngine);
        result = tempList.mid(0, count);
    } else {
        // Fair 模式：从未被抽中的人中选
        if (m_pickedIndices.size() >= maxCount)
            m_pickedIndices.clear();

        QList<int> unpickedIndices;
        for (int i = 0; i < m_names.size(); ++i) {
            if (!m_pickedIndices.contains(i))
                unpickedIndices.append(i);
        }

        // 随机打乱未抽中的索引
        std::shuffle(unpickedIndices.begin(), unpickedIndices.end(), m_randomEngine);

        int pickNum = std::min(count, static_cast<int>(unpickedIndices.size()));
        QList<int> pickedThisRound;
        for (int i = 0; i < pickNum; ++i) {
            int idx = unpickedIndices[i];
            m_pickedIndices.append(idx);
            pickedThisRound.append(idx);
            result.append(m_names[idx]);
        }

        // 剩余不足时，从新一轮中补齐（排除本轮已抽中的，避免重复）
        if (result.size() < count) {
            m_pickedIndices.clear();
            QList<int> newPool;
            for (int i = 0; i < m_names.size(); ++i) {
                if (!pickedThisRound.contains(i))
                    newPool.append(i);
            }
            std::shuffle(newPool.begin(), newPool.end(), m_randomEngine);

            int remaining = count - result.size();
            for (int i = 0; i < remaining; ++i) {
                int idx = newPool[i];
                m_pickedIndices.append(idx);
                result.append(m_names[idx]);
            }
        }
    }

    // 记录历史
    if (!result.isEmpty()) {
        PickRecord rec;
        rec.timestamp = QDateTime::currentDateTime();
        rec.picked = result;
        rec.mode = m_pickMode;
        rec.count = count;
        m_history.prepend(rec); // 最新的放最前面
        // 限制历史记录数量
        while (m_history.size() > 100)
            m_history.removeLast();
        emit pickPerformed(result);
    }

    touch();
    return result;
}

QJsonObject Session::toJson() const {
    QJsonObject obj;
    obj["id"] = m_id;
    obj["name"] = m_name;
    obj["pickMode"] = static_cast<int>(m_pickMode);
    obj["pickCount"] = m_pickCount;
    obj["viewMode"] = static_cast<int>(m_viewMode);
    obj["pinned"] = m_pinned;
    obj["createdAt"] = m_createdAt.toString(Qt::ISODateWithMs);
    obj["updatedAt"] = m_updatedAt.toString(Qt::ISODateWithMs);

    QJsonArray namesArr;
    for (const QString &n : m_names)
        namesArr.append(n);
    obj["names"] = namesArr;

    QJsonArray pickedArr;
    for (int idx : m_pickedIndices)
        pickedArr.append(idx);
    obj["pickedIndices"] = pickedArr;

    QJsonArray historyArr;
    for (const auto &rec : m_history)
        historyArr.append(rec.toJson());
    obj["history"] = historyArr;

    return obj;
}

Session *Session::fromJson(const QJsonObject &obj, QObject *parent) {
    QString id = obj["id"].toString();
    if (id.isEmpty())
        return nullptr;

    auto *session = new Session(id, parent);
    session->m_name = obj["name"].toString();
    session->m_pickMode = static_cast<PickMode>(obj["pickMode"].toInt());
    session->m_pickCount = obj["pickCount"].toInt(1);
    session->m_viewMode = static_cast<ViewMode>(obj["viewMode"].toInt(static_cast<int>(ViewMode::Normal)));
    session->m_pinned = obj["pinned"].toBool(false);
    session->m_createdAt = QDateTime::fromString(obj["createdAt"].toString(), Qt::ISODateWithMs);
    session->m_updatedAt = QDateTime::fromString(obj["updatedAt"].toString(), Qt::ISODateWithMs);

    QJsonArray namesArr = obj["names"].toArray();
    for (const auto &v : namesArr)
        session->m_names.append(v.toString());

    QJsonArray pickedArr = obj["pickedIndices"].toArray();
    for (const auto &v : pickedArr)
        session->m_pickedIndices.append(v.toInt());

    QJsonArray historyArr = obj["history"].toArray();
    for (const auto &v : historyArr)
        session->m_history.append(PickRecord::fromJson(v.toObject()));

    return session;
}

void Session::reloadFrom(const Session &other) {
    if (m_id != other.m_id)
        return; // 只允许同ID重载

    bool nameDiff = m_name != other.m_name;
    bool namesDiff = m_names != other.m_names;
    bool modeDiff = m_pickMode != other.m_pickMode;
    bool viewDiff = m_viewMode != other.m_viewMode;
    bool pinnedDiff = m_pinned != other.m_pinned;

    m_name = other.m_name;
    m_names = other.m_names;
    m_pickMode = other.m_pickMode;
    m_pickCount = other.m_pickCount;
    m_viewMode = other.m_viewMode;
    m_pinned = other.m_pinned;
    m_history = other.m_history;
    m_pickedIndices = other.m_pickedIndices;
    m_createdAt = other.m_createdAt;
    m_updatedAt = other.m_updatedAt;

    if (nameDiff) emit nameChanged(m_name);
    if (namesDiff) emit namesChanged();
    if (modeDiff) emit pickModeChanged(m_pickMode);
    if (viewDiff) emit viewModeChanged(m_viewMode);
    if (pinnedDiff) emit pinnedChanged(m_pinned);
    emit changed();
}
