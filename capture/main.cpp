#if defined(_WIN64) || defined(_WIN32)

//#pragma comment(lib, "Ws2_32.lib")
//#pragma comment(lib, "Mswsock.lib")
//#pragma comment(lib, "AdvApi32.lib")

#include <Ws2tcpip.h>
#include <fileapi.h>
#include <string.h>
#include <winsock2.h>

#endif

#if defined(__unix__) || defined(__APPLE__)

#include <netinet/in.h>  ///< sockaddr_in
#include <sys/socket.h>  ///< socket

#endif

#define PY_SSIZE_T_CLEAN
#include <Python.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <fstream>
#include <iostream>

#include "../include/includer.h"

#define bzero(b, len) (memset((b), '\0', (len)), (void)0)  // bzero

#define BUFFER_SIZE 4096
#define DEBUG 0
#define DS 1

static const unsigned short DEFAULT_PORT = 20777;

void mkoutdir(const char *dirname) {
#if defined(_WIN64) || defined(_WIN32)
    if (CreateDirectoryA(dirname, NULL) ||
        GetLastError() == ERROR_ALREADY_EXISTS) {
        std::cout << "Directory " << dirname << " already exists" << std::endl;
    } else {
        std::cout << "Directory " << dirname << " created" << std::endl;
    }
#endif
#if defined(__unix__) || defined(__APPLE__)
    if (mkdir(dirname, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH) == -1) {
        if (errno == EEXIST) {
            std::wcout << "Folder '" << dirname << "' already exists"
                       << std::endl;
        } else {
            // something else
            std::cerr << "Cannot create folder error:" << strerror(errno)
                      << std::endl;
            throw std::runtime_error(strerror(errno));
        }
    } else {
        std::wcout << "Folder '" << dirname << "' created" << std::endl;
    }
#endif
}

int main(int argc, char **argv) {
#if defined(_WIN64) || defined(_WIN32)
    WSACleanup();
#endif
    unsigned short port = DEFAULT_PORT;

// Socket creation
#if defined(_WIN64) || defined(_WIN32)

    WSADATA wsaData;
    int iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (iResult != 0) {
        wprintf(L"WSAStartup failed with error: %d\n", iResult);
        exit(1);
    }

    SOCKET sockfd = socket(AF_INET, SOCK_DGRAM, 0);

    if (sockfd == INVALID_SOCKET)
        wprintf(L"socket function failed with error = %d\n", WSAGetLastError());
    else {
        wprintf(L"UDP Socket created\n");
        if (iResult == SOCKET_ERROR) {
            wprintf(L"closesocket failed with error = %d\n", WSAGetLastError());
            WSACleanup();
            exit(2);
        }
    }
#endif
#if defined(__unix__) || defined(__APPLE__)
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        if (DEBUG) std::cerr << "Could not create socket" << std::endl;
        exit(EXIT_FAILURE);
    }
    if (DEBUG) std::cout << "UDP Socket created" << std::endl;
#endif

    // Just for rebooting the server faster
    int optval = 1;
#if defined(_WIN64) || defined(_WIN32)
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (const char *)&optval,
               sizeof(int));
#endif
#if defined(__unix__) || defined(__APPLE__)
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (const void *)&optval,
               sizeof(int));
