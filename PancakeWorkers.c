
#include "PancakeWorkers.h"
#include "PancakeLogger.h"

/* Forward declaration */
STATIC void PancakeInternalCommunicationEvent(PancakeSocket *sock);

PANCAKE_API UByte PancakeRunWorker(PancakeWorker *worker) {
	pid_t pid;
	Int32 sockets[2];

	// Create sockets for internal communication
	if(socketpair(AF_UNIX, SOCK_STREAM, 0, sockets) == -1) {
		PancakeLoggerFormat(PANCAKE_LOGGER_ERROR, 0, "Can't create sockets for internal communication: %s", strerror(errno));
		return 0;
	}

	worker->masterSocket = sockets[0];
	worker->workerSocket.fd = sockets[1];
	worker->workerSocket.flags = 0;
	worker->workerSocket.onRead = PancakeInternalCommunicationEvent;

	// Fork child from master
	pid = fork();

	if(pid == -1) {
		PancakeLoggerFormat(PANCAKE_LOGGER_ERROR, 0, "Can't fork: %s", strerror(errno));
		return 0;
	} else if(pid) {
		/* Master */

		worker->pid = pid;
		return 1;
	} else {
		/* Child */

		worker->pid = getpid();
		PancakeCurrentWorker = worker;

		// Register communication socket
		PancakeNetworkAddReadSocket(&worker->workerSocket);

		PancakeDebug {
			PancakeLoggerFormat(PANCAKE_LOGGER_SYSTEM, 0, "PID: %i", worker->pid);
		}

		worker->run();

		return 2;
	}
}

STATIC void PancakeInternalCommunicationEvent(PancakeSocket *sock) {
	UByte instruction;

	read(sock->fd, &instruction, 1);

	switch(instruction) {
		default:
		case PANCAKE_WORKER_GRACEFUL_SHUTDOWN_INT:
			PancakeDoShutdown = 1;
			break;
	}
}
