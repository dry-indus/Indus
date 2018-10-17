#include "sqlTree.h"
#include <QSqlQuery>

using namespace sql_tree_space;

template <typename Container>
bool isEqual(const Container &c1, const Container &c2)
{
    return (c1.size() == c2.size()) &&
            qEqual(c1.constBegin(), c1.constEnd(), c2.constBegin());
}

template <typename E>
bool isEqual(const QSet<E> &set1, const QSet<E> &set2)
{
    auto list1 = set1.toList();
    qSort(list1);
    auto list2 = set2.toList();
    qSort(list2);
    return isEqual(list1, list2);
}

static bool match(const NodeFeature &nodeFeature, const SqlTableFeature &sqlTableFeature)
{
    if (nodeFeature.typeName == sqlTableFeature.tableName) {
        if (nodeFeature.identifPorpertyName == sqlTableFeature.majorKeyName) {
            if (isEqual(nodeFeature.childTypeNameSet, sqlTableFeature.foreignKeyNameSet)) {
                if (isEqual(nodeFeature.porpertySet, sqlTableFeature.porpertyFiledSet)) {
                    return true;
                }
            }
        }
    }
    return false;
}

QList<NodeFeature>::const_iterator NodeFeature::findNodeFeature(const QList<NodeFeature> &features, const QString &typeName)
{
    auto constIterator = features.constBegin();
    for (; constIterator != features.constEnd(); ++constIterator)
    {
        if (constIterator->typeName == typeName)
            return constIterator;
    }
    return features.constEnd();
}

void Tree::take(const Node &node)
{

}

QList<NodeFeature> Tree::nodeFeatureList() const
{
    auto list = featureSet.toList();
    qSort(list);
    return list;
}

QList<Node> Tree::nodes() const {
    return nodeMap.values();
}


SqlSynchro::SqlSynchro(SqlTree &tree, QSharedPointer<SqlInterface> sqlInterface) :
    m_tree(tree),
    m_sqlInterfacePtr(sqlInterface)
{

}

void SqlSynchro::expandSql(const NodeFeature &nodeFeature)
{
    Q_ASSERT(m_sqlInterfacePtr);
    Q_ASSERT(!nodeFeature.typeName.isEmpty());

    QString tableName = nodeFeature.typeName;
    auto sqlTableFeature = m_tree.querySqlTableFeature(tableName);

    // 没有对应该类型节点的数据库表，则创建
    if (sqlTableFeature.tableName.isEmpty()){
        createTable(nodeFeature.typeName, nodeFeature.porpertySet.toList(), nodeFeature.childTypeNameSet.toList());
    }

    auto diffNodePorpertySet = nodeFeature.porpertySet - sqlTableFeature.porpertyFiledSet;
    appendFiledToTable(tableName, diffNodePorpertySet.toList());

    auto diffChildFtNameSet = nodeFeature.childTypeNameSet - sqlTableFeature.foreignKeyNameSet;
    appendForeignKeyFiledToTable(tableName, diffChildFtNameSet.toList());
}

void SqlSynchro::convergenceSql(const SqlTableFeature &tableFeature)
{
    Q_ASSERT(m_sqlInterfacePtr);
    Q_ASSERT(!tableFeature.tableName.isEmpty());
    QString tableName = tableFeature.tableName;
    auto nodeFeatureList = m_tree.tree.nodeFeatureList();
    auto findIt = NodeFeature::findNodeFeature(nodeFeatureList, tableName);
    if (findIt == nodeFeatureList.constEnd()) {
        destoryTable(tableName);
        return;
    }

    auto diffFiledSet = (tableFeature.porpertyFiledSet - findIt->porpertySet) +
            (tableFeature.foreignKeyNameSet - findIt->childTypeNameSet);
    removeFiledFormTable(tableName, diffFiledSet.toList());
}

void SqlSynchro::appendFiledToTable(const QString &table, const QList<FiledPorperty> &fileds)
{

}

SqlTree::SqlTree()
{

}

SqlTree::SqlTree(QSharedPointer<SqlInterface> sqlInterface):
    m_sqlInterface(sqlInterface),
    synchro(*this, sqlInterface)
{

}

