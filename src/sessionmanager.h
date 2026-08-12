#ifndef SESSIONMANAGER_H
#define SESSIONMANAGER_H

#include <QHash>
#include <QObject>
#include <QStringList>

class Session;

class SessionManager : public QObject {
    Q_OBJECT
public:
    explicit SessionManager(QObject *parent = nullptr);
    ~SessionManager() override;

    // --- 会话操作 ---
    Session *createSession(const QString &name);
    Session *copySession(const QString &sourceId, const QString &newName);
    Session *getSession(const QString &id) const;
    QList<Session *> allSessions() const;
    QStringList allSessionNames() const;
    QStringList allSessionIds() const;
    bool deleteSession(const QString &id);
    bool renameSession(const QString &id, const QString &newName);
    void addLoadedSession(Session *session); // 外部加载的 session 加入管理

    // --- 持久化 ---
    bool saveSession(Session *session);
    void saveAll();
    void loadAll();

    // --- 刷新（对比磁盘和内存，处理增删改）---
    struct RefreshResult {
        QList<Session *> added;    // 磁盘新增、已加载到内存
        QStringList deletedIds;    // 磁盘上被删除的id
        QList<Session *> modified; // 磁盘比内存新，已重载到内存
    };
    RefreshResult refresh();

    QString sessionsDir() const;

signals:
    void sessionCreated(Session *session);
    void sessionDeleted(const QString &id);
    void sessionChanged(Session *session);

private:
    QString sessionFilePath(const QString &id) const;
    void connectSessionSignals(Session *session);

    QHash<QString, Session *> m_sessions;
};

#endif // SESSIONMANAGER_H
