/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2015 Synergy Si Ltd.
 * 
 * This package is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * found in the file COPYING that should have accompanied this file.
 * 
 * This package is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "SecureListenSocket.h"

#include "SecureSocket.h"
#include "net/NetworkAddress.h"
#include "net/SocketMultiplexer.h"
#include "net/TSocketMultiplexerMethodJob.h"
#include "arch/XArch.h"

static const char s_certificateFilename[] = { "Synergy.pem" };

//
// SecureListenSocket
//

SecureListenSocket::SecureListenSocket(
		IEventQueue* events,
		SocketMultiplexer* socketMultiplexer) :
	TCPListenSocket(events, socketMultiplexer)
{
}

SecureListenSocket::~SecureListenSocket()
{
	SecureSocketSet::iterator it;
	for (it = m_secureSocketSet.begin(); it != m_secureSocketSet.end(); it++) {
		delete *it;
	}
	m_secureSocketSet.clear();
}

IDataSocket*
SecureListenSocket::accept()
{
	SecureSocket* socket = NULL;
	try {
		socket = new SecureSocket(
						m_events,
						m_socketMultiplexer,
						ARCH->acceptSocket(m_socket, NULL));
		socket->initSsl(true);

		if (socket != NULL) {
			setListeningJob();
		}

		// TODO: customized certificate path
		String certificateFilename = ARCH->getProfileDirectory();
#if SYSAPI_WIN32
		certificateFilename.append("\\");
#elif SYSAPI_UNIX
		certificateFilename.append("/");
#endif
		certificateFilename.append(s_certificateFilename);

		bool loaded = socket->loadCertificates(certificateFilename);
		if (!loaded) {
			delete socket;
			return NULL;
		}

		socket->secureAccept();

		m_secureSocketSet.insert(socket);

		return dynamic_cast<IDataSocket*>(socket);
	}
	catch (XArchNetwork&) {
		if (socket != NULL) {
			delete socket;
			setListeningJob();
		}
		return NULL;
	}
	catch (std::exception &ex) {
		if (socket != NULL) {
			delete socket;
			setListeningJob();
		}
		throw ex;
	}
}

void
SecureListenSocket::deleteSocket(void* socket)
{
	SecureSocketSet::iterator it;
	it = m_secureSocketSet.find((IDataSocket*)socket);
	if (it != m_secureSocketSet.end()) {
		delete *it;
		m_secureSocketSet.erase(it);
	}
}
