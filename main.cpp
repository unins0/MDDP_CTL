#include <iostream>
#include <libusb-1.0/libusb.h>
#include <vector>
#include <cstring>

static const uint16_t MDDP_VID = 0x2fc6;
static const uint16_t MDDP_PID = 0xf06a;

// USB commands
uint8_t GET_ALL[3] = {0xC0, 0xA5, 0xA3};
uint8_t GET_VOLUME[3] = {0xC0, 0xA5, 0xA2};
uint8_t SET_FILTER[3] = {0xC0, 0xA5, 0x01};
uint8_t SET_GAIN[3] = {0xC0, 0xA5, 0x02};
uint8_t SET_VOLUME[3] = {0xC0, 0xA5, 0x04};
uint8_t SET_INDICATOR[3] = {0xC0, 0xA5, 0x06};

// USB protocol constants for Dawn Pro
static constexpr uint8_t REQUEST_TYPE_WRITE = 0x21;
static constexpr uint8_t REQUEST_TYPE_READ = 0xA1;
static constexpr uint8_t REQUEST_ID_WRITE = 0xA0;
static constexpr uint8_t REQUEST_ID_READ = 0xA1;
static constexpr uint16_t REQUEST_VALUE = 0x0000;
static constexpr uint16_t REQUEST_INDEX = 0x09A0;

// data indexes in response buffer
static constexpr size_t VOLUME_IDX = 4;
static constexpr size_t FILTER_IDX = 3;
static constexpr size_t GAIN_IDX = 4;
static constexpr size_t INDICATOR_IDX = 5;
static constexpr size_t DATA_BUFFER_SIZE = 7;

static const uint8_t volumeTable[][2] = {
    {255,0},{200,1},{180,2},{170,3},{160,4},{150,5},{140,6},{130,7},{122,8},{116,9},
    {110,10},{106,11},{102,12},{98,13},{94,14},{90,15},{88,16},{86,17},{84,18},{82,19},
    {80,20},{78,21},{76,22},{74,23},{72,24},{70,25},{68,26},{66,27},{64,28},{62,29},{60,30},
    {58,31},{56,32},{54,33},{52,34},{50,35},{48,36},{46,37},{44,38},{42,39},{40,40},{38,41},
    {36,42},{34,43},{32,44},{30,45},{28,46},{26,47},{24,48},{22,49},{20,50},{18,51},{16,52},
    {14,53},{12,54},{10,55},{8,56},{6,57},{4,58},{2,59},{0,60}
    };

static const char *filterTable[] = {
    "Fast Roll Off Low Latency",
    "Fast Roll Off Phase Compensated",
    "Slow Roll Off Low Latency",
    "Slow Roll Off Phase Compensated",
    "Non Oversampling"
};


std::vector<uint8_t> read(libusb_device_handle *dac, uint8_t *request) {
    std::vector<uint8_t> data(DATA_BUFFER_SIZE, 0);
    auto transfer = libusb_control_transfer(
        dac,
        LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_OTHER,
        REQUEST_ID_WRITE,
        REQUEST_VALUE,
        REQUEST_INDEX,
        request,
        3,
        0
        );
    if (transfer < 0) {
        std::cerr << "Error submitting transfer: " << libusb_error_name(transfer) << std::endl;
    }
    transfer = libusb_control_transfer(
        dac,
        LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_OTHER | LIBUSB_ENDPOINT_IN,
        REQUEST_ID_READ,
        REQUEST_VALUE,
        REQUEST_INDEX,
        &data[0],
        DATA_BUFFER_SIZE,
        0
        );
    if (transfer < 0) {
        std::cerr << "Error submitting transfer: " << libusb_error_name(transfer) << std::endl;
    }
    return data;
}

void write(libusb_device_handle *dac, uint8_t *request) {
    auto transfer = libusb_control_transfer(
            dac,
            LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_OTHER,
            REQUEST_ID_WRITE,
            REQUEST_VALUE,
            REQUEST_INDEX,
            &request[0],
            4,
            0);
    if (transfer < 0) {
        std::cerr << "Error submitting transfer: " << libusb_error_name(transfer) << std::endl;
    }
}

int to_normal(uint8_t raw) {
    for (const auto &entry : volumeTable) {
        if (raw == entry[0]) {
            return entry[1];
        }
    }
    return -1;
}


