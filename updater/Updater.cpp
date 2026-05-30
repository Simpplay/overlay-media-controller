#include <string>
#include <filesystem>
#include <iostream>
#include <fstream>
#include <chrono>
#include <thread>
#include <CLI/CLI.hpp>

#include <Windows.h>
#include <tlhelp32.h>

static void log(const std::string& message)
{
    std::cout << message << std::endl;
    std::ofstream logFile("updater_log.txt", std::ios::app);
    if (logFile.is_open())
    {
        auto now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
        logFile << std::ctime(&now) << ": " << message << std::endl;
    }
}

static bool waitForProcessExit(DWORD pid)
{
    HANDLE process = OpenProcess(SYNCHRONIZE, FALSE, pid);
    if (!process)
    {
        return true; // Assume it already exited
    }

    log("Waiting for process " + std::to_string(pid) + " to exit...");
    WaitForSingleObject(process, 30000); // Wait up to 30 seconds

    DWORD exitCode;
    if (GetExitCodeProcess(process, &exitCode) && exitCode == STILL_ACTIVE)
    {
        log("Process did not exit in time, attempting to terminate...");
        TerminateProcess(process, 1);
    }

    CloseHandle(process);
    return true;
}

static int updateProgram(const std::string& program_path, const std::string& update_path, const std::string& executable_name)
{
    namespace fs = std::filesystem;

    try
    {
        log("Starting update process...");
        log("Program path: " + program_path);
        log("Update path: " + update_path);

        fs::path programRoot(program_path);
        fs::path updateRoot(update_path);

        if (!fs::exists(programRoot))
        {
            log("Error: Program directory does not exist: " + programRoot.string());
            return 1;
        }

        if (!fs::exists(updateRoot))
        {
            log("Error: Update directory does not exist: " + updateRoot.string());
            return 1;
        }

        // Give some time for file locks to be released
        std::this_thread::sleep_for(std::chrono::milliseconds(500));

        // Copy all files/directories from the update
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

                bool success = false;
                for (int i = 0; i < 5; ++i) // Retry up to 5 times
                {
                    std::error_code ec;
                    fs::copy_file(entry.path(), destination, fs::copy_options::overwrite_existing, ec);
                    if (!ec)
                    {
                        success = true;
                        break;
                    }
                    log("Retrying copy to " + destination.string() + " (attempt " + std::to_string(i + 1) + ")...");
                    std::this_thread::sleep_for(std::chrono::milliseconds(500));
                }

                if (!success)
                {
                    log("Error: Failed to copy " + entry.path().string() + " to " + destination.string());
                    // Continue anyway, maybe it's not critical
                }
            }
        }

        log("File copy completed.");

        // Re-launch the main program
        fs::path executable = programRoot / executable_name;
        if (!fs::exists(executable))
        {
            log("Warning: Executable not found at " + executable.string() + ", trying app.exe");
            executable = programRoot / "app.exe";
        }

        if (fs::exists(executable))
        {
            log("Launching " + executable.string());
            STARTUPINFOA si{};
            PROCESS_INFORMATION pi{};
            si.cb = sizeof(si);

            if (!CreateProcessA(
                executable.string().c_str(),
                nullptr,
                nullptr,
                nullptr,
                FALSE,
                DETACHED_PROCESS,
                nullptr,
                programRoot.string().c_str(),
                &si,
                &pi))
            {
                log("Error: Failed to launch program. Error code: " + std::to_string(GetLastError()));
                return 2;
            }

            CloseHandle(pi.hProcess);
            CloseHandle(pi.hThread);
        }
        else
        {
            log("Error: Could not find any executable to launch.");
            return 2;
        }

        return 0;
    }
    catch (const std::exception& e)
    {
        log("Exception during update: " + std::string(e.what()));
        return 3;
    }
}

int main(int argc, char* argv[])
{
    CLI::App app{ "Overlay Media Controller Updater Tool" };

    DWORD pid = 0;
    std::string program_path;
    std::string update_path;
    std::string executable_name = "app.exe";

    app.add_option("--pid", pid)->required();
    app.add_option("--program", program_path)->required();
    app.add_option("--update", update_path)->required();
    app.add_option("--executable", executable_name);

    CLI11_PARSE(app, argc, argv);

    log("Updater started. PID to wait for: " + std::to_string(pid));

    if (pid != 0)
    {
        waitForProcessExit(pid);
    }

    int result = updateProgram(program_path, update_path, executable_name);
    log("Update finished with result: " + std::to_string(result));

    return result;
}