#include <iostream>
#include <string>
#include <thread>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <signal.h>
#include <chrono>
#include <sstream>
#include <iomanip>

class SimpleHTTPServer {
private:
    int server_fd;
    int port;
    bool running;
    
    const std::string index_html = R"(<!DOCTYPE html>
<html lang="ru">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Простой HTTP Сервер</title>
    <style>
        body {
            font-family: Arial, sans-serif;
            max-width: 800px;
            margin: 50px auto;
            padding: 20px;
            background-color: #f5f5f5;
        }
        .container {
            background: white;
            padding: 30px;
            border-radius: 10px;
            box-shadow: 0 2px 10px rgba(0,0,0,0.1);
        }
        h1 {
            color: #333;
            text-align: center;
            margin-bottom: 30px;
        }
        .info {
            background: #e3f2fd;
            padding: 15px;
            border-radius: 5px;
            margin: 20px 0;
        }
        .status {
            background: #d4edda;
            padding: 15px;
            border-radius: 5px;
            margin: 20px 0;
            color: #155724;
        }
        .nav {
            margin: 20px 0;
        }
        .nav a {
            display: inline-block;
            margin: 5px 10px;
            padding: 8px 15px;
            background: #2196f3;
            color: white;
            text-decoration: none;
            border-radius: 4px;
        }
        .nav a:hover {
            background: #1976d2;
        }
        .code {
            background: #f4f4f4;
            padding: 10px;
            border-radius: 4px;
            font-family: monospace;
            margin: 10px 0;
            white-space: pre;
        }
    </style>
</head>
<body>
    <div class="container">
        <h1>🌐 HTTP Сервер работает!</h1>
        
        <div class="status">
            <strong>✅ Успешно!</strong><br>
            Сервер доступен по публичному IP и работает корректно!
        </div>
        
        <div class="info">
            <strong>Информация о сервере:</strong><br>
            Порт: )" + std::to_string(port) + R"(<br>
            Протокол: HTTP/1.1<br>
            Время запуска: )" + getCurrentTime() + R"(<br>
            Архитектура: Thread-per-connection
        </div>
        
        <div class="nav">
            <h3>Доступные страницы:</h3>
            <a href="/">Главная</a>
            <a href="/about">О сервере</a>
            <a href="/time">Текущее время</a>
            <a href="/headers">Заголовки запроса</a>
            <a href="/test">Тестовая страница</a>
        </div>
        
        <div>
            <h3>Как протестировать:</h3>
            <div class="code">curl http://)" + getServerIP() + R"(:)" + std::to_string(port) + R"(/

echo "GET / HTTP/1.1\r\nHost: )" + getServerIP() + R"(\r\n\r\n" | nc )" + getServerIP() + R"( )" + std::to_string(port) + R"(</div>
        </div>
    </div>
</body>
</html>)";

public:
    SimpleHTTPServer(int p) : port(p), running(false), server_fd(-1) {}
    
    ~SimpleHTTPServer() {
        stop();
    }
    
    std::string getCurrentTime() const {
        auto now = std::chrono::system_clock::now();
        auto time_t = std::chrono::system_clock::to_time_t(now);
        return std::string(std::ctime(&time_t));
    }
    
    std::string getServerIP() const {
        // Попытка получить внешний IP (упрощенно)
        return "51.250.19.205"; // Ваш публичный IP
    }
    
    bool start() {
        std::cout << "=== ЗАПУСК HTTP СЕРВЕРА ===" << std::endl;
        std::cout << "Попытка запуска на порту " << port << "..." << std::endl;
        
        // Создание сокета
        server_fd = socket(AF_INET, SOCK_STREAM, 0);
        if (server_fd < 0) {
            std::cerr << "❌ Ошибка создания сокета: " << strerror(errno) << std::endl;
            return false;
        }
        std::cout << "✓ Сокет создан (fd=" << server_fd << ")" << std::endl;
        
        // Настройка сокета для переиспользования адреса
        int opt = 1;
        if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
            std::cerr << "❌ Ошибка setsockopt SO_REUSEADDR: " << strerror(errno) << std::endl;
            close(server_fd);
            return false;
        }
        std::cout << "✓ SO_REUSEADDR установлен" << std::endl;
        
        // Настройка адреса сервера
        struct sockaddr_in address;
        memset(&address, 0, sizeof(address));
        address.sin_family = AF_INET;
        address.sin_addr.s_addr = INADDR_ANY;  // 🔥 ВАЖНО: слушаем на ВСЕХ интерфейсах!
        address.sin_port = htons(port);
        
        std::cout << "✓ Адрес настроен: 0.0.0.0:" << port << " (все интерфейсы)" << std::endl;
        
        // Привязка к адресу
        if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
            std::cerr << "❌ Ошибка bind: " << strerror(errno) << std::endl;
            std::cerr << "   Возможные причины:" << std::endl;
            std::cerr << "   - Порт " << port << " уже занят" << std::endl;
            std::cerr << "   - Недостаточно прав (для портов < 1024)" << std::endl;
            std::cerr << "   - Firewall блокирует порт" << std::endl;
            close(server_fd);
            return false;
        }
        std::cout << "✓ Bind выполнен успешно" << std::endl;
        
        // Начало прослушивания
        if (listen(server_fd, 10) < 0) {
            std::cerr << "❌ Ошибка listen: " << strerror(errno) << std::endl;
            close(server_fd);
            return false;
        }
        std::cout << "✓ Listen запущен (backlog=10)" << std::endl;
        
        running = true;
        
        std::cout << "\n🎉 HTTP сервер успешно запущен!" << std::endl;
        std::cout << "🌐 Локальный доступ:    http://localhost:" << port << std::endl;
        std::cout << "🌍 Публичный доступ:    http://51.250.19.205:" << port << std::endl;
        std::cout << "📝 Логи запросов:" << std::endl;
        std::cout << std::string(50, '-') << std::endl;
        
        return true;
    }
    
    void run() {
        while (running) {
            struct sockaddr_in client_address;
            socklen_t client_len = sizeof(client_address);
            
            std::cout << "Ожидание входящих соединений..." << std::endl;
            
            // Принятие соединения
            int client_socket = accept(server_fd, (struct sockaddr*)&client_address, &client_len);
            if (client_socket < 0) {
                if (running) {
                    std::cerr << "❌ Ошибка accept: " << strerror(errno) << std::endl;
                }
                continue;
            }
            
            // Обработка в отдельном потоке
            std::thread client_thread(&SimpleHTTPServer::handleClient, this, client_socket, client_address);
            client_thread.detach();
        }
    }
    
private:
    void handleClient(int client_socket, struct sockaddr_in client_address) {
        // Получение IP клиента
        char client_ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &client_address.sin_addr, client_ip, INET_ADDRSTRLEN);
        int client_port = ntohs(client_address.sin_port);
        
        auto start_time = std::chrono::steady_clock::now();
        
        std::cout << "\n🔗 Новое соединение от " << client_ip << ":" << client_port << std::endl;
        
        // Чтение запроса
        char buffer[4096];
        memset(buffer, 0, sizeof(buffer));
        
        ssize_t bytes_read = recv(client_socket, buffer, sizeof(buffer) - 1, 0);
        if (bytes_read <= 0) {
            std::cout << "❌ Ошибка чтения запроса или пустой запрос" << std::endl;
            close(client_socket);
            return;
        }
        
        std::string request(buffer);
        std::cout << "📨 Получен запрос (" << bytes_read << " байт):" << std::endl;
        std::cout << "--- НАЧАЛО ЗАПРОСА ---" << std::endl;
        std::cout << request.substr(0, std::min(request.length(), size_t(500)));
        if (request.length() > 500) std::cout << "\n... (обрезано)";
        std::cout << std::endl << "--- КОНЕЦ ЗАПРОСА ---" << std::endl;
        
        // Парсинг первой строки запроса
        size_t first_line_end = request.find("\r\n");
        if (first_line_end == std::string::npos) {
            first_line_end = request.find("\n");
        }
        
        std::string request_line = request.substr(0, first_line_end);
        
        // Логирование в формате Common Log Format
        auto now = std::chrono::system_clock::now();
        auto time_t = std::chrono::system_clock::to_time_t(now);
        
        // Парсинг метода и пути
        std::istringstream iss(request_line);
        std::string method, path, version;
        iss >> method >> path >> version;
        
        std::cout << "📋 Parsed: " << method << " " << path << " " << version << std::endl;
        
        // Генерация ответа
        std::string response = generateResponse(method, path, request, client_ip);
        
        // Отправка ответа
        ssize_t bytes_sent = send(client_socket, response.c_str(), response.length(), 0);
        
        auto end_time = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
        
        if (bytes_sent > 0) {
            std::cout << "✅ Ответ отправлен (" << bytes_sent << " байт) за " << duration.count() << "мс" << std::endl;
        } else {
            std::cout << "❌ Ошибка отправки ответа: " << strerror(errno) << std::endl;
        }
        
        // Закрытие соединения
        close(client_socket);
        std::cout << "🔚 Соединение с " << client_ip << ":" << client_port << " закрыто" << std::endl;
    }
    
    std::string generateResponse(const std::string& method, const std::string& path, 
                               const std::string& full_request, const std::string& client_ip) {
        std::string status_line;
        std::string headers;
        std::string body;
        
        std::cout << "🔧 Генерация ответа для: " << method << " " << path << std::endl;
        
        if (method == "GET") {
            if (path == "/" || path == "/index.html") {
                status_line = "HTTP/1.1 200 OK\r\n";
                headers += "Content-Type: text/html; charset=utf-8\r\n";
                body = index_html;
            }
            else if (path == "/about") {
                status_line = "HTTP/1.1 200 OK\r\n";
                headers += "Content-Type: text/html; charset=utf-8\r\n";
                body = generateAboutPage();
            }
            else if (path == "/time") {
                status_line = "HTTP/1.1 200 OK\r\n";
                headers += "Content-Type: text/html; charset=utf-8\r\n";
                body = generateTimePage();
            }
            else if (path == "/headers") {
                status_line = "HTTP/1.1 200 OK\r\n";
                headers += "Content-Type: text/html; charset=utf-8\r\n";
                body = generateHeadersPage(full_request);
            }
            else if (path == "/test") {
                status_line = "HTTP/1.1 200 OK\r\n";
                headers += "Content-Type: text/html; charset=utf-8\r\n";
                body = generateTestPage(client_ip);
            }
            else {
                status_line = "HTTP/1.1 404 Not Found\r\n";
                headers += "Content-Type: text/html; charset=utf-8\r\n";
                body = generate404Page(path);
            }
        }
        else if (method == "HEAD") {
            status_line = "HTTP/1.1 200 OK\r\n";
            headers += "Content-Type: text/html; charset=utf-8\r\n";
            headers += "Content-Length: " + std::to_string(index_html.length()) + "\r\n";
            body = "";
        }
        else {
            status_line = "HTTP/1.1 405 Method Not Allowed\r\n";
            headers += "Content-Type: text/html; charset=utf-8\r\n";
            headers += "Allow: GET, HEAD\r\n";
            body = generateMethodNotAllowedPage(method);
        }
        
        // Общие заголовки
        headers += "Server: SimpleHTTPServer/1.0\r\n";
        headers += "Connection: close\r\n";
        headers += "Access-Control-Allow-Origin: *\r\n"; // Для CORS
        headers += "X-Client-IP: " + client_ip + "\r\n";
        
        auto now = std::chrono::system_clock::now();
        auto time_t = std::chrono::system_clock::to_time_t(now);
        char date_buffer[256];
        struct tm* gmt = gmtime(&time_t);
        strftime(date_buffer, sizeof(date_buffer), "%a, %d %b %Y %H:%M:%S GMT", gmt);
        headers += "Date: " + std::string(date_buffer) + "\r\n";
        
        if (!body.empty()) {
            headers += "Content-Length: " + std::to_string(body.length()) + "\r\n";
        }
        
        std::string full_response = status_line + headers + "\r\n" + body;
        
        std::cout << "📤 Ответ готов: " << full_response.substr(0, full_response.find("\r\n")) 
                  << " (" << full_response.length() << " байт)" << std::endl;
        
        return full_response;
    }
    
    std::string generateAboutPage() {
        return R"(<!DOCTYPE html>
<html>
<head>
    <meta charset="UTF-8">
    <title>О сервере</title>
    <style>
        body { font-family: Arial, sans-serif; max-width: 800px; margin: 50px auto; padding: 20px; }
        .container { background: white; padding: 30px; border-radius: 10px; box-shadow: 0 2px 10px rgba(0,0,0,0.1); }
    </style>
</head>
<body>
    <div class="container">
        <h1>О Simple HTTP Server</h1>
        <p><strong>Версия:</strong> 1.0</p>
        <p><strong>Порт:</strong> )" + std::to_string(port) + R"(</p>
        <p><strong>Протокол:</strong> HTTP/1.1</p>
        <p><strong>Поддерживаемые методы:</strong> GET, HEAD</p>
        <p><strong>Архитектура:</strong> Thread-per-connection</p>
        <p><strong>Связывание:</strong> 0.0.0.0 (все интерфейсы)</p>
        
        <h2>Сетевые интерфейсы:</h2>
        <ul>
            <li>localhost (127.0.0.1)</li>
            <li>Публичный IP (51.250.19.205)</li>
        </ul>
        
        <p><a href="/">← Назад на главную</a></p>
    </div>
</body>
</html>)";
    }
    
    std::string generateTimePage() {
        auto now = std::chrono::system_clock::now();
        auto time_t = std::chrono::system_clock::to_time_t(now);
        
        std::ostringstream oss;
        oss << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S %Z");
        std::string current_time = oss.str();
        
        return R"(<!DOCTYPE html>
<html>
<head>
    <meta charset="UTF-8">
    <title>Текущее время</title>
    <style>
        body { font-family: Arial, sans-serif; max-width: 600px; margin: 50px auto; padding: 20px; }
        .time { font-size: 2em; text-align: center; margin: 30px 0; color: #2196f3; }
    </style>
</head>
<body>
    <h1>Текущее время сервера</h1>
    <div class="time">)" + current_time + R"(</div>
    <p><a href="/">← Назад</a></p>
</body>
</html>)";
    }
    
    std::string generateHeadersPage(const std::string& request) {
        std::string headers_html = "<h2>Заголовки вашего запроса:</h2><pre style='background:#f4f4f4;padding:15px;border-radius:4px;'>";
        headers_html += request;
        headers_html += "</pre>";
        
        return R"(<!DOCTYPE html>
<html>
<head>
    <meta charset="UTF-8">
    <title>Заголовки запроса</title>
    <style>
        body { font-family: Arial, sans-serif; max-width: 800px; margin: 50px auto; padding: 20px; }
    </style>
</head>
<body>
    <h1>Анализ HTTP запроса</h1>
    )" + headers_html + R"(
    <p><a href="/">← Назад</a></p>
</body>
</html>)";
    }
    
    std::string generateTestPage(const std::string& client_ip) {
        return R"(<!DOCTYPE html>
<html>
<head>
    <meta charset="UTF-8">
    <title>Тестовая страница</title>
    <style>
        body { font-family: Arial, sans-serif; max-width: 600px; margin: 50px auto; padding: 20px; }
        .info { background: #e3f2fd; padding: 15px; border-radius: 5px; margin: 20px 0; }
        .success { background: #d4edda; padding: 15px; border-radius: 5px; color: #155724; }
    </style>
</head>
<body>
    <h1>🧪 Тестовая страница</h1>
    
    <div class="success">
        <h3>✅ Тест успешен!</h3>
        <p>Сервер корректно обрабатывает запросы и отвечает клиентам.</p>
    </div>
    
    <div class="info">
        <h3>Информация о соединении:</h3>
        <p><strong>Ваш IP:</strong> )" + client_ip + R"(</p>
        <p><strong>Время запроса:</strong> )" + getCurrentTime() + R"(</p>
        <p><strong>Сервер:</strong> SimpleHTTPServer/1.0</p>
    </div>
    
    <p><a href="/">← Назад</a></p>
</body>
</html>)";
    }
    
    std::string generate404Page(const std::string& path) {
        return R"(<!DOCTYPE html>
<html>
<head>
    <meta charset="UTF-8">
    <title>404 - Страница не найдена</title>
    <style>
        body { font-family: Arial, sans-serif; max-width: 600px; margin: 50px auto; padding: 20px; text-align: center; }
        .error { color: #d32f2f; font-size: 4em; margin: 20px 0; }
    </style>
</head>
<body>
    <div class="error">404</div>
    <h1>Страница не найдена</h1>
    <p>Запрошенная страница <strong>)" + path + R"(</strong> не существует на этом сервере.</p>
    <p><a href="/">🏠 Перейти на главную</a></p>
</body>
</html>)";
    }
    
    std::string generateMethodNotAllowedPage(const std::string& method) {
        return R"(<!DOCTYPE html>
<html>
<head>
    <meta charset="UTF-8">
    <title>405 - Метод не поддерживается</title>
    <style>
        body { font-family: Arial, sans-serif; max-width: 600px; margin: 50px auto; padding: 20px; text-align: center; }
        .error { color: #d32f2f; font-size: 4em; margin: 20px 0; }
    </style>
</head>
<body>
    <div class="error">405</div>
    <h1>Метод не поддерживается</h1>
    <p>HTTP метод <strong>)" + method + R"(</strong> не поддерживается этим сервером.</p>
    <p>Поддерживаемые методы: GET, HEAD</p>
    <p><a href="/">🏠 Перейти на главную</a></p>
</body>
</html>)";
    }
    
public:
    void stop() {
        if (running) {
            running = false;
            if (server_fd >= 0) {
                close(server_fd);
                std::cout << "\n🛑 HTTP сервер остановлен" << std::endl;
            }
        }
    }
};

int main() {
    std::cout << "=== ИСПРАВЛЕННЫЙ HTTP СЕРВЕР ===\n" << std::endl;
    
    SimpleHTTPServer server(8080);
    
    if (!server.start()) {
        std::cerr << "❌ Не удалось запустить сервер" << std::endl;
        return 1;
    }
    
    std::cout << "\n💡 Диагностические команды:" << std::endl;
    std::cout << "  • Проверить локально: curl http://localhost:8080/" << std::endl;
    std::cout << "  • Проверить публично: curl http://51.250.19.205:8080/" << std::endl;
    std::cout << "  • Проверить порт: netstat -tlnp | grep 8080" << std::endl;
    std::cout << "  • Проверить firewall: sudo ufw status" << std::endl;
    std::cout << "\n🔄 Сервер будет работать постоянно..." << std::endl;
    
    server.run();
    
    return 0;
}
