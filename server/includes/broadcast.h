#ifndef BROADCAST_H
#define BROADCAST_H

#include "common.h"

void broadcast_packet(Packet *packet, Client clients[], int client_count, int exclude_sock);

#endif

