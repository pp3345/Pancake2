#include "PancakeLinuxPoll.h"
#include "../PancakeNetwork.h"
#include "../PancakeConfiguration.h"
#include "../PancakeLogger.h"
#include "../PancakeScheduler.h"

Int32 PancakeLinuxPollFD = -1;

/* Forward declarations */
STATIC UByte PancakeLinuxPollInitialize();
STATIC UByte PancakeLinuxPollShutdown();
STATIC UByte PancakeLinuxPollServerInitialize();
STATIC void PancakeLinuxPollWait();
STATIC inline void PancakeLinuxPollAddReadSocket(PancakeSocket *socket);
STATIC inline void PancakeLinuxPollAddWriteSocket(PancakeSocket *socket);
STATIC inline void PancakeLinuxPollAddReadWriteSocket(PancakeSocket *socket);

STATIC inline void PancakeLinuxPollRemoveSocket(PancakeSocket *socket);
STATIC inline void PancakeLinuxPollRemoveReadSocket(PancakeSocket *socket);
STATIC inline void PancakeLinuxPollRemoveWriteSocket(PancakeSocket *socket);

STATIC inline void PancakeLinuxPollSetReadSocket(PancakeSocket *socket);
STATIC inline void PancakeLinuxPollSetWriteSocket(PancakeSocket *socket);
STATIC inline void PancakeLinuxPollSetSocket(PancakeSocket *socket);

STATIC inline void PancakeLinuxPollOnSocketClose(PancakeSocket *socket);

static PancakeSocket *currentSocket;

PancakeModule PancakeLinuxPoll = {
		"LinuxPoll",
		PancakeLinuxPollInitialize,
		NULL,
		PancakeLinuxPollShutdown,
		0
};


PancakeServerArchitecture PancakeLinuxPollServer = {
		(String) {"LinuxPoll", sizeof("LinuxPoll") - 1},

		PancakeLinuxPollWait,

		PancakeLinuxPollAddReadSocket,
		PancakeLinuxPollAddWriteSocket,
		PancakeLinuxPollAddReadWriteSocket,

		PancakeLinuxPollRemoveReadSocket,
		PancakeLinuxPollRemoveWriteSocket,
		PancakeLinuxPollRemoveSocket,

		PancakeLinuxPollSetReadSocket,
		PancakeLinuxPollSetWriteSocket,
		PancakeLinuxPollSetSocket,

		PancakeLinuxPollOnSocketClose,

		PancakeLinuxPollServerInitialize,

		NULL
};

STATIC UByte PancakeLinuxPollInitialize() {
	PancakeRegisterServerArchitecture(&PancakeLinuxPollServer);

	return 1;
}

STATIC UByte PancakeLinuxPollShutdown() {
	if(PancakeLinuxPollFD != -1) {
		close(PancakeLinuxPollFD);
	}

	return 1;
}

STATIC UByte PancakeLinuxPollServerInitialize() {
	// Just test epoll here, to make sure everything will work
	// The actual epoll instance must be created in the workers as it can't be shared

	// Initialize with size 32 on old kernels
	PancakeLinuxPollFD = epoll_create(32);

	if(PancakeLinuxPollFD == -1) {
		PancakeLoggerFormat(PANCAKE_LOGGER_ERROR, 0, "Can't create epoll instance: %s", strerror(errno));
		return 0;
	}

	close(PancakeLinuxPollFD);
	PancakeLinuxPollFD = -1;

	return 1;
}

STATIC inline void PancakeLinuxPollAddReadSocket(PancakeSocket *socket) {
	if(socket->flags & PANCAKE_LINUX_POLL_SOCKET) {
		if(!(socket->flags & PANCAKE_LINUX_POLL_IN)) {
			struct epoll_event event;

			socket->flags |= PANCAKE_LINUX_POLL_IN;
			event.events = EPOLLIN | EPOLLRDHUP | (socket->flags & PANCAKE_LINUX_POLL_OUT ? EPOLLOUT : 0);
			event.data.ptr = (void*) socket;

			epoll_ctl(PancakeLinuxPollFD, EPOLL_CTL_MOD, socket->fd, &event);
		}
	} else {
		struct epoll_event event;

		socket->flags |= PANCAKE_LINUX_POLL_IN | PANCAKE_LINUX_POLL_SOCKET;
		event.events = EPOLLIN | EPOLLRDHUP;
		event.data.ptr = (void*) socket;

		epoll_ctl(PancakeLinuxPollFD, EPOLL_CTL_ADD, socket->fd, &event);
	}
}

