// konpak_cli.cpp -- KonPak command line tool
//
// Usage:
//   konpak create  <output.konpak> <file1> [file2 ...] [--pass <password>]
//   konpak add     <archive.konpak> <file> [--as <path>] [--pass <password>]
//   konpak remove  <archive.konpak> <path> [--pass <password>]
//   konpak extract <archive.konpak> [path] [--out <dir>] [--pass <password>]
//   konpak list    <archive.konpak> [--pass <password>]
//   konpak help

#include "konpak.hpp"
#include <iostream>
#include <string>
#include <vector>
#include <filesystem>
#include <algorithm>

namespace fs = std::filesystem;

static void printHelp() {
    std::cout << R"(
KonPak -- .konpak archive tool

Usage:
  konpak create  <output.konpak> <file> [file2 ...] [--pass <pw>]
      Create a new .konpak from one or more files.
      Files are stored with their filename as the pack path.
      Use --as to override the pack path (only works with one file).

  konpak add     <archive.konpak> <file> [--as <packpath>] [--pass <pw>]
      Add or replace a file in an existing archive.

  konpak remove  <archive.konpak> <packpath> [--pass <pw>]
      Remove a file from an archive by its pack path.

  konpak extract <archive.konpak> [packpath] [--out <dir>] [--pass <pw>]
      Extract one file or all files from an archive.
      If no packpath is given, extracts everything to --out (default: current dir).

  konpak list    <archive.konpak> [--pass <pw>]
      List all files in an archive.

  konpak help
      Show this message.

Notes:
  If --pass is not provided, the tool will prompt for a password interactively.
  Pack paths use forward slashes: sprites/player.png
)" << "\n";
}

static std::string getPassword(const std::vector<std::string>& args) {
    for (size_t i = 0; i + 1 < args.size(); i++)
        if (args[i] == "--pass") return args[i + 1];

    // Interactive prompt (no echo)
    std::cout << "Password: " << std::flush;
    std::string pw;
#ifdef _WIN32
    HANDLE hStdin = GetStdHandle(STD_INPUT_HANDLE);
    DWORD mode;
    GetConsoleMode(hStdin, &mode);
    SetConsoleMode(hStdin, mode & ~ENABLE_ECHO_INPUT);
    std::getline(std::cin, pw);
    SetConsoleMode(hStdin, mode);
#else
    // termios echo disable
    system("stty -echo");
    std::getline(std::cin, pw);
    system("stty echo");
#endif
    std::cout << "\n";
    return pw;
}

static std::string getArg(const std::vector<std::string>& args,
                           const std::string& flag,
                           const std::string& def = "") {
    for (size_t i = 0; i + 1 < args.size(); i++)
        if (args[i] == flag) return args[i + 1];
    return def;
}

static std::string packPathFromDisk(const std::string& diskPath) {
    return fs::path(diskPath).filename().string();
}

// -----------------------------------------------------------------------
// Commands
// -----------------------------------------------------------------------

