#ifndef NODE_H
#define NODE_H

#include <QString>
#include <QSharedPointer>
#include <typeinfo>
#include <QDebug>

#define CAST \
    virtual auto cast()->decltype(this) { \
    return this; \
    }

#define CLONE \
    virtual Inner *clone() const{ \
    auto &local = *this; \
    typedef std::remove_const<std::remove_reference<decltype(local)>::type>::type CurrentClass; \
    return new CurrentClass(*this); \
    }

#define INNER \
    public: \
    CAST \
    CLONE

struct Inner
{
    INNER
    public:
    virtual ~Inner() { qDebug() << "~Inner()"; }

    inline bool isVaild() const { return m_isVaild; }

    /*!
     * \brief destory 销毁
     */
    virtual void destory() { m_isVaild  = false; }

protected:
    Inner() : m_isVaild(true) {}

    Inner(const Inner &other)
        : m_isVaild(other.isVaild()){}

private:
    struct Invaild {};

    Inner(Invaild) : m_isVaild(false) {}

    bool m_isVaild;

    friend class OutCaseBase;
};

class TestNodeData : public Inner
{
    INNER
    public:
        TestNodeData &operator=(const TestNodeData &other)
    {
        a = other.a;
        return *this;
    }

    QString a = "A";
};

class StationNodeData : public Inner
{
    INNER
    public:
        StationNodeData &operator=(const StationNodeData& other)
    {
        a = other.a;
        return *this;
    }

    int a;
};

class OutCaseBase
{
    typedef QSharedPointer<Inner> NodeDataPtr;
    typedef Inner::Invaild InvaildData;

public:
    explicit OutCaseBase() : m_dataPtr(new Inner(InvaildData())) {}

    OutCaseBase(const OutCaseBase &other) : m_dataPtr(other.m_dataPtr->clone()) {}

    OutCaseBase(OutCaseBase &&other) : m_dataPtr(new Inner(InvaildData())) {
        // 窃取 other 的 InnerTank
        m_dataPtr.swap(other.m_dataPtr);
    }

    inline void setData(const Inner &data)
    {
        NodeDataPtr cloneData(data.clone());
        Q_ASSERT(cloneData);
        m_dataPtr.swap(cloneData);
    }

    inline void setData(Inner &&data)
    {
        setData(static_cast<const Inner &>(data)); // 使用static_cast得到data的左值引用
        data.destory(); // 销毁data 以完成窃取
    }

    template<typename T>
    inline const T &inner() const
    {
        Q_ASSERT(m_dataPtr);
        auto d = m_dataPtr->cast();
        Q_ASSERT(d);
        Q_ASSERT(typeid(T) == typeid(*d));
        return *static_cast<const T *>(d); // static_cast仅为指针添加const
    }

    template<typename T>
    T innerPtr() const
    {
        Q_ASSERT(m_dataPtr);
        auto d = m_dataPtr->cast();
        Q_ASSERT(d);
        return dynamic_cast<T const>(d);
    }

    inline void clear()
    {
        NodeDataPtr temp;
        m_dataPtr.swap(temp);
    }

private:
    NodeDataPtr m_dataPtr; // share指针, 管理Inner
};

#endif // NODE_H
