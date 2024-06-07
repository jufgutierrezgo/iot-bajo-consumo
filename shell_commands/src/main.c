#include <zephyr/zephyr.h>
#include <zephyr/shell/shell.h>
#include <stdlib.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(shell_example, LOG_LEVEL_INF);

static int cmd_my_command(const struct shell *shell, size_t argc, char **argv)
{
    // Example command implementation
    shell_print(shell, "My command was executed with %d arguments", argc - 1);
    // for (int i = 1; i < argc; i++) {
    //     shell_print(shell, "arg[%d]: %s", i, argv[i]);
    // }
    return 0;
}

SHELL_CMD_REGISTER(my_command, NULL, "Description of my command", cmd_my_command);

void main(void)
{
    // Application main function
    LOG_INF("Shell example started");
}
