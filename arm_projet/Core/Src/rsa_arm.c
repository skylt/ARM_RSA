#include <rsa_arm.h>
#include "main.h"
#include "mbedtls.h"

#include "base64.h"
#include "bignum.h"
#include "ctr_drbg.h"
#include "hmac_drbg.h"
#include "md_internal.h"
#include "platform.h"
#include "rsa.h"
#include <stdbool.h>
#include <string.h>
#include <time.h>
e_rsa_status rsa_status;


#define B64_SHA_LEN 64
#define PWD_LEN 10
#define SHA_LEN 512

char rsa_passwd[PWD_LEN];
char rsa_b64_sha_buf[B64_SHA_LEN];
char rsa_sha_buf[SHA_LEN];
char* rsa_custom_seed = "jhfbksosijnhneasd28";

mbedtls_rsa_context rsa_ctx;
mbedtls_hmac_drbg_context hmac_ctx;

static void error_led(void) {
	/* blink red led */
	for (size_t i = 0; i < 10; i++) {
		HAL_GPIO_TogglePin(LD3_GPIO_Port, LD3_Pin);
		HAL_Delay(200);
	}

}

static void succes_led(void) {
	/* blink green led */
	for (size_t i = 0; i < 6; i++) {
		HAL_GPIO_TogglePin(LD1_GPIO_Port, LD1_Pin);
		HAL_Delay(1000);
	}
}

void rsa_init(void) {
	mbedtls_hmac_drbg_init(&hmac_ctx);
	mbedtls_rsa_init(&rsa_ctx, MBEDTLS_RSA_PKCS_V15, 0);
	rsa_status = waiting_key_gen;
	rsa_gen_private_key();
}

void rsa_gen_private_key(void) {
	const mbedtls_md_info_t* md_info = mbedtls_md_info_from_type(MBEDTLS_MD_SHA256);

	int ret = mbedtls_hmac_drbg_seed_buf(
			&hmac_ctx,
			md_info,
			(unsigned char *) rsa_custom_seed,
			strlen(rsa_custom_seed)
	);

	if (ret) {
		error_led();
		return;
	}

	ret = mbedtls_rsa_gen_key(
			&rsa_ctx,
			mbedtls_hmac_drbg_random,
			&hmac_ctx,
			1024,
			65537
	);

	if (ret) {
		error_led();
		return;
	}
	rsa_status = waiting_passwd_init;
	succes_led();
}

void rsa_init_passwd(char *passwd) {
	printf("Please enter a 10 character password\r\n");
	scanf("%10s", passwd);
	rsa_status = waiting_sha;
}

void rsa_sign_buffer(char* buffer) {
	char signedSHA[500];

	int ret = mbedtls_rsa_rsassa_pkcs1_v15_sign(
			&rsa_ctx,
			mbedtls_hmac_drbg_random,
			&hmac_ctx,
			MBEDTLS_RSA_PRIVATE,
			MBEDTLS_MD_SHA256,
			SHA_LEN,
			rsa_sha_buf,
			signedSHA
	);

	if (ret) {
		printf("HMAC signature failed\r\n");
		error_led();
		return;
	}

	char b64tosend[500];
	int lenWritten;

	ret = mbedtls_base64_encode(
			b64tosend,
			500,
			&lenWritten,
			signedSHA,
			32
	);

	if (ret) {
		printf("BASE64 sig send failed \r\n");
		error_led();
		return;
	}

	printf(b64tosend);
	succes_led();
}



void rsa_receive_sha(char* sha_buf, char *sha_buf_decoded) {
	char fmt[128];
	sprintf(fmt, "%%%ds", B64_SHA_LEN);
	scanf(fmt, sha_buf);

	int wlen;
	int ret = mbedtls_base64_decode(
			sha_buf_decoded,
			SHA_LEN,
			&wlen,
			sha_buf,
			B64_SHA_LEN
	);

	if (ret) {
		error_led();
		return;
	}

	succes_led();
	rsa_status = ready;
}


static void rsa_printf_mbedtls_mpi(mbedtls_mpi *mpi) {
	char buf[512];
	size_t len = 0;

	mbedtls_mpi_write_string(mpi, 16, &buf, 510, &len);
	printf("##############");
	buf[len] = '\0';
	printf("%s\r\n", buf);
	printf("##############");
}

void rsa_print_public_key(void) {
	rsa_printf_mbedtls_mpi(&rsa_ctx.N);
	rsa_printf_mbedtls_mpi(&rsa_ctx.E);
}


int rsa_check_password(char *passwd) {
	char entered_passwd[16];

	printf("Please enter password: \r\n");
	scanf("%10s", entered_passwd);

	if (strncmp(entered_passwd, passwd, 10)) {
		printf("Failed entering password\r\n");
		return -1;
	}

	printf("Password correct\r\n");
	return 0;
}


void rsa_handler(void) {
	printf("Please enter your choice:\r\n1: Send pubkey\r\n2: Init Passwd\r\n3: Receive SHA\r\n4: Sign SHA");
	int choice = 1;
	scanf("%d", &choice);
	switch(choice)
	{
		case 1: // Send Pubkey
			if (rsa_status == waiting_key_gen) {
				printf("Error: Key not generated, please reboot\r\n");
				error_led();
				return;
			}
			rsa_print_public_key();
			break;

		case 2: // Init passwd
			if (rsa_status != waiting_passwd_init) {
				printf("Cannot change password once initialized \r\n");
				return;
			}
			rsa_init_passwd(rsa_passwd);
			break;

		case 3: // Receive SHA
			if (rsa_status != waiting_sha) {
				printf("Not waiting for sha\r\n");
				return;
			}
			rsa_receive_sha(&rsa_b64_sha_buf, &rsa_sha_buf);
			break;


		case 4: // Get passwd and send signature
			if (rsa_status != ready) {
				printf("SHA not send or password not initialized\r\n");
				return;
			}

			if (!rsa_check_password(rsa_passwd)) {
				return;
			}
			rsa_sign_buffer(&rsa_sha_buf);

			break;

		default:
			printf("This choice is not yet available\r\n");
			break;
	}
}
