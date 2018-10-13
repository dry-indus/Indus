#include "multiDatabase.h"

using namespace multi_database_space;

// DatabaseSettings
DatabaseSettings::DatabaseSettings(const DatabaseModeEnum &databastMode, const QString &databaseType, const QString &connectionName)
{
    m_databaseMode = databastMode;
    m_databaseType = databaseType;
    m_connectionName = connectionName;
}

DatabaseSettings::DatabaseSettings(const QString &databaseType, const QString &connectionName, const QString &nameModeName):
    DatabaseSettings(DatabaseNameMode, databaseType, connectionName)
{
    m_nameModeName = nameModeName;
}

DatabaseSettings::DatabaseSettings(const QString &databaseType, const QString &connectionName, const QString &hostModeHostName, const QString &hostModeDatabaseName, const QString &hostModeUserName, const QString &hostModePassword):
    DatabaseSettings(DatabaseHostMode, databaseType, connectionName)
{
    m_hostModeHostName = hostModeHostName;
    m_hostModeDatabaseName = hostModeDatabaseName;
    m_hostModeUserName = hostModeUserName;
    m_hostModePassword = hostModePassword;
}

// ConnectSettings
ConnectSettings::ConnectSettings(const int &maxOpenTime, const QueryMode &queryMode, const int &minWaitTime)
{
    m_maxOpenTime = maxOpenTime;
    m_queryMode = queryMode;
    m_minWaitTime = minWaitTime;
}

// Query
Query::Query(QSharedPointer<QSqlDatabase> dataBase, QSharedPointer<QMutex> mutex):
    m_query(new QSqlQuery(*dataBase)),
    m_mutex(mutex),
    m_database(dataBase)
{

}

Query::Query(Query &&other)
{
    swap(other);
}

Query::~Query(void)
{
    if(m_query)
    {
        delete m_query;
    }
    if(m_mutex)
    {
        m_mutex->unlock();
    }
}

void Query::swap(Query &other)
{
    swapQuery(other);
    swapMutex(other);
}

void Query::swapQuery(Query &other)
{
    auto *tempQuery = m_query;
    m_query = other.m_query;
    other.m_query = tempQuery;
}

void Query::swapMutex(Query &other)
{
    m_mutex.swap(other.m_mutex);
}

// ConnectNode
ConnectNode::ConnectNode(const DatabaseSettings &dataBaseSettings, const ConnectSettings &connectSettings):
    m_dataBaseSettings(dataBaseSettings),
    m_connectSettings(connectSettings)
{
    m_connectionName = QString("%1(%2)").arg(m_dataBaseSettings.connectionName()).arg(QString::number(qint64(QThread::currentThread()), 16));

    m_mutex = QSharedPointer<QMutex>(new QMutex(QMutex::Recursive));

    if(m_connectSettings.maxOpenTime())
    {
        m_autoClose = QSharedPointer<QTimer>(new QTimer);
        m_autoClose->setSingleShot(true);
        m_autoClose->setInterval(m_connectSettings.maxOpenTime());
        m_autoClose->moveToThread(qApp->thread());
        m_autoClose->setParent(qApp);

        connect(m_autoClose.data(), SIGNAL(timeout()), this, SLOT(close()), Qt::DirectConnection);
        connect(this, SIGNAL(startConnectionTiming()), m_autoClose.data(), SLOT(start()));
        connect(this, SIGNAL(stopConnectionTiming()), m_autoClose.data(), SLOT(stop()));
    }

    createDataBase();
}

ConnectNode::~ConnectNode(void)
{
    m_autoClose->disconnect();
    removeDataBase();
}

Query ConnectNode::query(void)
{
    if(!m_database)
    {
        createDataBase();
    }

    if(!m_database->isOpen())
    {
        m_database->open();
    }

    if(m_mutex){ m_mutex->lock(); }

    emit startConnectionTiming();
    return Query(m_database, m_mutex);
}

