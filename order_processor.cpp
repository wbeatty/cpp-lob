#include "market.hpp"
#include <chrono>
#include <charconv>
#include <getopt.h>
#include <iostream>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include "random.cpp"


Market::Market()
    : orderPoolBuffer(std::make_unique<std::byte[]>(ORDER_POOL_BYTES)),
    orderPool(orderPoolBuffer.get(), ORDER_POOL_BYTES), orderQueue(4095) {
    buyTree = nullptr;
    sellTree = nullptr;
    lowestSell = nullptr;
    highestBuy = nullptr;
    inputFile = nullptr;
}

void Market::getOptions(int argc, char **argv) {
    int option_index = 0, option = 0;
    constexpr struct option options[] = {
        {"help", no_argument, nullptr, 'h'},
        {"file", required_argument, nullptr, 'f'},
        {"random", no_argument, nullptr, 'r'},
    };

    while ((option=getopt_long(argc, argv, "hf:r", options, &option_index)) != -1) {
        switch (option) {
            case 'h':
                std::cout << "Usage: cpp-lob [-h] [-f filename] [-r]\n";
                std::cout << "Options:\n";
                std::cout << "\t-h\tPrint this help message\n";
                std::cout << "\t-f [filename]\tSpecify file name\n";
                std::cout << "\t-r\tGenerate 1M random orders in selected file\n";
                exit(0);
            case 'f':
                inputFile = optarg;
                break;
            case 'r':
                create_random_input(inputFile);
                break;
            default:
                throw std::runtime_error("Unknown option");
        }
    }

}

Order *Market::createOrder() {
    void *mem = orderPool.allocate(sizeof(Order), alignof(Order));
    return new (mem) Order();
}

void Market::readOrders() {
    const int fd = open(inputFile, O_RDONLY, S_IRUSR | S_IWUSR);

    if (fd == -1) {
        throw std::runtime_error("Couldn't open file");
    }

    struct stat sb{};

    if (fstat(fd, &sb) == -1) {
        close(fd);
        throw std::runtime_error("Couldn't get file size");
    }
    if (sb.st_size == 0) {
        close(fd);
        throw std::runtime_error("File is empty");
    }

    void *raw = mmap(nullptr, sb.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
    if (raw == MAP_FAILED) {
        close(fd);
        throw std::runtime_error("Couldn't mmap file");
    }
    close(fd);

    const char *file_in_memory = static_cast<const char *>(raw);
    const char *const file_end = file_in_memory + sb.st_size;
    madvise(raw, sb.st_size, MADV_SEQUENTIAL);

    uint32_t idNumber = 0;
    const char *p = file_in_memory;

    std::cout << "Reading orders...\n";
    while (p != file_end) {
        if (*p == '#') {
            while (p < file_end && *p != '\n') p++;
            if (p < file_end) p++;
            continue;
        }

        auto *order = createOrder();
        order->buyOrder = (*p == 'B');
        p += 2;

        auto [ptr1, ec1] = std::from_chars(p, file_end, order->limitPrice);
        if (ec1 != std::errc()) {
            munmap(raw, sb.st_size);
            throw std::runtime_error("Invalid limit price");
        }
        p = ptr1 + 1;

        auto [ptr2, ec2] = std::from_chars(p, file_end, order->shares);
        if (ec2 != std::errc()) {
            munmap(raw, sb.st_size);
            throw std::runtime_error("Invalid shares");
        }
        p = ptr2;

        if (p < file_end && *p == '\n') p++;

        order->idNumber = idNumber++;
        order->entryTime = std::chrono::steady_clock::now().time_since_epoch().count();

        while (!queueOrder(order)) {}
    }
    munmap(raw, sb.st_size);
    inputDone.store(true, std::memory_order_release);
    std::cout << "All orders read.\n";
}

bool Market::queueOrder(Order *order) {
    if (!orderQueue.push(order)) {
        return false;
    }
    return true;
}
