#include <iostream>
#include <thread>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <random>

using namespace std;
const int MAX_QUEUE_SIZE = 10;

struct Request {
    int groupID;  
    int type;      
    int priority;  
};

struct Device {
    int id;          
    int groupID;    
    bool isBusy;     
    int currentType; 
    int currentPriority;
    thread thread;

    Device() : id(0), groupID(0), isBusy(false), currentType(0), currentPriority(0) {}

    Device(const Device& other) :
        id(other.id), groupID(other.groupID), isBusy(other.isBusy),
        currentType(other.currentType), currentPriority(other.currentPriority) {}
};

int getRandomNumber(int min, int max) {
    srand(time(NULL));
    int num = min + rand() % (max - min + 1);
    return num;
}

Request generateRequest(int groupID) {
    Request request;
    request.groupID = groupID;
    request.type = getRandomNumber(0, 2);
    request.priority = getRandomNumber(0, 2);
    return request;
}

void processRequests(Device* device, queue<Request>& queue, mutex& mutex, condition_variable& cv) {
    while (true) {
        unique_lock<std::mutex> lock(mutex);

        while (queue.empty()) {
            cv.wait(lock);
        }
        Request request = queue.front();
        queue.pop();
        lock.unlock();

        device->isBusy = true;
        device->currentType = request.type;
        device->currentPriority = request.priority;
        cout << "Device " << device->id << " in group " << device->groupID <<
            " is processing request with type " << request.type << " and priority " << request.priority << endl;
        this_thread::sleep_for(chrono::milliseconds(getRandomNumber(1000, 5000)));
        device->isBusy = false;

        cv.notify_one();
    }
}

int main() {
    const int NUM_GROUPS = 3;
    const int NUM_DEVICES_PER_GROUP = 2;

    queue<Request> request_queue; // Очередь заявок
    vector<Device> devices; // Вектор приборов
    mutex mutex; // Мьютекс для доступа к очереди заявок
    condition_variable cv; // Условная переменная для оповещения потоков о появлении новых заявок

    thread request_generator([&]() {
        while (true) {
            unique_lock<std::mutex> lock(mutex);

            if (request_queue.size() >= MAX_QUEUE_SIZE) {
                cv.wait(lock);
                continue;
            }

            int groupID = getRandomNumber(0, NUM_GROUPS - 1);
            Request request = generateRequest(groupID);
            request_queue.push(request);
            cout << "New request from group " << groupID << " with type " << request.type << " and priority " << request.priority << endl;

            cv.notify_all();

            this_thread::sleep_for(chrono::milliseconds(getRandomNumber(1000, 5000)));
        }
    });

    for (int i = 0; i < NUM_GROUPS; i++) {
        for (int j = 0; j < NUM_DEVICES_PER_GROUP; j++) {
            Device device;
            device.id = j + 1;
            device.groupID = i;
            device.isBusy = false;
            device.currentType = -1;
            device.currentPriority = -1;
            device.thread = thread(processRequests, &device, ref(request_queue), ref(mutex), ref(cv));
            devices.push_back(device);
        }
    }

    cin.get();

    request_generator.join();

    for (Device& device : devices) {
        device.thread.join();
    }

    return 0;
}
   