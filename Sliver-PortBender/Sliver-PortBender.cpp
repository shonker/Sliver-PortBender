// Sliver-PortBender.cpp : Definisce le funzioni esportate per la DLL.
//
#include "pch.h"
#include "Sliver-PortBender.h"
#include "Arguments.h"

#pragma warning(disable : 4996)

PortBenderWrapper::PortBenderWrapper(PortBender _portbender) : portbender(_portbender) {
    
}

PortBenderWrapper::~PortBenderWrapper() {
}

PortBenderWrapper::PortBenderWrapper(const PortBenderWrapper& source) : portbender(source.portbender) {

}

void PortBenderWrapper::start(){
   this->thread_ptr = std::make_unique<std::thread>(&PortBender::Start,&this->portbender);
}

void PortBenderWrapper::stop() {
    this->portbender.Stop();
    this->thread_ptr->join();
    this->thread_ptr.reset();
}

 std::pair<UINT16,UINT16> PortBenderWrapper::getData(){
     std::pair<UINT16, UINT16> ret;

     ret.first = this->portbender.getFakeDstPort();
     ret.second = this->portbender.getRedirectPort();
     return ret;
}

std::tuple<int, bool> PortBenderManager::add(PortBender portbender) {
    auto ret = this->manager.emplace(this->max, std::make_shared<PortBenderWrapper>(portbender));
    std::tuple<int, bool> tp;

    if (ret.second == true) {
        tp = std::make_tuple(this->max, true);
        this->max++;
    }
    else {
        tp = std::make_tuple(-1, false);

    }
    return tp;
}

bool PortBenderManager::start(int key) {
    auto ret = this->manager.find(key);
    if (ret == this->manager.end()) {
        return false;
    }
    ret->second->start();
    return true;
}

bool PortBenderManager::remove(int key) {
    auto ret = this->manager.erase(key);
    if (ret == 0) {
        return false;
    }
    return true;
}

bool PortBenderManager::stop(int key) {
    auto ret = this->manager.find(key);
    if (ret == this->manager.end()) {
        return false;
    }
    ret->second->stop();
    return true;
}

std::pair<UINT16,UINT16> PortBenderManager::getData(int key) {
    auto ret = this->manager.find(key);
    if (ret == this->manager.end()) {
        return std::pair<UINT16,UINT16>(-1,-1);
    }
    return ret->second->getData();
}

std::unique_ptr<PortBenderManager> manager{ nullptr };

int func(const char* buff, int n) {
    return 0;
}

int fakeEntryPoint() {
    char buff[100] = { '\0' };
    strcpy(buff, "1 redirect 445 8445");
    entrypoint(buff, strlen(buff), func);
    Sleep(1000 * 120);
    strcpy(buff, "0 remove 0");
    entrypoint(buff, strlen(buff), func);
    return 0;
}

int entrypoint(char* argsBuffer, uint32_t bufferSize, goCallback callback)
{
    if (manager == nullptr) {
        manager = std::make_unique<PortBenderManager>();
    }
    
    int cmd = -1;
    if (bufferSize < 1)
    {
        std::string msg{ "You must provide a command.\n\t0 = stop\n\t1 = start\n\t2 = get logs" };
        callback(msg.c_str(), msg.length());
    }
    else
    {
        cmd = argsBuffer[0] - '0'; // atoi would return 0 if it couldn't convert, this will only return 0 if the first char is 0
    }
    switch (cmd) {
    case 0: //stop
    {
        try {
            Arguments args = Arguments(argsBuffer + 2);

            if (args.Action == "remove") {
                if (manager->stop(args.id) && manager->remove(args.id)) {
                    char buffer[100] = { '\0' };
                    sprintf(buffer, "successfully removed portBender with Id% d\n", args.id);
                    callback(buffer, strlen(buffer));
                }
                else {
                    char buffer[100] = { '\0' };
                    sprintf(buffer, "unable to remove portBender with Id %d\n", args.id);
                    callback(buffer, strlen(buffer));
                }
            }
        }
        catch (const std::invalid_argument&) {
            std::string msg("Redirect Usage : PortBender redirect FakeDstPort RedirectedPort\n");
            callback(msg.c_str(), msg.length());
            msg = "Backdoor Usage: PortBender backdoor FakeDstPort RedirectedPort password\n";
            callback(msg.c_str(), msg.length());
            msg = "Example:\n";
            callback(msg.c_str(), msg.length());
            msg = "\tPortBender redirect 445 8445\n";
            callback(msg.c_str(), msg.length());
            msg = "\tPortBender backdoor 443 3389 praetorian.antihacker\n";
            callback(msg.c_str(), msg.length());
        }
        catch (const std::exception& e) {
            std::string msg(e.what());
            callback(msg.c_str(), msg.length());
        }
        break;
    }

    case 1: //start
    {
        try {
            Arguments args = Arguments(argsBuffer + 2);

            if (args.Action == "backdoor") {
                /*std::cout << "Initializing PortBender in backdoor mode" << std::endl;
                PortBender backdoor = PortBender(args.FakeDstPort, args.RedirectPort, args.Password);
                backdoor.Start();*/
            }
            else if (args.Action == "redirect") {
                std::string msg("Initializing PortBender in redirector mode\n");
                callback(msg.c_str(), msg.length());
                auto tp = manager->add(PortBender(args.FakeDstPort, args.RedirectPort));
                if (std::get<1>(tp)) {
                    manager->start(std::get<0>(tp));
                }
            }
        }
        catch (const std::invalid_argument&) {
            std::string msg("Redirect Usage : PortBender redirect FakeDstPort RedirectedPort\n");
            callback(msg.c_str(), msg.length());
            msg = "Backdoor Usage: PortBender backdoor FakeDstPort RedirectedPort password\n";
            callback(msg.c_str(), msg.length());
            msg = "Example:\n";
            callback(msg.c_str(), msg.length());
            msg = "\tPortBender redirect 445 8445\n";
            callback(msg.c_str(), msg.length());
            msg = "\tPortBender backdoor 443 3389 praetorian.antihacker\n";
            callback(msg.c_str(), msg.length());
        }
        catch (const std::exception& e) {
            std::string msg(e.what());
            callback(msg.c_str(), msg.length());
        }
        break;
    }
    case 2: //list
    {
        auto data = manager->getData(0);
        char buffer[100] = { '\0' };
        sprintf(buffer, "data for id 0: fakeport redirectport", data.first, data.second);
        break;
    }
    default:
    {
        std::string msg{ "invalid command received" };
        callback(msg.c_str(), msg.length());
    }
    }
    return 0;

}