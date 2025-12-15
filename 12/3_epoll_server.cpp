#include <iostream>
#include <string>
#include <vector>
#include <unordered_map>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <cstring>
#include <errno.h>
#include <chrono>
#include <thread>

// Состояние соединения для демонстрации state machine
enum class ConnectionState {
    READING_HEADER,
    READING_BODY,
    PROCESSING,
    WRITING_RESPONSE,
    CLOSING
};

struct ClientConnection {
    int fd;
    ConnectionState state;
    std::string readBuffer;
    std::string writeBuffer;
    size_t writeOffset;
    std::chrono::steady_clock::time_point lastActivity;
    
    ClientConnection(int clientFd) : fd(clientFd), state(ConnectionState::READING_HEADER),
                                   writeOffset(0), lastActivity(std::chrono::steady_clock::now()) {}
};

class EpollServer {
private:
    int serverFd;
    int epollFd;
    int port;
    bool running;
    std::unordered_map<int, std::unique_ptr<ClientConnection>> clients;
    
    static const int MAX_EVENTS = 1000;
    static const int TIMEOUT_MS = 1000;
    static const int CLIENT_TIMEOUT_SEC = 30;
    
public:
    EpollServer(int p) : serverFd(-1), epollFd(-1), port(p), running(false) {}
    
    ~EpollServer() {
        stop();
    }
    
