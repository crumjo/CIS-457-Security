#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/select.h>
#include <pthread.h>
#include <openssl/conf.h>
#include <openssl/evp.h>
#include <openssl/err.h>
#include <openssl/pem.h>
#include <openssl/rand.h>
#include <openssl/rsa.h>
#include <string.h>

void handleErrors(void);
int rsa_encrypt(unsigned char* in, size_t inlen, EVP_PKEY *key, unsigned char* out);
int rsa_decrypt(unsigned char* in, size_t inlen, EVP_PKEY *key, unsigned char* out);
int encrypt(unsigned char *plaintext, int plaintext_len, unsigned char *key,
            unsigned char *iv, unsigned char *ciphertext);

int decrypt(unsigned char *ciphertext, int ciphertext_len, unsigned char *key,
            unsigned char *iv, unsigned char *plaintext);

struct thread_args
{
	int clientsocket;
	char username[100];
	struct sockaddr_in *clientaddr;
	unsigned char key[32];
};

struct client_list
{
	char username[100];
	unsigned char client_key[32];
};

struct symmetric_key_msg
{
	int encryptedkey_len;
	unsigned char encrypted_key [256];
};

struct std_msg
{
	int ciphertext_len;
	unsigned char ciphertext [5000];
	unsigned char iv[16];
};

struct client_list clist[1024];

int adminpassword = 1234;


void* worker(void* args)
{

	struct thread_args targs;
	memcpy(&targs, args, sizeof(struct thread_args));
	char receive[5000];
	char *command = (char*) malloc(sizeof(char)*100);
	char *text = (char*) malloc(sizeof(char)*100);
	int admin = 0;

	while(strcmp(receive, "/quit\n") != 0)
	{
		struct std_msg smsg;
		strcpy (command, " ");
		strcpy (text, " ");
		if (recv(targs.clientsocket, &smsg, sizeof(struct std_msg), 0) < 1) break;

		decrypt(smsg.ciphertext, smsg.ciphertext_len, targs.key, smsg.iv, (unsigned char*)receive);
		printf("Recveived: %s\n\n", receive);

		strcpy(text, receive); // copy receive in
		if (strchr(text, ' ')  != NULL)
		{
			strcpy(command, strsep(&text, " ")); //get command
		}
		else if (strchr (text, '\n') != NULL)
		{
			strcpy(command, strsep(&text, "\n"));
		}
		if (strchr(text, '\n') != NULL)
		{
			strcpy(text, strsep(&text, "\n"));
		}

		printf("%s : %s\n\n", command, text);
		

		if (strcmp(command, "/admin") == 0)
		{
			unsigned char msg[40];
			if (atoi(text) == adminpassword)
			{
				printf("Admin rights granted to %s\n", targs.username);
				strcpy((char*)msg, "admin rights granted\n");
				struct std_msg omsg;
				RAND_pseudo_bytes(omsg.iv,16);
				omsg.ciphertext_len = encrypt(msg, strlen((char*)msg), targs.key, omsg.iv, omsg.ciphertext);
				send (targs.clientsocket, &omsg, sizeof(struct std_msg), 0);
				admin = 1;
			}
			else
			{
				printf("Admin rights not granted to %s\n", targs.username);
				strcpy((char*)msg, "admin rights denied\n");
				admin = 0;
				struct std_msg omsg;
				RAND_pseudo_bytes(omsg.iv,16);
				omsg.ciphertext_len = encrypt(msg, strlen((char*)msg), targs.key, omsg.iv, omsg.ciphertext);
				send (targs.clientsocket, &omsg, sizeof(struct std_msg), 0);
			}
		}
		else if (strcmp(command, "/w") == 0)
		{
			int flag = 0;
			char *dstusername = (char*) malloc(sizeof(char)*100);
			strcpy (dstusername, strsep(&text, " "));
			for (int i = 0; i < 1024; i++)
			{
				if (strcmp(clist[i].username, dstusername) == 0)
				{
					flag = 1;
					struct std_msg omsg;
					RAND_pseudo_bytes(omsg.iv,16);
					omsg.ciphertext_len = encrypt((unsigned char*)text, strlen(text), clist[i].client_key,
												  omsg.iv, omsg.ciphertext);
					send (i, &omsg, sizeof(struct std_msg), 0);
				}
			}

			if (!flag)
			{
				unsigned char msg [40];
				strcpy((char*) msg, "no such client");
				struct std_msg omsg;
				RAND_pseudo_bytes(omsg.iv,16);
				omsg.ciphertext_len = encrypt((unsigned char*)msg, strlen((char*)msg), targs.key,
												omsg.iv, omsg.ciphertext);
				send (targs.clientsocket, &omsg, sizeof(struct std_msg), 0);
			}
			free(dstusername);
		}
		else if (strcmp(command, "/broadcast") == 0)
		{
			for (int i = 0; i < 1024; i++)
			{
				if (strcmp(clist[i].username, "") != 0 && i != targs.clientsocket)
				{
					struct std_msg omsg;
					RAND_pseudo_bytes(omsg.iv,16);
					omsg.ciphertext_len = encrypt((unsigned char*)text, strlen(text), clist[i].client_key,
													omsg.iv, omsg.ciphertext);
					send(i, &omsg, sizeof(struct std_msg), 0);
				}
			}
		}
		else if (strcmp(command, "/kick") == 0)
		{
			char msg[1000];

			if (admin)
			{
				for (int i = 0; i < 1024; i++)
				{
					if (strcmp(clist[i].username, text) == 0)
					{
						printf("Kicking %s\n", clist[i].username);
						strcpy(msg, "kicked\n");
						strcpy(clist[i].username, "");
						struct std_msg omsg;
						RAND_pseudo_bytes(omsg.iv,16);
						omsg.ciphertext_len = encrypt((unsigned char*)msg, strlen(msg), targs.key,
														omsg.iv, omsg.ciphertext);
						send(i, &omsg, sizeof(struct std_msg), 0);
					}
				}
			}
			strcpy(msg, text);
			strcat(msg, " was kicked\n");
			struct std_msg omsg;
			RAND_pseudo_bytes(omsg.iv,16);
			omsg.ciphertext_len = encrypt((unsigned char*)msg, strlen(msg), targs.key,
											omsg.iv, omsg.ciphertext);
			send (targs.clientsocket, &omsg, sizeof(struct std_msg), 0);
		}
		else if (strcmp(command, "/clientlist") == 0)
		{
			printf("Compiling client list\n");
			char msg [1024*100];
			for (int i = 0; i < 1024; i++)
			{
				if (strcmp(clist[i].username, "") != 0 && strlen(clist[i].username)>1)
				{
					strcat (msg, clist[i].username);
					strcat (msg, "\n");
				}
			}
			printf("Compiled List:\n%s", msg);
			struct std_msg omsg;
			RAND_pseudo_bytes(omsg.iv,16);
			omsg.ciphertext_len = encrypt((unsigned char*)msg, strlen(msg), targs.key,
											omsg.iv, omsg.ciphertext);
			sendto (targs.clientsocket, &omsg, sizeof(struct std_msg), 0,(struct sockaddr*)&targs.clientaddr, sizeof(targs.clientaddr));
		}
	}
	free(command);
	printf("Client %s closed\n", targs.username);
	return 0;
}

