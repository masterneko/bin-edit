#include <iostream>
#include <fstream>
#include <vector>
#include <bitset>
#include <cstdint>
#include <cstring>
#include <csignal>
#include <cctype>

#ifdef __unix__
#include <unistd.h>
#include <termios.h>
#endif

void signal_handler(int signal_num)
{
    std::cout << "\e[?25h" << std::endl; /* show cursor in case it wasn't */

    std::signal(signal_num, SIG_DFL);
    std::raise(signal_num);
}

#define KEY_UP ((uint32_t)0x415b1b)
#define KEY_DOWN ((uint32_t)0x425b1b)
#define KEY_RIGHT ((uint32_t)0x435b1b)
#define KEY_LEFT ((uint32_t)0x445b1b)

static uint32_t read_key()
{
    static struct termios oldt, newt;

    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~(ICANON | ECHO);

    tcsetattr(STDIN_FILENO, TCSANOW, &newt);
    std::cout << "\e[?25l"; /* hide cursor */
    std::cout.flush();

    uint8_t a = 0, b = 0, c = 0;

    read(STDIN_FILENO, &a, 1);

    if (a == 27)
    {
        read(STDIN_FILENO, &b, 1);

        if (b == 91)
        {
            read(STDIN_FILENO, &c, 1);
        }
    }

    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
    std::cout << "\e[?25h"; /* show cursor */
    std::cout.flush();

    return (c << 16) | (b << 8) | a;
}

static void erase_lines(size_t n)
{
    if (n == 0) return;

    for (size_t i = 1; i < n; i++)
    {
        // move cursor up, clear line
        std::cout << "\x1b[1A\x1b[2K";
    }

    std::cout << "\x1b[J\r"; /* clear from cursor to end of screen */
    std::cout.flush();
}

static std::ostream& print_byte_as_ascii(std::ostream& stream, char c)
{
    switch (c)
    {
        case '\0': stream << "'\\0'"; break;
        case '\n': stream << "'\\n'"; break;
        case '\r': stream << "'\\r'"; break;
        case '\t': stream << "'\\t'"; break;
        case '\b': stream << "'\\b'"; break;
        case '\a': stream << "'\\a'"; break;
        case '\f': stream << "'\\f'"; break;
        case '\v': stream << "'\\v'"; break;
        default:
        {
            if(std::isprint(c))
            {
                stream << "'" << c << "'";
            }

            break;
        }
    }

    return stream;
}

int main(int argc, const char* argv[])
{
    std::signal(SIGINT, signal_handler);

    if (argc < 2)
    {
        std::cout << "error: need a file path" << std::endl;

        return EXIT_FAILURE;
    }

    const char* filename = argv[1];
    std::ofstream(filename, std::ios::app).close(); /* touch file */
    std::fstream file(filename, std::ios::in | std::ios::binary);

    if (!file.is_open())
    {
        std::cerr << "error opening file: " << std::strerror(errno) << std::endl;

        return EXIT_FAILURE;
    }

    file.seekg(0, std::ios::end);

    size_t length = file.tellg();
    std::vector<char> bytes(length);

    file.seekg(0);

    if (!file.read(&bytes[0], length))
    {
        std::cerr << "error while reading contents of file" << std::endl;

        return EXIT_FAILURE;
    }

    std::cout << "arrow up   - go to previous byte\n";
    std::cout << "arrow down - go to next byte\n";
    std::cout << "arrow < >  - switch between bits\n";
    std::cout << "space      - toggle bit\n";
    std::cout << "i          - append new byte\n";
    std::cout << "d          - delete current byte\n";
    std::cout << "s          - save to file\n";
    std::cout << "q          - save and exit" << std::endl;

    size_t byte_index = 0, bit_index = 0;
    const char* message = nullptr;
    bool quit = false;

    while (true)
    {
        if (bytes.empty())
        {
            std::cout << "\n" << "(buffer empty)";
            std::cout.flush();
        }
        else
        {
            std::cout << "Byte: " << byte_index << ", Bit: " << bit_index;
            if (byte_index == bytes.size() - 1) std::cout << " EOF";
            if (message) std::cout << " " << message;
            std::cout << "\n";

            std::bitset<8> byte = bytes[byte_index];

            // std::bitset::operator[](0) is the LSB.
            // Higher indices refer to more significant bits.
            // If a forward loop were used then the bits would be
            // printed backwards.
            // MSB (higher index) -> LSB (lower index)
            for (size_t i = byte.size(); i-- > 0;)
            {
                if (i == bit_index)
                {
                    // highlight background
                    std::cout << "\x1b[30;47m" << byte[i] << "\x1b[0m";
                }
                else
                {
                    std::cout << byte[i];
                }
            }

            std::cout << " ";
            print_byte_as_ascii(std::cout, (char)byte.to_ulong());

            std::cout.flush();
        }

        if (quit) break;

        message = nullptr;

        uint32_t key = read_key();

        switch (key)
        {
            case ' ':
            {
                if (bytes.empty()) break;

                std::bitset<8> byte = bytes[byte_index];

                byte[bit_index].flip();
                bytes[byte_index] = (char)byte.to_ulong();

                break;
            }
            case 'i':
            {
                if (bytes.empty())
                {
                    bytes.push_back(0);
                    byte_index = 0;
                }
                else
                {
                    bytes.insert(bytes.begin() + byte_index + 1, 0);
                    byte_index++;
                }

                break;
            }
            case 'd':
            {
                if (bytes.empty()) break;

                bytes.erase(bytes.begin() + byte_index);

                if (byte_index >= bytes.size()) byte_index = bytes.size() - 1;

                break;
            }
            case 'q': quit = true;
            case 's':
            {
                file.close();
                file.open(filename, std::ios::out | std::ios::binary | std::ios::trunc);
                file.write(bytes.data(), bytes.size());
                message = file.fail() ? std::strerror(errno) : "SAVED";
                file.flush();

                break;
            }
            case KEY_RIGHT:
            {
                if (bit_index == 0 || bytes.empty()) break;

                bit_index--;

                break;
            }
            case KEY_LEFT:
            {
                if (bit_index == 7 || bytes.empty()) break;

                bit_index++;

                break;
            }
            case KEY_UP:
            {
                if (byte_index == 0 || bytes.empty()) break;

                byte_index--;

                break;
            }
            case KEY_DOWN:
            {
                if (byte_index == (bytes.size() - 1) || bytes.empty()) break;

                byte_index++;

                break;
            }
            default: break;
        }

        erase_lines(2);
    }

    std::cout << std::endl;

    return EXIT_SUCCESS;
}