    bool start() {
        // Создание серверного сокета
        serverFd = socket(AF_INET, SOCK_STREAM, 0);
        if (serverFd == -1) {
            std::cerr << "Ошибка создания сокета: " << strerror(errno) << std::endl;
            return false;
        }
        
        // Настройка сокета
        int opt = 1;
        setsockopt(serverFd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
        
        // Неблокирующий режим для серверного сокета
        if (!setNonBlocking(serverFd)) {
            close(serverFd);
            return false;
        }
        
        // Привязка к адресу
        struct sockaddr_in serverAddr;
        memset(&serverAddr, 0, sizeof(serverAddr));
        serverAddr.sin_family = AF_INET;
        serverAddr.sin_addr.s_addr = INADDR_ANY;
        serverAddr.sin_port = htons(port);
        
        if (bind(serverFd, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
            std::cerr << "Ошибка bind: " << strerror(errno) << std::endl;
            close(serverFd);
            return false;
        }
        
        if (listen(serverFd, 1000) < 0) {
            std::cerr << "Ошибка listen: " << strerror(errno) << std::endl;
            close(serverFd);
            return false;
        }
        
        // Создание epoll instance
        epollFd = epoll_create1(EPOLL_CLOEXEC);
        if (epollFd == -1) {
            std::cerr << "Ошибка epoll_create1: " << strerror(errno) << std::endl;
            close(serverFd);
            return false;
        }
        
        // Добавление серверного сокета в epoll
        struct epoll_event ev;
        ev.events = EPOLLIN | EPOLLET; // Edge-triggered для демонстрации
        ev.data.fd = serverFd;
        
        if (epoll_ctl(epollFd, EPOLL_CTL_ADD, serverFd, &ev) == -1) {
            std::cerr << "Ошибка epoll_ctl (сервер): " << strerror(errno) << std::endl;
            close(epollFd);
            close(serverFd);
            return false;
        }
        
        running = true;
        std::cout << "Epoll сервер запущен на порту " << port << std::endl;
        std::cout << "Режим: Edge-triggered" << std::endl;
        return true;
    }
    
    void run() {
        if (!running) return;
        
        std::cout << "Epoll сервер готов обрабатывать события..." << std::endl;
        
        struct epoll_event events[MAX_EVENTS];
        
        while (running) {
            // Ожидание событий
            int nfds = epoll_wait(epollFd, events, MAX_EVENTS, TIMEOUT_MS);
            
            if (nfds == -1) {
                if (errno == EINTR) continue; // Прерван сигналом
                std::cerr << "Ошибка epoll_wait: " << strerror(errno) << std::endl;
                break;
            }
            
            // Обработка событий
            for (int i = 0; i < nfds; i++) {
                if (events[i].data.fd == serverFd) {
                    // Новое входящее соединение
                    handleNewConnection();
                } else {
                    // Событие на клиентском соединении
                    handleClientEvent(events[i]);
                }
            }
            
            // Периодическая очистка неактивных соединений
            if (nfds == 0) { // timeout случился
                cleanupInactiveClients();
                printStats();
            }
        }
    }
    
    void handleNewConnection() {
        // В Edge-triggered режиме нужно принимать ВСЕ доступные соединения
        while (true) {
            struct sockaddr_in clientAddr;
            socklen_t clientLen = sizeof(clientAddr);
            
            int clientFd = accept(serverFd, (struct sockaddr*)&clientAddr, &clientLen);
            
            if (clientFd == -1) {
                if (errno == EAGAIN || errno == EWOULDBLOCK) {
                    // Больше нет ожидающих соединений
                    break;
                } else {
                    std::cerr << "Ошибка accept: " << strerror(errno) << std::endl;
                    break;
                }
            }
            
            // Настройка клиентского сокета
            if (!setNonBlocking(clientFd)) {
                close(clientFd);
                continue;
            }
            
            // Добавление в epoll
            struct epoll_event ev;
            ev.events = EPOLLIN | EPOLLET; // Edge-triggered
            ev.data.fd = clientFd;
            
            if (epoll_ctl(epollFd, EPOLL_CTL_ADD, clientFd, &ev) == -1) {
                std::cerr << "Ошибка epoll_ctl (клиент): " << strerror(errno) << std::endl;
                close(clientFd);
                continue;
            }
            
            // Создание объекта соединения
            clients[clientFd] = std::make_unique<ClientConnection>(clientFd);
            
            // Информация о клиенте
            char clientIP[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, &clientAddr.sin_addr, clientIP, INET_ADDRSTRLEN);
            int clientPort = ntohs(clientAddr.sin_port);
            
            std::cout << "Новое соединение: " << clientIP << ":" << clientPort 
                      << " (fd: " << clientFd << "), всего клиентов: " << clients.size() << std::endl;
        }
    }
    
    void handleClientEvent(const struct epoll_event& event) {
        int clientFd = event.data.fd;
        
        auto clientIt = clients.find(clientFd);
        if (clientIt == clients.end()) {
            std::cerr << "Событие для неизвестного клиента: " << clientFd << std::endl;
            return;
        }
        
        auto& client = clientIt->second;
        client->lastActivity = std::chrono::steady_clock::now();
        
        try {
            if (event.events & EPOLLIN) {
                handleClientRead(*client);
            }
            
            if (event.events & EPOLLOUT) {
                handleClientWrite(*client);
            }
            
            if (event.events & (EPOLLERR | EPOLLHUP)) {
                std::cout << "Соединение закрыто/ошибка для fd: " << clientFd << std::endl;
                removeClient(clientFd);
                return;
            }
            
            // Обновление интересующих событий в зависимости от состояния
            updateClientEvents(*client);
            
        } catch (const std::exception& e) {
            std::cerr << "Ошибка обработки клиента " << clientFd << ": " << e.what() << std::endl;
            removeClient(clientFd);
        }
    }
    
    void handleClientRead(ClientConnection& client) {
        // В ET режиме нужно читать ВСЕ доступные данные
        while (true) {
            char buffer[4096];
            ssize_t bytesRead = recv(client.fd, buffer, sizeof(buffer), 0);
            
            if (bytesRead > 0) {
                client.readBuffer.append(buffer, bytesRead);
                std::cout << "Прочитано " << bytesRead << " байт от fd " << client.fd 
                          << ", общий буфер: " << client.readBuffer.size() << " байт" << std::endl;
            } else if (bytesRead == 0) {
                // Соединение закрыто клиентом
                std::cout << "Клиент закрыл соединение: " << client.fd << std::endl;
                removeClient(client.fd);
                return;
            } else {
                if (errno == EAGAIN || errno == EWOULDBLOCK) {
                    // Больше данных нет
                    break;
                } else {
                    throw std::runtime_error("Ошибка recv: " + std::string(strerror(errno)));
                }
            }
        }
        
        // Обработка прочитанных данных через state machine
        processClientData(client);
    }
    
    void processClientData(ClientConnection& client) {
        switch (client.state) {
            case ConnectionState::READING_HEADER: {
                // Простой протокол: ищем строку с длиной сообщения
                size_t headerEnd = client.readBuffer.find("\n");
                if (headerEnd != std::string::npos) {
                    std::string headerLine = client.readBuffer.substr(0, headerEnd);
                    client.readBuffer.erase(0, headerEnd + 1);
                    
                    // Парсинг команды
                    if (headerLine == "PING") {
                        // Простая команда без тела
                        client.state = ConnectionState::PROCESSING;
                        processCommand(client, "PING", "");
                    } else if (headerLine.substr(0, 5) == "ECHO ") {
                        // Команда с данными
                        std::string data = headerLine.substr(5);
                        client.state = ConnectionState::PROCESSING;
                        processCommand(client, "ECHO", data);
                    } else if (headerLine.substr(0, 5) == "DATA ") {
                        // Команда с отдельным телом
                        try {
                            size_t dataSize = std::stoul(headerLine.substr(5));
                            if (client.readBuffer.size() >= dataSize) {
                                std::string data = client.readBuffer.substr(0, dataSize);
                                client.readBuffer.erase(0, dataSize);
                                client.state = ConnectionState::PROCESSING;
                                processCommand(client, "DATA", data);
                            } else {
                                client.state = ConnectionState::READING_BODY;
                            }
                        } catch (const std::exception& e) {
                            prepareErrorResponse(client, "Invalid DATA command");
                        }
                    } else {
                        prepareErrorResponse(client, "Unknown command: " + headerLine);
                    }
                }
                break;
            }
            
            case ConnectionState::READING_BODY: {
                // Реализация чтения тела сообщения
                // Для упрощения пропускаем
                client.state = ConnectionState::PROCESSING;
                processCommand(client, "DATA", client.readBuffer);
                break;
            }
            
            default:
                // Другие состояния обрабатываются в других методах
                break;
        }
    }
    
    void processCommand(ClientConnection& client, const std::string& command, const std::string& data) {
        std::cout << "Обработка команды '" << command << "' от fd " << client.fd << std::endl;
        
        std::string response;
        
        if (command == "PING") {
            response = "PONG\n";
        } else if (command == "ECHO") {
            response = "ECHO_RESPONSE: " + data + "\n";
        } else if (command == "DATA") {
            response = "DATA_RECEIVED: " + std::to_string(data.size()) + " bytes\n";
        } else {
            response = "ERROR: Unknown command\n";
        }
        
        client.writeBuffer += response;
        client.state = ConnectionState::WRITING_RESPONSE;
        client.writeOffset = 0;
    }
    
    void prepareErrorResponse(ClientConnection& client, const std::string& error) {
        client.writeBuffer = "ERROR: " + error + "\n";
        client.state = ConnectionState::WRITING_RESPONSE;
        client.writeOffset = 0;
    }
    
    void handleClientWrite(ClientConnection& client) {
        if (client.state != ConnectionState::WRITING_RESPONSE) {
            return;
        }
        
        // В ET режиме нужно записывать столько, сколько возможно
        while (client.writeOffset < client.writeBuffer.size()) {
            ssize_t bytesWritten = send(client.fd, 
                                       client.writeBuffer.c_str() + client.writeOffset,
                                       client.writeBuffer.size() - client.writeOffset, 
                                       0);
            
            if (bytesWritten > 0) {
                client.writeOffset += bytesWritten;
                std::cout << "Записано " << bytesWritten << " байт в fd " << client.fd 
                          << ", осталось: " << (client.writeBuffer.size() - client.writeOffset) << std::endl;
            } else if (bytesWritten == -1) {
                if (errno == EAGAIN || errno == EWOULDBLOCK) {
                    // Сокет больше не готов к записи
                    break;
                } else {
                    throw std::runtime_error("Ошибка send: " + std::string(strerror(errno)));
                }
            }
        }
        
        // Если все данные отправлены
        if (client.writeOffset >= client.writeBuffer.size()) {
            client.writeBuffer.clear();
            client.writeOffset = 0;
            client.state = ConnectionState::READING_HEADER;
            std::cout << "Ответ полностью отправлен клиенту " << client.fd << std::endl;
        }
    }
    
    void updateClientEvents(ClientConnection& client) {
        uint32_t events = EPOLLET; // Всегда edge-triggered
        
        // Всегда слушаем чтение для обнаружения закрытия соединения
        events |= EPOLLIN;
        
        // Добавляем EPOLLOUT только если есть что писать
        if (client.state == ConnectionState::WRITING_RESPONSE && 
            client.writeOffset < client.writeBuffer.size()) {
            events |= EPOLLOUT;
        }
        
        struct epoll_event ev;
        ev.events = events;
        ev.data.fd = client.fd;
        
        if (epoll_ctl(epollFd, EPOLL_CTL_MOD, client.fd, &ev) == -1) {
            std::cerr << "Ошибка epoll_ctl MOD для " << client.fd << ": " << strerror(errno) << std::endl;
        }
    }
    
    void removeClient(int clientFd) {
        if (epoll_ctl(epollFd, EPOLL_CTL_DEL, clientFd, nullptr) == -1) {
            std::cerr << "Ошибка epoll_ctl DEL для " << clientFd << ": " << strerror(errno) << std::endl;
        }
        
        close(clientFd);
        clients.erase(clientFd);
        
        std::cout << "Клиент " << clientFd << " удален, осталось: " << clients.size() << std::endl;
    }
    
    void cleanupInactiveClients() {
        auto now = std::chrono::steady_clock::now();
        auto timeout = std::chrono::seconds(CLIENT_TIMEOUT_SEC);
        
        std::vector<int> toRemove;
        
        for (const auto& [fd, client] : clients) {
            if (now - client->lastActivity > timeout) {
                toRemove.push_back(fd);
            }
        }
        
        for (int fd : toRemove) {
            std::cout << "Удаление неактивного клиента: " << fd << std::endl;
            removeClient(fd);
        }
    }
    
    bool setNonBlocking(int fd) {
        int flags = fcntl(fd, F_GETFL, 0);
        if (flags == -1) {
            std::cerr << "Ошибка fcntl F_GETFL: " << strerror(errno) << std::endl;
            return false;
        }
        
        if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1) {
            std::cerr << "Ошибка fcntl F_SETFL: " << strerror(errno) << std::endl;
            return false;
        }
        
        return true;
    }
    
    void printStats() {
        std::cout << "\n=== СТАТИСТИКА EPOLL СЕРВЕРА ===" << std::endl;
        std::cout << "Активных соединений: " << clients.size() << std::endl;
        
        // Статистика по состояниям
        std::unordered_map<ConnectionState, int> stateStats;
        for (const auto& [fd, client] : clients) {
            stateStats[client->state]++;
        }
        
        std::cout << "По состояниям:" << std::endl;
        for (const auto& [state, count] : stateStats) {
            std::cout << "  ";
            switch (state) {
                case ConnectionState::READING_HEADER: std::cout << "READING_HEADER"; break;
                case ConnectionState::READING_BODY: std::cout << "READING_BODY"; break;
                case ConnectionState::PROCESSING: std::cout << "PROCESSING"; break;
                case ConnectionState::WRITING_RESPONSE: std::cout << "WRITING_RESPONSE"; break;
                case ConnectionState::CLOSING: std::cout << "CLOSING"; break;
            }
            std::cout << ": " << count << std::endl;
        }
        
        std::cout << "===============================" << std::endl;
    }
    
    void stop() {
        if (running) {
            running = false;
            
            // Закрытие всех клиентских соединений
            for (const auto& [fd, client] : clients) {
                close(fd);
            }
            clients.clear();
            
            if (epollFd != -1) {
                close(epollFd);
                epollFd = -1;
            }
            
            if (serverFd != -1) {
                close(serverFd);
                serverFd = -1;
            }
            
            std::cout << "Epoll сервер остановлен" << std::endl;
        }
    }
};

// Тестовый клиент для демонстрации
class EpollTestClient {
private:
    std::string serverIP;
    int serverPort;
    
public:
    EpollTestClient(const std::string& ip, int port) : serverIP(ip), serverPort(port) {}
    