SqlTree::~SqlTree()
{

}

void SqlTree::save(SaveModel model)
{
    if (!canSave()) {
        if (SqlTree::free != model){
            synchronizeSqlFeature(model);
            save(model);
        }
    } else {
        auto changedNodes = tree.changedNodeList();
        saveNodes(changedNodes);
    }
    destoryTakenNodes();
}

void SqlTree::saveAll(SqlTree::SaveModel model)
{
    if (!canSave()) {
        if (SqlTree::free != model) {
            synchronizeSqlFeature(model);
            saveAll(model);
        }
    } else {
        auto nodes = tree.nodes();
        saveNodes(nodes);
    }
    destoryTakenNodes();
}

bool SqlTree::load()
{

}

Node SqlTree::takeNode(const QString &uid)
{
    auto findIt = tree.find(uid);
    if (findIt != tree.end()) {
        auto nodes = Node::getAssociatedNodes(*findIt);
        takenNodeSet << QSet::fromList(nodes);
    }
}

void SqlTree::synchronizeSqlFeature(SqlTree::SaveModel model) const
{
    auto nodeFeatureList = tree.nodeFeatureList();
    foreach (const auto &nodeFt, nodeFeatureList) {
        synchro.expandSql(nodeFt);
    }

    if (SqlTree::force == model){
        auto tableFeatures = querySqlTableFeature();
        foreach (const auto &feature, tableFeatures) {
            synchro.convergenceSql(feature);
        }
    }
}

bool SqlTree::canSave() const
{
    // 数据库无表、sqlTree与目标数据库表不匹配
    // 则不能保存sqlTree数据到目标数据库

    auto nodeFeatureList = tree.nodeFeatureList();
    foreach (const auto &nodeFt, nodeFeatureList) {
        if (!matchNodeFeature(nodeFt))
            return false;
    }

    return true;
}

void SqlTree::saveNodes(const QList<Node> &nodes) const
{
    if (nodes.isEmpty())
        return;

    foreach (const auto &node, nodes) {
        if (node.isValid()) {
            // 节点的类型名与数据库表名有对应关系
            Q_ASSERT(m_sqlInterface.hasTable(node.typeName()));
            QString tableName = node.typeName();
            QMap valMap = node.propertyMap();
            m_sqlInterface->prepareUniqueInsert(tableName, valMap);
        }
    }
    m_sqlInterface->exeBath();
}

void SqlTree::destoryTakenNodes()
{
    foreach (auto &node, takenNodeSet) {
        tree.destory(node);
    }
    takenNodeSet.clear();
}

QStringList SqlTree::getSqlTableNameList()
{
    Q_ASSERT(m_sqlInterface);
    m_sqlInterface->tables();
}

bool SqlTree::matchNodeFeature(const NodeFeature &feature) const
{
    SqlTableFeature feature = querySqlTableFeature(feature.tableName);
    return match(feature, feature);
}

SqlTableFeature SqlTree::querySqlTableFeature(const QString &tableNane) const
{
    Q_ASSERT(m_sqlInterface);
    SqlTableFeature feature;
    if (m_sqlInterface->hasTable(tableNane))
    {
        QStringList propertyFiledList = m_sqlInterface->propertyFiledList(tableNane);
        QStringList foreignKeyFiled = m_sqlInterface->foreignKeyFiledList(tableNane);
        QString majorKeyName = m_sqlInterface->majorKeyName(tableNane);

        feature.tableName = tableNane;
        feature.porpertyFiledSet = propertyFiledList;
        feature.majorKeyName = majorKeyName;
        feature.foreignKeyNameSet = foreignKeyFiled;
    }
    return feature;
}

QList<SqlTableFeature> SqlTree::querySqlTableFeature() const
{
    Q_ASSERT(m_sqlInterface);
    QList<SqlTableFeature> featureList;
    auto dbTableList = getSqlTableNameList();
    foreach (const QString &tableName, dbTableList) {
        auto feature = querySqlTableFeature(tableName);
        if (!feature.tableName.isEmpty()) {
            featureList << feature;
        }
    }
    return featureList;
}
