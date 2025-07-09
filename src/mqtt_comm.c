#include "include/mqtt_comm.h"
#include "lwip/apps/mqtt.h"
#include "lwip/err.h"
#include <string.h>
#include <stdio.h>

/* ---------- Cliente MQTT global ---------- */
static mqtt_client_t *client;

/* ---------- callback de conexão ---------- */
static void mqtt_connection_cb(mqtt_client_t *c, void *arg,
                               mqtt_connection_status_t status)
{
    if (status == MQTT_CONNECT_ACCEPTED) {
        printf("Conectado ao broker MQTT!\n");
    } else {
        printf("Falha ao conectar, status: %d\n", status);
    }
}

/* ---------- setup / reconexão ------------ */
void mqtt_setup(const char *client_id,
                const char *broker_ip,
                const char *user,
                const char *pass)
{
    ip_addr_t broker;
    if (!ip4addr_aton(broker_ip, &broker)) {
        printf("IP do broker inválido!\n");
        return;
    }

    if (!client) client = mqtt_client_new();

    struct mqtt_connect_client_info_t ci = {0};
    ci.client_id  = client_id;
    ci.keep_alive = 30;
    if (user && *user) ci.client_user = user;
    if (pass && *pass) ci.client_pass = pass;

    mqtt_client_connect(client, &broker, MQTT_PORT,
                        mqtt_connection_cb, NULL, &ci);
}

/* ---------- publish (agora retorna bool) ---------- */
bool mqtt_comm_publish(const char *topic,
                       const uint8_t *data,
                       size_t len)
{
    if (!mqtt_client_is_connected(client))
        return false;

    err_t err = mqtt_publish(client,
                             topic,
                             data,
                             len,
                             0,   /* QoS 0 */
                             0,   /* retain */
                             NULL, NULL);

    return err == ERR_OK;
}

/* ---------- subscribe ---------- */
static mqtt_message_callback_t user_cb = NULL;

static void inpub_topic_cb(void *arg, const char *topic, u32_t tot_len) {
    /* nada a fazer – só capturamos no data_cb */
}
static void inpub_data_cb(void *arg, const u8_t *data,
                          u16_t len, u8_t flags)
{
    if ((flags & MQTT_DATA_FLAG_LAST) && user_cb) {
        user_cb((const char *)arg, data, len);
    }
}

void mqtt_comm_subscribe(const char *topic,
                         mqtt_message_callback_t callback)
{
    user_cb = callback;

    err_t err = mqtt_subscribe(client, topic, 0, NULL, NULL);
    if (err != ERR_OK) {
        printf("Erro ao inscrever em %s: %d\n", topic, err);
        return;
    }

    mqtt_set_inpub_callback(client,
                            inpub_topic_cb,
                            inpub_data_cb,
                            (void *)topic);
}