    bool testConnection(const std::string& command, const std::string& data = "") {
        int clientFd = socket(AF_INET, SOCK_STREAM, 0);
        if (clientFd == -1) {
            std::cerr << "Ошибка создания клиентского сокета: " << strerror(errno) << std::endl;
            return false;
        }
        
        struct sockaddr_in serverAddr;
        memset(&serverAddr, 0, sizeof(serverAddr));
        serverAddr.sin_family = AF_INET;
        serverAddr.sin_port = htons(serverPort);
        inet_pton(AF_INET, serverIP.c_str(), &serverAddr.sin_addr);
        
        if (connect(clientFd, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
            std::cerr << "Ошибка подключения: " << strerror(errno) << std::endl;
            close(clientFd);
            return false;
        }
        
        // Отправка команды
        std::string message = command;
        if (!data.empty() && command != "ECHO") {
            message += " " + std::to_string(data.size()) + "\n" + data;
        } else if (command == "ECHO") {
            message += " " + data;
        }
        message += "\n";
        
        send(clientFd, message.c_str(), message.size(), 0);
        
        // Получение ответа
        char buffer[1024];
        memset(buffer, 0, sizeof(buffer));
        ssize_t bytesReceived = recv(clientFd, buffer, sizeof(buffer) - 1, 0);
        
        if (bytesReceived > 0) {
            std::cout << "Клиент получил ответ: " << buffer;
        } else {
            std::cout << "Клиент не получил ответ" << std::endl;
        }
        
        close(clientFd);
        return bytesReceived > 0;
    }
};

void runEpollTests() {
    std::this_thread::sleep_for(std::chrono::seconds(1));
    
    std::cout << "\n=== ТЕСТИРОВАНИЕ EPOLL СЕРВЕРА ===" << std::endl;
    
    EpollTestClient client("127.0.0.1", 8080);
    
    // Тест различных команд
    std::vector<std::pair<std::string, std::string>> tests = {
        {"PING", ""},
        {"ECHO", "Hello World!"},
        {"DATA", "This is test data for DATA command"},
        {"UNKNOWN", ""}
    };
    
    for (const auto& [command, data] : tests) {
        std::cout << "\nТест команды: " << command << std::endl;
        client.testConnection(command, data);
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }
    
    std::cout << "\n=== ТЕСТ МНОЖЕСТВЕННЫХ СОЕДИНЕНИЙ ===" << std::endl;
    
    // Быстрое создание множественных соединений
    std::vector<std::thread> clientThreads;
    for (int i = 0; i < 10; i++) {
        clientThreads.emplace_back([&client, i]() {
            client.testConnection("ECHO", "Message from client " + std::to_string(i));
        });
    }
    
    for (auto& thread : clientThreads) {
        thread.join();
    }
}

int main() {
    std::cout << "=== ДЕМОНСТРАЦИЯ EPOLL СЕРВЕРА ===\n" << std::endl;
    
    EpollServer server(8080);
    
    if (!server.start()) {
        return 1;
    }
    
    // Запуск сервера в отдельном потоке
    std::thread serverThread(&EpollServer::run, &server);
    
    // Тестирование
    std::thread testThread(runEpollTests);
    
    testThread.join();
    
    // Даем время для завершения обработки
    std::this_thread::sleep_for(std::chrono::seconds(3));
    
    server.stop();
    serverThread.join();

    return 0;
}
