#ifndef TREE_H
#define TREE_H

#include <QMap>
#include "node.h"

typedef QMap<QString, Node> NodeMap;

class Tree
{
private:
    Node root;
    QSet<NodeFeature> featureSet;
    NodeMap nodeMap;
    QSet<Node> changeNodeSet;

public:
    explicit Tree();
    Node createNode();
    /*!
     * \brief take 从树上取下uid指定的节点和其子节点
     * \param uid 唯一标识标识
     * \return uid指定的节点
     */
    Node take(const QString &uid);
    void take(const Node &node);
    void destory(Node &node);
    NodeMap::iterator find(const QString &uid) const;
    QList<NodeFeature> nodeFeatureList() const;
    QList<Node> nodes() const;

    NodeMap::iterator end() const;

    QList<Node> changedNodeList() const;
};
#endif // TREE_H