bool ConnectNode::createDataBase(void)
{
    if(m_mutex) { m_mutex->lock(); }

    if(m_database) {
        removeDataBase();
    }

    if (!m_database) {
        m_database = QSharedPointer<QSqlDatabase>(new QSqlDatabase(QSqlDatabase::addDatabase(m_dataBaseSettings.databaseType(), m_connectionName)));
    }

    switch(m_dataBaseSettings.databaseMode())
    {
    case DatabaseNameMode: {
        m_database->setDatabaseName(m_dataBaseSettings.nameModeName());
        break;
    }
    case DatabaseHostMode: {
        m_database->setHostName(m_dataBaseSettings.hostModeHostName());
        m_database->setDatabaseName(m_dataBaseSettings.hostModeDatabaseName());
        m_database->setUserName(m_dataBaseSettings.hostModeUserName());
        m_database->setPassword(m_dataBaseSettings.hostModePassword());
        break;
    }
    default:{
        if(m_mutex){ m_mutex->unlock(); }
        return false;
    }
    }

    bool flag = open();
    if(m_mutex){ m_mutex->unlock(); }
    return flag;
}

void ConnectNode::removeDataBase(void)
{
    if(m_mutex){ m_mutex->lock(); }

    QSqlDatabase::removeDatabase(m_connectionName);

    if(m_mutex){ m_mutex->unlock(); }
}

bool ConnectNode::open(void)
{
    if(!m_database) {
        createDataBase();
    }

    if(m_mutex){ m_mutex->lock(); }

    emit startConnectionTiming();
    const bool flag = m_database->open();

    if(m_mutex){ m_mutex->unlock(); }
    return flag;
}

void ConnectNode::close(void)
{
    if(m_mutex)
    {
        if(m_mutex->tryLock())
        {
            m_mutex->unlock();
            emit stopConnectionTiming();
            if (m_database) {
                m_database->close();
            }
        }
        else
        {
            emit startConnectionTiming();
        }
    }
    else
    {
        emit stopConnectionTiming();
        if (m_database) {
            m_database->close();
        }
    }
}

// Control
Control::Control(const DatabaseSettings &databaseSettings, const ConnectSettings &connectSettings):
    m_databaseSettings(databaseSettings),
    m_connectSettings(connectSettings),
    hasMinWaitTime(false)
{
    if(m_connectSettings.queryMode() == QueryAutoMode)
    {
        if(databaseSettings.databaseType() == "QMYSQL") {
            m_connectSettings.setQueryMode(QueryMultiMode);
        } else if(databaseSettings.databaseType() == "QODBC") {
            m_connectSettings.setQueryMode(QueryMultiMode);
        } else {
            m_connectSettings.setQueryMode(QuerySingleMode);
        }
    }
    if(m_connectSettings.queryMode() == QuerySingleMode)
    {
        insertConnectNode(ThreadId(QThread::currentThread()));
    }

    if(m_connectSettings.minWaitTime() == -1)
    {
        if(databaseSettings.databaseType() == "QMYSQL") {
            m_connectSettings.setMinWaitTime(0);
        } else if(databaseSettings.databaseType() == "QODBC") {
            m_connectSettings.setMinWaitTime(0);
        } else {
            m_connectSettings.setMinWaitTime(5);
            hasMinWaitTime = true;
            m_wait.start();
        }
    }
    else
    {
        hasMinWaitTime = true;
        m_wait.start();
    }
}

Control::~Control(void)
{
    destroyAllConnection();
}

void Control::destroyAllConnection(void)
{
    m_mutex.lock();

    qDeleteAll(m_node) ;
    m_node.clear();

    m_mutex.unlock();
}

Query Control::query(void)
{
    wait();

    const auto &&currentThread = qint64(QThread::currentThread());
    if(m_connectSettings.queryMode() == QueryMultiMode)
    {
        const auto &&now = m_node.find(currentThread);
        if(now != m_node.end()) {
            return Query((*now)->query());
        } else {
            m_mutex.lock();
            insertConnectNode(currentThread);
            m_mutex.unlock();
            return query();
        }
    }
    else
    {
        if (m_node.isEmpty()) {
            insertConnectNode(currentThread);
            return query();
        }
        return m_node.first()->query();
    }
}

void Control::insertConnectNode(const ThreadId &key)
{
    m_node.insert(key, new ConnectNode(m_databaseSettings, m_connectSettings));
}

void Control::wait()
{
    if(hasMinWaitTime)
    {
        const auto &&flag = m_connectSettings.minWaitTime() - m_wait.elapsed();
        m_wait.restart();
        if(flag > 0) {
            QThread::msleep(flag);
        }
    }
}
