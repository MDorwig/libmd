#include "daksbase.h"
#include "socket.h"
#include <openssl/bio.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/rand.h>
#include <openssl/objects.h>
#include <openssl/x509.h>
#include <openssl/x509v3.h>

#ifndef SSL_FOLDER
	#define SSL_FOLDER "ssl/"
#endif

#define CA_LIST					SSL_FOLDER"ca-bundle.crt"
#define CERTS_LOCAL			SSL_FOLDER"local_certs/"
#define CLIENT_CERTFILE SSL_FOLDER"client.crt"
#define CLIENT_KEYFILE	SSL_FOLDER"client.key"
#define SERVER_CERTFILE SSL_FOLDER"server.crt"
#define SERVER_KEYFILE	SSL_FOLDER"server.key"
#define SSL_DHFILE			SSL_FOLDER"dh1024.pem"
#define SSL_RANDOMFILE	SSL_FOLDER"random.pem"
#define DAKS_CA         SSL_FOLDER"daks_ca/daks_ca.crt"

#define PASSWORD "tetronikAEN"

static struct
{
	SSL_CTX 				*	client;
	SSL_CTX 				*	client_non;
	SSL_CTX 				*	server;
	X509_STORE    	*	cert_store;
	X509_STORE_CTX 	*	cert_ctx;
}ssl_context;

static int err_cb(const char *str, size_t len, void *u);
static int check_cert(SSL * ssl, ssl_verify & verify);

static int password_cb(char *buf,int num,  int rwflag,void *userdata)
{
  if(num < (int)strlen(PASSWORD)+1)
    return(0);

  strcpy(buf,PASSWORD);

  return(strlen(PASSWORD));
}

class ssl_connection
{
public:
	ssl_connection(CSocket * s)
	{
		skt = s ;
		ctx = NULL;
		ssl = NULL;
		bio = NULL;

	}

	virtual ~ssl_connection()
	{
		if (ssl)
		{
			SSL_shutdown(ssl);
			SSL_free(ssl);
		}
	}

	CSocket * skt;

	SSL_CTX *ctx;
	SSL			*ssl;
	BIO			*bio;

	virtual int handleHsResult(int r)
	{
		int error;

		switch(SSL_get_error(ssl,r))
		{
			case SSL_ERROR_NONE					:
				error = 0;
			break;

			case SSL_ERROR_ZERO_RETURN	:
				error = ECONNABORTED;
			break;

			case SSL_ERROR_WANT_READ		:
				error = EWOULDBLOCK;
			break;

			case SSL_ERROR_WANT_WRITE		:
				error = EWOULDBLOCK;
			break;

			case SSL_ERROR_SYSCALL			:
				if(SSL_want_read(ssl) || SSL_want_write(ssl))
					error = EWOULDBLOCK;
				else
					error = EPIPE;
			break;

			default:
				ERR_print_errors_cb(err_cb, this);
				error = EPIPE;
			break;
		}
		return(error);
	}
	virtual int handleHandshake(ssl_verify & verify) = 0;
};

class ssl_client_connection : public ssl_connection
{
public:

	typedef enum
	{
		onConnect,
		do_handshake,
		check,
		ready,
		err
	}state_e;

	state_e state;

	ssl_client_connection(CSocket	*s) : ssl_connection(s)
	{
		state = onConnect;
	}
	const char * stateName(state_e st)
	{
		switch(st)
		{
			case	onConnect: return "onConnect";
			case 	do_handshake: return "do_handshake";
			case 	check:	return "check";
			case  ready:  return "ready";
			case	err:		return "err";
		}
		return "???";
	}

	void setState(state_e st)
	{
		state = st ;
		//printf("%s(%s)\n",__FUNCTION__,stateName(st));
	}

	virtual int handleHandshake(ssl_verify & verify)
	{
		int ret = -1;

		do
		{
			switch(state)
			{
				case onConnect:
				{
					int r = SSL_connect(ssl);
					ret = handleHsResult(r);
					setState(do_handshake);
				}
				break;

				case do_handshake:
				{
					int r = SSL_do_handshake(ssl);
					ret = handleHsResult(r);
					if(ret != 0)
						break;
					setState(check);
				}

				case check:
				{
					SSL_set_mode(ssl, SSL_MODE_AUTO_RETRY);
					int c = check_cert(ssl, verify);
					if(c != 0)
					{
						ERR_print_errors_cb(err_cb, this);
					}
					ret = 0;
					setState(ready);
				}
				break;

				case ready:
				{
					if(SSL_is_init_finished(ssl))
						return 0;
					setState(do_handshake);
				}
				break;

				case err:
					ret = EINVAL;
				break;

				default:
				break;
			}
		} while (ret == 0 && !SSL_is_init_finished(ssl));

		if (ret != 0 && ret != EWOULDBLOCK)
		{
			tprintf("could not connect using SSL (%i)\n",ERR_GET_REASON(ERR_peek_last_error()));
			ERR_print_errors_cb(err_cb, this);
			setState(err);
		}
		return(ret);
	}
};

