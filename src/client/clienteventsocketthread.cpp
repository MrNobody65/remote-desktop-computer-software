#include <clienteventsocketthread.h>

EventSocketThread::EventSocketThread(EventSocketThreadCallback *callback, wxString &ip, std::queue<msg> &msgQueue, wxCriticalSection &mQcs)
    : callback(callback), ip(ip), msgQueue(msgQueue), mQcs(mQcs) {}
EventSocketThread::~EventSocketThread()
{
    callback->OnEventSocketThreadDestruction();
}

wxThread::ExitCode EventSocketThread::Entry()
{
    SOCKET clientSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (clientSocket == INVALID_SOCKET)
    {
        std::cerr << "Failed to create socket." << "\n";
        return nullptr;
    }

    sockaddr_in serverAddress{};
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(20565);                 
    serverAddress.sin_addr.s_addr = inet_addr(ip.mb_str());

    if (connect(clientSocket, reinterpret_cast<sockaddr *>(&serverAddress), sizeof(serverAddress)) == SOCKET_ERROR)
    {
        std::cerr << "Failed to connect to server." << "\n";
        closesocket(clientSocket);
        return nullptr;
    }

    while (true)
    {
        if (wxThread::This()->TestDestroy())
        {
            closesocket(clientSocket);
            return nullptr;
        }
        
        msg msg;
        {
            wxCriticalSectionLocker lock(mQcs);
            if (!msgQueue.empty()) 
            {
                msg = msgQueue.front();
                msgQueue.pop();
            }
            else continue; 
        }

        char* msgData = new char[22];
        memcpy(msgData, &msg, 22);

        if (send(clientSocket, (char *)msgData, 22, 0) == SOCKET_ERROR)
        {
            std::cerr << "Failed to send event." << "\n";
            closesocket(clientSocket);
            delete[] msgData;
            return nullptr;
        }
    }

    closesocket(clientSocket);

    return nullptr;
}