#endif

    // Server address
    struct sockaddr_in addr;
    bzero((char *)&addr, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = ntohs(port);

// Binding
#if defined(_WIN64) || defined(_WIN32)
    if (bind(sockfd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        if (DEBUG) wprintf(L"Could not bind the socket");
        exit(3);
    }
    if (DEBUG) wprintf(L"Socket bound on port %d\n", port);
#endif
#if defined(__unix__) || defined(__APPLE__)
    if (bind(sockfd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        if (DEBUG) std::cerr << "Could not bind the socket" << std::endl;
    }
    if (DEBUG) std::cout << "Socket bound on port " << port << std::endl;
#endif

    // main loop
    int n = 0;
    char buffer[BUFFER_SIZE];
    struct sockaddr_in clientAddr;
    socklen_t clientLen = sizeof(clientAddr);

    PacketHeader h_packet;
    PacketMotionData p_Motion;
    PacketSessionData p_Session;
    PacketLapData p_Lap;
    PacketEventData p_Event;
    PacketParticipantsData p_Participants;
    PacketCarSetupData p_CarSetup;
    PacketCarTelemetryData p_CarTelemetry;
    PacketCarStatusData p_CarStatus;
    PacketFinalClassificationData p_FinalClassification;
    PacketLobbyInfoData p_LobbyInfo;
    PacketCarDamageData p_CarDamage;
    PacketSessionHistoryData p_SessionHistory;

    // Create or check if exists the output folder
    const char *dirname = "tmp";
    mkoutdir(dirname);

    // DEBUG ONLY
    bool pm, ps, pl, pe, pp, pcs, pt, pcst, pf, pli, pcd, psh = false;

    std::cout << "Waiting for data..." << std::endl;
    while (1) {
        n = recvfrom(sockfd, buffer, BUFFER_SIZE, 0,
                     (struct sockaddr *)&clientAddr, &clientLen);

        if (n < 0) {
#if defined(_WIN64) || defined(_WIN32)
            if (DEBUG) wprintf(L"Error receiving data\n");
            WSACleanup();
#endif
#if defined(__unix__) || defined(__APPLE__)
            if (DEBUG) std::cerr << "Error receiving data" << std::endl;
#endif
            exit(EXIT_FAILURE);
        }
        h_packet.get(buffer);
        if (DEBUG) {
            std::wcout << "Packet ID: " << h_packet.m_packetId << std::endl;
        }

        if (DS) h_packet.to_csv(dirname);

        switch (h_packet.m_packetId) {
            case PACKET_ID_MOTION:  // 0
                if (DEBUG) {
                    if (pm) {
#if defined(_WIN64) || defined(_WIN32)
                        WSACleanup();
#endif
                        exit(0);
                    }
                    pm = true;
                }
                p_Motion.get(buffer);
                if (DEBUG) p_Motion.print();
                if (DS) p_Motion.to_csv(dirname);
                break;
            case PACKET_ID_SESSION:  // 1
                if (DEBUG) {
                    if (ps) {
#if defined(_WIN64) || defined(_WIN32)
                        WSACleanup();
#endif
                        exit(0);
                    }
                    ps = true;
                }
                p_Session.get(buffer);
                if (DEBUG) p_Session.print();
                if (DS) p_Session.to_csv(dirname);
                break;
            case PACKET_ID_LAP_DATA:  // 2
                if (DEBUG) {
                    if (pl) {
#if defined(_WIN64) || defined(_WIN32)
                        WSACleanup();
#endif
                        exit(0);
                    }
                    pl = true;
                }
                p_Lap.get(buffer);
                if (DEBUG) p_Lap.print();
                if (DS) p_Lap.to_csv(dirname);
                break;
            case PACKET_ID_EVENT:  // 3
                if (DEBUG) {
                    if (pe) {
#if defined(_WIN64) || defined(_WIN32)
                        WSACleanup();
#endif
                        exit(0);
                    }
                    pe = true;
                }
                p_Event.get(buffer);
                if (DEBUG) p_Event.print();
                if (DS) p_Event.to_csv(dirname);
                break;
            case PACKET_ID_PARTICIPANTS:  // 4
                if (DEBUG) {
                    if (pp) {
#if defined(_WIN64) || defined(_WIN32)
                        WSACleanup();
#endif
                        exit(0);
                    }
                    pp = true;
                }
                p_Participants.get(buffer);
                if (DEBUG) p_Participants.print();
                if (DS) p_Participants.to_csv(dirname);
                break;
            case PACKET_ID_CAR_SETUPS:  // 5
                if (DEBUG) {
                    if (pcs) {
#if defined(_WIN64) || defined(_WIN32)
                        WSACleanup();
#endif
                        exit(0);
                    }
                    pcs = true;
                }
                p_CarSetup.get(buffer);
                if (DEBUG) p_CarSetup.print();
                if (DS) p_CarSetup.to_csv(dirname);
                break;
            case PACKET_ID_CAR_TELEMETRY:  // 6
                if (DEBUG) {
                    if (pt) {
#if defined(_WIN64) || defined(_WIN32)
                        WSACleanup();
#endif
                        exit(0);
                    }
                    pt = true;
                }
                p_CarTelemetry.get(buffer);
                if (DEBUG) p_CarTelemetry.print();
                if (DS) p_CarTelemetry.to_csv(dirname);
                break;
            case PACKET_ID_CAR_STATUS:  // 7
                if (DEBUG) {
                    if (pcst) {
#if defined(_WIN64) || defined(_WIN32)
                        WSACleanup();
#endif
                        exit(0);
                    }
                    pcst = true;
                }
                p_CarStatus.get(buffer);
                if (DEBUG) p_CarStatus.print();
                if (DS) p_CarStatus.to_csv(dirname);
                break;
            case PACKET_ID_FINAL_CLASSIFICATION:  // 8
                if (DEBUG) {
                    if (pf) {
#if defined(_WIN64) || defined(_WIN32)
                        WSACleanup();
#endif
                        exit(0);
                    }
                    pf = true;
                }
                p_FinalClassification.get(buffer);
                if (DEBUG) p_FinalClassification.print();
                if (DS) p_FinalClassification.to_csv(dirname);
                break;
            case PACKET_ID_LOBBY_INFO:  // 9
                if (DEBUG) {
                    if (pli) {
#if defined(_WIN64) || defined(_WIN32)
                        WSACleanup();
#endif
                        exit(0);
                    }
                    pli = true;
                }
                p_LobbyInfo.get(buffer);
                if (DEBUG) p_LobbyInfo.print();
                if (DS) p_LobbyInfo.to_csv(dirname);
                break;
            case PACKET_ID_CAR_DAMAGE: //10
                if (DEBUG) {
                    if (pcd) {
#if defined(_WIN64) || defined(_WIN32)
                        WSACleanup();
#endif
                        exit(0);
                    }
                    pcd = true;
                }
                p_CarDamage.get(buffer);
                if (DEBUG) p_CarDamage.print();
                if (DS) p_CarDamage.to_csv(dirname);
                break;
            case PACKET_ID_SESSION_HISTORY: //11
                if (DEBUG) {
                    if (psh) {
#if defined(_WIN64) || defined(_WIN32)
                        WSACleanup();
#endif
                        exit(0);
                    }
                    psh = true;
                }
                p_SessionHistory.get(buffer);
                if (DEBUG) p_SessionHistory.print();
                if (DS) p_SessionHistory.to_csv(dirname);
                break;
            default:
                std::cout << "Default occurred: ";
                h_packet.print();
                std::cout << std::endl;
#if defined(_WIN64) || defined(_WIN32)
                WSACleanup();
#endif
                exit(-1);
                break;
        }
#if defined(_WIN64) || defined(_WIN32)
        if (!DEBUG) {
            std::cout
                << std::flush
                << "\r                                                      \r";
        }
#endif
#if defined(__linux__) || defined(__APPLE__)
        if (!DEBUG) {
            std::cout << std::flush << "\33[2K\r";
        }
#endif
        uint8 session = p_Session.m_sessionType;
        if (!DEBUG) {
            std::cout << "Session: " << Session_Type[session] << " ";
            if (session == 0) {
                std::cout << "Unknown Session";
            } else {
                if (session < 10) {
                    int seconds = p_Session.m_sessionTimeLeft;
                    int minutes = seconds / 60;
                    int hours = minutes / 60;
                    std::cout << "Time remaining: " << hours << ":" << minutes
                              << ":"
                              << seconds - ((hours * 3600) + (minutes * 60))
                              << " seconds";
                } else {
                    std::cout << "Lap: "
                              << (int)p_Lap.m_lapData[h_packet.m_playerCarIndex]
                                     .m_currentLapNum
                              << "/" << (int)p_Session.m_totalLaps
                              << " - Time: "
                              << p_Lap.m_lapData[h_packet.m_playerCarIndex]
                                     .m_currentLapTimeInMS;
                }
                std::cout << " - Position: "
                          << (int)p_Lap.m_lapData[h_packet.m_playerCarIndex]
                                 .m_carPosition;
            }
        }

        std::string eventString = (const char *)p_Event.m_eventStringCode;
        eventString = eventString.substr(0, 4);
        const char *eventConstChar = (const char *)eventString.c_str();
#if defined(_WIN64) || defined(_WIN32)
        if (strcmp(eventConstChar, "SEND") == 0) {
            wprintf(L"Session %s Ended\n", Session_Type[session]);
            if (rename(dirname, p_Session.GetSessionName())) {
                wprintf(L"Error renaming directory\n");
                WSACleanup();
                exit(2);
            }
        }
#endif
#if defined(__linux__) || defined(__APPLE__)
        if (std::strcmp(eventConstChar, "SEND") == 0) {
            std::wcout << "Session " << Session_Type[session] << " Ended"
                       << std::endl;
            std::cout << std::endl << dirname << std::endl;
            if (rename(dirname, p_Session.GetSessionName())) {
                std::cerr << "Error renaming directory" << std::endl;
                exit(2);
            }
        }
#endif
    }

#if defined(_WIN64) || defined(_WIN32)
    wprintf(L"Script ended, would you like to unify the data? (y/n) ");
#endif
#if defined(__linux__) || defined(__APPLE__)
    std::wcout << "Script ended, would you like to unify the data? (y/n) ";
#endif
    char input;
    std::cin >> input;
    fflush(stdin);
    if (input == 'y') {
        // PyObject *obj = Py_BuildValue("s", "analyzer/unify_car_data.py");
        FILE *file = fopen("analyzer/unify_car_data.py", "r+");
        if (file != NULL) {
            PyRun_SimpleFile(file, "test.py");
        } else {
            PyErr_Print();
            PyErr_Clear();
        }
    }

    return 0;
}