class ssl_server_connection : public ssl_connection
{
public:

	typedef enum
	{
		onAccept,
		do_handshake,
		check,
		ready,
		err
	} state_e;

	state_e state;

	ssl_server_connection(CSocket	* s) : ssl_connection(s)
	{
		state = onAccept;
	}

	virtual int handleHandshake(ssl_verify & verify)
	{
		int ret = -1;

		switch(state)
		{
			case onAccept:
			{
				int r = SSL_accept(ssl);
				ret = handleHsResult(r);
				state = do_handshake;
			}
			break;

			case do_handshake:
			{
				int r = SSL_do_handshake(ssl);
				ret = handleHsResult(r);
				if(ret != 0)
					break;
				state = check;
			}
			break;

			case check:
			{
				SSL_set_mode(ssl, SSL_MODE_AUTO_RETRY);
				int c = check_cert(ssl, verify);

				if(c != 0)
				{
					ERR_print_errors_cb(err_cb, this);
				}
				ret = 0;
				state = ready;
			}
			break;

			case ready:
			{
				if(SSL_is_init_finished(ssl))
					return 0;
				state = do_handshake;
			}
			break;

			case err:
				ret = EINVAL;
			break;

			default:
			break;
		}

		if (ret != 0 && ret != EWOULDBLOCK)
		{
			tprintf("could not connect using SSL (%i)\r\n",ERR_GET_REASON(ERR_peek_last_error()));
			ERR_print_errors_cb(err_cb, this);
			state = err;
		}
		return ret;
	}
};


#define BIO_test_flags(bio,bit) ((bio)->flags & (bit)) != 0

static int raw_sock_bwrite(BIO *b, const char *d, int l)
{
	ssl_connection * c = (ssl_connection*)b->ptr;

	int ret = 0;

	if(!BIO_test_flags(b,BIO_FLAGS_READ))
		BIO_clear_retry_flags(b);
	else
		BIO_clear_flags(b,BIO_FLAGS_WRITE);

	while(l > 0)
	{
		int wr = send(c->skt->m_fd,d, l, 0);

		if(wr > 0)
		{
			d += wr;
			l -= wr;
			ret += wr;
		}
		else
		{
			if (errno == EWOULDBLOCK)
			{
				BIO_set_retry_write( b );
				break;
			}
			else
			{
				ret = -1;
				break;
			}
		}
	}

	return(ret);
}

static int raw_sock_bread(BIO *b, char *d, int l)
{
	ssl_connection * c = (ssl_connection *)b->ptr;

	int ret = 0;

	if(!BIO_test_flags(b,BIO_FLAGS_WRITE))
		BIO_clear_retry_flags(b);
	else
		BIO_clear_flags(b,BIO_FLAGS_READ);

	ret = c->skt->ReadSocket(d,l);
	return ret;
}

static int raw_sock_bputs(BIO *b, const char *)
{
	return(-1);
}

static int raw_sock_bgets(BIO *b, char *, int)
{
	return(-1);
}

static long raw_sock_ctrl(BIO *b, int cmd, long num, void *ptr)
{
	if ( cmd == BIO_CTRL_FLUSH )
	{
		/* The OpenSSL library needs this */
		return 1;
	}

	return(0);
}

static int raw_sock_create(BIO *b)
{
	b->init = 1;
	b->shutdown = 1;
	b->num = 0;
	b->ptr = NULL;
	b->flags = 0;

	return(1);
}
static int raw_sock_destroy(BIO *b)
{
	b->init = 0;
	b->shutdown = 0;
	b->num = 0;
	b->ptr = NULL;
	b->flags = 0;

	return(1);
}

static long raw_sock_callback_ctrl(BIO *b, int, bio_info_cb *i)
{
	return(0);
}

static BIO_METHOD* BIO_raw_sock()
{
	static BIO_METHOD raw_bio =
	{
		BIO_TYPE_SOURCE_SINK,
		"raw_socket",
		raw_sock_bwrite,
		raw_sock_bread,
		raw_sock_bputs,
		raw_sock_bgets,
		raw_sock_ctrl,
		raw_sock_create,
		raw_sock_destroy,
		raw_sock_callback_ctrl
	};

	return &raw_bio;
}

int CSocket::SecureHandshake(ssl_verify & verify)
{
	ssl_connection * c = m_sslcon;

	if (c == NULL)
		return EINVAL;
	int ret = c->handleHandshake(verify);
	return ret;
}

