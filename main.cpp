/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   main.cpp
 * Author: ducnt
 *
 * Created on July 26, 2018, 1:32 PM
 */

#include <iostream>
#include <stdio.h>
#include <string>
#include <csignal>

#include <boost/shared_ptr.hpp>

#include <Poco/Thread.h>
#include <Poco/ThreadPool.h>
#include <Poco/Runnable.h>
#include <Poco/NotificationQueue.h>
#include <Poco/Notification.h>
#include "Poco/AutoPtr.h"


Poco::NotificationQueue g_nofiQueue;
bool g_flag = true;

class Object {
private:
    int m_idata;
    std::string m_sdata;

public:
    Object(int in, std::string s) : m_idata(in), m_sdata(s) {

    }
    
    std::string getData() {
        return this->m_sdata;
    }

    void doSomething() {
        std::cout << "test " << std::endl;
    }

};

//this is task worker need to do.

class Task : public Poco::Runnable {
private:
    std::string m_sData;
public:
    Task(std::string s): m_sData(s) {
        std::cout << "create task" << m_sData << std::endl;
    }
    
    ~Task() {
        std::cout << "destroy task" << m_sData << std::endl;
    }
    
    virtual void run() {
        std::cout << m_sData <<  " do longer task" << std::endl;
        Poco::Thread::sleep(1000);
        std::cout << "finished task" << std::endl;
    }
};

class notiJob : public Poco::Notification {
public:

    notiJob(Object o) : m_oData(o) {
    }

    ~notiJob() {
        std::cout << "destroy notification" << std::endl;
    }

    Object getData() {
        return this->m_oData;
    }

private:
    Object m_oData;
};

//This thread using for assign task for worker in pool
class Distributor : public Poco::Runnable {
public:

    virtual void run() {
        std::cout << "distributor's job" << std::endl;

        Poco::AutoPtr<Poco::Notification> pNoti(g_nofiQueue.waitDequeueNotification()); //
        while (pNoti && g_flag) {
            notiJob* pWorkNf = dynamic_cast<notiJob*> (pNoti.get()); //get notification from NotificationQueue
            if (pWorkNf && Poco::ThreadPool::defaultPool().available() > 0) {
                boost::shared_ptr<Task> worker(new Task(pWorkNf->getData().getData()));
                Poco::ThreadPool::defaultPool().start(*(worker.get())); // set task for Worker
                std::cout << "$$$$check crash" << std::endl;
                pNoti = g_nofiQueue.waitDequeueNotification(); // get Net nofitication if available in queue
            }
        }

    }
};

void handle_Signal(int signum) {
    std::cout << "signal emit " << signum << std::endl;
    g_flag = false;
}

int main() {
    //    Poco::ThreadPool threadPool("threadPoolDemo",
    //                                );
//    signal(SIGSEGV, handle_Signal);

    Distributor di;
    Poco::Thread thread;

    thread.start(di);
    int count = 0;
    while (count < 20) {
        std::string s = "object" + std::to_string(count);
        Object ob(count, s);
        g_nofiQueue.enqueueNotification(new notiJob(ob));
        ++count;
    }
    
    sleep(10);
//    raise(SIGSEGV);
    
    thread.join();
    return 0;
}
