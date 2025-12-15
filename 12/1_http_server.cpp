#include <iostream>
#include <string>
#include <map>
#include <sstream>
#include <thread>
#include <fstream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <algorithm>
#include <vector>

struct HTTPRequest {
    std::string method;
    std::string path;
    std::string version;
    std::map<std::string, std::string> headers;
    std::string body;
    
    void display() const {
        std::cout << "=== HTTP ЗАПРОС ===" << std::endl;
        std::cout << "Метод: " << method << std::endl;
        std::cout << "Путь: " << path << std::endl;
        std::cout << "Версия: " << version << std::endl;
        std::cout << "Заголовки:" << std::endl;
        for (const auto& header : headers) {
            std::cout << "  " << header.first << ": " << header.second << std::endl;
        }
        if (!body.empty()) {
            std::cout << "Тело (" << body.length() << " байт):" << std::endl;
            std::cout << body << std::endl;
        }
        std::cout << "==================" << std::endl;
    }
};

struct HTTPResponse {
    std::string version;
    int statusCode;
    std::string statusMessage;
    std::map<std::string, std::string> headers;
    std::string body;
    
    HTTPResponse(int code = 200, const std::string& message = "OK") 
        : version("HTTP/1.1"), statusCode(code), statusMessage(message) {
        headers["Server"] = "SimpleHTTP/1.0";
        headers["Connection"] = "close";
    }
    
    void setContentType(const std::string& type) {
        headers["Content-Type"] = type;
    }
    
    void setBody(const std::string& content) {
        body = content;
        headers["Content-Length"] = std::to_string(content.length());
    }
    
    std::string toString() const {
        std::stringstream ss;
        ss << version << " " << statusCode << " " << statusMessage << "\r\n";
        
        for (const auto& header : headers) {
            ss << header.first << ": " << header.second << "\r\n";
        }
        
        ss << "\r\n" << body;
        return ss.str();
    }
    
    void display() const {
        std::cout << "=== HTTP ОТВЕТ ===" << std::endl;
        std::cout << "Статус: " << statusCode << " " << statusMessage << std::endl;
        std::cout << "Заголовки:" << std::endl;
        for (const auto& header : headers) {
            std::cout << "  " << header.first << ": " << header.second << std::endl;
        }
        if (!body.empty()) {
            std::cout << "Тело (" << body.length() << " байт):" << std::endl;
            if (body.length() > 200) {
                std::cout << body.substr(0, 200) << "..." << std::endl;
            } else {
                std::cout << body << std::endl;
            }
        }
        std::cout << "=================" << std::endl;
    }
};

class HTTPParser {
public:
    static HTTPRequest parseRequest(const std::string& rawRequest) {
        HTTPRequest request;
        std::istringstream stream(rawRequest);
        std::string line;
        
        // Парсинг строки запроса
        if (std::getline(stream, line)) {
            // Удаляем \r если есть
            if (!line.empty() && line.back() == '\r') {
                line.pop_back();
            }
            
            std::istringstream lineStream(line);
            lineStream >> request.method >> request.path >> request.version;
        }
        
        // Парсинг заголовков
        while (std::getline(stream, line) && !line.empty() && line != "\r") {
            if (!line.empty() && line.back() == '\r') {
                line.pop_back();
            }
            
            size_t colonPos = line.find(':');
            if (colonPos != std::string::npos) {
                std::string name = line.substr(0, colonPos);
                std::string value = line.substr(colonPos + 1);
                
                // Убираем пробелы
                name.erase(0, name.find_first_not_of(" \t"));
                name.erase(name.find_last_not_of(" \t") + 1);
                value.erase(0, value.find_first_not_of(" \t"));
                value.erase(value.find_last_not_of(" \t") + 1);
                
                // Приводим имя заголовка к нижнему регистру для упрощения
                std::transform(name.begin(), name.end(), name.begin(), ::tolower);
                
                request.headers[name] = value;
            }
        }
        
        // Парсинг тела (если есть)
        std::string bodyLine;
        while (std::getline(stream, bodyLine)) {
            request.body += bodyLine + "\n";
        }
        
        // Убираем последний \n если он есть
        if (!request.body.empty() && request.body.back() == '\n') {
            request.body.pop_back();
        }
        
        return request;
    }
};

