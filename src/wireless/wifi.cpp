#include "wifi.h"

#include "../lvgl/src/misc/lv_log.h"

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
#include <wpa_ctrl.h>
#include <sys/stat.h>
#include <sstream>
#include <ifaddrs.h>
#include <arpa/inet.h>

static char reply[2048];

#if BUILD_ARM
static const char *wifi_open = "1";
static const char *wifi_close = "0";

bool get_wifi_power()
{
    uint8_t temp[2];
    int fd = open(WIFI_POWER, O_RDONLY);
    read(fd, temp, 2);
    close(fd);

    return temp[0] == '1';
}

void set_wifi_power(bool enable)
{
    int fd = open(WIFI_POWER, O_WRONLY);
    write(fd, enable ? wifi_open : wifi_close, 2);
    close(fd);
}
#endif

bool have_wifi_device()
{
    struct sockaddr_in *sin = NULL;
    struct ifaddrs *ifa = NULL, *ifList;

    bool find = false;

    if (getifaddrs(&ifList) < 0)
    {
        return false;
    }

    for (ifa = ifList; ifa != NULL; ifa = ifa->ifa_next)
    {
        if (strstr(ifa->ifa_name, WIFI_NAME))
        {
            find = true;
            break;
        }
    }

    freeifaddrs(ifList);

    return find;
}

bool is_wpa_supplicant_running()
{
    std::string ctrl_path = WIFI_PATH;
    struct stat buffer;
    return (stat(ctrl_path.c_str(), &buffer) == 0);
}

bool wifi_wpa_start()
{
    return std::system(WIFI_RUN) == 0;
}

void terminate_wpa_supplicant()
{
    // std::string ctrl_path = "/var/run/wpa_supplicant/" WIFI_NAME;
    // wpa_ctrl *ctrl = wpa_ctrl_open(ctrl_path.c_str());
    // if (ctrl)
    // {
    //     char reply[128];
    //     size_t len = sizeof(reply);
    //     wpa_ctrl_request(ctrl, "TERMINATE", 9, reply, &len, nullptr);
    //     wpa_ctrl_close(ctrl);
    // }

    std::system(WIFI_STOP);
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

bool wifi_get_state(wifi_state *state)
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

void wifi_test()
{
#if BUILD_ARM
    if (!get_wifi_power())
    {
        set_wifi_power(true);
    }
#endif
    while (!have_wifi_device())
    {
        usleep(500000);
    }

    // terminate_wpa_supplicant();

    // usleep(500000);

    // wifi_wpa_start();

    // usleep(5000000);

    // std::vector<wifi_item_t> list;

    // if (wifi_scan(list))
    // {
    // }

    // std::string wifi = "coloryr";
    // std::string psk = "1234567890";
    // wifi_connect(wifi, psk);

    wifi_state state;
    wifi_get_state(&state);

    int16_t level;
    wifi_get_level(&level);
}