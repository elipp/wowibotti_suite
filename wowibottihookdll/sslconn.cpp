#pragma comment(lib, "libcrypto_static.lib")
#pragma comment(lib, "libssl_static.lib")

#include <cstdio>
#include <io.h>
#include <cstring>
#include <winsock2.h>
#include <Windows.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/engine.h>
#include <openssl/conf.h>
#include <assert.h>
#include <string>

#include "sslconn.h"

#define HOST_NAME "207.180.211.23"
#define HOST_PORT "4433"


static SSL_CTX* ctx = NULL;
static BIO *web = NULL;
static SSL *ssl = NULL;

static int connected_to_governor = 0;

static void init_openssl_library(void)
{
	(void)SSL_library_init();
	SSL_load_error_strings();
	ERR_load_crypto_strings(); 

}

std::string get_working_directory() {
	char buffer[MAX_PATH];
	GetModuleFileName(NULL, buffer, MAX_PATH);
	std::string::size_type pos = std::string(buffer).find_last_of("\\/");
	return std::string(buffer).substr(0, pos);
}


int setup_SSL() {
	init_openssl_library();

	int res = 0;

	const SSL_METHOD* method = SSLv23_method();
	if (!(NULL != method)) assert(("SSLv23_method", false));

	ctx = SSL_CTX_new(method);
	if (!(ctx != NULL)) assert(("SSL_CTX_new", false));

	/* Cannot fail ??? */
	SSL_CTX_set_verify(ctx, SSL_VERIFY_PEER, NULL);

	/* Cannot fail ??? */
	SSL_CTX_set_verify_depth(ctx, 4);

	/* Cannot fail ??? */
	const long flags = SSL_OP_NO_SSLv2 | SSL_OP_NO_SSLv3 | SSL_OP_NO_COMPRESSION;
	SSL_CTX_set_options(ctx, flags);

	res = SSL_CTX_load_verify_locations(ctx, "TLS\\ca.crt", NULL);
	if (!(1 == res)) assert(("load_verify_locations", false));

	//res = SSL_CTX_use_certificate_chain_file(ctx, "TLS\\client.crt"); // THIS ONE WORKS
																 //    res = SSL_CTX_use_certificate_chain_file(ctx, "ca.crt"); 
	//if (!(1 == res)) assert(("use_cert_chain_file", false));

	//res = SSL_CTX_use_PrivateKey_file(ctx, "TLS\\client.key", SSL_FILETYPE_PEM);
	//if (!(1 == res)) assert(("use PK file", false));

	return 1;
}

int cleanup_SSL() {

	FIPS_mode_set(0);
	CRYPTO_set_locking_callback(nullptr);
	CRYPTO_set_id_callback(nullptr);

	ERR_remove_state(0);

	SSL_COMP_free_compression_methods();

	ENGINE_cleanup();

	CONF_modules_free();
	CONF_modules_unload(1);

	COMP_zlib_cleanup();

	ERR_free_strings();
	EVP_cleanup();

	CRYPTO_cleanup_all_ex_data();
	return 1;
}

int connect_to_governor() {
	
	long res = 1;

	printf("Setting up a TLS connection to governor@%s\n", HOST_NAME ":" HOST_PORT);

	setup_SSL();

	
	web = BIO_new_ssl_connect(ctx);
	if (!(web != NULL)) {
		printf("new_ssl_connect failed\n");
		return 0;
	}

	res = BIO_set_conn_hostname(web, HOST_NAME ":" HOST_PORT);
	if (!(1 == res)) {
		printf("conn hostname failed\n");
		return 0;
	}

	BIO_get_ssl(web, &ssl);
	if (!(ssl != NULL)) {
		printf("get_ssl failed\n");
		return 0;
	}

	const char* const PREFERRED_CIPHERS = "HIGH:!aNULL:!kRSA:!PSK:!SRP:!MD5:!RC4";
	res = SSL_set_cipher_list(ssl, PREFERRED_CIPHERS);
	if (!(1 == res)) {
		printf("set_cipher_list failed\n");
		return 0;
	}

	res = SSL_set_tlsext_host_name(ssl, HOST_NAME);
	if (!(1 == res)) {
		printf("set_tlsext failed\n");
		return 0;
	}

	res = BIO_do_connect(web);
	if (!(1 == res)) {
		printf("do_connect failed\n");
		return 0;
	}

	res = BIO_do_handshake(web);
	if (!(1 == res)) {
		printf("do_handshake failed\n");
		return 0;
	}

	/* Step 1: verify a server certificate was presented during the negotiation */
	X509* cert = SSL_get_peer_certificate(ssl);
	if (cert) { X509_free(cert); } /* Free immediately */
	if (NULL == cert) {
		printf("get_peer_cert failed\n");
		return 0;
	}

	/* Step 2: verify the result of chain verification */
	/* Verification performed according to RFC 4158    */
	res = SSL_get_verify_result(ssl);
	if (!(X509_V_OK == res)) {
		printf("get_verify_result failed\n");
		return 0;
	}

	/* Step 3: hostname verification */
	/* An exercise left to the reader */

	connected_to_governor = 1;

	int len = 0;
	const char *login = "GOVERNOR_REMOTECLIENT:avaruuteen_ni_asemalle";
	len = send_to_governor(login, strlen(login) + 1);
	if (len <= 0) {
		printf("Error: sending login data to governor failed (governor not online?)\n");
		connected_to_governor = 0;
		return 0;
	}

	char buff[1024];

	do {
		buff[0] = '\0';
		len = BIO_read(web, buff, 1024);
		buff[len] = '\0';
		if (len > 0) {
			if (strcmp(buff, "AUTH_OK") == 0) {
				printf("Connection established! (Got AUTH_OK from remote governor)\n");
				break;
			}
			else {
				printf("Authentication with remote governor failed! (got %s)\n", buff);
				connected_to_governor = 0;
				return 0;
			}
		}
		else {
			printf("BIO_read for governor remote failed :(\n");
			connected_to_governor = 0;
			return 0;
		}

	} while (len > 0 || BIO_should_retry(web));

	return 1;
}

int send_to_governor(const void* data, int data_len) {
	if (!connected_to_governor) return 0;

	int rlen = 0;
	do { rlen = BIO_write(web, data, data_len); } while (BIO_should_retry(web));
	
	if (rlen <= 0) {
		printf("sending message to governor failed! Connection error\n");
		connected_to_governor = 0;
		return -1;
	}
	else {
		return rlen;
	}
}

int disconnect_from_governor() {

	if (!connected_to_governor) return 1;

	if (ssl != NULL) 
		SSL_shutdown(ssl);
	
	if (ssl != NULL) 
		SSL_free(ssl);
	
	//if (web != NULL)
	//	BIO_free_all(web);

	cleanup_SSL();
}
