#include <fcntl.h>
#include <stdbool.h>
#include <stdint.h>
#include <unistd.h>
#include <vector>
#include <net/if.h>
#include <sys/ioctl.h>
#include <arpa/inet.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <cstdlib>
#include <sys/stat.h>
#include <sstream>
#include <ifaddrs.h>
#include <arpa/inet.h>

#include <wpa_ctrl.h>
#include "lvgl/src/misc/lv_log.h"

#include "wifi.h"
#include "io/wireless.h"
#include "io/gpio.h"

static char reply[2048];

void wifi_wait_ready()
{
#ifdef BUILD_ARM
    if (!get_wireless_power() || !wifi_have_device())
    {
        set_wireless_power(true);
        do
        {
            usleep(100000); // 100ms
        } while (!wifi_have_device());
    }
#endif
}

void wifi_wait_deactivate()
{
#ifdef BUILD_ARM
    if (wifi_is_wpa_supplicant_running())
    {
        wifi_terminate_wpa_supplicant();
    }
    if (get_wireless_power())
    {
        set_wireless_power(false);
    }
    if (wifi_have_device())
    {
        wireless_delete();
        do
        {
            usleep(100000); // 100ms
        } while (wifi_have_device());
    }
#endif
}

bool wifi_have_device()
{
    struct if_nameindex *if_list, *if_entry;

    if_list = if_nameindex();
    if (if_list == NULL)
    {
        perror("if_nameindex");
        return 1;
    }

    int exists = 0;
    for (if_entry = if_list; if_entry->if_index != 0 || if_entry->if_name != NULL; if_entry++)
    {
        if (strcmp(if_entry->if_name, WIFI_NAME) == 0)
        {
            exists = 1;
            break;
        }
    }

    if_freenameindex(if_list);

    if (exists)
    {
        return true;
    }
    else
    {
        return false;
    }
}

bool wifi_is_wpa_supplicant_running()
{
    std::string ctrl_path = WIFI_PATH;
    struct stat buffer;
    return (stat(ctrl_path.c_str(), &buffer) == 0);
}

bool wifi_wpa_start()
{
    return std::system(WIFI_RUN) == 0;
}

void wifi_terminate_wpa_supplicant()
{
    std::system(WIFI_STOP);
}

bool wifi_remove()
{
    wpa_ctrl *ctrl = wpa_ctrl_open(WIFI_PATH);
    if (!ctrl)
    {
        LV_LOG_ERROR("Failed to connect wpa");
        return false;
    }

    size_t len = sizeof(reply);
    reply[2047] = 0;

    int net_id;
    int size;

    int ret = wpa_ctrl_request(ctrl, WIFI_REMOVE_ALL_NETWORK, sizeof(WIFI_REMOVE_ALL_NETWORK), reply, &len, nullptr);
    if (ret != 0)
    {
        LV_LOG_ERROR("Failed to send command");
        goto wifi_error;
    }
    if (!strstr(reply, WIFI_RES_OK))
    {
        LV_LOG_ERROR("Failed to remove network %s", reply);
        goto wifi_error;
    }

    wpa_ctrl_close(ctrl);
    return true;
wifi_error:
    wpa_ctrl_close(ctrl);
    return false;
}

bool wifi_connect(std::string &ssid, std::string &psk)
{
    wpa_ctrl *ctrl = wpa_ctrl_open(WIFI_PATH);
    if (!ctrl)
    {
        LV_LOG_ERROR("Failed to connect wpa");
        return false;
    }

    size_t len = sizeof(reply);
    reply[2047] = 0;

    int net_id;
    int size;

    int ret = wpa_ctrl_request(ctrl, WIFI_REMOVE_ALL_NETWORK, sizeof(WIFI_REMOVE_ALL_NETWORK), reply, &len, nullptr);
    if (ret != 0)
    {
        LV_LOG_ERROR("Failed to send command");
        goto wifi_error;
    }
    if (!strstr(reply, WIFI_RES_OK))
    {
        LV_LOG_ERROR("Failed to remove network %s", reply);
        goto wifi_error;
    }

    len = sizeof(reply);
    ret = wpa_ctrl_request(ctrl, WIFI_ADD_NETWORK, sizeof(WIFI_ADD_NETWORK), reply, &len, nullptr);
    if (ret != 0)
    {
        LV_LOG_ERROR("Failed to send command");
        goto wifi_error;
    }

    reply[len] = '\0';
    net_id = atoi(reply);

    size = snprintf(reply, sizeof(reply), WIFI_SET_NETWORK_SSID, net_id, ssid.c_str());
    len = sizeof(reply);
    ret = wpa_ctrl_request(ctrl, reply, size, reply, &len, nullptr);
    if (ret != 0)
    {
        LV_LOG_ERROR("Failed to send command");
        goto wifi_error;
    }
    if (!strstr(reply, WIFI_RES_OK))
    {
        LV_LOG_ERROR("Failed to set SSID: %s", reply);
        goto wifi_error;
    }

    size = snprintf(reply, sizeof(reply), WIFI_SET_NETWORK_PSK, net_id, psk.c_str());
    len = sizeof(reply);
    ret = wpa_ctrl_request(ctrl, reply, size, reply, &len, nullptr);
    if (ret != 0)
    {
        LV_LOG_ERROR("Failed to send command");
        goto wifi_error;
    }
    if (!strstr(reply, WIFI_RES_OK))
    {
        LV_LOG_ERROR("Failed to set PSK: %s", reply);
        goto wifi_error;
    }

    size = snprintf(reply, sizeof(reply), WIFI_ENABLE_NETWORK, net_id);
    len = sizeof(reply);
    ret = wpa_ctrl_request(ctrl, reply, len, reply, &len, nullptr);
    if (ret != 0)
    {
        LV_LOG_ERROR("Failed to send command");
        goto wifi_error;
    }
    if (!strstr(reply, WIFI_RES_OK))
    {
        LV_LOG_ERROR("Failed to enable network: %s", reply);
        goto wifi_error;
    }

    size = snprintf(reply, sizeof(reply), WIFI_SELECT_NETWORK, net_id);
    len = sizeof(reply);
    ret = wpa_ctrl_request(ctrl, reply, len, reply, &len, nullptr);
    if (ret != 0)
    {
        LV_LOG_ERROR("Failed to send command");
        goto wifi_error;
    }
    if (!strstr(reply, WIFI_RES_OK))
    {
        LV_LOG_ERROR("Failed to select network: %s", reply);
        goto wifi_error;
    }

    wpa_ctrl_close(ctrl);
    return true;

wifi_error:
    wpa_ctrl_close(ctrl);
    return false;
}

