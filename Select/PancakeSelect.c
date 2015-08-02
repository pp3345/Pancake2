#include "PancakeSelect.h"
#include "../PancakeLogger.h"
#include "../PancakeNetwork.h"
#include "../PancakeScheduler.h"

STATIC UByte PancakeSelectInitialize();
STATIC void PancakeSelectWait();

STATIC inline void PancakeSelectAddReadSocket(PancakeSocket *socket);
STATIC inline void PancakeSelectAddWriteSocket(PancakeSocket *socket);
STATIC inline void PancakeSelectAddReadWriteSocket(PancakeSocket *socket);

STATIC inline void PancakeSelectRemoveSocket(PancakeSocket *socket);
STATIC inline void PancakeSelectRemoveReadSocket(PancakeSocket *socket);
STATIC inline void PancakeSelectRemoveWriteSocket(PancakeSocket *socket);

STATIC inline void PancakeSelectSetReadSocket(PancakeSocket *socket);
STATIC inline void PancakeSelectSetWriteSocket(PancakeSocket *socket);
STATIC inline void PancakeSelectSetSocket(PancakeSocket *socket);

STATIC inline void PancakeSelectOnSocketClose(PancakeSocket *socket);

static fd_set PancakeSelectReadFDSet;
static fd_set PancakeSelectWriteFDSet;
static Int32 PancakeSelectMaxFD = FD_SETSIZE;
static PancakeSocket *PancakeSelectSockets[FD_SETSIZE];
static PancakeSocket *currentSocket = NULL;

PancakeModule PancakeSelect = {
		"Select",
		PancakeSelectInitialize,
		NULL,
		NULL,
		0
};

PancakeServerArchitecture PancakeSelectServer = {
		StaticString("Select"),

		PancakeSelectWait,

		PancakeSelectAddReadSocket,
		PancakeSelectAddWriteSocket,
		PancakeSelectAddReadWriteSocket,

		PancakeSelectRemoveReadSocket,
		PancakeSelectRemoveWriteSocket,
		PancakeSelectRemoveSocket,

		PancakeSelectSetReadSocket,
		PancakeSelectSetWriteSocket,
		PancakeSelectSetSocket,

		PancakeSelectOnSocketClose,

		NULL,

		NULL
};

STATIC UByte PancakeSelectInitialize() {
	PancakeRegisterServerArchitecture(&PancakeSelectServer);

	FD_ZERO(&PancakeSelectReadFDSet);
	FD_ZERO(&PancakeSelectWriteFDSet);

	memset(PancakeSelectSockets, 0, FD_SETSIZE * sizeof(PancakeSocket*));

	return 1;
}

STATIC inline void PancakeSelectDetermineMaxFD() {
	Int32 i;

	if(PancakeSelectMaxFD <= 1) {
		PancakeSelectMaxFD = 0;
		return;
	}

	for(i = PancakeSelectMaxFD - 1; i > 0; i--) {
		if(PancakeSelectSockets[i] != NULL) {
			PancakeSelectMaxFD = i;
			return;
		}
	}
}

STATIC inline void PancakeSelectAddReadSocket(PancakeSocket *socket) {
	if(socket->fd > PancakeSelectMaxFD) {
		PancakeSelectMaxFD = socket->fd;
	}

	FD_SET(socket->fd, &PancakeSelectReadFDSet);
	PancakeSelectSockets[socket->fd] = socket;
}

STATIC inline void PancakeSelectAddWriteSocket(PancakeSocket *socket) {
	if(socket->fd > PancakeSelectMaxFD) {
		PancakeSelectMaxFD = socket->fd;
	}

	FD_SET(socket->fd, &PancakeSelectReadFDSet);
	FD_SET(socket->fd, &PancakeSelectWriteFDSet);
	PancakeSelectSockets[socket->fd] = socket;
}

STATIC inline void PancakeSelectAddReadWriteSocket(PancakeSocket *socket) {
	if(socket->fd > PancakeSelectMaxFD) {
		PancakeSelectMaxFD = socket->fd;
	}

	FD_SET(socket->fd, &PancakeSelectReadFDSet);
	FD_SET(socket->fd, &PancakeSelectWriteFDSet);
	PancakeSelectSockets[socket->fd] = socket;
}

STATIC inline void PancakeSelectRemoveSocket(PancakeSocket *socket) {
	if(socket->fd == PancakeSelectMaxFD) {
		PancakeSelectDetermineMaxFD();
	}

	FD_CLR(socket->fd, &PancakeSelectReadFDSet);
	FD_CLR(socket->fd, &PancakeSelectWriteFDSet);
	PancakeSelectSockets[socket->fd] = NULL;
}

