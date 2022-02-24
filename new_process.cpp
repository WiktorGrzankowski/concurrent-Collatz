#include <iostream>
#include "lib/infint/InfInt.h"

#include "collatz.hpp"
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>

int main(int argc, char *argv[]) {
    int desc, buf_len, desc2;
//    printf("dzien dobry\n");
    const int contest_count = strtol(argv[3], NULL, 10);
//    printf("przezylem wczytanie\n");
    desc = open(argv[1], O_RDONLY);
    if (desc == -1) {
        exit(1);
    }
    size_t len[contest_count];
    std::string contest[contest_count];
//    std::cout << "dziecko wbilo na kwadrat i otworzylo desc a rozmiar "
//    << contest_count << '\n';
    for (int i = 0; i < contest_count; ++i) {
//        std::cout << "guwniak chce cos zapisac\n";
        if (read(desc, &len[i], sizeof(size_t)) < 0) {
            std::cout << "zle wczytana dlugosc\n";
            exit(1);
        }
        contest[i].resize(len[i]);
        if (read(desc, (void *) contest[i].c_str(), len[i]) < 0) {
            exit(1);
        }
//        std::cout << "guwniak wczytal len = " << len[i] <<
//        " a liczbe " << contest[i] << " " << i << '\n';
    }

    for (int i = 0; i < contest_count; ++i) {
        InfInt n = InfInt(contest[i].c_str());
        uint64_t result = calcCollatz(n);
        desc2 = open(argv[2], O_WRONLY);
        if (desc2 == -1) {
            std::cout << "nieudany desc2 :(\n";
            exit(1);
        }

        if (write(desc2, &result, sizeof(uint64_t)) < 0)
            exit(1);
//        std::cout << "dziecko zapisalo " << result << " " << i << "\n";

//        if (close(desc2))
//            exit(1);
    }
    if (close(desc2))
        exit(1);
    if (close(desc))
        exit(1);

    exit(0);
}