STATIC inline void PancakeLinuxPollAddWriteSocket(PancakeSocket *socket) {
	if(socket->flags & PANCAKE_LINUX_POLL_SOCKET) {
		if(!(socket->flags & PANCAKE_LINUX_POLL_OUT)) {
			struct epoll_event event;

			socket->flags |= PANCAKE_LINUX_POLL_OUT;
			event.events = EPOLLOUT | EPOLLRDHUP | (socket->flags & PANCAKE_LINUX_POLL_IN ? EPOLLIN : 0);
			event.data.ptr = (void*) socket;

			epoll_ctl(PancakeLinuxPollFD, EPOLL_CTL_MOD, socket->fd, &event);
		}
	} else {
		struct epoll_event event;

		socket->flags |= PANCAKE_LINUX_POLL_OUT | PANCAKE_LINUX_POLL_SOCKET;
		event.events = EPOLLOUT | EPOLLRDHUP;
		event.data.ptr = (void*) socket;

		epoll_ctl(PancakeLinuxPollFD, EPOLL_CTL_ADD, socket->fd, &event);
	}
}

STATIC inline void PancakeLinuxPollAddReadWriteSocket(PancakeSocket *socket) {
	if(socket->flags & PANCAKE_LINUX_POLL_SOCKET) {
		if((socket->flags & (PANCAKE_LINUX_POLL_OUT | PANCAKE_LINUX_POLL_IN)) != (PANCAKE_LINUX_POLL_OUT | PANCAKE_LINUX_POLL_IN)) {
			struct epoll_event event;

			socket->flags |= PANCAKE_LINUX_POLL_OUT | PANCAKE_LINUX_POLL_IN;
			event.events = EPOLLOUT | EPOLLRDHUP | EPOLLIN;
			event.data.ptr = (void*) socket;

			epoll_ctl(PancakeLinuxPollFD, EPOLL_CTL_MOD, socket->fd, &event);
		}
	} else {
		struct epoll_event event;

		socket->flags |= PANCAKE_LINUX_POLL_OUT | PANCAKE_LINUX_POLL_IN | PANCAKE_LINUX_POLL_SOCKET;
		event.events = EPOLLOUT | EPOLLIN | EPOLLRDHUP;
		event.data.ptr = (void*) socket;

		epoll_ctl(PancakeLinuxPollFD, EPOLL_CTL_ADD, socket->fd, &event);
	}
}

STATIC inline void PancakeLinuxPollRemoveSocket(PancakeSocket *socket) {
	if(socket->flags & PANCAKE_LINUX_POLL_SOCKET) {
		epoll_ctl(PancakeLinuxPollFD, EPOLL_CTL_DEL, socket->fd, NULL);
		socket->flags ^= PANCAKE_LINUX_POLL_SOCKET;
	}
}

STATIC inline void PancakeLinuxPollRemoveReadSocket(PancakeSocket *socket) {
	if(socket->flags & PANCAKE_LINUX_POLL_IN) {
		struct epoll_event event;

		event.events = EPOLLRDHUP | (socket->flags & PANCAKE_LINUX_POLL_OUT ? EPOLLOUT : 0);
		event.data.ptr = (void*) socket;

		epoll_ctl(PancakeLinuxPollFD, EPOLL_CTL_MOD, socket->fd, &event);
		socket->flags ^= PANCAKE_LINUX_POLL_IN;
	}
}

STATIC inline void PancakeLinuxPollRemoveWriteSocket(PancakeSocket *socket) {
	if(socket->flags & PANCAKE_LINUX_POLL_OUT) {
		struct epoll_event event;

		event.events = EPOLLRDHUP | (socket->flags & PANCAKE_LINUX_POLL_IN ? EPOLLIN : 0);
		event.data.ptr = (void*) socket;

		epoll_ctl(PancakeLinuxPollFD, EPOLL_CTL_MOD, socket->fd, &event);
		socket->flags ^= PANCAKE_LINUX_POLL_OUT;
	}
}

STATIC inline void PancakeLinuxPollSetReadSocket(PancakeSocket *socket) {
	struct epoll_event event;

	event.events = EPOLLRDHUP | EPOLLIN;
	event.data.ptr = (void*) socket;

	socket->flags |= PANCAKE_LINUX_POLL_IN;

	if(socket->flags & PANCAKE_LINUX_POLL_SOCKET) {
		epoll_ctl(PancakeLinuxPollFD, EPOLL_CTL_MOD, socket->fd, &event);

		if(socket->flags & PANCAKE_LINUX_POLL_OUT) {
			socket->flags ^= PANCAKE_LINUX_POLL_OUT;
		}
	} else {
		socket->flags |= PANCAKE_LINUX_POLL_SOCKET;

		epoll_ctl(PancakeLinuxPollFD, EPOLL_CTL_ADD, socket->fd, &event);
	}
}

