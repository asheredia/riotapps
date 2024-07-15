#include <stdio.h>
#include <string.h>
#include "net/sock/udp.h"
#include "shell.h"
#include "msg.h"

#define MAIN_QUEUE_SIZE     (8)
#define MAX_BUF_SIZE        (128)

uint8_t buf[MAX_BUF_SIZE];

int main(void)
{
    sock_udp_ep_t local = SOCK_IPV6_EP_ANY;
    sock_udp_t sock;
 
    local.port = 55555;
 
    if (sock_udp_create(&sock, &local, NULL, 0) < 0) {
        puts("Error creating UDP sock");
        return 1;
    }
 
    while (1) {
        sock_udp_ep_t remote;
        ssize_t res;
 
        if ((res = sock_udp_recv(&sock, buf, sizeof(buf), SOCK_NO_TIMEOUT, &remote)) >= 0) {
            puts("Received a message");

            // Asegurarse de que el buffer recibido es una cadena terminada en nulo
            if (res < MAX_BUF_SIZE) {
                buf[res] = '\0';
            } else {
                buf[MAX_BUF_SIZE - 1] = '\0';
            }

            // Mostrar el mensaje recibido completo
            printf("Received message: %s\n", buf);

            char *name = strtok((char *)buf, ",");
            char *sensor_data = strtok(NULL, ":");

            if (name && sensor_data) {
                char *heart_rate = strtok(NULL, " ");
                if (heart_rate) {
                    printf("Client name: %s\n", name);
                    printf("Sensor data: Pulso cardiaco: %s bpm\n", heart_rate);
                } else {
                    puts("Error parsing heart rate");
                }
            } else {
                puts("Error parsing message");
            }

        }
    }
    return 0;
}

