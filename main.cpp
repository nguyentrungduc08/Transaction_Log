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

#define SIGDUM 8888

Poco::NotificationQueue g_notiQueue;
bool g_flag = true;

//demo object data

class Object {
private:
    int m_idata;
    std::string m_sdata;

public:

    Object(int in, std::string s) : m_idata(in), m_sdata(s) {
        std::cout << "!Create Object " << this->m_sdata << std::endl;
    }
    
    Object(const Object &ob) {
        std::cout << "!Copy constructor object " << this->m_sdata << std::endl;
    }

    ~Object() {
        std::cout << "!Destroy Object " << this->m_sdata << std::endl;
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

    Task(std::string s) : m_sData(s) {
        std::cout << "-Create task " << m_sData << std::endl;
    }

    Task(const Task& obj) {
        std::cout << "-Copy constructor task" << m_sData << std::endl;
    }

    ~Task() {
        std::cout << "-Destroy task " << m_sData << std::endl;
    }

    virtual void run() {
        std::cout << m_sData << " do longer task" << std::endl;
        Poco::Thread::sleep(1000);
        std::cout << m_sData << " finished task" << std::endl;
    }
};

class notiJob : public Poco::Notification {
public:

    notiJob(const Object &o) : m_oData(o) {
        std::cout << "+Create notification " << m_oData.getData() << std::endl;
    }
    
    ~notiJob() {
        std::cout << "+Destroy notification " << m_oData.getData() << std::endl;
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
        std::cout << "Distributor's job" << std::endl;

        Poco::AutoPtr<Poco::Notification> pNoti(g_notiQueue.waitDequeueNotification()); //
        while (pNoti && g_flag) {
            notiJob* pWorkNf = dynamic_cast<notiJob*> (pNoti.get()); //get notification from NotificationQueue
            if (pWorkNf && Poco::ThreadPool::defaultPool().available() > 0) {
                    
                boost::shared_ptr<Task> worker(new Task(pWorkNf->getData().getData()));
                
                Poco::ThreadPool::defaultPool().start(*(worker.get())); // set task for Worker
                
                if (pWorkNf->getData().getData() == "object19"){
                    return;
                }
                
                pNoti = g_notiQueue.waitDequeueNotification(); // get Net nofitication if available in queue
            }
//            std::cout << "log......\n";
        }

        std::cout << "Distributor DONE" << std::endl;
    }
};

void handle_Signal(int signum) {
    std::cout << "-----------signal emit-----------" << signum << std::endl;
    g_flag = false;
}

int main() {
    Distributor di;
    Poco::Thread thread;

    thread.start(di);
    
//    sleep(1);

    int count = 0;
    while (count < 20) {
        std::string s = "object" + std::to_string(count); 
        Object ob(count, s); // call contructor object
        
        /*
         - call copy constructor object
         - call constructor object notiJob
         */
        g_notiQueue.enqueueNotification(new notiJob(ob)); 
        
        ++count;
    }

    thread.join();

    Poco::ThreadPool::defaultPool().joinAll();

    return 0;
}
