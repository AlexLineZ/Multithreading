#include <iostream>
#include <thread>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <signal.h>

using namespace std;
bool gSignalStatus = false;

void signal_handler(int signal) {
    cout << "Hui" << endl;
    gSignalStatus = true;
}

struct Request {
    int groupID;  
    int type;      
    int priority;  

    Request() : groupID(-1), type(-1), priority(-1) {};
    Request(int id, int currentType, int currentPriority) : groupID(id), type(currentType), priority(currentPriority) {};
};

struct Device {
    int id;          
    int groupID;    
    bool isBusy;     
    int currentType; 
    int currentPriority;
    thread thread;

    Device(): id(-1), groupID(-1), isBusy(false), currentType(-1), currentPriority(-1) {}
    Device(const Device& device): id(device.id), groupID(device.groupID), isBusy(device.isBusy), currentType(device.currentType), currentPriority(device.currentPriority) {}
};

int getRandomNumber(int min, int max) {
    return min + rand() % (max + 1 - min);
}

Request generateRequest(int groupID) {
    Request request(groupID, getRandomNumber(0, 2), getRandomNumber(0, 2));
    return request;
}

void processRequests(Device* device, queue<Request>& queue, mutex& mutex, condition_variable& cv) {
    while (!gSignalStatus) {
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
        cout << "Device " << device->id << " in group " << device->groupID << " is processing request with type " << request.type << " and priority " << request.priority << endl;
        this_thread::sleep_for(chrono::milliseconds(getRandomNumber(100, 2000)));
        device->isBusy = false;

        cv.notify_one();
    }
}

int main() {
    signal(SIGINT, signal_handler);

    int countGroups, countDevicesInGroup, maxQueueSize;

    cout << "Enter number of groups: ";
    cin >> countGroups;
    cout << "Enter the number of devices: ";
    cin >> countDevicesInGroup;
    cout << "Enter maximum queue size: ";
    cin >> maxQueueSize;

    vector<vector<Device>> devices(countGroups, vector<Device>(countDevicesInGroup));
    queue<Request> requests;
    mutex mutex;
    condition_variable cv;

    for (int i = 0; i < countGroups; i++) {
        for (int j = 0; j < countDevicesInGroup; j++) {
            devices[i][j].id = j;
            devices[i][j].groupID = i;
            devices[i][j].thread = thread(processRequests, &devices[i][j], ref(requests), ref(mutex), ref(cv));
            devices[i][j].thread.detach();
        }
    }
    bool queueIsFull = false;

    thread requestGenerator([&requests, &mutex, &cv, countGroups, maxQueueSize, &queueIsFull]() {
        int currentGroup = 0;
        while (!gSignalStatus) {
            unique_lock<std::mutex> lock(mutex);
            while (requests.size() >= maxQueueSize) {
                cout << "Queue is full :(" << endl;
                queueIsFull = true;
                cv.wait(lock);
            }

            queueIsFull = false;
            lock.unlock();

            Request request = generateRequest(currentGroup);
            lock.lock();
            requests.push(request);
            lock.unlock();
            cv.notify_one();

            currentGroup = (currentGroup + 1) % countGroups;
            this_thread::sleep_for(chrono::milliseconds(100));
        }
    });
    requestGenerator.detach();

    while (!gSignalStatus) {
        cout << "Device status:" << endl;
        for (int i = 0; i < countGroups; i++) {
            for (int j = 0; j < countDevicesInGroup; j++) {
                cout << "Group " << i << ", Device " << j << ": ";
                if (devices[i][j].isBusy) {
                    cout << "executes a request with type " << devices[i][j].currentType << " and priority " << devices[i][j].currentPriority << endl;
                }
                else {
                    cout << "idle" << endl;
                }
            }
        }
        cout << endl << "Queue size: " << requests.size() << " / " << maxQueueSize << endl;

        this_thread::sleep_for(chrono::seconds(1));
    }

    return 0;
}
   