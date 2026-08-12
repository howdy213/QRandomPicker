#include "sessionmanager.h"
#include "session.h"
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QUuid>

SessionManager::SessionManager(QObject *parent)
    : QObject(parent) {
    // 确保 sessions 目录存在
    QDir dir(sessionsDir());
    if (!dir.exists())
        dir.mkpath(".");
}

SessionManager::~SessionManager() {
    saveAll();
    qDeleteAll(m_sessions);
}

QString SessionManager::sessionsDir() const {
    return QDir::currentPath() + "/sessions";
}

QString SessionManager::sessionFilePath(const QString &id) const {
    return sessionsDir() + "/" + id + ".json";
}

void SessionManager::connectSessionSignals(Session *session) {
    connect(session, &Session::changed, this, [this, session]() {
        emit sessionChanged(session);
    });
    connect(session, &Session::nameChanged, this, [this, session]() {
        emit sessionChanged(session);
    });
}

Session *SessionManager::createSession(const QString &name) {
    auto *session = new Session(this);
    session->setName(name);
    m_sessions.insert(session->id(), session);
    connectSessionSignals(session);
    saveSession(session);
    emit sessionCreated(session);
    return session;
}

Session *SessionManager::copySession(const QString &sourceId, const QString &newName) {
    Session *source = getSession(sourceId);
    if (!source)
        return nullptr;

    auto *session = new Session(this);
    session->setName(newName);
    session->setNames(source->names());
    session->setPickMode(source->pickMode());
    session->setPickCount(source->pickCount());
    session->setViewMode(source->viewMode());

    m_sessions.insert(session->id(), session);
    connectSessionSignals(session);
    saveSession(session);
    emit sessionCreated(session);
    return session;
}

Session *SessionManager::getSession(const QString &id) const {
    return m_sessions.value(id, nullptr);
}

QList<Session *> SessionManager::allSessions() const {
    return m_sessions.values();
}

QStringList SessionManager::allSessionNames() const {
    QStringList names;
    for (auto *s : m_sessions)
        names.append(s->name());
    return names;
}

QStringList SessionManager::allSessionIds() const {
    return m_sessions.keys();
}

void SessionManager::addLoadedSession(Session *session) {
    if (!session)
        return;
    if (m_sessions.contains(session->id())) {
        // 已存在则替换（保留旧指针的父对象关系处理由调用方负责）
        return;
    }
    session->setParent(this);
    m_sessions.insert(session->id(), session);
    connectSessionSignals(session);
    emit sessionCreated(session);
}

bool SessionManager::deleteSession(const QString &id) {
    Session *session = m_sessions.take(id);
    if (!session)
        return false;

    QFile::remove(sessionFilePath(id));
    emit sessionDeleted(id);
    session->deleteLater();
    return true;
}

bool SessionManager::renameSession(const QString &id, const QString &newName) {
    Session *session = getSession(id);
    if (!session)
        return false;
    session->setName(newName);
    saveSession(session);
    return true;
}

bool SessionManager::saveSession(Session *session) {
    if (!session)
        return false;

    QDir dir(sessionsDir());
    if (!dir.exists())
        dir.mkpath(".");

    QJsonObject json = session->toJson();
    QJsonDocument doc(json);

    QString path = sessionFilePath(session->id());
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
        return false;

    file.write(doc.toJson(QJsonDocument::Indented));
    file.close();
    return true;
}

void SessionManager::saveAll() {
    for (auto *session : std::as_const(m_sessions))
        saveSession(session);
}

void SessionManager::loadAll() {
    QDir dir(sessionsDir());
    if (!dir.exists())
        return;

    QStringList filters;
    filters << "*.json";
    dir.setNameFilters(filters);
    QStringList files = dir.entryList(QDir::Files | QDir::NoDotAndDotDot);

    for (const QString &fileName : std::as_const(files)) {
        QString filePath = dir.absoluteFilePath(fileName);
        QFile file(filePath);
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
            continue;

        QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
        file.close();

        Session *session = Session::fromJson(doc.object(), this);
        if (session) {
            m_sessions.insert(session->id(), session);
            connectSessionSignals(session);
            emit sessionCreated(session);
        }
    }
}

SessionManager::RefreshResult SessionManager::refresh() {
    RefreshResult result;

    QDir dir(sessionsDir());
    QStringList filters;
    filters << "*.json";
    dir.setNameFilters(filters);
    QFileInfoList fileInfos = dir.entryInfoList(QDir::Files | QDir::NoDotAndDotDot);

    // 构建磁盘文件的 id -> fileInfo 映射
    QHash<QString, QFileInfo> diskFiles;
    for (const auto &fi : fileInfos) {
        QString id = fi.completeBaseName(); // 去掉.json的部分
        if (!id.isEmpty())
            diskFiles.insert(id, fi);
    }

    QStringList memIds = m_sessions.keys();

    // 1. 磁盘有但内存没有 -> 新增加载
    for (auto it = diskFiles.constBegin(); it != diskFiles.constEnd(); ++it) {
        const QString &id = it.key();
        if (!memIds.contains(id)) {
            QFile file(it.value().absoluteFilePath());
            if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
                QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
                file.close();
                Session *session = Session::fromJson(doc.object(), this);
                if (session) {
                    m_sessions.insert(id, session);
                    connectSessionSignals(session);
                    result.added.append(session);
                    emit sessionCreated(session);
                }
            }
        }
    }

    // 2. 内存有但磁盘没有 -> 标记删除（不删除内存session，由调用方处理提示）
    for (const QString &id : std::as_const(memIds)) {
        if (!diskFiles.contains(id)) {
            result.deletedIds.append(id);
        }
    }

    // 3. 两边都有但磁盘更新时间 > 内存 updatedAt -> 重载
    for (const QString &id : std::as_const(memIds)) {
        if (!diskFiles.contains(id))
            continue;
        Session *s = m_sessions[id];
        if (!s)
            continue;
        QDateTime diskTime = diskFiles[id].lastModified();
        // 如果磁盘文件比内存的更新时间更新（差距>1秒），则重载
        if (diskTime.isValid() && diskTime.addSecs(1) > s->updatedAt()) {
            QFile file(diskFiles[id].absoluteFilePath());
            if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
                QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
                file.close();
                Session *loaded = Session::fromJson(doc.object(), nullptr);
                if (loaded && loaded->id() == s->id()) {
                    s->reloadFrom(*loaded);
                    delete loaded;
                    result.modified.append(s);
                    emit sessionChanged(s);
                } else {
                    delete loaded;
                }
            }
        }
    }

    return result;
}
