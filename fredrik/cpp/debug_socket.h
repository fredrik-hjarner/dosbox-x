// AUTO_DISASSEMBLER_MODE does not need UnixSocketSender at all.
// because it writes to a file instead of a socket.
#ifndef AUTO_DISASSEMBLER_MODE

#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/un.h>

class UnixSocketSender {
private:
    int sock_fd;
    struct sockaddr_un addr;
    bool is_connected;
    
public:
    UnixSocketSender() : sock_fd(-1), is_connected(false) {
        // Create socket
        sock_fd = socket(AF_UNIX, SOCK_STREAM, 0);
        if (sock_fd == -1) {
            // Handle socket creation error
            std::cerr << "\n\n*** ERROR: Failed to create debug socket: " << strerror(errno) << " (errno: " << errno << ") ***\n\n" << std::endl;
            return;
        }
        
        // Setup address structure
        memset(&addr, 0, sizeof(addr));
        addr.sun_family = AF_UNIX;
        strncpy(addr.sun_path, "/tmp/debug_socket", sizeof(addr.sun_path) - 1);

        this->connect(); // connect immediately in constructor.
    }
    
    bool connect() {
        if (is_connected) return true;
        if (sock_fd == -1) {
            std::cerr << "\n\n*** ERROR: connect: Failed to create debug socket: " << strerror(errno) << " (errno: " << errno << ") ***\n\n" << std::endl;
            return false;
        }
        
        is_connected = (::connect(sock_fd, (struct sockaddr*)&addr, sizeof(addr)) == 0);
        if (!is_connected) {
            std::cerr << "\n\n*** ERROR: connect: Failed to connect to debug socket: " << strerror(errno) << " (errno: " << errno << ") ***\n\n" << std::endl;
        }
        return is_connected;
    }
    
    bool write(const std::string& message) {
        if (!is_connected) {
            std::cerr << "\n\n*** ERROR: write: Failed to connect to debug socket: " << strerror(errno) << " (errno: " << errno << ") ***\n\n" << std::endl;
            return false;
        }

        // TODO: This might be faster but I don't know.
        // Just send the message directly
        std::size_t length = ::write(sock_fd, message.c_str(), message.length());
        if (length != message.length()) {
            std::cerr << "\n\n*** ERROR: write: Failed to send message: " << strerror(errno) << " (errno: " << errno << ") ***\n\n" << std::endl;
            is_connected = false;
            return false;
        }
        return true;
    }
    
    ~UnixSocketSender() {
        if (sock_fd != -1) {
            close(sock_fd);
        }
    }
};

static UnixSocketSender debugSocket;

#endif