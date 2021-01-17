#ifndef RSA_H_
#define RSA_H_

typedef enum {
	waiting_key_gen,
	waiting_passwd_init,
	waiting_sha,
	ready,
} e_rsa_status;

extern e_rsa_status rsa_status;

void rsa_init(void);
void rsa_handler(void);
void rsa_gen_private_key(void);

#endif
