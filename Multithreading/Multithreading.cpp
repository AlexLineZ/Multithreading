#include <iostream>
#include <thread>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <random>

using namespace std;

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

    Device() : id(-1), groupID(-1), isBusy(false), currentType(-1), currentPriority(-1) {}

    Device(const Device& other) :id(other.id), groupID(other.groupID), isBusy(other.isBusy), currentType(other.currentType), currentPriority(other.currentPriority) {}
};

int getRandomNumber(int min, int max) {
    static std::mt19937 generator(std::random_device{}());
    std::uniform_int_distribution<int> distribution(min, max);
    return distribution(generator);
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
    int numGroups, numDevicesPerGroup, maxQueueSize;

    cout << "Enter number of groups: ";
    cin >> numGroups;
    cout << "Enter number of devices per group: ";
    cin >> numDevicesPerGroup;
    cout << "Enter maximum queue size: ";
    cin >> maxQueueSize;

    vector<vector<Device>> devices(numGroups, vector<Device>(numDevicesPerGroup));
    queue<Request> requests;
    mutex mutex;
    condition_variable cv;

    // Create and start device threads
    for (int i = 0; i < numGroups; i++) {
        for (int j = 0; j < numDevicesPerGroup; j++) {
            devices[i][j].id = j;
            devices[i][j].groupID = i;
            devices[i][j].thread = thread(processRequests, &devices[i][j], ref(requests), ref(mutex), ref(cv));
            devices[i][j].thread.detach();
        }
    }

    // Start request generator thread
    thread requestGenerator([&requests, &mutex, &cv, numGroups, maxQueueSize]() {
        int currentGroup = 0;
        while (true) {
            if (requests.size() < maxQueueSize) {
                Request request = generateRequest(currentGroup);
                unique_lock<std::mutex> lock(mutex);
                requests.push(request);
                lock.unlock();
                cv.notify_one();
            }
            else {
                cout << "Queue is full :(" << endl;
            }
            currentGroup = (currentGroup + 1) % numGroups;
            this_thread::sleep_for(chrono::milliseconds(100));
        }
        });
    requestGenerator.detach();

    // Print device status and queue size every second
    while (true) {
        cout << "Device status:" << endl;
        for (int i = 0; i < numGroups; i++) {
            for (int j = 0; j < numDevicesPerGroup; j++) {
                cout << "Group " << i << ", Device " << j << ": ";
                if (devices[i][j].isBusy) {
                    cout << "busy processing request with type " << devices[i][j].currentType <<
                        " and priority " << devices[i][j].currentPriority << endl;
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
   