int CSocket::SecureOpen(int with_cert)
{
	ssl_connection * c = new ssl_client_connection(this);

	if(with_cert)
		c->ctx = ssl_context.client;
	else
		c->ctx = ssl_context.client_non;

	c->ssl = SSL_new(c->ctx);
	if(c->ssl == NULL)
	{
		tprintf("could not create a new SSL connection\r\n");
		ERR_print_errors_cb(err_cb, c);

		delete c;
		return EACCES;
	}

	c->bio = BIO_new(BIO_raw_sock());
	if(c->bio == NULL)
	{
		tprintf("could not create a new tfl-connection\r\n");
		ERR_print_errors_cb(err_cb, c);

		delete c;
		return EACCES;
	}

	BIO_set_nbio(c->bio,1);
	SSL_set_mode(c->ssl,SSL_MODE_ENABLE_PARTIAL_WRITE | SSL_MODE_ACCEPT_MOVING_WRITE_BUFFER);

	c->bio->ptr = c;

	SSL_set_bio(c->ssl,c->bio,c->bio);

	SSL_set_verify_depth(c->ssl,10);

	m_sslcon = c;

	return 0;
}


int CSocket::SecureOnAccept(ssl_verify & verify)
{
	ssl_connection * c = new ssl_server_connection(this);

	c->bio = BIO_new(BIO_raw_sock());
	if(c->bio == NULL)
	{
		tprintf("could not create a new tfl-connection\r\n");
		ERR_print_errors_cb(err_cb, c);

		delete c;
		return EACCES;
	}

	BIO_set_nbio(c->bio,1);
	c->bio->ptr = c;

	c->ctx = ssl_context.server;

	c->ssl = SSL_new(c->ctx);
	if(c->ssl == NULL)
	{
		tprintf("could not create a new SSL connection\r\n");
		ERR_print_errors_cb(err_cb, c);

		delete c;
		return EACCES;
	}

	SSL_set_bio(c->ssl,c->bio,c->bio);

	SSL_set_mode(c->ssl,SSL_MODE_ENABLE_PARTIAL_WRITE | SSL_MODE_ACCEPT_MOVING_WRITE_BUFFER);

	m_sslcon = c;
	return 0;
}

int CSocket::SecureOnConnect(ssl_verify & verify)
{
	ssl_connection * c = m_sslcon;

	if (c == NULL)
		return EINVAL;
	return c->handleHandshake(verify);
}

int CSocket::SecureRead(void *data, int maxlen)
{
	ssl_connection * c = m_sslcon;
	if (c == NULL)
	{
		errno = EINVAL;
		return -1 ;
	}

	int ret = 0;

	ssl_verify v;

	while(maxlen > 0)
	{
		int error = c->handleHandshake(v);

		if (error != 0)
		{
			if (ret == 0)
			{
				errno = error ;
				ret = -1 ;
			}
			return ret;
		}
		int rd = SSL_read(c->ssl,data,maxlen);
		error = SSL_get_error(c->ssl,rd);
		switch (error)
		{
			case SSL_ERROR_NONE:
				maxlen -= rd;
				ret += rd;
				data = (char*)data+rd;
				error = 0;
			break;

			case SSL_ERROR_ZERO_RETURN:
			case SSL_ERROR_SYSCALL:
				if (ret == 0)
					ret = rd;
				maxlen = 0;
			break;

			case SSL_ERROR_WANT_WRITE:
			case SSL_ERROR_WANT_READ:
				if (ret == 0)
				{
					errno = EWOULDBLOCK;
					ret = -1 ;
				}
				maxlen = 0;
			break;

			default:
			{
				ERR_print_errors_cb(err_cb, c);
				error = EPIPE;
				maxlen = 0;
				assert(false);//TFLTriggerEvent(c->sockarr, c->sockix, c->need_onRead, c->need_onWrite, 1);
			}
			break;
		}
	}
	return ret;
}

int CSocket::SecureWrite(const void *data, int len)
{
	ssl_connection * c	= m_sslcon;
	if (c == NULL)
	{
		errno = EINVAL;
		return -1;
	}

	int ret = 0;

	ssl_verify v;

	while(len > 0)
	{
		int error = c->handleHandshake(v);

		if (error != 0)
		{
			if (ret == 0)
			{
				errno = error ;
				ret = -1 ;
			}
			return ret;
		}

		int wr = SSL_write(c->ssl,data,len);

		switch(SSL_get_error(c->ssl,wr))
		{
			case SSL_ERROR_NONE:
			{
				len -= wr;
				ret += wr;
				data = (char*)data+wr;
				error = 0;
			}
			break;

			case SSL_ERROR_ZERO_RETURN:
			{
				len = 0;
				errno = ECONNABORTED;
				ret = -1;
			}
			break;

			case SSL_ERROR_WANT_READ:
			case SSL_ERROR_WANT_WRITE:
			{
				if (ret == 0)
				{
					errno = EWOULDBLOCK;
					ret = -1 ;
				}
				len = 0;
			}
			break;

			case SSL_ERROR_SYSCALL:
			{
				ret = -1 ;
				len = 0;
			}
			break;

			default:
			{
				ERR_print_errors_cb(err_cb, c);
				error = EPIPE;
				len = 0;
				assert(false);//TFLTriggerEvent(c->sockarr, c->sockix, c->need_onRead, c->need_onWrite, 1);
			}
			break;
		}
	}
	return ret;
}

