#ifndef __MULTIDATABASE_H_
#define __MULTIDATABASE_H_

// C++ lib import
#include <functional>

// Qt lib import
#include <QtCore>
#include <QtSql>
#include <QSharedPointer>

#define PropertyDeclare(Type, Name, setName, ...)                   \
    private:                                                        \
    Type m_ ## Name __VA_ARGS__;                                    \
    public:                                                         \
    inline const Type &Name(void) const { return m_ ## Name; }      \
    inline void setName(const Type &Name) { m_ ## Name = Name; }    \
    private:

namespace multi_database_space
{

enum DatabaseModeEnum
{
    DatabaseNameMode,
    DatabaseHostMode
};

enum QueryMode
{
    QueryAutoMode, // 根据配置的配置数据库驱动类型自动选择QueryMode，默认QMYSQL、QODBC类型使用 QueryMultiMode
    QueryMultiMode, // 推荐多线程query数据库的场景下使用
    QuerySingleMode // 推荐仅主线程query数据库的场景下使用
};

class DatabaseSettings
{
private:
    PropertyDeclare(DatabaseModeEnum, databaseMode, setDatabaseMode)

    PropertyDeclare(QString, databaseType, setDatabaseType)
    PropertyDeclare(QString, connectionName, setConnectionName)

    // File mode
    PropertyDeclare(QString, nameModeName, setNameModeName)

    // Host mode
    PropertyDeclare(QString, hostModeHostName, setHostModeHostName)
    PropertyDeclare(QString, hostModeDatabaseName, setHostModeDatabaseName)
    PropertyDeclare(QString, hostModeUserName, setHostModeUserName)
    PropertyDeclare(QString, hostModePassword, setHostModePassword)

    private:
        DatabaseSettings(const DatabaseModeEnum &databastMode, const QString &databaseType, const QString &connectionName);

public:
    DatabaseSettings(const QString &databaseType, const QString &connectionName, const QString &nameModeName);

    DatabaseSettings(const QString &databaseType, const QString &connectionName, const QString &hostModeHostName, const QString &hostModeDatabaseName, const QString &hostModeUserName, const QString &hostModePassword);
};

/*!
 * \brief The ConnectSettings class
 * maxOpenTime: 打开数据库最大时间, 默认打开数据库起60秒内未进行查询，就自动断开。避免打开数据库后长时间空置，造成资源浪费和意外断开
 * QueryMode：查询模式，默认为QueryAutoMode，推荐大型数据库如MySql使用QueryMultiMode，例如Sqlite的轻量级数据库使用QuerySingleMode，详见enum QueryMode
 * minWaitTime: 查询数据库最小等待时间(最小查询间隔)，比如Sqlite数据库，密集查询时可能出错，可控制最小查询间隔，如10ms
 */
class ConnectSettings
{
private:
    PropertyDeclare(int, maxOpenTime, setMaxOpenTime)
    PropertyDeclare(QueryMode, queryMode, setQueryMode)
    PropertyDeclare(int, minWaitTime, setMinWaitTime)

    public:
        ConnectSettings(const int &maxOpenTime = 60 * 1000, const QueryMode &queryMode = QueryAutoMode, const int &minWaitTime = -1);
};

/*!
 * \brief The Query class
 * 对 QSqlQuery 的薄封装，以提供线程安全。
 */
class Query
{
private:
    QSharedPointer<QSqlQuery> m_query;
    QSharedPointer<QMutex> m_mutex;
    QSharedPointer<QSqlDatabase> m_database;

public:
    /*!
     * \brief Query
     * 内部资源使用swap技术，以提高资源利用效率。
     * \param other
     */
    Query(Query &&other);

    ~Query(void);

    inline QSharedPointer<QSqlQuery> operator->(void) { return m_query; }

    inline QSqlQuery &operator*(void) { return *m_query; }

    void swap(Query &other);

private:
    Query(QSharedPointer<QSqlDatabase> dataBase, QSharedPointer<QMutex> mutex);

    friend class Query;
    friend class ConnectNode;
};

/*!
 * \brief The ConnectNode class
 * 线程为单位的与数据库的连接点。
 * 提供超时关闭数据库的功能。
 */
class ConnectNode: public QObject
{
    Q_OBJECT

private:
    QSharedPointer<QSqlDatabase> m_database;
    QString m_connectionName;

    DatabaseSettings m_dataBaseSettings;
    ConnectSettings m_connectSettings;

    QSharedPointer<QTimer> m_autoClose;
    QSharedPointer<QMutex> m_mutex;

public:
    /*!
     * \brief ConnectNode 根据指定的数据库设置和连接设置(默认自动模式)的配置数据库和连接点
     * \param dataBaseSettings 数据库设置
     * \param connectSettings 连接设置
     */
    ConnectNode(const DatabaseSettings &dataBaseSettings, const ConnectSettings &connectSettings);

    ~ConnectNode(void);

    Query query(void);

signals:
    void startConnectionTiming(void);

    void stopConnectionTiming(void);

public slots:
    bool createDataBase(void);

    void removeDataBase(void);

    bool open(void);

    void close(void);

};

typedef qint64 ThreadId;
/*!
 * \brief The Control class
 * 维护数据库信息
 * 维护Query对象在不同线程中对数据库的原子操作。
 */
class Control
{
private:
    DatabaseSettings m_databaseSettings;
    ConnectSettings m_connectSettings;
    QMap<ThreadId, ConnectNode *> m_node;

    QMutex m_mutex;
    QTime m_wait;
    bool hasMinWaitTime;

public:
    /*!
     * \brief Control 根据指定的数据库设置和连接设置(默认自动模式)的初始化控制器
     * \param databaseSettings 数据库设置
     * \param connectSettings 连接设置
     */
    Control(const DatabaseSettings &databaseSettings, const ConnectSettings &connectSettings = ConnectSettings());

    Control(const Control &) = delete;

    Control(Control &&) = delete;

    ~Control(void);

public:
    /*!
     * \brief destroyAllConnection 销毁所有与数据库的连接
     */
    void destroyAllConnection(void);

    /*!
     * \brief query 获得一个连接到数据库的Query对象。
     * \return Query 对象
     */
    Query query(void);

private:
    inline void insertConnectNode(const ThreadId &key);

    void wait();

};

} // multi_database_space

#endif //__MULTIDATABASE_H_
