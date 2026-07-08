#include <SDL.h>
#include <SDL_system.h>

#include <limits.h>
#include <string>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include <windows.h>

int PASCAL WinMain(HINSTANCE instance, HINSTANCE previous, char *command_line, int command_show);

static bool path_exists(char const *path)
{
    struct stat info;
    return path && stat(path, &info) == 0;
}

static void select_resource_root(void)
{
    static char const *config_path = "assets/redalert/allies/INSTALL/REDALERT.INI";
    if (path_exists(config_path)) {
        return;
    }

    char const *internal_path = SDL_AndroidGetInternalStoragePath();
    if (!internal_path || !internal_path[0]) {
        return;
    }

    std::string resource_root = std::string(internal_path) + "/redalert-root";
    if (chdir(resource_root.c_str()) != 0) {
        return;
    }
    if (!path_exists(config_path)) {
        chdir(internal_path);
    }
}

extern "C" int SDL_main(int argc, char **argv)
{
    select_resource_root();

    std::string command_line;
    for (int index = 1; index < argc; ++index) {
        if (!command_line.empty()) {
            command_line += ' ';
        }
        command_line += argv[index];
    }

    return WinMain((HINSTANCE)0, (HINSTANCE)0, command_line.empty() ? (char *)"" : (char *)command_line.c_str(), SW_RESTORE);
}
