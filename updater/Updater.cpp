
#include <string>
#include <filesystem>
#include <iostream>
#include <CLI/CLI.hpp>

#include <Windows.h>
#include <tlhelp32.h>

static bool waitForProcessExit(DWORD pid)
{
    HANDLE process =
        OpenProcess(SYNCHRONIZE,
            FALSE,
            pid);

    if (!process)
    {
        return false;
    }

    WaitForSingleObject(process, INFINITE);

    CloseHandle(process);
    return true;
}

static int updateProgram(const std::string& program_path, const std::string& update_path)
{
    namespace fs = std::filesystem;

    try
    {
        std::cout << "Updating program..." << std::endl;

        fs::path programRoot(program_path);
        fs::path updateRoot(update_path);

        if (!fs::exists(programRoot))
        {
            std::cerr << "Program directory does not exist: "
                << programRoot << std::endl;
            return 1;
        }

        if (!fs::exists(updateRoot))
        {
            std::cerr << "Update directory does not exist: "
                << updateRoot << std::endl;
            return 1;
        }

        // Copiar todos los archivos/directorios de la actualización
        for (const auto& entry : fs::recursive_directory_iterator(updateRoot))
        {
            fs::path relative = fs::relative(entry.path(), updateRoot);
            fs::path destination = programRoot / relative;

            if (entry.is_directory())
            {
                fs::create_directories(destination);
            }
            else if (entry.is_regular_file())
            {
                fs::create_directories(destination.parent_path());

                fs::copy_file(
                    entry.path(),
                    destination,
                    fs::copy_options::overwrite_existing);
            }
        }

        // Opcional: eliminar carpeta temporal de actualización
        fs::remove_all(updateRoot);

        std::cout << "Update completed." << std::endl;

        // Reabrir programa principal
        fs::path executable = programRoot / "app.exe";

        STARTUPINFOA si{};
        PROCESS_INFORMATION pi{};

        si.cb = sizeof(si);

        if (!CreateProcessA(
            executable.string().c_str(),
            nullptr,
            nullptr,
            nullptr,
            FALSE,
            0,
            nullptr,
            programRoot.string().c_str(),
            &si,
            &pi))
        {
            std::cerr << "Failed to launch program. Error: "
                << GetLastError() << std::endl;
            return 2;
        }

        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);

        return 0;
    }
    catch (const std::exception& e)
    {
        std::cerr << "Update failed: " << e.what() << std::endl;
        return 3;
    }
}

// Usage:
// -pid=Process ID of the program to update
// -program="Path to the root of the program"
// -update="Path to the updated root directory"

int main(int argc, char* argv[])
{
    CLI::App app{ "Overlay Media Controller Updater Tool" };

    DWORD pid;
    std::string program_path;
    std::string update_path;

    app.add_option("--pid", pid)->required();
    app.add_option("--program", program_path)->required();
    app.add_option("--update", update_path)->required();

    // Parse CLI arguments and automatically handle errors / --help
    CLI11_PARSE(app, argc, argv);

    waitForProcessExit(pid);
    return updateProgram(program_path, update_path);
}