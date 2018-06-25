#ifndef NODE_H
#define NODE_H

#include <QString>
#include <QSharedPointer>
#include <typeinfo>
#include <QDebug>

#define CLONE \
    public: \
    virtual AbstructData *clone() const { \
    auto local = *this; \
    using CurrentClass = decltype(local); \
    CurrentClass *cc = new CurrentClass(); \
    *cc = local; \
    return cc; \
    }

struct AbstructData
{
    CLONE
    public:
        AbstructData &operator=(const AbstructData &other) {
        return *this;
    }

    template <typename T>
    const T &castData() const
    {
        try {
            dynamic_cast<const T &>(*this);
        } catch(const std::bad_cast &bc) {
            QString errorInfo = QString("%1 dynamic_cast %2 is %3")
                    .arg(typeid(*this).name()).arg(typeid(T).name()).arg(bc.what());
            qFatal(errorInfo.toStdString().c_str());
        }
        return dynamic_cast<const T &>(*this);;
    }

protected:
    AbstructData() {}
    AbstructData(const AbstructData &) {}

    friend class Node;
};

class TestNodeData : public AbstructData
{
public:
    TestNodeData &operator=(const TestNodeData &other)
    {
        a = other.a;
        return *this;
    }

    QString a = "A";

    CLONE
};

class StationNodeData : public AbstructData
{
    CLONE
    public:
        StationNodeData &operator=(const StationNodeData& other)
    {
        a = other.a;
        return *this;
    }

    int a;
};

class Node
{
    typedef QSharedPointer<AbstructData> NodeDataPtr;

public:
    Node()
        : m_dataPtr(new AbstructData()) {}

    Node(const Node *other)
        : m_dataPtr(other->m_dataPtr) { }

    void setData(const AbstructData &data)
    {
        NodeDataPtr cloneData(data.clone());
        Q_ASSERT(cloneData);
        m_dataPtr.swap(cloneData);
    }

    const AbstructData &data() const
    {
        Q_ASSERT(m_dataPtr);
        return *m_dataPtr;
    }

    void clear()
    {
        NodeDataPtr temp;
        m_dataPtr.swap(temp);
    }

private:
    NodeDataPtr m_dataPtr;
};

#endif // NODE_H