int main(int argc, char **argv)
{

	int sockfd = socket(AF_INET,SOCK_STREAM,0);

	int port;

	printf("Welcome to a simple server\n\n");
	printf("Please provde a port number: ");

	scanf("%d", &port);

	struct sockaddr_in serveraddr,clientaddr;
	serveraddr.sin_family=AF_INET;
	serveraddr.sin_port=htons(port);
	serveraddr.sin_addr.s_addr=INADDR_ANY;

	bind(sockfd,(struct sockaddr*)&serveraddr,sizeof(serveraddr));
	listen(sockfd,10);

	int clientsocket;
	char *username = (char*)malloc(sizeof(char)*100);


	for (int i = 0; i < 1024; i++)
	{
		strcpy(clist[i].username, "");
	}

	while(1)
	{

		socklen_t len = sizeof(clientaddr);
		clientsocket = accept(sockfd,(struct sockaddr*)&clientaddr,&len);

		ERR_load_crypto_strings();
		OpenSSL_add_all_algorithms();
		OPENSSL_config(NULL);

		unsigned char buf[5000];
		unsigned char decryptedkey [32];
		struct symmetric_key_msg skmsg;

		FILE* privf = fopen("RSApriv.pem","rb");
		EVP_PKEY *privkey;
  		privkey = PEM_read_PrivateKey(privf,NULL,NULL,NULL);

		recvfrom(clientsocket, &buf, sizeof(buf), 0, (struct sockaddr*)&clientaddr,&len);
		memcpy(&skmsg, buf, sizeof(struct symmetric_key_msg));
		
		int decryptedkey_len = rsa_decrypt(skmsg.encrypted_key,skmsg.encryptedkey_len, privkey, decryptedkey);
		//printf("ENCRYPTED KEY %d: %s\n\n", skmsg.encryptedkey_len,skmsg.encrypted_key);
		//printf("KEY: %s\n\n", decryptedkey);

		//printf("%s %d\n", decryptedkey, decryptedkey_len);

		//Receive client username
		int uniqname = 1;
		do
		{
			uniqname = 1;
			if (recvfrom(clientsocket, username, 100, 0, (struct sockaddr*)&clientaddr,&len) != -1 )
			{
				strcpy(username, strsep(&username, "\n"));
				for (int i = 0; i<1024; i++)
				{
					if (strcmp(clist[i].username, username) == 0)
					{
						uniqname = 0;
						send(i, &uniqname, sizeof(uniqname), 0);
						break;
					}
				}
			}	
			else
				printf("recv error");

			if (uniqname == 1)
			{
				send(clientsocket, &uniqname, sizeof(uniqname), 0);
				strcpy (clist[clientsocket].username, username);
				memcpy (clist[clientsocket].client_key, decryptedkey, decryptedkey_len);
				printf("New Client: %s\n", username);
			}
		} while (uniqname == 0);
		

		struct thread_args *args = malloc(sizeof(struct thread_args));
		memcpy(&args->clientsocket, &clientsocket, sizeof(int));
		memcpy(&args->clientaddr, &clientaddr, sizeof(struct sockaddr_in));
		memcpy(&args->key, decryptedkey, decryptedkey_len);
		strcpy(args->username, username);

		pthread_t tid;
		pthread_create(&tid, NULL, worker, args);
		pthread_detach(tid);
	}

	
}


