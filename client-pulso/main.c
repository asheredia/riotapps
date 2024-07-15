#include <stdio.h>
#include <string.h>
#include "net/af.h"
#include "net/protnum.h"
#include "net/ipv6/addr.h"
#include "net/sock/udp.h"
#include "xtimer.h"
#include "shell.h"
#include "msg.h"
#include "net/gnrc/netif.h"
#include "net/gnrc/ipv6.h"
#include "periph/adc.h"

#define MAIN_QUEUE_SIZE (8)
static msg_t _main_msg_queue[MAIN_QUEUE_SIZE];

#define MY_ADC_LINE        ADC_LINE(0) // Línea ADC (ajustar según tu hardware)
#define ADC_RES            ADC_RES_10BIT // Resolución del ADC
#define SAMPLING_INTERVAL  10 // Intervalo de muestreo en ms
#define THRESHOLD          512 // Umbral para detectar un latido del corazón (ajustar según necesidad)

char buf[128];

void configure_global_ipv6(void) {
    // Interfaz de red
    gnrc_netif_t *netif = gnrc_netif_iter(NULL);

    if (netif == NULL) {
        puts("No network interface found");
        return;
    }

    // Dirección IPv6 global a asignar
    const char *ipv6_addr_str = "2001:db8:0:2:c8c9:a3ff:fe5d:93a8";
    ipv6_addr_t ipv6_addr;
    if (ipv6_addr_from_str(&ipv6_addr, ipv6_addr_str) == NULL) {
        puts("Error: unable to parse IPv6 address");
        return;
    }

    // Asignar dirección IPv6 global a la interfaz
    if (gnrc_netif_ipv6_addr_add(netif, &ipv6_addr, 64, GNRC_NETIF_IPV6_ADDRS_FLAGS_STATE_VALID) < 0) {
        puts("Error: unable to add IPv6 address");
        return;
    }

    puts("IPv6 global address configured");
}

void start_client(void) {
    printf("Prueba de sensor de pulso\n");

    sock_udp_ep_t local = SOCK_IPV6_EP_ANY;
    sock_udp_t sock;

    local.port = 55555; // Puerto del cliente, puede ser cualquier número

    if (sock_udp_create(&sock, &local, NULL, 0) < 0) {
        puts("Error creating UDP sock");
        return;
    }

    int sample;
    int last_sample = 0;
    int pulse_count = 0;
    uint32_t start_time = xtimer_now_usec();

    while (1) {
        // Leer el valor del ADC
        sample = adc_sample(MY_ADC_LINE, ADC_RES);

        // Detectar flancos ascendentes
        if (sample > THRESHOLD && last_sample <= THRESHOLD) {
            pulse_count++;
        }

        // Guardar la muestra actual como la última muestra
        last_sample = sample;

        // Esperar el siguiente intervalo de muestreo
        xtimer_usleep(SAMPLING_INTERVAL * 1000);

        // Calcular el tiempo transcurrido
        uint32_t elapsed_time = xtimer_now_usec() - start_time;

        // Calcular la frecuencia cardíaca cada segundo
        if (elapsed_time >= 1000000) {
            int heart_rate = (pulse_count * 60 * 1000000) / elapsed_time;
            printf("Gomez, Frecuencia cardíaca: %d bpm\n", heart_rate);

            // Crear una cadena con los datos del sensor
            sprintf(buf, "Gomez, pulso cardiaco: %d bpm", heart_rate);

            // Enviar datos al servidor UDP
            sock_udp_ep_t remote = { .family = AF_INET6 };
            remote.port = 55555; // Puerto del servidor
            // Establece la dirección IPv6 del servidor
            ipv6_addr_from_str((ipv6_addr_t *)&remote.addr.ipv6, "2001:db8:0:4:c8c9:a3ff:fe5d:a313");

            if (sock_udp_send(&sock, buf, strlen(buf), &remote) < 0) {
                puts("Error sending sensor data");
                sock_udp_close(&sock);
                return;
            }

            printf("Sensor data sent: %s\n", buf);

            pulse_count = 0;
            start_time = xtimer_now_usec();
        }
    }
}

int main(void) {
    /* Configurar la cola de mensajes para recibir paquetes de red rápidamente */
    msg_init_queue(_main_msg_queue, MAIN_QUEUE_SIZE);
    puts("RIOT network stack example application");

    /* Configurar la red IPv6 global */
    configure_global_ipv6();

    /* Inicializar el ADC */
    if (adc_init(MY_ADC_LINE) < 0) {
        printf("Error inicializando MY_ADC_LINE(%u)\n", MY_ADC_LINE);
        return 1;
    }

    /* Iniciar el cliente UDP */
    start_client();

    /* Iniciar el shell */
    puts("All up, running the shell now");
    char line_buf[SHELL_DEFAULT_BUFSIZE];
    shell_run(NULL, line_buf, SHELL_DEFAULT_BUFSIZE);

    /* Esto nunca debería alcanzarse */
    return 0;
}
