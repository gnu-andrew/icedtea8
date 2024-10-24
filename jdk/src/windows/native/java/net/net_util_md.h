/*
 * Copyright (c) 1997, 2013, Oracle and/or its affiliates. All rights reserved.
 * DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER.
 *
 * This code is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 only, as
 * published by the Free Software Foundation.  Oracle designates this
 * particular file as subject to the "Classpath" exception as provided
 * by Oracle in the LICENSE file that accompanied this code.
 *
 * This code is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * version 2 for more details (a copy is included in the LICENSE file that
 * accompanied this code).
 *
 * You should have received a copy of the GNU General Public License version
 * 2 along with this work; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 * Please contact Oracle, 500 Oracle Parkway, Redwood Shores, CA 94065 USA
 * or visit www.oracle.com if you need additional information or have any
 * questions.
 */
#include <winsock2.h>
#include <WS2tcpip.h>
#include <iphlpapi.h>
#include <icmpapi.h>

/* used to disable connection reset messages on Windows XP */
#ifndef SIO_UDP_CONNRESET
#define SIO_UDP_CONNRESET _WSAIOW(IOC_VENDOR,12)
#endif

#ifndef IN6_IS_ADDR_ANY
#define IN6_IS_ADDR_ANY(a)      \
    (((a)->s6_words[0] == 0) && ((a)->s6_words[1] == 0) &&      \
    ((a)->s6_words[2] == 0) && ((a)->s6_words[3] == 0) &&       \
    ((a)->s6_words[4] == 0) && ((a)->s6_words[5] == 0) &&       \
    ((a)->s6_words[6] == 0) && ((a)->s6_words[7] == 0))
#endif

#ifndef IPV6_V6ONLY
#define IPV6_V6ONLY     27 /* Treat wildcard bind as AF_INET6-only. */
#endif

#define MAX_BUFFER_LEN          2048
#define MAX_HEAP_BUFFER_LEN     65536

/* true if SO_RCVTIMEO is supported by underlying provider */
extern jboolean isRcvTimeoutSupported;

void NET_ThrowCurrent(JNIEnv *env, char *msg);

/*
 * Return default Type Of Service
 */
int NET_GetDefaultTOS(void);

typedef union {
    struct sockaddr     him;
    struct sockaddr_in  him4;
    struct sockaddr_in6 sa6;
} SOCKETADDRESS;

/*
 * passed to NET_BindV6. Both ipv4_fd and ipv6_fd must be created and unbound
 * sockets. On return they may refer to different sockets.
 */
struct ipv6bind {
    SOCKETADDRESS       *addr;
    SOCKET               ipv4_fd;
    SOCKET               ipv6_fd;
};

#define SOCKETADDRESS_LEN(X)    \
        (((X)->him.sa_family==AF_INET6)? sizeof(struct SOCKADDR_IN6) : \
                         sizeof(struct sockaddr_in))

#define SOCKETADDRESS_COPY(DST,SRC) {                           \
    if ((SRC)->sa_family == AF_INET6) {                         \
        memcpy ((DST), (SRC), sizeof (struct sockaddr_in6));    \
    } else {                                                    \
        memcpy ((DST), (SRC), sizeof (struct sockaddr_in));     \
    }                                                           \
}

#define SET_PORT(X,Y) {                         \
    if ((X)->him.sa_family == AF_INET) {        \
        (X)->him4.sin_port = (Y);               \
    } else {                                    \
        (X)->him6.sin6_port = (Y);              \
    }                                           \
}

#define GET_PORT(X) ((X)->him.sa_family==AF_INET ?(X)->him4.sin_port: (X)->him6.sin6_port)

#define IS_LOOPBACK_ADDRESS(x) ( \
    ((x)->him.sa_family == AF_INET) ? \
        (ntohl((x)->him4.sin_addr.s_addr)==INADDR_LOOPBACK) : \
        (IN6ADDR_ISLOOPBACK (x)) \
)

JNIEXPORT int JNICALL NET_SocketClose(int fd);

JNIEXPORT int JNICALL NET_Timeout(int fd, long timeout);

int NET_Socket(int domain, int type, int protocol);

void NET_ThrowByNameWithLastError(JNIEnv *env, const char *name,
         const char *defaultDetail);

void NET_ThrowSocketException(JNIEnv *env, char* msg);

/*
 * differs from NET_Timeout() as follows:
 *
 * If timeout = -1, it blocks forever.
 *
 * returns 1 or 2 depending if only one or both sockets
 * fire at same time.
 *
 * *fdret is (one of) the active fds. If both sockets
 * fire at same time, *fd == fd always.
 */
JNIEXPORT int JNICALL NET_Timeout2(int fd, int fd1, long timeout, int *fdret);

JNIEXPORT int JNICALL NET_BindV6(struct ipv6bind* b, jboolean exclBind);

#define NET_WAIT_READ   0x01
#define NET_WAIT_WRITE  0x02
#define NET_WAIT_CONNECT        0x04

extern jint NET_Wait(JNIEnv *env, jint fd, jint flags, jint timeout);

JNIEXPORT int JNICALL NET_WinBind(int s, struct sockaddr *him, int len,
                                   jboolean exclBind);

/* XP versions of the native routines */

JNIEXPORT jobject JNICALL Java_java_net_NetworkInterface_getByName0_XP
    (JNIEnv *env, jclass cls, jstring name);

JNIEXPORT jobject JNICALL Java_java_net_NetworkInterface_getByIndex0_XP
  (JNIEnv *env, jclass cls, jint index);

JNIEXPORT jobject JNICALL Java_java_net_NetworkInterface_getByInetAddress0_XP
    (JNIEnv *env, jclass cls, jobject iaObj);

JNIEXPORT jobjectArray JNICALL Java_java_net_NetworkInterface_getAll_XP
    (JNIEnv *env, jclass cls);

JNIEXPORT jboolean JNICALL Java_java_net_NetworkInterface_supportsMulticast0_XP
(JNIEnv *env, jclass cls, jstring name, jint index);

JNIEXPORT jboolean JNICALL Java_java_net_NetworkInterface_isUp0_XP
(JNIEnv *env, jclass cls, jstring name, jint index);

JNIEXPORT jboolean JNICALL Java_java_net_NetworkInterface_isP2P0_XP
(JNIEnv *env, jclass cls, jstring name, jint index);

JNIEXPORT jbyteArray JNICALL Java_java_net_NetworkInterface_getMacAddr0_XP
(JNIEnv *env, jclass cls, jstring name, jint index);

JNIEXPORT jint JNICALL Java_java_net_NetworkInterface_getMTU0_XP
(JNIEnv *env, jclass class, jstring name, jint index);

JNIEXPORT jboolean JNICALL Java_java_net_NetworkInterface_isLoopback0_XP
(JNIEnv *env, jclass cls, jstring name, jint index);
