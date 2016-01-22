/*
 * Collection of isolated functions used in different part of banjax
 *
 * Copyright (c) 2013 eQualit.ie under GNU AGPL v3.0 or later
 * 
 * Vmon: June 2013, Initial version
 *       Oct 2013, send_zmq_mess
 */

/* Check if the ATS version is the right version for this plugin
   that is version 2.0 or higher for now
   */
#ifndef UTIL_H
#define UTIL_H

#include <zmq.hpp>
#include <string>
#include <climits>

#include<openssl/aes.h>

enum ZMQ_ERROR {
    CONNECT_ERROR,
    SEND_ERROR
};

unsigned int const c_cipher_block_size = AES_BLOCK_SIZE;
unsigned int const c_cipher_key_size = 32;
unsigned int const c_gcm_tag_size = 16;
unsigned int const c_gcm_iv_size = 12;
unsigned int const c_max_enc_length = INT_MAX;

int check_ts_version();

void send_zmq_mess(zmq::socket_t& zmqsock, const std::string mess, bool more = false);

/**
 * encrypt using AES-CGM-256 and send a message throw zmq socket.
 *
 * This is NOT thread safe you need to use mutex
 * before calling
 * throw exception if it gets into error
 * @param mess: string sent encrypted
 */
void send_zmq_encrypted_message(zmq::socket_t& zmqsock, const std::string mess, uint8_t* encryption_key, bool more = false);

/**
 * Uses AES256 to encrypt the data 
 *
 * @param iv is a buffer of size 12 bytes contatining iv
 * @param key is a buffer 32 bytes as we are using AES256
 * @param ciphertext should be buffer of size planitext_len + 16 - 1 
 * @param tag is a buffer 16 bytes. 
 */
size_t gcm_encrypt(const uint8_t *plaintext, size_t plaintext_len, 
                   const uint8_t *key, const uint8_t *iv,
                   uint8_t *ciphertext, uint8_t *tag);

/**
 * Escape all quotes this is for the reason of logging. then
 * add a quote to the beginnig and the end of the string 
 *
 * @param unprocessed_log_string for which quote being replaced
 * 
 * @return the string enclosed in quotes with all middle quotes escaped
 *
 */
string encapsulate_in_quotes(std::string& unprocessed_log_string);

#endif
