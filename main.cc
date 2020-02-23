#include <cerrno>
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

void write_tmp_file(ofstream &tmp_out, string filename) {
    if (filename == "-") {
        cerr << "hereed: reading from stdin..." << endl;

        char buf[4096];
        do {
            cin.read(buf, 4096);
            tmp_out.write(buf, cin.gcount());
        } while (!cin.eof() && !cin.bad());

        cin.clear();
    } else {
        ifstream in(filename);
        if (!in) {
            int err = errno;
            cerr << "Warning: failed to open " << filename << ":"
                 << strerror(err) << endl;

            return;
        }

        char buf[4096];
        do {
            in.read(buf, 4096);

            tmp_out.write(buf, in.gcount());
        } while (!in.eof() && !in.fail());
    }
}

string find_editor() {
    char *default_editor = getenv("EDITOR");
    if (default_editor != nullptr) {
        return default_editor;
    }

    default_editor = getenv("VISUAL");
    if (default_editor != nullptr) {
        return default_editor;
    }

    default_editor = getenv("SELECTED_EDITOR");
    if (default_editor != nullptr){
        return default_editor;
    }

    cerr << "Warning: Unable to detect default editor" << endl;

    return "nano";
}

int main(int argc, char **argv) {
    string editor = find_editor();

    filesystem::path tmp_base = "/tmp";
    filesystem::path tmp;
    do {
        tmp = tmp_base / ("hereed_" + to_string(rand()));

    } while (filesystem::exists(tmp));

    {
        ofstream tmp_out(tmp.string());

        for (argv++; *argv; ++argv) {
            write_tmp_file(tmp_out, *argv);
        }
    }

    pid_t pid = fork();
    if (pid < 0) {
        perror("hereed: fork()");

        filesystem::remove(tmp);

        return 1;
    } else if (pid == 0) {
        if (execlp(editor.c_str(), editor.c_str(), tmp.c_str(), 0) == -1) {
            perror("hereed: execlp()");

            return 1;
        }
    }

    cerr << "Waiting for your editor to finish" << endl;

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
