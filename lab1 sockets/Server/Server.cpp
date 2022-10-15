/*
Client-server application
Author: Tarasiuk Oleksandr
Type: Server
Var: 16
Short name: Гра "морський бій".
Task: Грає лише користувач на клієнті.
    Сервер відповідає лише про результат попадання/знищення кораблика.
    Достатньо на сервері мати єдине статичне представлення поля з кораблями.
    Клієнт в матриці 10х10 повинен передавати координати вистрілу.
    Сервер (без зайвої графічної реалізації, - достатньо використовувати
    кілька символів для позначень у матриці із символів)
    передає матрицю 10х10 із необхідними позначками вистрілів та попадань.
    Клієнт в ході гри може її завершити гру раніше.
    Після завершення сеансу сервер до клієнта передає статистику гри:
    кількість стрільб, кількість попадань та промахів.
*/

#include <iostream>
#include <winsock2.h>
#include <fstream>
#include <string>
#pragma comment(lib, "ws2_32.lib")
#pragma warning(disable: 4996)

void errorCheck(int returnValue, const char *action) {
    if (returnValue == SOCKET_ERROR) {
        if (WSAGetLastError() == WSAECONNREFUSED) {
            printf("Server not found/refused the connection.\n");
        }
        else if (WSAGetLastError() == WSAECONNRESET) {
            printf("Server closed the connection.\n");
        }
        else {
            printf("Stop by error [%d] on %s\n", WSAGetLastError(), action);
        };

        WSACleanup(); exit(1);
    };
}

SOCKET constructSocket() {
    SOCKET constSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (constSocket == INVALID_SOCKET) {
        std::cout << "Error!" << "\nDuring socket construction. \nCode: " << WSAGetLastError() << std::endl;
        WSACleanup();
        exit(1);
    }
    return constSocket;
};

std::string getTimeNow(){
    std::string result;

    std::time_t unixtime = std::time(nullptr);
    result = std::asctime(std::localtime(&unixtime));
    result.pop_back();

    return result;
};

int sendMessage(SOCKET& sock, const char* sendbuf, std::ofstream& logfile){
    int sendResult;

    sendResult = send(sock, sendbuf, 255, 0);
    errorCheck(sendResult, "send");
    logfile << getTimeNow() << " | SERVER >> socket-" << sock << ":" << sendbuf << std::endl;
    logfile.flush();

    return sendResult;
};

bool hitCheck(std::string initFld, std::string coord) {
    int x = stoi(coord) / 10;
    int y = stoi(coord) % 10;
    if (initFld[(x + 1) * 2 - 2 + y * 20] == 'X') {
        return true;
    }
    return false;
}