STATIC inline void PancakeLinuxPollSetWriteSocket(PancakeSocket *socket) {
	struct epoll_event event;

	event.events = EPOLLRDHUP | EPOLLOUT;
	event.data.ptr = (void*) socket;

	socket->flags |= PANCAKE_LINUX_POLL_OUT;

	if(socket->flags & PANCAKE_LINUX_POLL_SOCKET) {
		epoll_ctl(PancakeLinuxPollFD, EPOLL_CTL_MOD, socket->fd, &event);

		if(socket->flags & PANCAKE_LINUX_POLL_IN) {
			socket->flags ^= PANCAKE_LINUX_POLL_IN;
		}
	} else {
		socket->flags |= PANCAKE_LINUX_POLL_SOCKET;

		epoll_ctl(PancakeLinuxPollFD, EPOLL_CTL_ADD, socket->fd, &event);
	}
}

STATIC inline void PancakeLinuxPollSetSocket(PancakeSocket *socket) {
	struct epoll_event event;

	event.events = EPOLLRDHUP;
	event.data.ptr = (void*) socket;

	if(socket->flags & PANCAKE_LINUX_POLL_SOCKET) {
		epoll_ctl(PancakeLinuxPollFD, EPOLL_CTL_MOD, socket->fd, &event);

		if(socket->flags & PANCAKE_LINUX_POLL_IN) {
			socket->flags ^= PANCAKE_LINUX_POLL_IN;
		}

		if(socket->flags & PANCAKE_LINUX_POLL_OUT) {
			socket->flags ^= PANCAKE_LINUX_POLL_OUT;
		}
	} else {
		socket->flags |= PANCAKE_LINUX_POLL_SOCKET;

		epoll_ctl(PancakeLinuxPollFD, EPOLL_CTL_ADD, socket->fd, &event);
	}
}

STATIC inline void PancakeLinuxPollOnSocketClose(PancakeSocket *socket) {
	// Socket will automatically be removed from epoll instance on close(), no need to do EPOLL_CTL_DEL operation

	// Make sure we don't execute further events on this socket
	if(socket == currentSocket) {
		currentSocket = NULL;
	}
}

STATIC void PancakeLinuxPollWait() {
	// Initialize with size 32 on old kernels
	PancakeLinuxPollFD = epoll_create(32);

	if(PancakeLinuxPollFD == -1) {
		PancakeLoggerFormat(PANCAKE_LOGGER_ERROR, 0, "Can't create epoll instance: %s", strerror(errno));
		return;
	}

	// Activate listen sockets
	PancakeNetworkActivateListenSockets();

	while(1) {
		struct epoll_event events[32];
		Int32 numEvents, i;

		numEvents = epoll_wait(PancakeLinuxPollFD, events, 32, PancakeSchedulerGetNextExecutionTimeOffset() * 1000);

		if(UNEXPECTED(numEvents == -1)) {
			if(PancakeDoShutdown) {
				return;
			}

			if(errno == EINTR) {
				continue;
			}

			PancakeLoggerFormat(PANCAKE_LOGGER_ERROR, 0, "epoll_wait failed: %s", strerror(errno));
			return;
		}

		// Network events first
		for(i = 0; i < numEvents; i++) {
			PancakeSocket *sock = (PancakeSocket*) events[i].data.ptr;

			if(((events[i].events & EPOLLHUP) && !(events[i].events & EPOLLRDHUP)) || (events[i].events & EPOLLERR)) {
				sock->onRemoteHangup(sock);
				PancakeCheckHeap();
				continue;
			}

			currentSocket = sock;

			if(events[i].events & EPOLLIN) {
				sock->onRead(sock);
				PancakeCheckHeap();
			}

			if(events[i].events & EPOLLOUT) {
				// Socket has been closed in onRead()
				if(!currentSocket) {
					continue;
				}

				sock->onWrite(sock);
				PancakeCheckHeap();
			}

			if(events[i].events & EPOLLRDHUP) {
				// Socket has been closed in onWrite()
				if(!currentSocket) {
					continue;
				}

				sock->onRemoteHangup(sock);
				PancakeCheckHeap();
			}
		}

		// Scheduler events second
		PancakeSchedulerRun();

		if(UNEXPECTED(PancakeDoShutdown)) {
			return;
		}
	}
}
