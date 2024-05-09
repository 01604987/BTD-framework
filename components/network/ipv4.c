#include "ipv4.h"
#include <sys/socket.h>
#include <netdb.h>            // struct addrinfo
#include <arpa/inet.h>
#include "esp_log.h"

static const char *TAG = "Network";



char rx_buffer[64];
char host_ip[] = HOST_IP_ADDR;
int addr_family = 0;
int ip_protocol = 0;
int sock;
struct sockaddr_in dest_addr;
int conn_err = 0;


void init_network(){
    
    dest_addr.sin_family = AF_INET;
    dest_addr.sin_port = htons(PORT);
    inet_pton(AF_INET, host_ip, &dest_addr.sin_addr);
    addr_family = AF_INET;
    ip_protocol = IPPROTO_IP;

    sock = socket(addr_family, SOCK_STREAM, ip_protocol);
    if (sock < 0) {
        ESP_LOGE(TAG, "Unable to create socket: errno %d", errno);
    }
    ESP_LOGI(TAG, "Socket created");//, connecting to %s:%d", host_ip, PORT);
}

int connect_to_sock(){
    ESP_LOGI(TAG, "Connecting to %s:%d", host_ip, PORT);

    printf("Waiting to connect...\n");

    int err = connect(sock, (struct sockaddr *)&dest_addr, sizeof(dest_addr));
    if (err != 0) {
        ESP_LOGE(TAG, "Socket unable to connect: errno %d", errno);
        conn_err = 1;
        return 0;
    }
    //ESP_LOGI(TAG, "Successfully connected");
    printf("Connected\n");
    conn_err = 0;
    return 1;
}


void send_buf(const void *dataptr, size_t size){
    int err = send(sock, dataptr, size, 0);
    if (err < 0) {
        ESP_LOGE(TAG, "Error occurred during sending: errno %d", errno);
        conn_err = 1;
    }
}

char* recv_buf(){
    int len = recv(sock, rx_buffer, sizeof(rx_buffer) - 1, 0);
    // Error occurred during receiving
    if (len < 0) {
        ESP_LOGE(TAG, "recv failed: errno %d", errno);
        conn_err = 1;
        return "0";
    } else {
        // Data received
        rx_buffer[len] = 0; // Null-terminate whatever we received and treat like a string
        ESP_LOGI(TAG, "Received %d bytes from %s:", len, host_ip);
        ESP_LOGI(TAG, "%s", rx_buffer);
        return rx_buffer;
    }
}

void shutdown_conn(){
    shutdown(sock, 0);
}

void close_sock() {
    close(sock);
}

int check_conn() {
    // host socket closed
    if (sock == -1) {
        return 0;
    // host socket still open
    } else {
        return 1;
    }
}