static int cmdCreate(const std::vector<std::string>& args) {
    if (args.size() < 2) {
        std::cerr << "Usage: konpak create <output.konpak> <file> [file2 ...]\n";
        return 1;
    }
    std::string outPath = args[0];
    std::string pw = getPassword(args);

    KonPak::Pack pack;
    pack.password = pw;

    std::string asPath = getArg(args, "--as");

    for (size_t i = 1; i < args.size(); i++) {
        if (args[i] == "--pass" || args[i] == "--as") { i++; continue; }
        std::string diskPath = args[i];
        std::string packPath = (!asPath.empty() && args.size() == 2)
            ? asPath
            : packPathFromDisk(diskPath);
        try {
            pack.addFile(diskPath, packPath);
            std::cout << "  + " << packPath << "  (" << diskPath << ")\n";
        } catch (std::exception& e) {
            std::cerr << "Error: " << e.what() << "\n";
            return 1;
        }
    }

    try {
        pack.save(outPath);
        std::cout << "\nCreated: " << outPath
                  << "  (" << pack.entries.size() << " file(s))\n";
    } catch (std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
    return 0;
}

static int cmdAdd(const std::vector<std::string>& args) {
    if (args.size() < 2) {
        std::cerr << "Usage: konpak add <archive.konpak> <file> [--as <path>]\n";
        return 1;
    }
    std::string archivePath = args[0];
    std::string diskPath    = args[1];
    std::string packPath    = getArg(args, "--as", packPathFromDisk(diskPath));
    std::string pw          = getPassword(args);

    KonPak::Pack pack;
    pack.password = pw;

    // Load existing if file exists
    if (fs::exists(archivePath)) {
        try {
            pack.load(archivePath);
        } catch (std::exception& e) {
            std::cerr << "Error loading archive: " << e.what() << "\n";
            return 1;
        }
    }

    try {
        pack.addFile(diskPath, packPath);
        pack.save(archivePath);
        std::cout << "Added: " << packPath << " -> " << archivePath << "\n";
    } catch (std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
    return 0;
}

static int cmdRemove(const std::vector<std::string>& args) {
    if (args.size() < 2) {
        std::cerr << "Usage: konpak remove <archive.konpak> <packpath>\n";
        return 1;
    }
    std::string archivePath = args[0];
    std::string packPath    = args[1];
    std::string pw          = getPassword(args);

    KonPak::Pack pack;
    pack.password = pw;
    try {
        pack.load(archivePath);
        if (!pack.remove(packPath)) {
            std::cerr << "Not found in archive: " << packPath << "\n";
            return 1;
        }
        pack.save(archivePath);
        std::cout << "Removed: " << packPath << "\n";
    } catch (std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
    return 0;
}

static int cmdExtract(const std::vector<std::string>& args) {
    if (args.empty()) {
        std::cerr << "Usage: konpak extract <archive.konpak> [packpath] [--out <dir>]\n";
        return 1;
    }
    std::string archivePath = args[0];
    std::string outDir      = getArg(args, "--out", ".");
    std::string pw          = getPassword(args);

    // Optional specific file
    std::string specificFile;
    if (args.size() >= 2 && args[1][0] != '-')
        specificFile = args[1];

    KonPak::Pack pack;
    pack.password = pw;
    try {
        pack.load(archivePath);
    } catch (std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }

    fs::create_directories(outDir);

    auto extractEntry = [&](const KonPak::Entry& e) {
        fs::path outPath = fs::path(outDir) / e.path;
        fs::create_directories(outPath.parent_path());
        pack.extractFile(e.path, outPath.string());
        std::cout << "  -> " << outPath.string() << "\n";
    };

    if (!specificFile.empty()) {
        auto* e = pack.find(specificFile);
        if (!e) {
            std::cerr << "Not found: " << specificFile << "\n";
            return 1;
        }
        extractEntry(*e);
    } else {
        for (auto& e : pack.entries)
            extractEntry(e);
        std::cout << "\nExtracted " << pack.entries.size() << " file(s) to " << outDir << "\n";
    }
    return 0;
}

static int cmdList(const std::vector<std::string>& args) {
    if (args.empty()) {
        std::cerr << "Usage: konpak list <archive.konpak>\n";
        return 1;
    }
    std::string archivePath = args[0];
    std::string pw          = getPassword(args);

    KonPak::Pack pack;
    pack.password = pw;
    try {
        pack.load(archivePath);
    } catch (std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }

    std::cout << archivePath << "  (" << pack.entries.size() << " file(s))\n\n";
    for (auto& e : pack.entries) {
        std::cout << "  " << e.path
                  << "  [" << e.sizeRaw << " bytes raw, "
                  << e.sizePacked << " packed]\n";
    }
    return 0;
}

// -----------------------------------------------------------------------
// main
// -----------------------------------------------------------------------
int main(int argc, char** argv) {
    if (argc < 2) { printHelp(); return 0; }

    std::string cmd = argv[1];
    std::vector<std::string> args;
    for (int i = 2; i < argc; i++) args.push_back(argv[i]);

    if (cmd == "create")  return cmdCreate(args);
    if (cmd == "add")     return cmdAdd(args);
    if (cmd == "remove")  return cmdRemove(args);
    if (cmd == "extract") return cmdExtract(args);
    if (cmd == "list")    return cmdList(args);
    if (cmd == "help")    { printHelp(); return 0; }

    std::cerr << "Unknown command: " << cmd << "\n";
    printHelp();
    return 1;
}