void CSocket::SecureClose()
{
	if (m_sslcon != NULL)
	{
		delete m_sslcon;
		m_sslcon = NULL;
	}
}

void CSocket::setCert(ssl_verify *v)
{
	if(p_peer_cert.valid)
		return;

	p_peer_cert.valid = TRUE;

	if (!v->has_cert)
	{
		p_peer_cert.state = no_cert;
	}
	else
	{
		p_peer_cert.state = no_valid_cert;

		if(v->cert_signed)
			p_peer_cert.state = authenticated;
		else if(!v->cert_sane)
			p_peer_cert.state = broken;
		else if(v->cert_revoked)
			p_peer_cert.state = revoked;
		else if(v->cert_outdated)
			p_peer_cert.state = outdated;
		else if(v->cert_rejected)
			p_peer_cert.state = rejected;
		else if(v->cert_self_signed)
			p_peer_cert.state = self_signed;

		strcpy(p_peer_cert.commonName,v->cert_commonName);
		strcpy(p_peer_cert.dnsName,v->cert_DNSname);
		strcpy(p_peer_cert.cipher,v->cipher);
		strcpy(p_peer_cert.error,v->error);

		p_peer_cert.valid_since = v->valid_since;
		p_peer_cert.valid_until = v->valid_until;
	}
	OnSSLConnect(0);
}

void CSocket::OnSSLAccept(int nerr)
{
	OnAccept(nerr);
}

void CSocket::OnSSLConnect(int nerr)
{
	OnConnect(nerr);
}

void CSocket::OnSSLClose(int nerr)
{
	OnClose(nerr);
}

void CSocket::OnSSLReceive(int nerr)
{
	OnReceive(nerr);
}

void CSocket::OnSSLWrite(int nerr)
{
	OnWrite(nerr);
}

BOOL CSocket::SecureInputPending()
{
	char buf[8];
	return SSL_peek(m_sslcon->ssl,buf,sizeof buf);
}

static time_t ASN1_UTCTIME_get(const ASN1_UTCTIME *s)
	{
	struct tm tm;
	int offset;

	memset(&tm,'\0',sizeof tm);

#define g2(p) (((p)[0]-'0')*10+(p)[1]-'0')
	tm.tm_year=g2(s->data);
	if(tm.tm_year < 50)
		tm.tm_year+=100;
	tm.tm_mon=g2(s->data+2)-1;
	tm.tm_mday=g2(s->data+4);
	tm.tm_hour=g2(s->data+6);
	tm.tm_min=g2(s->data+8);
	tm.tm_sec=g2(s->data+10);
	if(s->data[12] == 'Z')
		offset=0;
	else
		{
		offset=g2(s->data+13)*60+g2(s->data+15);
		if(s->data[12] == '-')
			offset= -offset;
		}
#undef g2

	return mktime(&tm)-offset*60; /* FIXME: mktime assumes the current timezone
	                               * instead of UTC, and unless we rewrite OpenSSL
				       * in Lisp we cannot locally change the timezone
				       * without possibly interfering with other parts
	                               * of the program. timegm, which uses UTC, is
				       * non-standard.
	                               * Also time_t is inappropriate for general
	                               * UTC times because it may a 32 bit type. */
}

#define ADD_STRING(s,offs,n,nl) \
	{ \
	if(offs > 0) \
		s[offs++] = ';'; \
	int m = sizeof(s) - offs - 1;\
	if (nl < m) \
		m = nl; \
	memcpy(s + offs,n,m);\
	offs += m; \
	s[offs] = 0; \
	}

