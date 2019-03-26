#pragma once

int connect_to_governor();
int send_to_governor(const void* data, int data_len);
int disconnect_from_governor();