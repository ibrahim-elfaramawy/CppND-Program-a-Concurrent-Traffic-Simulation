#include <iostream>
#include <random>
#include <future>
#include "TrafficLight.h"

/* Implementation of class "MessageQueue" */

template <typename T>
T MessageQueue<T>::receive()
{
    // FP.5a : The method receive should use std::unique_lock<std::mutex> and _condition.wait() 
    // to wait for and receive new messages and pull them from the queue using move semantics. 
    // The received object should then be returned by the receive function. 
    
    // protect queue modification under the lock
    std::unique_lock<std::mutex> receiveLock(_mutex);

    _condition.wait(receiveLock,[this] {return !_queue.empty();});

    // remove the first element of the queue after moving it into newMsg
    T newMsg = std::move(_queue.front());
    _queue.pop_front();

    return newMsg;
}

template <typename T>
void MessageQueue<T>::send(T &&msg)
{
    // FP.4a : The method send should use the mechanisms std::lock_guard<std::mutex> 
    // as well as _condition.notify_one() to add a new message to the queue and afterwards send a notification.

    // protect messageQueue modification under the lock
    std::lock_guard<std::mutex> sendLock(_mutex);

    
    _queue.clear();

    // add new msg to the queue
    _queue.emplace_back(std::move(msg));

    // notify the client after pushing the new message into the messageQueue
    _condition.notify_one();
}


/* Implementation of class "TrafficLight" */


TrafficLight::TrafficLight()
{
    _currentPhase = TrafficLightPhase::red;
    _messageQueue = std::make_shared<MessageQueue<TrafficLightPhase>>();
}

void TrafficLight::waitForGreen()
{
    // FP.5b : add the implementation of the method waitForGreen, in which an infinite while-loop 
    // runs and repeatedly calls the receive function on the message queue. 
    // Once it receives TrafficLightPhase::green, the method returns.
    while (true)
    {
        // sleep to reduce CPU usage
        std::this_thread::sleep_for(std::chrono::milliseconds(1));

        if(_messageQueue->receive() == TrafficLightPhase::green)
            return;
    }
}

TrafficLightPhase TrafficLight::getCurrentPhase()
{
    return _currentPhase;
}

void TrafficLight::setCurrentPhase(TrafficLightPhase currentPhase)
{
    _currentPhase = currentPhase;
}

void TrafficLight::simulate()
{
    // FP.2b : Finally, the private method „cycleThroughPhases“ should be started in a thread when the public method „simulate“ is called. To do this, use the thread queue in the base class. 
    // launch „cycleThroughPhases“ in a thread
    threads.emplace_back(std::thread(&TrafficLight::cycleThroughPhases, this));
}

// virtual function which is executed in a thread
void TrafficLight::cycleThroughPhases()
{
    // FP.2a : Implement the function with an infinite loop that measures the time between two loop cycles 
    // and toggles the current phase of the traffic light between red and green and sends an update method 
    // to the message queue using move semantics. The cycle duration should be a random value between 4 and 6 seconds. 
    // Also, the while-loop should use std::this_thread::sleep_for to wait 1ms between two cycles. 

    std::random_device rd; // A random number from hardware
    std::mt19937 gen(rd()); // A seed for the random number generator
    std::uniform_int_distribution<> distr(4,6); // define the range which the generator will operate on

    double cycleDuration = distr(gen); // duration of a single simulation cycle in ms

    std::chrono::time_point<std::chrono::system_clock> startCycle;

    // init stop watch
    startCycle = std::chrono::system_clock::now();
    
    while (true)
    {
        // sleep at every iteration to reduce CPU usage
        std::this_thread::sleep_for(std::chrono::milliseconds(1));

        // compute time difference to stop watch
        long endCycle = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now() - startCycle).count();

        if (endCycle >= cycleDuration)
        {
            // Toggle the traffic light
            if(getCurrentPhase() == TrafficLightPhase::red)
                setCurrentPhase(TrafficLightPhase::green);
            else
                setCurrentPhase(TrafficLightPhase::red);

            // sends an update method to the message queue using move semantics.
            std::future<void> ftr = std::async(std::launch::async,&MessageQueue<TrafficLightPhase>::send,_messageQueue, std::move(_currentPhase));
            ftr.wait(); 
            
            // reset stop watch for next cycle
            startCycle = std::chrono::system_clock::now();
            cycleDuration = distr(gen);
        }
    }
}
