/* Copyright (C) 2015-2021, Wazuh Inc.
 * All right reserved.
 *
 * This program is free software; you can redistribute it
 * and/or modify it under the terms of the GNU General Public
 * License (version 2) as published by the FSF - Free Software
 * Foundation.
 */

#include "active_responses.h"

#define LOCK_PATH "active-response/bin/fw-drop"
#define LOCK_FILE "active-response/bin/fw-drop/pid"
#define DEFAULT_FW_CMD "/bin/firewall-cmd"

int main (int argc, char **argv) {
    (void)argc;
    char rule[COMMANDSIZE];
    char log_msg[LOGSIZE];
    char lock_path[PATH_MAX];
    char lock_pid_path[PATH_MAX];
    char *srcip = NULL;
    int action = OS_INVALID;
    cJSON *input_json = NULL;
    struct utsname uname_buffer;

    action = setup_and_check_message(argv, &input_json);
    if ((action != ADD_COMMAND) && (action != DELETE_COMMAND)) {
        return OS_INVALID;
    }

    // Get srcip
    srcip = get_srcip_from_json(input_json);
    if (!srcip) {
        write_debug_file(argv[0], "Cannot read 'srcip' from data");
        cJSON_Delete(input_json);
        return OS_INVALID;
    }

    if (action == ADD_COMMAND) {
        char **keys = NULL;
        int action2 = OS_INVALID;

        os_calloc(2, sizeof(char *), keys);
        os_strdup(srcip, keys[0]);
        keys[1] = NULL;

        action2 = send_keys_and_check_message(argv, keys);

        os_free(keys);

        // If necessary, abort execution
        if (action2 != CONTINUE_COMMAND) {
            cJSON_Delete(input_json);

            if (action2 == ABORT_COMMAND) {
                write_debug_file(argv[0], "Aborted");
                return OS_SUCCESS;
            } else {
                return OS_INVALID;
            }
        }
    }

    int ip_version = get_ip_version(srcip);
    memset(rule, '\0', COMMANDSIZE);
    if (ip_version == 4) {
        snprintf(rule, COMMANDSIZE -1, "rule family=ipv4 source address=%s drop", srcip);
    } else if (ip_version == 6) {
        snprintf(rule, COMMANDSIZE -1, "rule family=ipv6 source address=%s drop", srcip);
    } else {
        memset(log_msg, '\0', LOGSIZE);
        snprintf(log_msg, LOGSIZE -1, "Unable to run active response (invalid IP: '%s').", srcip);
        write_debug_file(argv[0], log_msg);
        cJSON_Delete(input_json);
        return OS_INVALID;
    }

    if (uname(&uname_buffer) != 0) {
        write_debug_file(argv[0], "Cannot get system name");
        cJSON_Delete(input_json);
        return OS_INVALID;
    }

    if (!strcmp("Linux", uname_buffer.sysname)) {
        char arg1[COMMANDSIZE];
        char fw_cmd[COMMANDSIZE];

        memset(arg1, '\0', COMMANDSIZE);
        if (action == ADD_COMMAND) {
            strcpy(arg1, "--add-rich-rule=");
        } else {
            strcpy(arg1, "--remove-rich-rule=");
        }
        memset(fw_cmd, '\0', COMMANDSIZE);
        strcpy(fw_cmd, DEFAULT_FW_CMD);

        // Checking if firewall-cmd is present
        if (access(fw_cmd, F_OK) < 0) {
            char fw_cmd_path[PATH_MAX];
            memset(fw_cmd_path, '\0', PATH_MAX);
            snprintf(fw_cmd_path, PATH_MAX - 1, "/usr%s", fw_cmd);
            if (access(fw_cmd_path, F_OK) < 0) {
                memset(log_msg, '\0', LOGSIZE);
                snprintf(log_msg, LOGSIZE -1, "The firewall-cmd file '%s' is not accessible: %s (%d)", fw_cmd_path, strerror(errno), errno);
                write_debug_file(argv[0], log_msg);
                cJSON_Delete(input_json);
                return OS_INVALID;
            }
            memset(fw_cmd, '\0', COMMANDSIZE);
            strncpy(fw_cmd, fw_cmd_path, COMMANDSIZE - 1);
        }

        memset(lock_path, '\0', PATH_MAX);
        memset(lock_pid_path, '\0', PATH_MAX);
        snprintf(lock_path, PATH_MAX - 1, "%s", LOCK_PATH);
        snprintf(lock_pid_path, PATH_MAX - 1, "%s", LOCK_FILE);

        // Taking lock
        if (lock(lock_path, lock_pid_path, argv[0], basename(argv[0])) == OS_INVALID) {
            memset(log_msg, '\0', LOGSIZE);
            snprintf(log_msg, LOGSIZE -1, "Unable to take lock. End.");
            write_debug_file(argv[0], log_msg);
            cJSON_Delete(input_json);
            return OS_INVALID;
        }

        int count = 0;
        bool flag = true;
        while (flag) {
            char system_command[LOGSIZE];
            memset(system_command, '\0', LOGSIZE);
            snprintf(system_command, LOGSIZE -1, "%s %s\"%s\"", fw_cmd, arg1, rule);
            if (system(system_command) != 0) {
                count++;
                write_debug_file(argv[0], "Unable to run firewall-cmd");
                sleep(count);

                if (count > 4) {
                    flag = false;
                }
            } else {
                flag = false;
            }
        }
        unlock(lock_path, argv[0]);

    } else {
        write_debug_file(argv[0], "Invalid system");
    }

    write_debug_file(argv[0], "Ended");

    cJSON_Delete(input_json);

    return OS_SUCCESS;
}
