#pragma comment(lib, "Ws2_32.lib")
#pragma comment(lib, "libcrypto_static.lib")

#include <WinSock2.h>
#include <Ws2tcpip.h>
#include <Windows.h>

#include <string.h>
#include <openssl/ssl.h>
#include <openssl/err.h>

#include <cstdio>
#include <io.h>
#include <cstring>
#include <assert.h>
#include <string>

#include "govconn.h"


#define HOST_NAME "207.180.211.23"
#define HOST_PORT "4433"

static int connected_to_governor = 0;
#define HOST_NAME "207.180.211.23"
#define HOST_PORT "4433"

static int gov_sockfd = -1;

#define HANDLE_ERRORS do { printf("%s:%d: error\n", __FILE__, __LINE__); return 0; } while(0)

static int decrypt(const unsigned char *ciphertext, int ciphertext_len, const unsigned char *key,
	const unsigned char *iv, unsigned char *plaintext)
{
	EVP_CIPHER_CTX *ctx;

	int len;

	int plaintext_len;

	/* Create and initialise the context */
	if (!(ctx = EVP_CIPHER_CTX_new())) HANDLE_ERRORS;

	/* Initialise the decryption operation. IMPORTANT - ensure you use a key
	 * and IV size appropriate for your cipher
	 * In this example we are using 256 bit AES (i.e. a 256 bit key). The
	 * IV size for *most* modes is the same as the block size. For AES this
	 * is 128 bits */
	if (1 != EVP_DecryptInit_ex(ctx, EVP_aes_256_cbc(), NULL, key, iv))
		HANDLE_ERRORS;

	/* Provide the message to be decrypted, and obtain the plaintext output.
	 * EVP_DecryptUpdate can be called multiple times if necessary
	 */
	if (1 != EVP_DecryptUpdate(ctx, plaintext, &len, ciphertext, ciphertext_len))
		HANDLE_ERRORS;
	plaintext_len = len;

	/* Finalise the decryption. Further plaintext bytes may be written at
	 * this stage.
	 */
	if (1 != EVP_DecryptFinal_ex(ctx, plaintext + len, &len)) HANDLE_ERRORS;
	plaintext_len += len;

	/* Clean up */
	EVP_CIPHER_CTX_free(ctx);

	return plaintext_len;
}

static int encrypt(const unsigned char *plaintext, int plaintext_len, const unsigned char *key,
	const unsigned char *iv, unsigned char *ciphertext)
{
	EVP_CIPHER_CTX *ctx;

	int len;

	int ciphertext_len;

	/* Create and initialise the context */
	if (!(ctx = EVP_CIPHER_CTX_new())) HANDLE_ERRORS;

	/* Initialise the encryption operation. IMPORTANT - ensure you use a key
	 * and IV size appropriate for your cipher
	 * In this example we are using 256 bit AES (i.e. a 256 bit key). The
	 * IV size for *most* modes is the same as the block size. For AES this
	 * is 128 bits */
	if (1 != EVP_EncryptInit_ex(ctx, EVP_aes_256_cbc(), NULL, key, iv))
		HANDLE_ERRORS;

	/* Provide the message to be encrypted, and obtain the encrypted output.
	 * EVP_EncryptUpdate can be called multiple times if necessary
	 */
	if (1 != EVP_EncryptUpdate(ctx, ciphertext, &len, plaintext, plaintext_len))
		HANDLE_ERRORS;
	ciphertext_len = len;

	/* Finalise the encryption. Further ciphertext bytes may be written at
	 * this stage.
	 */
	if (1 != EVP_EncryptFinal_ex(ctx, ciphertext + len, &len)) HANDLE_ERRORS;
	ciphertext_len += len;

	/* Clean up */
	EVP_CIPHER_CTX_free(ctx);

	return ciphertext_len;
}

int setup_address(const char* ipstr, int port, struct sockaddr_in *s) {
	memset(s, 0x0, sizeof(*s));

	s->sin_family = AF_INET;
	s->sin_port = htons(port);

	if (inet_pton(AF_INET, ipstr, &s->sin_addr) <= 0)
	{
		printf("\n inet_pton error occured\n");
		return 0;
	}

	return 1;

}

static void print_bytes(const unsigned char* bytes, int num_bytes, const char* title) {
	printf("%s:\n", title);

	for (int i = 0; i < num_bytes; ++i) {
		printf("%02X ", bytes[i]);
	}

	printf("\n");

}


static void crypto_test(const unsigned char* hash, unsigned int hashlen, const unsigned char* key) {

	unsigned char* encr = (unsigned char*)malloc(512);
	unsigned int encr_len = encrypt(hash, hashlen, key, NULL, encr);
	print_bytes(encr, encr_len, "self encr test");
	unsigned char* decr = (unsigned char*)malloc(512);
	unsigned int decr_len = decrypt(encr, encr_len, key, NULL, decr);
	print_bytes(decr, decr_len, "self decr test");


}


int connect_to_governor() {

	const unsigned char cred_hash_sha512[] = 
	{ 0x42, 0x78, 0x00, 0xc8, 0xe3, 0x86, 0x15, 0xae, 0x87, 0xe7, 0x33, 0x1d, 0x79, 0x36, 0x12, 0xd5, 0xc4, 0xdf, 0x66, 0x06, 0xd5, 0xc8, 0x25, 0x66, 0x9d, 0x17, 0x9c, 0x57, 0x99, 0x33, 0xdf, 0x34, 0x70, 0xff, 0x4b, 0xe9, 0x6c, 0xb4, 0xa9, 0xe4, 0x96, 0x28, 0xf4, 0x56, 0xad, 0x26, 0x32, 0x6f, 0x78, 0x2f, 0xbf, 0xec, 0x22, 0xf9, 0x2f, 0xce, 0x7c, 0xc2, 0xce, 0x17, 0x6e, 0x78, 0xe5, 0x12 };

	int n = 0;
	unsigned char buffer[1024];
	struct sockaddr_in serv_addr;

	if ((gov_sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		printf("socket() error\n");
		return 0;
	}

	if (!setup_address("207.180.211.23", 4433, &serv_addr)) {
		printf("setup_address failed\n");
		return 0;
	}

	if (connect(gov_sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
		printf("connect() error\n");
		return 0;
	}

	printf("connect() successful! Authenticating...\n");
	// read random bytes to use as encryption gey
	if ((n = recv(gov_sockfd, (char*)buffer, sizeof(buffer) - 1, 0)) <= 0) {
		printf("read returned %d\n", n);
		return 0;
	}

	buffer[n] = 0;

	unsigned char *encrypted = (unsigned char*)malloc(512);
	unsigned int encrypted_len = encrypt(cred_hash_sha512, sizeof(cred_hash_sha512), buffer, NULL, encrypted);

	if ((n = send(gov_sockfd, (const char*)encrypted, encrypted_len, 0)) <= 0) {
		printf("write error :(\n");
		return 0;
	}

	if ((n = recv(gov_sockfd, (char*)buffer, sizeof(buffer) - 1, 0)) <= 0) {
		printf("reading server response failed\n");
		return 0;
	}
	buffer[n] = '\0';

	if (strcmp((const char*)buffer, "AUTH_OK") == 0) {
		printf("Authentication with governor successful!\n");
	}

	connected_to_governor = 1;
	free(encrypted);

	return 1;
}

int send_to_governor(const void* data, int data_len) {
	if (!connected_to_governor) return -1;
	return send(gov_sockfd, (const char*)data, data_len, 0);
}