static int check_cert(SSL *ssl, ssl_verify & verify)
{
	X509 *peer;

#ifdef X509_V_ERR_DIFFERENT_CRL_SCOPE
	const SSL_CIPHER *sslciph = SSL_get_current_cipher(ssl);
#else
  SSL_CIPHER *sslciph = SSL_get_current_cipher(ssl);
#endif

	if(sslciph == NULL)
		return EINVAL;

	SSL_CIPHER_description(sslciph, verify.cipher, sizeof(verify.cipher));

	//tprintf("Peer '%s': Encryption Description: %s\n",verify.peer,verify.cipher);

	long ret = SSL_get_verify_result(ssl);

	// ocsp??

	verify.cert_sane = 1;

	strncpy(verify.error,X509_verify_cert_error_string(ret),sizeof(verify.error)-1);
	verify.error[sizeof(verify.error)-1] = 0;

	//tprintf("     Error: %s\n",verify.error);

	switch(ret)
	{
		case X509_V_OK:
		{
			verify.cert_signed = 1;
		}
		break;

		case X509_V_ERR_CERT_REVOKED:
		{
			verify.cert_revoked = 1;
		}
		break;

		case X509_V_ERR_UNABLE_TO_GET_ISSUER_CERT:
			// certificate chain broken
		break;

		case X509_V_ERR_UNABLE_TO_GET_CRL:
			// no certificate revoke list
		break;

		case X509_V_ERR_UNABLE_TO_DECRYPT_CERT_SIGNATURE:
		case X509_V_ERR_UNABLE_TO_DECRYPT_CRL_SIGNATURE:
		case X509_V_ERR_UNABLE_TO_DECODE_ISSUER_PUBLIC_KEY:
		case X509_V_ERR_CERT_SIGNATURE_FAILURE:
		case X509_V_ERR_CRL_SIGNATURE_FAILURE:
			verify.cert_sane = 0;
		break;

		case X509_V_ERR_CERT_NOT_YET_VALID:
		case X509_V_ERR_CRL_NOT_YET_VALID:
		case X509_V_ERR_CERT_HAS_EXPIRED:
		case X509_V_ERR_CRL_HAS_EXPIRED:
			verify.cert_outdated = 1;
		break;

		case X509_V_ERR_ERROR_IN_CERT_NOT_BEFORE_FIELD:
		case X509_V_ERR_ERROR_IN_CERT_NOT_AFTER_FIELD:
		case X509_V_ERR_ERROR_IN_CRL_LAST_UPDATE_FIELD:
		case X509_V_ERR_ERROR_IN_CRL_NEXT_UPDATE_FIELD:
		case X509_V_ERR_OUT_OF_MEM:
			verify.cert_sane = 0;
		break;

		case X509_V_ERR_DEPTH_ZERO_SELF_SIGNED_CERT:
		case X509_V_ERR_SELF_SIGNED_CERT_IN_CHAIN:
			verify.cert_signed = 1;
			verify.cert_self_signed = 1;
		break;

		case X509_V_ERR_UNABLE_TO_GET_ISSUER_CERT_LOCALLY:
		case X509_V_ERR_UNABLE_TO_VERIFY_LEAF_SIGNATURE:
		case X509_V_ERR_CERT_CHAIN_TOO_LONG:
		case X509_V_ERR_INVALID_CA:
		case X509_V_ERR_INVALID_NON_CA:
		case X509_V_ERR_PATH_LENGTH_EXCEEDED:
//		case X509_V_ERR_PROXY_PATH_LENGTH_EXCEEDED:
//		case X509_V_ERR_PROXY_CERTIFICATES_NOT_ALLOWED:
		case X509_V_ERR_INVALID_PURPOSE:
		case X509_V_ERR_CERT_UNTRUSTED:
			break;

		case X509_V_ERR_CERT_REJECTED:
			verify.cert_rejected = 1;
		break;

		case X509_V_ERR_APPLICATION_VERIFICATION:
		case X509_V_ERR_SUBJECT_ISSUER_MISMATCH:
		case X509_V_ERR_AKID_SKID_MISMATCH:
		case X509_V_ERR_AKID_ISSUER_SERIAL_MISMATCH:
		case X509_V_ERR_KEYUSAGE_NO_CERTSIGN:
		case X509_V_ERR_UNABLE_TO_GET_CRL_ISSUER:
		case X509_V_ERR_UNHANDLED_CRITICAL_EXTENSION:
		case X509_V_ERR_KEYUSAGE_NO_CRL_SIGN:
//		case X509_V_ERR_KEYUSAGE_NO_DIGITAL_SIGNATURE:
		case X509_V_ERR_UNHANDLED_CRITICAL_CRL_EXTENSION:
//		case X509_V_ERR_INVALID_EXTENSION:
//		case X509_V_ERR_INVALID_POLICY_EXTENSION:
//		case X509_V_ERR_NO_EXPLICIT_POLICY:
			break;

#ifdef X509_V_ERR_DIFFERENT_CRL_SCOPE
		case X509_V_ERR_DIFFERENT_CRL_SCOPE:
		case X509_V_ERR_UNSUPPORTED_EXTENSION_FEATURE:
 		case X509_V_ERR_UNNESTED_RESOURCE:

		case X509_V_ERR_PERMITTED_VIOLATION:
		case X509_V_ERR_EXCLUDED_VIOLATION:
		case X509_V_ERR_SUBTREE_MINMAX:
		break;

		case X509_V_ERR_UNSUPPORTED_CONSTRAINT_TYPE:
		case X509_V_ERR_UNSUPPORTED_CONSTRAINT_SYNTAX:
		case X509_V_ERR_UNSUPPORTED_NAME_SYNTAX:
			verify.cert_sane = 0;
		break;

		case X509_V_ERR_CRL_PATH_VALIDATION_ERROR:
		break;
#endif

		default:
		break;
	}

	/*Check the cert chain. The chain length
	is automatically checked by OpenSSL when
	we set the verify depth in the ctx */

	/*Check the common name*/
	peer=SSL_get_peer_certificate(ssl);
	if(peer)
	{
		X509_NAME *subjectName = X509_get_subject_name(peer);

		if(subjectName)
		{
			// Common name
			int cnLoc = X509_NAME_get_index_by_NID(subjectName, NID_commonName, -1);
			int offs_sn = 0;
			int offs_dns = 0;

			while (cnLoc != -1)
			{
				X509_NAME_ENTRY* cnEntry = X509_NAME_get_entry(subjectName, cnLoc);

				ASN1_STRING* cnData = X509_NAME_ENTRY_get_data(cnEntry);

				if(offs_sn < (int)sizeof(verify.cert_commonName)-1 && cnData)
				{
					ADD_STRING(verify.cert_commonName,offs_sn,cnData->data,cnData->length);
				}

				cnLoc = X509_NAME_get_index_by_NID(subjectName, NID_commonName, cnLoc);
			}

			int subjectAltNameLoc = X509_get_ext_by_NID(peer, NID_subject_alt_name, -1);

			if(subjectAltNameLoc != -1)
			{
				X509_EXTENSION *extension = X509_get_ext(peer, subjectAltNameLoc);

				GENERAL_NAMES *generalNames = (GENERAL_NAMES*)X509V3_EXT_d2i(extension);

				if(generalNames)
				{
					for(int i = 0; i < sk_GENERAL_NAME_num(generalNames); ++i)
					{
						GENERAL_NAME *generalName = sk_GENERAL_NAME_value(generalNames, i);

						switch(generalName->type)
						{
							case GEN_OTHERNAME:
							{
								OTHERNAME* otherName = generalName->d.otherName;

								ASN1_UTF8STRING* xmppAddrValue = otherName->value->value.utf8string;

								if(offs_sn < (int)sizeof(verify.cert_commonName)-1 && xmppAddrValue)
								{
									ADD_STRING(verify.cert_commonName,offs_sn,ASN1_STRING_data(xmppAddrValue), ASN1_STRING_length(xmppAddrValue));
								}
							}
							break;

							case GEN_DNS:
							{
								// DNSName
								if(offs_dns < (int)sizeof(verify.cert_DNSname)-1)
								{
									ADD_STRING(verify.cert_DNSname,offs_dns,ASN1_STRING_data(generalName->d.dNSName),ASN1_STRING_length(generalName->d.dNSName));
								}
							}
							break;

							default:
							break;
						}
					}
					GENERAL_NAMES_free(generalNames);
				}
			}
		}

		verify.valid_since = ASN1_UTCTIME_get(X509_get_notBefore(peer));
		verify.valid_until = ASN1_UTCTIME_get(X509_get_notAfter(peer));

		verify.has_cert = 1;

#if 0
		STACK_OF(X509) *sk = SSL_get_peer_cert_chain(ssl);

		if(sk)
		{
			for (int i=0; i<sk_X509_num(sk); i++)
			{
				char buf[256];

				X509 *x = sk_X509_value(sk,i);

				X509_NAME_oneline(X509_get_subject_name(x),buf,sizeof buf);
				buf[strlen(buf)-1] = 0;
				tprintf("     %2d %s\n",i,buf);
				X509_NAME_oneline(X509_get_issuer_name(x),buf,sizeof buf);
				buf[strlen(buf)-1] = 0;
				tprintf("         i:%s\n",buf);

				time_t tt = ASN1_UTCTIME_get(X509_get_notBefore(x));
				tm *t = gmtime(&tt);
				strcpy(buf,asctime(t)); buf[strlen(buf)-1] = 0;
				tprintf("           nb:%s\n",buf);

				tt = ASN1_UTCTIME_get(X509_get_notAfter(x));
				t = gmtime(&tt);
				strcpy(buf,asctime(t)); buf[strlen(buf)-1] = 0;
				tprintf("           na:%s\n",buf);
			}
		}
#endif

		X509_free(peer);
	}
	else
	{
		tprintf("     no certificate\n");
		//ERR_print_errors_cb(err_cb, c);

		return EINVAL; // berr_exit("Certificate doesn’t verify");
	}

	return 0;
}