int get_volume(libusb_device_handle *dac){
    return to_normal(read(dac, GET_VOLUME)[VOLUME_IDX]);
}


uint8_t to_uint8(uint8_t normal) {
    for (const auto &entry : volumeTable) {
        if (normal == entry[1]) {
            return entry[0];
        }
    }
    return -1;
}


const char *get_indicator(libusb_device_handle *dac) {
    int indicator = read(dac, GET_ALL)[INDICATOR_IDX];
    switch (indicator) {
        case 0: return "on";
        case 1: return "Temp off";
        default: return "Off";
    }
}

const char *get_gain(libusb_device_handle *dac) {
    int gain = static_cast<int>(read(dac, GET_ALL)[GAIN_IDX]);
    return (gain == 0) ? "Low" : "High";
}


const char *get_filter(libusb_device_handle *dac) {
    int filter = static_cast<int>(read(dac, GET_ALL)[FILTER_IDX]);
    return filterTable[filter];
}


void set_volume(libusb_device_handle *dac, char *argv){
    uint8_t cmd[4] = {SET_VOLUME[0], SET_VOLUME[1], SET_VOLUME[2], to_uint8((uint8_t)atoi(argv))};
    write(dac, cmd);
}


void set_filter(libusb_device_handle *dac, char *argv){
    uint8_t cmd[4] = {SET_FILTER[0], SET_FILTER[1], SET_FILTER[2], (uint8_t)std::atoi(argv)};
    write(dac, cmd);
}


void set_gain(libusb_device_handle *dac, char *argv){
    uint8_t cmd[4] = {SET_GAIN[0], SET_GAIN[1], SET_GAIN[2], (uint8_t)std::atoi(argv)};
    write(dac, cmd);
}


void set_indicator(libusb_device_handle *dac, char *argv){
    uint8_t cmd[4] = {SET_INDICATOR[0], SET_INDICATOR[1], SET_INDICATOR[2], (uint8_t)std::atoi(argv)};
    write(dac, cmd);
}

int main(int argc, char *argv[]) {
    int status = libusb_init(NULL);
    if (status != 0) {
        std::cerr << "Error cold't intit libusb" << std::endl;
        return -status;
    }
    //libusb_set_debug(NULL, LIBUSB_LOG_LEVEL_DEBUG);
    libusb_device_handle *dac = libusb_open_device_with_vid_pid(NULL, MDDP_VID, MDDP_PID);
    if (!dac) {
        std::cerr << "dac not connected" << std::endl;
        return -1;
    }
    if (strcmp(argv[1], "get") == 0){
        if (strcmp(argv[2], "status") == 0){
            std::vector<uint8_t> data = read(dac, GET_ALL);
            std::cout << "Volume: " << get_volume(dac) << std::endl;
            std::cout << "Filter: " << filterTable[data[FILTER_IDX]] << std::endl;
            if (data[GAIN_IDX] == 0) { std::cout << "Gain: " << "Low" << std::endl; }
            else { std::cout << "Gain: " << "High" << std::endl; };
            std::cout << "Indicator: " << get_indicator(dac) << std::endl;
        } else if (strcmp(argv[2], "volume") == 0) {
            std::cout << get_volume(dac) << std::endl;
        } else if (strcmp(argv[2], "filter") == 0) {
            std::cout << get_filter(dac) << std::endl;
        }else if (strcmp(argv[2], "gain") == 0) {
            std::cout << get_gain(dac) << std::endl;
        } else if (strcmp(argv[2], "indicator") == 0) {
            std::cout << get_indicator(dac) << std::endl;
        } else {
            std::cerr << "Invalid command usage: get <status|volume|filter|gain|indicator>" << std::endl;
        }
    } else if (strcmp(argv[1], "set") == 0){
        if (strcmp(argv[2], "volume") == 0){
            set_volume(dac,argv[3]);
        } else if (strcmp(argv[2], "filter") == 0){
            set_filter(dac,argv[3]);
        } else if (strcmp(argv[2], "gain") == 0){
            set_gain(dac, argv[3]);
        } else if (strcmp(argv[2], "indicator") == 0){
            set_indicator(dac, argv[3]);
        }
    } else {
        std::cout << argv[1] << std::endl;
        std::cerr << "Invalid command usage: get <status|volume|filter|gain|indicator>" << std::endl;
    }
    libusb_close(dac);
    libusb_exit(NULL);
    return 0;
}
