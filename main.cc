#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>

#include <fcntl.h>
#include <getopt.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

using namespace std;

static bool option_verbose;
static bool option_ignore_fail;

void write_tmp_file(ofstream &tmp_out, string filename) {
    if (filename == "-") {
        if (option_verbose) {
            cerr << "hereed: reading from stdin..." << endl;
        }

        char buf[4096];
        do {
            cin.read(buf, 4096);
            tmp_out.write(buf, cin.gcount());
        } while (!cin.eof() && !cin.bad());

        cin.clear();
    } else {
        ifstream in(filename);
        if (!in) {
            if (option_verbose) {
                int err = errno;
                cerr << "Warning: failed to open " << filename << ":"
                     << strerror(err) << endl;
            }

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
    if (default_editor != nullptr) {
        return default_editor;
    }

    if (option_verbose) {
        cerr << "Warning: Unable to detect default editor" << endl;
    }

    return "nano";
}

static option options[] = {{"help", no_argument, 0, 'h'},
                           {"version", no_argument, 0, 'V'},
                           {"ignore-fail", no_argument, 0, 's'},
                           {"verbose", no_argument, 0, 'v'},
                           {0, 0, 0, 0}};

static void print_version() { cout << "hereed 1.0" << endl; }

static void print_help() {
    cout << "usage: hereed [OPTION]... [--] [FILE]..." << endl;
    cout << "if FILE is `-', this reads from stdin." << endl << endl;
    cout << "options:" << endl;
    cout << " -s, --ignore-fail success even if editor exited with fail"
         << endl;
    cout << " -v, --verbose     print verbose output" << endl;
    cout << "     --help        print this help and exit" << endl;
    cout << "     --version     print version and exit" << endl << endl;
    ;
    cout << "source code repository: https://github.com/kofuk/hereed" << endl;
}

int main(int argc, char **argv) {
    string editor = find_editor();

    int opt_char;
    for (;;) {
        opt_char = getopt_long(argc, argv, "vs", options, nullptr);

        if (opt_char == -1) break;

        switch (opt_char) {
        case 'h':
            print_help();
            return 0;

        case 's':
            option_ignore_fail = true;
            break;

        case 'v':
            option_verbose = true;
            break;

        case 'V':
            print_version();
            return 0;

        default:
            print_help();
            return 1;
        }
    }

    filesystem::path tmp_base = "/tmp";
    filesystem::path tmp;
    do {
        tmp = tmp_base / ("hereed_" + to_string(rand()));

    } while (filesystem::exists(tmp));

    {
        ofstream tmp_out(tmp.string());

        for (argv += optind; *argv; ++argv) {
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

    if (option_verbose) {
        cerr << "Waiting for your editor to finish" << endl;
    }

    int status;
    waitpid(pid, &status, 0);

    if (option_ignore_fail && status != 0) {
        if (option_verbose) {
            cerr << "Editor process exited with " + to_string(status) +
                        "; exiting hereed with failure."
                 << endl;
        }

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