class SimpleHTTPServer {
private:
    int serverSocket;
    int port;
    bool running;
    std::map<std::string, std::string> routes;
    
public:
    SimpleHTTPServer(int p) : port(p), running(false), serverSocket(-1) {
        setupDefaultRoutes();
    }
    
    ~SimpleHTTPServer() {
        stop();
    }
    
    void setupDefaultRoutes() {
        routes["/"] = R"(
<!DOCTYPE html>
<html>
<head>
    <title>Simple HTTP Server</title>
</head>
<body>
    <h1>Добро пожаловать на Simple HTTP Server!</h1>
    <p>Это демонстрационный HTTP/1.1 сервер.</p>
    <ul>
        <li><a href="/about">О сервере</a></li>
        <li><a href="/api/status">API статус</a></li>
        <li><a href="/test">Тестовая страница</a></li>
    </ul>
</body>
</html>
        )";
        
        routes["/about"] = R"(
<!DOCTYPE html>
<html>
<head>
    <title>О сервере</title>
</head>
<body>
    <h1>О Simple HTTP Server</h1>
    <p>Версия: 1.0</p>
    <p>Протокол: HTTP/1.1</p>
    <p>Поддерживаемые методы: GET, POST</p>
    <p><a href="/">Главная</a></p>
</body>
</html>
        )";
        
        routes["/api/status"] = R"({
    "status": "OK",
    "server": "SimpleHTTP/1.0",
    "version": "HTTP/1.1",
    "timestamp": ")" + std::to_string(time(nullptr)) + R"(",
    "uptime": "active"
})";
    }
    
    bool start() {
        serverSocket = socket(AF_INET, SOCK_STREAM, 0);
        if (serverSocket == -1) {
            std::cerr << "Ошибка создания HTTP сокета: " << strerror(errno) << std::endl;
            return false;
        }
        
        int opt = 1;
        setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
        
        struct sockaddr_in serverAddr;
        memset(&serverAddr, 0, sizeof(serverAddr));
        serverAddr.sin_family = AF_INET;
        serverAddr.sin_addr.s_addr = INADDR_ANY;
        serverAddr.sin_port = htons(port);
        
        if (bind(serverSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
            std::cerr << "Ошибка bind HTTP сервера: " << strerror(errno) << std::endl;
            close(serverSocket);
            return false;
        }
        
        if (listen(serverSocket, 10) < 0) {
            std::cerr << "Ошибка listen HTTP сервера: " << strerror(errno) << std::endl;
            close(serverSocket);
            return false;
        }
        
        std::cout << "HTTP сервер запущен на порту " << port << std::endl;
        std::cout << "Доступ: http://localhost:" << port << std::endl;
        running = true;
        return true;
    }
    
    HTTPResponse handleRequest(const HTTPRequest& request) {
        std::cout << "\n=== ОБРАБОТКА HTTP ЗАПРОСА ===" << std::endl;
        request.display();
        
        HTTPResponse response;
        
        if (request.method == "GET") {
            return handleGET(request);
        } else if (request.method == "POST") {
            return handlePOST(request);
        } else if (request.method == "HEAD") {
            HTTPResponse headResponse = handleGET(request);
            headResponse.body = ""; // HEAD возвращает только заголовки
            headResponse.headers["Content-Length"] = "0";
            return headResponse;
        } else if (request.method == "OPTIONS") {
            response.setContentType("text/plain");
            response.headers["Allow"] = "GET, POST, HEAD, OPTIONS";
            response.setBody("GET, POST, HEAD, OPTIONS");
        } else {
            response = HTTPResponse(405, "Method Not Allowed");
            response.setContentType("text/plain");
            response.headers["Allow"] = "GET, POST, HEAD, OPTIONS";
            response.setBody("Method not allowed: " + request.method);
        }
        
        return response;
    }
    
    HTTPResponse handleGET(const HTTPRequest& request) {
        HTTPResponse response;
        
        auto route = routes.find(request.path);
        if (route != routes.end()) {
            if (request.path.substr(0, 4) == "/api") {
                response.setContentType("application/json");
            } else {
                response.setContentType("text/html; charset=utf-8");
            }
            response.setBody(route->second);
        } else {
            response = HTTPResponse(404, "Not Found");
            response.setContentType("text/html; charset=utf-8");
            response.setBody(R"(
<!DOCTYPE html>
<html>
<head><title>404 Not Found</title></head>
<body>
    <h1>404 - Страница не найдена</h1>
    <p>Запрошенная страница не существует.</p>
    <p><a href="/">Главная</a></p>
</body>
</html>
            )");
        }
        
        return response;
    }
    
    HTTPResponse handlePOST(const HTTPRequest& request) {
        HTTPResponse response;
        
        if (request.path == "/api/echo") {
            response.setContentType("application/json");
            std::string jsonResponse = R"({
    "method": "POST",
    "path": ")" + request.path + R"(",
    "received_data": ")" + request.body + R"(",
    "content_type": ")" + 
            (request.headers.count("content-type") ? request.headers.at("content-type") : "unknown") + R"(",
    "timestamp": ")" + std::to_string(time(nullptr)) + R"("
})";
            response.setBody(jsonResponse);
        } else {
            response = HTTPResponse(404, "Not Found");
            response.setContentType("text/plain");
            response.setBody("POST endpoint not found: " + request.path);
        }
        
        return response;
    }
    
    void handleClient(int clientSocket) {
        char buffer[4096];
        memset(buffer, 0, sizeof(buffer));
        
        // Получение HTTP запроса
        ssize_t bytesReceived = recv(clientSocket, buffer, sizeof(buffer) - 1, 0);
        
        if (bytesReceived <= 0) {
            close(clientSocket);
            return;
        }
        
        std::string rawRequest(buffer);
        HTTPRequest request = HTTPParser::parseRequest(rawRequest);
        
        // Обработка запроса
        HTTPResponse response = handleRequest(request);
        response.display();
        
        // Отправка ответа
        std::string responseStr = response.toString();
        send(clientSocket, responseStr.c_str(), responseStr.length(), 0);
        
        close(clientSocket);
    }
    
    void run() {
        if (!running) return;
        
        std::cout << "HTTP сервер готов обрабатывать запросы...\n" << std::endl;
        
        while (running) {
            struct sockaddr_in clientAddr;
            socklen_t clientLen = sizeof(clientAddr);
            
            int clientSocket = accept(serverSocket, (struct sockaddr*)&clientAddr, &clientLen);
            if (clientSocket < 0) {
                if (running) {
                    std::cerr << "Ошибка accept: " << strerror(errno) << std::endl;
                }
                continue;
            }
            
            // Обработка в отдельном потоке
            std::thread clientThread(&SimpleHTTPServer::handleClient, this, clientSocket);
            clientThread.detach();
        }
    }
    
    void stop() {
        if (running) {
            running = false;
            if (serverSocket != -1) {
                close(serverSocket);
                std::cout << "HTTP сервер остановлен" << std::endl;
            }
        }
    }
};