static int err_cb(const char *str, size_t len, void *u)
{
	ssl_connection * c = (ssl_connection *)u;

	tprintf("%s",str);

	if(c == NULL)
		return(0);

	long ret = SSL_get_verify_result(c->ssl);

	if(ret != X509_V_OK)
	{
		tprintf("%s\n",X509_verify_cert_error_string(ret));
	}

	return(0);
}


static int Checkfile(const char * filename,const char * errstr)
{
	BIO* bio = BIO_new_file(filename, "r");
	if (bio == NULL)
	{
		tprintf("%s\n",errstr);
		return -1 ;
	}
	BIO_free(bio);
	return 0 ;
}

static int cert_verify_callback(int ok, X509_STORE_CTX* store)
{
	char data[255];

  if (!ok)
	{
		X509* cert = X509_STORE_CTX_get_current_cert(store);
		int err = X509_STORE_CTX_get_error(store);

		if(err == X509_V_ERR_UNABLE_TO_GET_ISSUER_CERT_LOCALLY || err == X509_V_ERR_CERT_UNTRUSTED)
		{
			if(ssl_context.cert_ctx && X509_STORE_CTX_init(ssl_context.cert_ctx, ssl_context.cert_store, cert, NULL))
			{
				err = X509_verify_cert(ssl_context.cert_ctx);

				if(err == X509_V_OK)
				{
					X509_NAME_oneline(X509_get_subject_name(cert), data, 255);
					tprintf("cert CN: '%s' is locally stored.\n",data);
					// is locally stored cert

					return(1);
				}
			}
		}

		//int depth = X509_STORE_CTX_get_error_depth(store);
		//tprintf("Error with certificate at depth: %d\n", depth);
		X509_NAME_oneline(X509_get_issuer_name(cert), data, 255);
		//tprintf("\tIssuer: %s\n", data);
		X509_NAME_oneline(X509_get_subject_name(cert), data, 255);
		//tprintf("\tSubject: %s\n", data);
		//tprintf("\tError %d: %s\n", err, X509_verify_cert_error_string(err));

		switch(err)
		{
			case X509_V_ERR_CRL_HAS_EXPIRED:
			case X509_V_ERR_UNABLE_TO_VERIFY_LEAF_SIGNATURE:
				{
				}
			case X509_V_ERR_UNABLE_TO_GET_ISSUER_CERT:

			case X509_V_ERR_DEPTH_ZERO_SELF_SIGNED_CERT:
			case X509_V_ERR_SELF_SIGNED_CERT_IN_CHAIN:
			case X509_V_ERR_UNABLE_TO_GET_ISSUER_CERT_LOCALLY:
			case X509_V_ERR_CERT_UNTRUSTED:
			{
				ok = 1;
				/* save cert
				fp = fopen(data,"wb");
				if(fp)
				{
					err = PEM_write_X509(fp, cert);
					fclose(fp);
				}
				*/
			}
			break;

			default:
			break;
		}
  }
  return ok;
}

static int initClientCtx(void)
{
	/* Load our keys and certificates*/
	if(!(SSL_CTX_use_certificate_chain_file(ssl_context.client,CLIENT_CERTFILE)))
	{
		tprintf("Can't read certificate file '%s'\n",CLIENT_CERTFILE);
		ERR_print_errors_cb(err_cb, NULL);

		SSL_CTX_free(ssl_context.client);

		return(1);
	}

	SSL_CTX_set_default_passwd_cb(ssl_context.client,password_cb);

	if(!(SSL_CTX_use_PrivateKey_file(ssl_context.client,CLIENT_KEYFILE,SSL_FILETYPE_PEM)))
	{
		tprintf("Can't read key file '%s'\n",CLIENT_KEYFILE);
		ERR_print_errors_cb(err_cb, NULL);

		SSL_CTX_free(ssl_context.client);

		return(1);
	}

	/* Load the CAs we trust*/
	if(!(SSL_CTX_load_verify_locations(ssl_context.client,CA_LIST,NULL))) //"ssl/")))
	{
		tprintf("Can't read CA list\n");
		ERR_print_errors_cb(err_cb, NULL);
	}

	SSL_CTX_set_default_verify_paths(ssl_context.client);

	SSL_CTX_set_verify_depth(ssl_context.client, 10);
	SSL_CTX_set_verify(ssl_context.client, SSL_VERIFY_PEER, cert_verify_callback);

	// client without cert

	if(!(SSL_CTX_load_verify_locations(ssl_context.client_non,CA_LIST,NULL))) //"ssl/")))
	{
		tprintf("Can't read CA list\n");
		ERR_print_errors_cb(err_cb, NULL);
	}

	SSL_CTX_set_default_verify_paths(ssl_context.client_non);

	SSL_CTX_set_verify_depth(ssl_context.client_non, 10);
	SSL_CTX_set_verify(ssl_context.client_non, SSL_VERIFY_PEER, cert_verify_callback);

	return(0);
}

static int initServerCtx(void)
{
	/* Load our keys and certificates*/
	if(!(SSL_CTX_use_certificate_chain_file(ssl_context.server,SERVER_CERTFILE)))
	{
		tprintf("Can't read certificate file '%s'\n",SERVER_KEYFILE);
		ERR_print_errors_cb(err_cb, NULL);
		SSL_CTX_free(ssl_context.server);

		return -1;
	}

	SSL_CTX_set_default_passwd_cb(ssl_context.server,password_cb);

	if(!(SSL_CTX_use_PrivateKey_file(ssl_context.server,SERVER_KEYFILE,SSL_FILETYPE_PEM)))
	{
		tprintf("Can't read key file '%s'\n",SERVER_KEYFILE);
		ERR_print_errors_cb(err_cb, NULL);
		SSL_CTX_free(ssl_context.server);

		return -1;
	}

	/* Load the CAs we trust*/
	if(!(SSL_CTX_load_verify_locations(ssl_context.server,CA_LIST,NULL))) //"ssl/")))
	{
		tprintf("Can't read CA list\n");
		ERR_print_errors_cb(err_cb, NULL);
	}

	SSL_CTX_set_verify_depth(ssl_context.server, 10);
	SSL_CTX_set_default_verify_paths(ssl_context.server);

	if (!SSL_CTX_check_private_key(ssl_context.server) )
	{
		tprintf("private key invalid!\n");
		ERR_print_errors_cb(err_cb, NULL);

		SSL_CTX_free(ssl_context.server);
		return -1;
	}

	SSL_CTX_set_verify(ssl_context.server, SSL_VERIFY_PEER, cert_verify_callback);

  // Load the random file, read 1024 << 10 bytes, add to PRNG for entropy
  RAND_load_file(SSL_RANDOMFILE, 1024 << 10);

	{
		// We need to load the Diffie-Hellman key exchange parameters.
		// First load dh1024.pem (you DID create it, didn't you?)
		BIO* bio = BIO_new_file(SSL_DHFILE, "r");
		// Did we get a handle to the file?
		if (bio == NULL)
		{
			tprintf("Couldn't open DH param file\n");
			ERR_print_errors_cb(err_cb, NULL);
			SSL_CTX_free(ssl_context.server);
			return -1;
		}

		// Read in the DH params.
		DH* ret = PEM_read_bio_DHparams(bio, NULL, NULL, NULL);
		// Free up the BIO object.
		BIO_free(bio);

		// Set up our SSL_CTX to use the DH parameters.
		if (SSL_CTX_set_tmp_dh(ssl_context.server, ret) < 0)
		{
			tprintf("Couldn't set DH parameters\n");
			ERR_print_errors_cb(err_cb, NULL);
			SSL_CTX_free(ssl_context.server);
			return -1;
		}
	}
	// Now we need to generate a RSA key for use.
	// 1024-bit key. If you want to use something stronger, go ahead but it must be a power of 2. Upper limit should be 4096.
	RSA* rsa = RSA_generate_key(1024, RSA_F4, NULL, NULL);

	// Set up our SSL_CTX to use the generated RSA key.
	if (!SSL_CTX_set_tmp_rsa(ssl_context.server, rsa))
	{
		// We don't break out here because it's not a requirement for the RSA key to be set. It does help to have it.
		tprintf("Couldn't set RSA key\n");
		ERR_print_errors_cb(err_cb, NULL);
		SSL_CTX_free(ssl_context.server);
		return -1;
	}
	// Free up the RSA structure.
	RSA_free(rsa);

  SSL_CTX_set_cipher_list(ssl_context.server, "ALL");

	ssl_context.cert_store = X509_STORE_new();
	ssl_context.cert_ctx   = X509_STORE_CTX_new();

	if (!X509_STORE_load_locations(ssl_context.cert_store,NULL,CERTS_LOCAL))
	{
		tprintf("Couldn't set cert paths\n");
		ERR_print_errors_cb(err_cb, NULL);
	}

	return 0 ;
}

int SecureSocketInit()
{
	SSL_library_init();
	SSL_load_error_strings();
	ERR_load_BIO_strings();
	OpenSSL_add_all_algorithms();

	RAND_write_file(SSL_RANDOMFILE);

	if (Checkfile(CLIENT_CERTFILE,"Couldn't open client certificate") == -1) 	return -11;
	if (Checkfile(CLIENT_KEYFILE,"Couldn't open client pk") == -1) 								return -1;
	if (Checkfile(SERVER_CERTFILE,"Couldn't open server certificate") == -1) 	return -1;
	if (Checkfile(SERVER_KEYFILE,"Couldn't open server pk") == -1) 						return -1;

	ssl_context.client = SSL_CTX_new(SSLv23_method());

	if (ssl_context.client == NULL)
	{
		tprintf("could not create a client context\n");
		return -1;
	}

	ssl_context.client_non = SSL_CTX_new(SSLv23_method());

	if (ssl_context.client_non == NULL)
	{
		tprintf("could not create a client_non context\n");
		return -1;
	}
	int ret = initClientCtx();
	if(ret)
		return ret;

	ssl_context.server			= SSL_CTX_new(SSLv23_server_method());

	if(ssl_context.server == NULL)
	{
		tprintf("could not create a new context\n");

		return EACCES;
	}

	ret = initServerCtx();
	if (ret)
		return ret;

	return 0 ;
}