void commandController(SOCKET& sock, char* clientText, std::ofstream& logfile, bool& start, int& ctry, int& chit){
    /*
    STATUS CODE:
    0 - Disconnected.
    1 - Connect/Finish.
    2 - Start/Restart.
    Send:
        s  - start
        fn - finish
        ## - coordinates

    Return:
        s-s - start success 
        s-f - start failed 
        h-s## - hit success
        f-s#### - finish success 
    */

    const int sendbufsize = 255;
    char sendbuf[sendbufsize] = "";

    std::string initialField = "X X O X O O X X X X\nO O O X O O O O O O\nO O O X O O O O O X\nO O O O O O O O O X\nO X O O O O O O O O\nO X O O O O X O O O\nO X O O O O O O O O\nO O O O O O O O O O\nO O O X O X O O X O\nX O O O O O O O X O";
    
    std::string recvString(clientText);
    if (recvString == "who")
    {
        strncpy(sendbuf, "Tarasiuk Oleksandr k-23, var 16. [Game sea battle]", sendbufsize);
        sendMessage(sock, sendbuf, logfile);
    }
    else if (recvString == "s"){
        start = true;
        std::cout << initialField << std::endl;
        strncpy(sendbuf,"s-s", sendbufsize);
        sendMessage(sock, sendbuf, logfile);
    }
    else if (isdigit(recvString[0]) != 0 && isdigit(recvString[1]) != 0 && !start) {
        strncpy(sendbuf, "s-f", sendbufsize);
        sendMessage(sock, sendbuf, logfile);
    }
    else if (isdigit(recvString[0]) != 0 && isdigit(recvString[1]) != 0 && start) {
        if (hitCheck(initialField, recvString)) {
            std::string h = "h-s";
            chit += 1;
            ctry += 1;
            strncpy(sendbuf, (h + recvString).c_str(), sendbufsize);
        }
        else {
            std::string h = "h-f";
            ctry += 1;
            strncpy(sendbuf, (h + recvString).c_str(), sendbufsize);
        }
        sendMessage(sock, sendbuf, logfile);
    }
    else if (recvString == "fn" && !start) {
        strncpy(sendbuf, "s-f", sendbufsize);
        sendMessage(sock, sendbuf, logfile);
    }
    else if (recvString == "fn") {
        std::string f = "f-s";
        std::string t = std::to_string(ctry+10);
        std::string h = std::to_string(chit+10);
        strncpy(sendbuf, (f+t+h).c_str(), sendbufsize);
        sendMessage(sock, sendbuf, logfile);
        start = false;
        ctry = 0;
        chit = 0;
    }
    std::string().swap(recvString);
};

int main(int args, char* argv[]) {
    WSAData wsaData;

    const int recvbuflen = 255;
    char recvbuf[recvbuflen] = "";
    
    int returnValue;

    SOCKET serverSocket;
    SOCKET сonnection;

    std::ofstream logfile;
    logfile.open("server-log.txt", std::ios_base::app);

    WORD DLLVersion = MAKEWORD(2, 2);
    returnValue = WSAStartup(DLLVersion, &wsaData);
    errorCheck(returnValue, "WSAStartup");

    SOCKADDR_IN addr;
    int sizeofaddr = sizeof(addr);
    addr.sin_addr.s_addr = inet_addr("127.0.0.1"); // Ip
    addr.sin_port = htons(1041); // 1025 + 16
    addr.sin_family = AF_INET;

    serverSocket = constructSocket();

    returnValue = bind(serverSocket, (SOCKADDR*)&addr, sizeofaddr);
    errorCheck(returnValue, "bind");

    returnValue = listen(serverSocket, SOMAXCONN);
    errorCheck(returnValue, "listen");

    std::cout << "waiting for connections...\n" << std::endl;

    сonnection = accept(serverSocket, (SOCKADDR*)&addr, &sizeofaddr);
    if (сonnection == 0) {
        std::cout << "Error: During сonnection. \nCode:" << WSAGetLastError() << std::endl;
        WSACleanup();
        return 1;
    }

    std::cout << "Connected!\n" << std::endl;
    logfile << getTimeNow() << " | SERVER-CONNECT [socket-" << сonnection << "]" << std::endl;
    logfile.flush();

    bool start = false;
    int countTry = 0;
    int countHit = 0;
    while (true){
        ZeroMemory(recvbuf, recvbuflen);
        returnValue = recv(сonnection, recvbuf, recvbuflen, 0);

        if (returnValue > 0)
        {
            logfile << getTimeNow() << " | SERVER << socket-" << сonnection << ":" << recvbuf << std::endl;
            logfile.flush();
            commandController(сonnection, recvbuf, logfile, start, countTry, countHit);
        }

        else if (returnValue == 0 || returnValue == -1)
        {
            std::cout << "Closed connection\n" << std::endl;
            logfile << getTimeNow() << " | SERVER-DISCONNECT [socket-" << сonnection << "]" << std::endl;
            logfile.flush();
            break;
        }

        else
        {
            std::cout << "Error! \n During recv" << returnValue << std::endl;
        };
    }

    closesocket(сonnection);
    WSACleanup();
    return 0;
}