class SimpleHTTPClient {
public:
    static HTTPResponse sendRequest(const std::string& host, int port, 
                                  const HTTPRequest& request) {
        int clientSocket = socket(AF_INET, SOCK_STREAM, 0);
        if (clientSocket == -1) {
            HTTPResponse errorResponse(500, "Socket Creation Error");
            return errorResponse;
        }
        
        struct sockaddr_in serverAddr;
        memset(&serverAddr, 0, sizeof(serverAddr));
        serverAddr.sin_family = AF_INET;
        serverAddr.sin_port = htons(port);
        inet_pton(AF_INET, host.c_str(), &serverAddr.sin_addr);
        
        if (connect(clientSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
            close(clientSocket);
            HTTPResponse errorResponse(500, "Connection Error");
            return errorResponse;
        }
        
        // Формирование и отправка запроса
        std::stringstream requestStr;
        requestStr << request.method << " " << request.path << " " << request.version << "\r\n";
        requestStr << "Host: " << host << ":" << port << "\r\n";
        
        for (const auto& header : request.headers) {
            requestStr << header.first << ": " << header.second << "\r\n";
        }
        
        requestStr << "\r\n" << request.body;
        
        std::string req = requestStr.str();
        send(clientSocket, req.c_str(), req.length(), 0);
        
        // Получение ответа
        char buffer[4096];
        memset(buffer, 0, sizeof(buffer));
        ssize_t bytesReceived = recv(clientSocket, buffer, sizeof(buffer) - 1, 0);
        
        close(clientSocket);
        
        if (bytesReceived <= 0) {
            HTTPResponse errorResponse(500, "Receive Error");
            return errorResponse;
        }
        
        // Простой парсинг ответа
        std::string rawResponse(buffer);
        return parseResponse(rawResponse);
    }
    
private:
    static HTTPResponse parseResponse(const std::string& rawResponse) {
        HTTPResponse response;
        std::istringstream stream(rawResponse);
        std::string line;
        
        // Парсинг статусной строки
        if (std::getline(stream, line)) {
            if (!line.empty() && line.back() == '\r') line.pop_back();
            
            std::istringstream lineStream(line);
            std::string statusCodeStr;
            lineStream >> response.version >> statusCodeStr;
            response.statusCode = std::stoi(statusCodeStr);
            
            // Получаем остальную часть как сообщение статуса
            std::string word;
            response.statusMessage = "";
            while (lineStream >> word) {
                if (!response.statusMessage.empty()) response.statusMessage += " ";
                response.statusMessage += word;
            }
        }
        
        // Парсинг заголовков
        while (std::getline(stream, line) && !line.empty() && line != "\r") {
            if (!line.empty() && line.back() == '\r') line.pop_back();
            
            size_t colonPos = line.find(':');
            if (colonPos != std::string::npos) {
                std::string name = line.substr(0, colonPos);
                std::string value = line.substr(colonPos + 1);
                
                // Убираем пробелы
                name.erase(0, name.find_first_not_of(" \t"));
                name.erase(name.find_last_not_of(" \t") + 1);
                value.erase(0, value.find_first_not_of(" \t"));
                value.erase(value.find_last_not_of(" \t") + 1);
                
                response.headers[name] = value;
            }
        }
        
        // Парсинг тела
        std::string bodyLine;
        while (std::getline(stream, bodyLine)) {
            response.body += bodyLine + "\n";
        }
        
        if (!response.body.empty() && response.body.back() == '\n') {
            response.body.pop_back();
        }
        
        return response;
    }
};

void testHTTPClient() {
    std::this_thread::sleep_for(std::chrono::seconds(1));
    
    std::cout << "\n=== ТЕСТИРОВАНИЕ HTTP КЛИЕНТА ===" << std::endl;
    
    std::vector<std::string> paths = {"/", "/about", "/api/status", "/nonexistent"};
    
    for (const std::string& path : paths) {
        HTTPRequest request;
        request.method = "GET";
        request.path = path;
        request.version = "HTTP/1.1";
        request.headers["User-Agent"] = "SimpleHTTPClient/1.0";
        request.headers["Accept"] = "text/html,application/json";
        request.headers["Connection"] = "close";
        
        std::cout << "\nОтправка GET " << path << ":" << std::endl;
        HTTPResponse response = SimpleHTTPClient::sendRequest("127.0.0.1", 8080, request);
        response.display();
    }
    
    // Тест POST запроса
    HTTPRequest postRequest;
    postRequest.method = "POST";
    postRequest.path = "/api/echo";
    postRequest.version = "HTTP/1.1";
    postRequest.headers["User-Agent"] = "SimpleHTTPClient/1.0";
    postRequest.headers["Content-Type"] = "application/json";
    postRequest.headers["Connection"] = "close";
    postRequest.body = R"({"test": "data", "message": "Hello from client!"})";
    postRequest.headers["Content-Length"] = std::to_string(postRequest.body.length());
    
    std::cout << "\nОтправка POST /api/echo:" << std::endl;
    HTTPResponse postResponse = SimpleHTTPClient::sendRequest("127.0.0.1", 8080, postRequest);
    postResponse.display();
}

int main() {
    std::cout << "=== ДЕМОНСТРАЦИЯ HTTP/1.1 СЕРВЕРА ===\n" << std::endl;
    
    SimpleHTTPServer server(8080);
    
    if (!server.start()) {
        return 1;
    }
    
    // Запуск сервера в отдельном потоке
    std::thread serverThread(&SimpleHTTPServer::run, &server);
    
    // Тестирование клиента
    std::thread clientThread(testHTTPClient);
    
    clientThread.join();
    
    // Даем время для завершения обработки
    std::this_thread::sleep_for(std::chrono::seconds(2));
    

    /// 
    server.stop();
    serverThread.join();
    
    return 0;
}
