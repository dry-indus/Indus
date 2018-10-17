#ifndef NODE_H
#define NODE_H

#include <QString>
#include <QSharedPointer>
#include <typeinfo>
#include <QDebug>

#define CLONE(class_name) \
    public: \
    typedef class_name type; \
    QSharedPointer<type> clone() const { \
    return QSharedPointer<type>(new type(*this)); \
    }

class NodePrivate;
typedef QSharedPointer<NodePrivate> NodePrivatePtr;
typedef QMap<QString, QVariant> PorpertyMap;

class NodePrivate
{
    CLONE(NodePrivate)
    private:
        QString m_typeName;
    NodePrivatePtr m_parent;
    QSet<NodePrivatePtr> childs;
    PorpertyMap m_porpertyMap;
    bool m_isVaild;

private:
    explicit NodePrivate(const QString &typeName, const NodePrivatePtr &parent) :
        m_typeName(typeName),
        m_isVaild(true),
        m_parent(parent) {}

    NodePrivate(const NodePrivate &other) :
        m_typeName(other.m_typeName),
        m_parent(other.m_parent),
        m_porpertyMap(other.m_porpertyMap),
        m_isVaild(other.m_isVaild()) {}

    NodePrivate(const NodePrivatePtr &other) :
        NodePrivate(*other) {}

    inline bool isVaild() const
    { return m_isVaild; }

    /*!
     * \brief repeal 废除
     */
    void repeal()
    { m_isVaild  = false; }

    inline setParent(const NodePrivatePtr &parent)
    { m_parent = parent; }

    void insetChild(const NodePrivatePtr &child)
    { childs.insert(child); }

    void take(const NodePrivatePtr &child)
    { childs.remove(child); }

public:
    virtual ~NodePrivate()
    { qDebug() << "~NodePrivate()"; }

    friend class Node;
};

class Node;
typedef QList<Node> NodeList;

/*!
 * \brief The Node class
 */
class Node
{
    CLONE(Node)

    private:
        NodePrivatePtr m_p; // share指针, 管理私有指针

public:
    explicit Node(const QString &uid, const QString &typeName, const Node &parent) :
        m_p(new NodePrivate(typeName, parent.m_p))
    {
        setProperty("uid", uid);
    }

    Node(const Node &other) : m_p(other.m_p->clone()) {}

    Node(Node &&other)
    {
        m_p.swap(other.m_p); // 窃取 other 的私有指针
    }

    inline QString typeName() const
    {
        Q_ASSERT(m_p);
        return m_p->m_typeName;
    }

    inline void setProperty(const QString &propertyName, const QVariant &variant) const
    {
        Q_ASSERT(m_p);
        if (m_p->isVaild()) {
            m_p->m_porpertyMap.insert(propertyName, variant);
        }
    }

    inline PorpertyMap::iterator property(const QString &propertyName) const
    {
        Q_ASSERT(m_p);
        if (m_p->isVaild()) {
            m_p->m_porpertyMap.find(propertyName);
        } else {
            m_p->m_porpertyMap.end();
        }
    }

    inline void clear()
    {
        Q_ASSERT(m_p);
        NodePrivatePtr temp;
        m_p.swap(temp);
        m_p->m_isVaild = temp->m_isVaild;
    }

    /*!
     * \brief repeal 废除
     */
    inline void repeal() const
    {
        Q_ASSERT(m_p);
        m_p->repeal();
    }

    inline NodeList childs() const
    {
        Q_ASSERT(m_p);
        m_p->childs.toList();
    }

    inline QStringList propertyNameList() const
    {
        Q_ASSERT(m_p);
        return m_p->m_porpertyMap.keys();
    }

    inline QStringList childTypeNameList() const
    {
        Q_ASSERT(m_p);
        QStringList list;
        foreach (const auto &child, m_p->childs) {
            list << child->typeName();
        }
        return list;
    }

};

#endif // NODE_H
