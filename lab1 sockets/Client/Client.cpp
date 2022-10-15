/*
Client-server application
Author: Tarasiuk Oleksandr
Type: Client
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

void errorCheck(int returnValue, const char* action) {
    if (returnValue == SOCKET_ERROR) {
        std::cout << "Error!" << "\nDuring " << action << "\nCode: " << WSAGetLastError() << std::endl;
        WSACleanup();
        exit(1);
    }
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

std::string getTimeNow()
{
    std::string result;

    std::time_t unixtime = std::time(nullptr);
    result = std::asctime(std::localtime(&unixtime));
    result.pop_back();

    return result;
};

int sendMessage(SOCKET& sock, const char* sendbuf, std::ofstream& logfile)
{
    int sendResult;

    sendResult = send(sock, sendbuf, 255, 0);
    errorCheck(sendResult, "send");
    logfile << getTimeNow() << " | SERVER >> socket-" << sock << ":" << sendbuf << std::endl;
    logfile.flush();

    return sendResult;
};

void editField(std::string& field, std::string coord, bool hit) {
    int x = stoi(coord) / 10;
    int y = stoi(coord) % 10;
    if (hit) {
        field[(x + 1) * 2 - 2 + y * 20] = 'H';
    }
    else {
        field[(x + 1) * 2 - 2 + y * 20] = 'S';
    }
}

int recvMessage(SOCKET& sock, char* recvbuf, int& recvbuflen, int& bytesRecv, std::ofstream& logfile, std::string& field)
{
    bytesRecv = recv(sock, recvbuf, recvbuflen, 0);
    if (bytesRecv == 0 || bytesRecv == WSAECONNRESET)
    {
        std::cout << ("Connection Closed.\n");
        return -1;
    };
    logfile << getTimeNow() << " | SERVER -> CLIENT" << ":" << recvbuf << std::endl;
    logfile.flush();

    std::string recvString(recvbuf);

    if (recvString == "s-s") {
        std::cout << "<- SERVER: You start the game.\n" << field << std::endl;
    }
    else if (recvString == "s-f") { 
        std::cout << "<- SERVER: Please, start the new game!\n"; 
    }
    else if (recvString.rfind("h-s", 0) == 0) {
        editField(field, recvString.substr(3, 2), true);
        std::cout << "<- SERVER: Hit!\n" << field << std::endl;
        
    }
    else if (recvString.rfind("h-f", 0) == 0) {
        editField(field, recvString.substr(3, 2), false);
        std::cout << "<- SERVER: Miss!\n" << field << std::endl;

    }
    else if (recvString.rfind("f-s", 0) == 0) {
        int ctry = stoi(recvString.substr(3, 2))-10;
        int chit = stoi(recvString.substr(5, 2))-10;
        std::cout << "<- SERVER: You end the game!"
                  <<"\nYour try count:" << ctry 
                  <<"\nYour hit count:" << chit << std::endl;

    }
    else std::cout << "<- SERVER:" << recvString.c_str() << std::endl;

    return bytesRecv;
};

int main(int args, char* argv[]) {

    WSAData wsaData;

    std::ofstream logfile;
    logfile.open("client-log.txt", std::ios_base::app);

    char recvbuf[255] = "";
    int recvbuflen = 255;
    int bytesRecv;
    int returnValue;

    std::string inputbuf;

    SOCKET сonnection;

    WORD DLLVersion = MAKEWORD(2, 2);
    returnValue = WSAStartup(DLLVersion, &wsaData);
    errorCheck(returnValue, "WSAStartup");

    SOCKADDR_IN addr;
    int sizeofaddr = sizeof(addr);
    addr.sin_addr.s_addr = inet_addr("127.0.0.1"); // Ip
    addr.sin_port = htons(1041); // 1025 + 16
    addr.sin_family = AF_INET;

    сonnection = constructSocket();

    returnValue = connect(сonnection, (SOCKADDR*)&addr, sizeofaddr);
    errorCheck(returnValue, "connection");

    std::cout << "Connected!\n" << std::endl;

    logfile << getTimeNow() << " | CLIENT-CONNECT]" << std::endl;
    logfile.flush();

    std::string startField = "0 0 0 0 0 0 0 0 0 0\n0 0 0 0 0 0 0 0 0 0\n0 0 0 0 0 0 0 0 0 0\n0 0 0 0 0 0 0 0 0 0\n0 0 0 0 0 0 0 0 0 0\n0 0 0 0 0 0 0 0 0 0\n0 0 0 0 0 0 0 0 0 0\n0 0 0 0 0 0 0 0 0 0\n0 0 0 0 0 0 0 0 0 0\n0 0 0 0 0 0 0 0 0 0\n";
    std::string Field = "";

    while (true)
    {
        std::string().swap(inputbuf);

        std::cout << "CLIENT ->: ";
        std::cin >> inputbuf;

        if (inputbuf == "start"){
            Field = startField;
            sendMessage(сonnection, "s", logfile);
            bytesRecv = recvMessage(сonnection, recvbuf, recvbuflen, bytesRecv, logfile, Field);
            if (bytesRecv == -1) break;
        }
        else if (inputbuf == "who"){
            sendMessage(сonnection, "who", logfile);
            bytesRecv = recvMessage(сonnection, recvbuf, recvbuflen, bytesRecv, logfile, Field);
            if (bytesRecv == -1) break;
        } 
        else if (inputbuf == "finish"){
            sendMessage(сonnection, "fn", logfile);
            bytesRecv = recvMessage(сonnection, recvbuf, recvbuflen, bytesRecv, logfile, Field);
            if (bytesRecv == -1) break;
        } 
        else if (inputbuf == "exit"){
            std::cout << "Closing the connection...\n" << std::endl;
            break;
        } 
        else if (inputbuf.rfind("x", 0) == 0 && inputbuf.rfind("y", 2) == 2 &&  inputbuf.length() == 4 && isdigit(inputbuf[1]) != 0 && isdigit(inputbuf[3]) != 0)
        {
            sendMessage(сonnection, (inputbuf.substr(1, 1) + inputbuf.substr(3, 1)).c_str(), logfile);
            bytesRecv = recvMessage(сonnection, recvbuf, recvbuflen, bytesRecv, logfile, Field);
            if (bytesRecv == -1) break;
        }
        else{
            std::cout << "  Availible command:\n"
                "    who - information about lab\n"
                "    start - start game-session\n"
                "    finish - stop game session\n"
                "    x#y# - indicates the coordinates of the shot | # - [0-9]\n"
                "    exit - close connection.\n" << std::endl;
        };
    };

    closesocket(сonnection);
    WSACleanup();
    std::cout << "[Connection closed.]\n" << std::endl;
    logfile << getTimeNow() << " | CLIENT-DISCONNECT]" << std::endl;
    logfile.flush();
    return 0;
}