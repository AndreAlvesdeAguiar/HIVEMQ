#ifndef MQTT_COMM_H
#define MQTT_COMM_H

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

/* ---------- API ---------- */

/* Conecta (ou reconecta) ao broker */
void mqtt_setup(const char *client_id,
                const char *broker_ip,
                const char *user,
                const char *pass);

/* Publica em QoS-0
 * @return true  -> sucesso
 *         false -> falhou / cliente desconectado                     */
bool mqtt_comm_publish(const char *topic,
                       const uint8_t *data,
                       size_t len);

/* Tipo de callback para mensagens recebidas */
typedef void (*mqtt_message_callback_t)(const char *topic,
                                        const uint8_t *payload,
                                        size_t len);

/* Inscreve-se em tópico */
void mqtt_comm_subscribe(const char *topic,
                         mqtt_message_callback_t callback);

#endif /* MQTT_COMM_H */