bool wifi_scan(std::vector<wifi_item_t> &list)
{
    wpa_ctrl *ctrl = wpa_ctrl_open(WIFI_PATH);
    if (!ctrl)
    {
        LV_LOG_ERROR("Failed to connect wpa");
        return false;
    }

    size_t len;
    reply[2047] = 0;

    for (;;)
    {
        len = sizeof(reply);
        wpa_ctrl_request(ctrl, WIFI_SCAN, sizeof(WIFI_SCAN), reply, &len, nullptr);
        if (strstr(reply, WIFI_RES_OK))
        {
            break;
        }

        usleep(1000000);
    }

    len = sizeof(reply);
    wpa_ctrl_request(ctrl, WIFI_SCAN_RESULTS, sizeof(WIFI_SCAN_RESULTS), reply, &len, nullptr);

    std::istringstream stream(reply);
    std::string line;
    while (std::getline(stream, line))
    {
        if (line.find("bssid") != std::string::npos)
        {
            continue;
        }
        wifi_item_t net;
        std::istringstream iss(line);
        iss >> net.bssid;
        iss.ignore(10, '\t');
        iss >> net.frequency;
        iss.ignore(10, '\t');
        iss >> net.level;
        iss.ignore(3, '\t');
        iss >> net.wpa;
        iss.ignore(3, '\t');
        iss >> net.ssid;
        list.push_back(net);
    }
    wpa_ctrl_close(ctrl);

    return true;
}

bool wifi_get_state(wifi_state *state, std::string &ssid)
{
    wpa_ctrl *ctrl = wpa_ctrl_open(WIFI_PATH);
    if (!ctrl)
    {
        LV_LOG_ERROR("Failed to connect wpa");
        return false;
    }

    size_t len;
    reply[2047] = 0;

    len = sizeof(reply);
    wpa_ctrl_request(ctrl, WIFI_STATUS, sizeof(WIFI_STATUS), reply, &len, nullptr);

    std::istringstream stream(reply);
    std::string line;
    while (std::getline(stream, line))
    {
        if (line.find("wpa_state") != std::string::npos)
        {
            uint32_t pos = line.find("=");
            if (pos == std::string::npos)
            {
                *state = WIFI_STATE_UNKNOW;
            }

            std::string temp = line.substr(pos);
            if (strstr(temp.c_str(), WIFI_DISCONNECTED))
            {
                *state = WIFI_STATE_DISCONNECTED;
            }
            else if (strstr(temp.c_str(), WIFI_SCANNING))
            {
                *state = WIFI_STATE_SCANNING;
            }
            else if (strstr(temp.c_str(), WIFI_COMPLETED))
            {
                *state = WIFI_STATE_COMPLETED;
            }
            break;
        }
        else if (line.find("ssid") != std::string::npos)
        {
            uint32_t pos = line.find("=");
            if (pos != std::string::npos)
            {
                ssid = line.substr(pos + 1);
            }
        }
    }
    wpa_ctrl_close(ctrl);

    return true;
}

bool wifi_get_level(int16_t *level)
{
    *level = 0;
    wpa_ctrl *ctrl = wpa_ctrl_open(WIFI_PATH);
    if (!ctrl)
    {
        LV_LOG_ERROR("Failed to connect wpa");
        return false;
    }

    size_t len;
    reply[2047] = 0;

    len = sizeof(reply);
    wpa_ctrl_request(ctrl, WIFI_STATUS, sizeof(WIFI_STATUS), reply, &len, nullptr);

    std::istringstream stream(reply);
    std::string line;
    bool connect;
    std::string bssid;
    while (std::getline(stream, line))
    {
        if (line.find("bssid=") != std::string::npos)
        {
            uint32_t pos = line.find("=") + 1;
            bssid = line.substr(pos);
        }
        else if (line.find("wpa_state=") != std::string::npos)
        {
            uint32_t pos = line.find("=");
            std::string temp = line.substr(pos);
            if (strstr(temp.c_str(), WIFI_COMPLETED))
            {
                connect = true;
            }
            break;
        }
    }

    wpa_ctrl_close(ctrl);

    if (!connect || bssid.empty())
    {
        return false;
    }

    std::vector<wifi_item_t> list;

    if (!wifi_scan(list))
    {
        return false;
    }

    for (const auto &item : list)
    {
        if (item.bssid == bssid)
        {
            *level = item.level;
            break;
        }
    }

    return true;
}
