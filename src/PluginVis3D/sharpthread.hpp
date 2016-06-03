#pragma once

#include <functional>

#include <QtCore/QThread>

class SharpThread : public QThread
{
    Q_OBJECT
    std::function<void()> method;
public:
    explicit SharpThread(std::function<void()> method, QObject *parent = 0) :
        QThread(parent),
        method(method)
    {

    }

private:
    virtual void run()
    {
        if(this->method) this->method();
    }

signals:

public slots:

};

// Local Variables:
// mode: c++
// tab-width: 4
// c-basic-offset: 4
// End:
