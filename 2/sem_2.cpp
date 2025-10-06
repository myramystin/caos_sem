#include <fcntl.h>
#include <unistd.h>
#include <cerrno>
#include <cstring>
#include <iostream>

static const char *S = "hello world";

void CoutWithEndl() {
    std::cout << S << std::endl;
}

void Cout() {
    std::cout << S;
}

void CoutMany() {
    for (size_t i = 0; i < 1000000; ++i) {
        std::cout << S;
    }
}

void WriteMany() {
    for (size_t i = 0; i < 1000000; ++i) {
        if (write(STDOUT_FILENO, S, 11) <= 0) {
            std::cerr << "write() failed: " << std::strerror(errno) << std::endl;
        }
    }
}

bool TryReadWrite() {
    constexpr size_t BUF_SZ = 4;
    char buf[BUF_SZ];

    // 100
    // int total_Read = 0;
    // char buf[100];

    // while (total_read != 100) {
    //     total_read += read(9, buf[total_read], 100 - total_read);
    //     // 0
    //     // -1
    // }

    while (true) {
        ssize_t n = read(STDIN_FILENO, buf, BUF_SZ);
        if (n == 0) {
            break;
        }
        if (n == -1) {
            std::cerr << "read() failed: " << std::strerror(errno) << std::endl;
            return false;
        }

        // NOTE! write не обязан за один раз записать весь буффер.
        // Для правильной работы тут должен быть цикл, который продолжает
        // делать write, пока суммарное количество записанных байт не равна
        // количеству байт, которое мы хотели записать

        if (write(STDOUT_FILENO, buf, n) <= 0) {
            std::cerr << "write() failed: " << std::strerror(errno) << std::endl;
            return false;
        }
    }
    return true;
}


bool TryReadFile() {
    const char *src = "input.txt";
    const char *dst = "output.txt";
    int in_fd = open(src, O_RDONLY);
    if (in_fd == -1) {
        std::cerr << errno << " " << std::strerror(errno) << std::endl;
        return false;
    }

    // O_APPEND - дописывать в конец файла
    // O_CREAT - создать файл, если его не существует
    // O_TRUNK - очистить файл, если он уже существует
    // O_RDONLY - открыть файл для чтения
    // O_WRONLY - открыть файл для записи
    // O_RDWR - открыть файл для чтения и записи
    //
    // если указан O_CREATE, то нужно также указать
    // права, с которыми будет создан файл

    // S_IROTH     // Other read permission (004)
    // S_IWOTH     // Other write permission (002)
    // S_IXOTH     // Other execute permission (001)
    // S_IRWXO     // Other read, write, execute (007) - combination of above

    int out_fd = open(dst, O_WRONLY | O_CREAT | O_TRUNC, 644);
    if (out_fd == -1) {
        std::cerr << std::strerror(errno) << std::endl;
        close(in_fd);
        return false;
    }

    constexpr size_t BUF_SZ = 16;
    char buf[BUF_SZ];

    ssize_t r;
    while ((r = read(in_fd, buf, BUF_SZ)) != 0) {
        if (r == -1) {
            std::cerr << std::strerror(errno) << std::endl;
            close(in_fd);
            close(out_fd);
            return false;
        }

        if (!write(STDOUT_FILENO, buf, static_cast<size_t>(r))) {
            close(in_fd);
            close(out_fd);
            return false;
        }

        if (!write(out_fd, buf, static_cast<size_t>(r))) {
            close(in_fd);
            close(out_fd);
            return false;
        }
    }

    if (close(in_fd) == -1) {
        std::cerr << std::strerror(errno) << std::endl;
    }
    if (close(out_fd) == -1) {
        std::cerr << std::strerror(errno) << std::endl;
    }
    return true;
}


int main() {
    int hex = 0x100;
    int oct = 0600;
    int bin = 0b100;

    std::cout << oct << " " << hex << " " << bin;
    return 0;
}
