#ifndef LOCK_H
#define LOCK_H

#include <exception>
#include <semaphore.h>
#include <thread>
#include <mutex>
#include <condition_variable>

//利用RAII将信号sem，条件变量直接用的condition_variable，锁直接用的mutex和unique_lock
//注意这里condition_variable只能和unique_lock的mutex配合,保证条件变量的原子操作
//其实信号sem在c++20有封装，但我这里还是11，就手动写了
class sem{
    private:
        sem_t sem_tmp;
    public:
        sem(){
            if(sem_init(&sem_tmp, 0, 0) != 0){//信号初始化，2,0代表不在进程内共享，只在当前进程的线程内共享，3.0是初始化值,返回0代表成功,-1是失败
                throw std::exception();
            }
        }
        sem(int num){
            if(sem_init(&sem_tmp, 0, num) != 0){
                throw std::exception();
            }
        }
        ~sem(){
            sem_destroy(&sem_tmp);
        }
        bool wait(){
            return sem_wait(&sem_tmp);//等待信号，信号大于1就让其减一，否则阻塞
        }
        bool post(){
            return sem_post(&sem_tmp);//释放信号，信号+1
        }
};


// class lock{
//     private:
//         std::mutex mt;
//         std::condition_variable cv;
//     public:
//         bool test(){
//             return true;
//         }
//         lock(){
//             std::unique_lock<std::mutex> guard(mt);//锁住，保证原子操作
//             cv.wait(guard);//等待cv的呼唤,并unlock锁
//             cv.wait(guard, test);//除了cv的呼唤还有test条件的满足
//             cv.notify_one();//呼唤阻塞的线程让其解锁
//         }
// };

#endif