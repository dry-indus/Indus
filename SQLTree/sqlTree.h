#ifndef SQLTREE_H
#define SQLTREE_H

#include <QSet>
#include <QMap>
#include <QString>
#include <QSharedPointer>

#include "node.h"

namespace sql_tree_space {

class SqlInterface
{
    virtual void open() = 0;
    virtual void close() = 0;
    virtual const QString majorKeyName(const QString &tableName) const = 0;
    virtual const QStringList propertyFiledList(const QString &tableName) const = 0;
    virtual const QStringList foreignKeyFiledList(const QString &tableName) const = 0;
    virtual const bool hasTable(const QString &tableName) const = 0;
    virtual QStringList tables() = 0;
    virtual bool uniqueInsert(const QString tableName, const QMap<QString, QVariant> &valMap) = 0;
    virtual bool update(const QString tableName, const QMap<QString, QVariant> &valMap) = 0;
    virtual void prepareUniqueInsert(const QString tableName, const QMap<QString, QVariant> &valMap) = 0;
    virtual void prepareUpdate(const QString tableName, const QMap<QString, QVariant> &valMap) = 0;
    virtual void exeBath() = 0;
};

struct NodePorperty
{
    QString name;
    int type;
};

struct NodeFeature
{
    QString typeName;
    QString identifPorpertyName; //标识属性名 如uid
    QSet<NodePorperty> porpertySet;
    QSet<QString> childTypeNameSet; //子节点类型名集合

    static QList<NodeFeature>::const_iterator findNodeFeature(const QList<NodeFeature> &features, const QString &typeName);
};

typedef NodePorperty FiledPorperty; //字段属性

struct SqlTableFeature
{
    QString tableName; // 表名
    QString majorKeyName; // 主键名
    QSet<FiledPorperty> porpertyFiledSet; // 属性字段集合
    QSet<QString> foreignKeyNameSet; // 外键字段集合
};

struct ForeignKeyFiled
{
    QString name;
    QString refTableName;
    QString refFile;
};

/*!
 * \brief The SqlSynchro class
 * 数据库同步器
 */
class SqlSynchro{
private:
    SqlTree &m_tree;
    QSharedPointer<SqlInterface> m_sqlInterfacePtr;

public:
    explicit SqlSynchro(SqlTree &tree, QSharedPointer<SqlInterface> sqlInterface);

    /*!
     * \brief expandSql 扩展数据库
     * \param nodeFeature
     */
    void expandSql(const NodeFeature &nodeFeature);
    /*!
     * \brief convergenceSql 收敛数据库
     * \param tableFeature
     */
    void convergenceSql(const SqlTableFeature &tableFeature);

private:
    void appendFiledToTable(const QString &table, const QList<FiledPorperty> &fileds);
    void appendForeignKeyFiledToTable(const QString &table, const QList<ForeignKeyFiled> &fileds);
    void removeFiledFormTable(const QString &table, const QStringList &fileds);
    void createTable(const QString &table, const QList<FiledPorperty> &fileds, const QList<ForeignKeyFiled> &foreignFileds);
    void destoryTable(const QString &table);

      Q_DISABLE_COPY(SqlTree)
};

/*!
 * \brief The SqlTree class
 */
class SqlTree
{
public:
    enum SaveModel
    {
        free,
        expand,
        force,
    };

private:
    QSharedPointer<SqlInterface> m_sqlInterface;
    Tree tree;
    SqlSynchro synchro;
    QSet<Node> takenNodeSet;

public:
    explicit SqlTree(QSharedPointer<SqlInterface> sqlInterface);
    ~SqlTree();
    /*!
     * \brief save 根据指定的保存模式将发生改变的节点保存至数据库中。保存模式参见enum SaveModel
     * 注意保后会销毁taken的节点
     * \param model 保存模式
     */
    void save(SaveModel model = expand);
    /*!
     * \brief saveAll 根据指定的保存模式将Tree中所有节点保存至数据库中。保存模式参见enum SaveModel
     * 注意保后会销毁taken的节点
     * \param model 保存模式
     */
    void saveAll(SaveModel model = expand);
    bool load();

    Node createNode(const QString &uid, const QString &typeName);
    Node takeNode(const QString &uid);

private:
    void synchronizeSqlFeature(SaveModel model) const;
    bool canSave() const;
    void saveNodes(const QList<Node> &nodes) const;
    void destoryTakenNodes();

private:
    QStringList getSqlTableNameList();
    bool matchNodeFeature(const NodeFeature &feature) const;
    SqlTableFeature querySqlTableFeature(const QString &tableNane) const;
    QList<SqlTableFeature> querySqlTableFeature() const;

    Q_DISABLE_COPY(SqlTree)

    friend class SqlSynchro;
};

} // sql_tree_space
#endif // SQLTREE_H
