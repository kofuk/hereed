#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>

#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

using namespace std;

int main(int argc, char **argv) {
    char *editor = getenv("EDITOR");

    if (editor == nullptr) {
        cerr << "EDITOR environment variable is not set" << endl;

        return 1;
    }

    filesystem::path tmp_base = "/tmp";

    filesystem::path tmp;

    do {
        tmp = tmp_base / ("hereed_" + to_string(rand()));

    } while (filesystem::exists(tmp));

    if (argc > 1 && !strcmp(argv[1], "-")) {
        cerr << "hereed: reading from stdin..." << endl;

        ofstream strm(tmp);
        char buf[4096];
        do {
            cin.read(buf, 4096);
            strm.write(buf, cin.gcount());
        } while (!cin.eof() && !cin.bad());
    } else {
        int fd = creat(tmp.c_str(), S_IRUSR | S_IWUSR);
        if (fd == -1) {
            perror("hereed: creat()");

            return 1;
        }

        if (close(fd) == -1) {
            perror("hereed: close()");

            return 1;
        }
    }

    pid_t pid = fork();
    if (pid < 0) {
        perror("hereed: fork()");

        filesystem::remove(tmp);

        return 1;
    } else if (pid == 0) {
        if (execlp(editor, editor, tmp.c_str(), 0) == -1) {
            perror("hereed: execlp()");

            return 1;
        }
    }

    int status;
    waitpid(pid, &status, 0);

    if (status != 0) {
        cerr << "Editor process exited with " + to_string(status) +
                    "; exiting hereed with failure."
             << endl;

        filesystem::remove(tmp);

        return 1;
    }

    ifstream strm(tmp.string());
    if (!strm) {
        filesystem::remove(tmp);

        return 1;
    }

    char buf[4096];

    do {
        strm.read(buf, 4096);

        cout.write(buf, strm.gcount());
    } while (!strm.eof() && !strm.bad());

    filesystem::remove(tmp);

    return 0;
}
