// Qt lib import
#include <QCoreApplication>
#include <QtConcurrent>
#include <QSqlError>

// JasonQt lib import
#include "multiDatabase/multiDatabase.h"

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    /*
     * 注：关于附加参数
     * 使用默认值即可。
     * maxOpenTime: 打开数据库最大时间, 默认打开数据库起60秒内未进行查询，就自动断开。避免打开数据库后长时间空置，造成资源浪费和意外断开
     * QueryMode：查询模式，默认为QueryAutoMode，推荐大型数据库如MySql使用QueryMultiMode，例如Sqlite的轻量级数据库使用QuerySingleMode，详见enum QueryMode
     * minWaitTime: 查询数据库最小等待时间(最小查询间隔)，比如Sqlite数据库，密集查询时可能出错，可控制最小查询间隔，如10ms
     */

    // Sqlite的连接方式                      类型        连接名      Sqlite文件路径   单次打开数据库最大时间        查询模式                最小等待时间
    multi_database_space::Control control({ "QSQLITE", "TestDB", "./test.db" }, { 60 * 1000, multi_database_space::QuerySingleMode, 10});

    // MySql的连接方式                      类型      连接名        IP           数据库     用户名        密码
    // JasonQt_Database::Control control({ "QMYSQL", "TestDB", "localhost", "JasonDB", "root", "YourPassword" });


    // SqlServer的连接方式                   类型      连接名         数据库                                         数据库名     用户名         密码
    // JasonQt_Database::Control control({ "QODBC", "TestDB", "Driver={SQL SERVER};server=iZ23kn6vmgkZ\\TEST;database=test;uid=sa;pwd=YourPassword;" });


    control.query()->exec("create table Test1( \
                              data int not NULL \
                          );");

    control.query()->exec("create table Test2( \
                              num integer primary key autoincrement,\
                              data1 int not NULL, \
                              data2 text not NULL \
                          );");

    auto insert = [&]()
    {
        auto query(control.query()); // query解引用（->）后返回的是 QSqlQuery

        query->prepare("insert into Test1 values(?)"); // 模拟插入操作

        query->addBindValue(rand() % 1280);

        if(!query->exec())
        {
            qDebug() << "Error" << __LINE__;
        }

        query->clear();

        query->prepare("insert into Test2 values(NULL, ?, ?)");

        query->addBindValue(rand() % 1280);
        QString buf;
        for(int now = 0; now < 50; now++)
        {
            buf.append('a' + (rand() % 26));
        }
        query->addBindValue(buf);

        if(!query->exec())
        {
            qDebug() << "Error" << __LINE__;
        }
    };
    auto delete_ = [&]()
    {
        auto query(control.query());

        query->prepare("delete from Test1 where data = ?");

        query->addBindValue(rand() % 1280);

        if(!query->exec())
        {
            qDebug() << "Error" << __LINE__;
        }

        query->clear();

        query->prepare("delete from Test2 where data1 = ?");

        query->addBindValue(rand() % 1280);

        if(!query->exec())
        {
            qDebug() << "Error" << __LINE__;
        }
    };
    auto update = [&]()
    {
        auto query(control.query());

        query->prepare("update Test1 set data = ? where data = ?");

        query->addBindValue(rand() % 1280);
        query->addBindValue(rand() % 1280);

        if(!query->exec())
        {
            qDebug() << "Error" << __LINE__;
        }

        query->clear();

        query->prepare("update Test2 set data1 = ?, data2 = ? where data1 = ?");

        query->addBindValue(rand() % 1280 + 1);
        QString buf;
        for(int now = 0; now < 50; now++)
        {
            buf.append('a' + (rand() % 26));
        }
        query->addBindValue(buf);
        query->addBindValue(rand() % 1280 + 1);

        if(!query->exec())
        {
            qDebug() << "Error" << __LINE__;
        }
    };
    auto select = [&]()
    {
        auto query(control.query());

        query->prepare("select * from Test1 where data = ?");

        query->addBindValue(rand() % 1280);

        if(!query->exec())
        {
            qDebug() << "Error" << __LINE__;
        }

        query->clear();

        query->prepare("select * from Test2 where data1 = ?");

        query->addBindValue(rand() % 1280);

        if(!query->exec())
        {
            qDebug() << "Error" << __LINE__;
        }
    };

    volatile int count = 0, last = 0;

    QTime time;
    time.start();

    QThreadPool::globalInstance()->setMaxThreadCount(10);

    for(int now = 0; now < 3; now++) // 开启3个线程测试
    {
        QtConcurrent::run([&]()
        {
            while(true)
            {
                count++;
                if(!(count % 1000))
                {
                    const auto &&now = time.elapsed();
                    qDebug() << now - last; // 打印每完成1000语句的时间
                    last = now;
                }

                switch(rand() % 20)
                {
                case 0:  { insert(); break; }
                case 1:  { delete_(); break; }
                case 2:  { update(); break; }
                default: { select(); break; }
                }
            }
            QThread::msleep(10);
        });
    }

    //    QTimer::singleShot(10000, [&](){
    //      control.destroyAllConnection();
    //    });

    return a.exec();
}