void handleErrors(void)
{
  ERR_print_errors_fp(stderr);
  abort();
}

int rsa_encrypt(unsigned char* in, size_t inlen, EVP_PKEY *key, unsigned char* out){ 
  EVP_PKEY_CTX *ctx;
  size_t outlen;
  ctx = EVP_PKEY_CTX_new(key, NULL);
  if (!ctx)
    handleErrors();
  if (EVP_PKEY_encrypt_init(ctx) <= 0)
    handleErrors();
  if (EVP_PKEY_CTX_set_rsa_padding(ctx, RSA_PKCS1_OAEP_PADDING) <= 0)
    handleErrors();
  if (EVP_PKEY_encrypt(ctx, NULL, &outlen, in, inlen) <= 0)
    handleErrors();
  if (EVP_PKEY_encrypt(ctx, out, &outlen, in, inlen) <= 0)
    handleErrors();
  return outlen;
}

int rsa_decrypt(unsigned char* in, size_t inlen, EVP_PKEY *key, unsigned char* out){ 
  EVP_PKEY_CTX *ctx;
  size_t outlen;
  ctx = EVP_PKEY_CTX_new(key,NULL);
  if (!ctx)
    handleErrors();
  if (EVP_PKEY_decrypt_init(ctx) <= 0)
    handleErrors();
  if (EVP_PKEY_CTX_set_rsa_padding(ctx, RSA_PKCS1_OAEP_PADDING) <= 0)
    handleErrors();
  if (EVP_PKEY_decrypt(ctx, NULL, &outlen, in, inlen) <= 0)
    handleErrors();
  if (EVP_PKEY_decrypt(ctx, out, &outlen, in, inlen) <= 0)
    handleErrors();
  return outlen;
}

int encrypt(unsigned char *plaintext, int plaintext_len, unsigned char *key,
	unsigned char *iv, unsigned char *ciphertext){
  EVP_CIPHER_CTX *ctx;
  int len;
  int ciphertext_len;
  if(!(ctx = EVP_CIPHER_CTX_new())) handleErrors();
  if(1 != EVP_EncryptInit_ex(ctx, EVP_aes_256_cbc(), NULL, key, iv))
    handleErrors();
  if(1 != EVP_EncryptUpdate(ctx, ciphertext, &len, plaintext, plaintext_len))
    handleErrors();
  ciphertext_len = len;
  if(1 != EVP_EncryptFinal_ex(ctx, ciphertext + len, &len)) handleErrors();
  ciphertext_len += len;
  EVP_CIPHER_CTX_free(ctx);
  return ciphertext_len;
}

int decrypt(unsigned char *ciphertext, int ciphertext_len, unsigned char *key,
	    unsigned char *iv, unsigned char *plaintext){
  EVP_CIPHER_CTX *ctx;
  int len;
  int plaintext_len;
  if(!(ctx = EVP_CIPHER_CTX_new())) handleErrors();
  if(1 != EVP_DecryptInit_ex(ctx, EVP_aes_256_cbc(), NULL, key, iv))
    handleErrors();
  if(1 != EVP_DecryptUpdate(ctx, plaintext, &len, ciphertext, ciphertext_len))
    handleErrors();
  plaintext_len = len;
  if(1 != EVP_DecryptFinal_ex(ctx, plaintext + len, &len)) handleErrors();
  plaintext_len += len;
  EVP_CIPHER_CTX_free(ctx);
  return plaintext_len;
}