STATIC inline void PancakeSelectRemoveReadSocket(PancakeSocket *socket) {
	if(socket->fd == PancakeSelectMaxFD) {
		PancakeSelectDetermineMaxFD();
	}

	FD_SET(socket->fd, &PancakeSelectReadFDSet);
	PancakeSelectSockets[socket->fd] = socket;
}

STATIC inline void PancakeSelectRemoveWriteSocket(PancakeSocket *socket) {
	if(socket->fd == PancakeSelectMaxFD) {
		PancakeSelectDetermineMaxFD();
	}

	FD_CLR(socket->fd, &PancakeSelectWriteFDSet);

	if(!FD_ISSET(socket->fd, &PancakeSelectReadFDSet))
		PancakeSelectSockets[socket->fd] = NULL;
}

STATIC inline void PancakeSelectSetReadSocket(PancakeSocket *socket) {
	if(socket->fd > PancakeSelectMaxFD) {
		PancakeSelectMaxFD = socket->fd;
	}

	FD_SET(socket->fd, &PancakeSelectReadFDSet);
	FD_CLR(socket->fd, &PancakeSelectWriteFDSet);
	PancakeSelectSockets[socket->fd] = socket;
}

STATIC inline void PancakeSelectSetWriteSocket(PancakeSocket *socket) {
	if(socket->fd > PancakeSelectMaxFD) {
		PancakeSelectMaxFD = socket->fd;
	}

	FD_SET(socket->fd, &PancakeSelectWriteFDSet);
	FD_SET(socket->fd, &PancakeSelectReadFDSet);
	PancakeSelectSockets[socket->fd] = socket;
}

STATIC inline void PancakeSelectSetSocket(PancakeSocket *socket) {
	if(socket->fd > PancakeSelectMaxFD) {
		PancakeSelectMaxFD = socket->fd;
	}

	FD_SET(socket->fd, &PancakeSelectReadFDSet);
	FD_CLR(socket->fd, &PancakeSelectWriteFDSet);
	PancakeSelectSockets[socket->fd] = socket;
}

STATIC inline void PancakeSelectOnSocketClose(PancakeSocket *socket) {
	// Make sure we don't execute further events on this socket
	if(socket == currentSocket) {
		currentSocket = NULL;
	}

	// Need to clear socket from fd sets
	FD_CLR(socket->fd, &PancakeSelectReadFDSet);
	FD_CLR(socket->fd, &PancakeSelectWriteFDSet);
	PancakeSelectSockets[socket->fd] = NULL;

	if(socket->fd == PancakeSelectMaxFD)
		PancakeSelectDetermineMaxFD();
}

STATIC void PancakeSelectWait() {
	// Activate listen sockets
	PancakeNetworkActivateListenSockets();

	PancakeSelectDetermineMaxFD();

	while(1) {
		fd_set readFDSet = PancakeSelectReadFDSet;
		fd_set writeFDSet = PancakeSelectWriteFDSet;
		Int32 numEvents;
		struct timeval timeout;

		timeout.tv_sec = PancakeSchedulerGetNextExecutionTimeOffset();
		timeout.tv_usec = 0;

		numEvents = select(PancakeSelectMaxFD + 1, &readFDSet, &writeFDSet, NULL, &timeout);

		if(UNEXPECTED(numEvents == -1)) {
			if(PancakeDoShutdown) {
				return;
			}

			if(errno == EINTR) {
				continue;
			}

			PancakeLoggerFormat(PANCAKE_LOGGER_ERROR, 0, "select failed: %s", strerror(errno));
			return;
		}

		// Network events first
		if(numEvents) {
			Int32 fd;

			for(fd = PancakeSelectMaxFD; numEvents && fd > 0; fd--) {
				currentSocket = PancakeSelectSockets[fd];

				if(currentSocket == NULL)
					continue;

				if(FD_ISSET(fd, &readFDSet)) {
					UByte buf[1];

					numEvents--;

					if(currentSocket->onRead == NULL || recv(fd, &buf, 1, MSG_PEEK | MSG_DONTWAIT) == 0) {
						PancakeAssert(currentSocket->onRemoteHangup != NULL);
						currentSocket->onRemoteHangup(currentSocket);
						PancakeCheckHeap();

						continue;
					} else {
						currentSocket->onRead(currentSocket);
					}

					PancakeCheckHeap();
				}

				if(FD_ISSET(fd, &writeFDSet)) {
					numEvents--; // Still need to decrement numEvents, even if currentSocket is NULL

					if(currentSocket) {
						currentSocket->onWrite(currentSocket);
						PancakeCheckHeap();
					}
				}
			}
		}

		// Scheduler events second
		PancakeSchedulerRun();

		if(UNEXPECTED(PancakeDoShutdown)) {
			return;
		}
	}
}
