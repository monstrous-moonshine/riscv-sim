#include "common.hpp"
#include "sim.hpp"
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <elf.h>
#include <fstream>
#include <memory>
#include <new>
#include <sys/mman.h>
#include <utility>

namespace {
    struct fn_munmap {
        void operator()(uint8_t *p) { munmap(p, MEM_SIZE); }
    };

    std::pair<size_t, std::unique_ptr<uint8_t[]>> read_file(const char *name) {
        std::ifstream fin(name, std::ios::binary);
        if (!fin.is_open()) {
            return std::make_pair(0, nullptr);
        }
        fin.seekg(0, std::ios_base::end);
        size_t size = fin.tellg();
        fin.seekg(0, std::ios_base::beg);
        uint8_t *buf = new (std::nothrow) uint8_t[size];
        if (!buf) {
            return std::make_pair(0, nullptr);
        }
        fin.read(reinterpret_cast<char *>(buf), size);
        if ((size_t)fin.gcount() != size) {
            // something went wrong?
        }
        return std::make_pair(size, std::unique_ptr<uint8_t[]>(buf));
    }
} // namespace

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <elf_file>\n", prog_name);
        exit(0);
    }
    auto [elf_size, elf] = read_file(argv[1]);
    if (!elf) {
        fprintf(stderr, "%s: error reading file '%s'\n", prog_name, argv[1]);
        exit(1);
    }
    // This is the memory given to the target program
    uint8_t *_mem = (uint8_t *)mmap(NULL, MEM_SIZE, PROT_READ | PROT_WRITE,
                                    MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (_mem == MAP_FAILED) {
        perror("mmap");
        exit(1);
    }
    // Wrap it in a smart pointer so that it's freed at exit
    std::unique_ptr<uint8_t, fn_munmap> mem(_mem);
    Elf32_Ehdr *ehdr = (Elf32_Ehdr *)elf.get();
    // Copy loadable segments into memory
    // TODO: validate the elf file first
    Elf32_Phdr *phdr = (Elf32_Phdr *)(elf.get() + ehdr->e_phoff);
    for (int i = 0; i < ehdr->e_phnum; i++) {
        if (phdr[i].p_type == PT_LOAD) {
            memcpy(&mem.get()[phdr[i].p_vaddr - ADDR_BASE],
                   &elf.get()[phdr[i].p_offset],
                   phdr[i].p_filesz);
        }
    }
    // release memory for elf file as it's no longer needed
    elf.reset();
    simulator sim(mem.get());
    sim.run();
}
