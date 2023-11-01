import socket
import logging
import pdb, sys

# Creating a logger object
logging.basicConfig(format='%(asctime)s %(levelname)-8s %(message)s', level=logging.DEBUG, datefmt='%Y-%m-%d %H:%M:%S')
logger = logging.getLogger(__name__.split('.')[0])

def socket_recv(sock, read_size):
    try:
        databuf = sock.recv(read_size)
        return databuf, None
    except socket.error as e:
        return None, e

def socket_create_listener(ip, port):
    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)

    # opt for quick port-reuse
    sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)

    addr = (ip, int(port))

    sock.bind(addr)
    sock.listen()

    return sock

def socket_connect(sock, ip, port):
    try:
        sock.connect((ip, int(port)))
        return None
    except socket.error as e:
        return e

class ProxySocket:
    def __init__(self, s1, s2):
        self.leg1 = s1
        self.leg2 = s2

    def on_data(self, sock, buf):
        target_leg = None
        is_leg1 = False
        if self.leg1 == sock:
            target_leg = self.leg2
            is_leg1 = False
        else:
            target_leg = self.leg1
            is_leg1 = True

        if target_leg == None:
            target_leg = self.confirm_connection(is_leg1)

        if target_leg != None:
            target_leg.sendall(buf)

        return target_leg

    def on_close(self, sock):
        other_leg = self.other_leg(sock)
        logger.debug("on_close: Closing other_leg {}".format(str(other_leg)))

        if None != other_leg:
            other_leg.close()
        return other_leg

    def other_leg(self, sock):
        if self.leg1 == sock:
            return self.leg2
        else:
            return self.leg1

    def get_closable_connections(self):
        closable = []

        if None != self.leg1:
            closable.append(self.leg1)

        if None != self.leg2:
            closable.append(self.leg2)

        return closable

    def close_connections(self):
        logger.debug("close-connections: Closing leg1 {}".format(str(self.leg1)))
        self.leg1.close()
        logger.debug("close-connections: Closing leg2 {}".format(str(self.leg1)))
        self.leg2.close()

        self.leg1 = None
        self.leg2 = None

    def confirm_connection(self, is_leg1):
        return None

    def __str__(self):
        str = "{} {}: leg1 {}, leg2 {}\n".format(__class__.__name__, hex(id(self)), self.leg1, self.leg2)
        return str

class OnTheFly(ProxySocket):
    def __init__(self, sock1, peer_ip, peer_port, gUDPMode):
        self.leg1 = sock1
        if gUDPMode == False:
            self.leg2 = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        else:
            self.leg2 = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        self.leg2.connect((peer_ip, int(peer_port)))
        self.peer_ip = peer_ip
        self.peer_port = int(peer_port)

        ProxySocket.__init__(self, self.leg1, self.leg2)

    def __str__(self):
        str = ProxySocket.__str__(self)

        str = str + "\nPeer => {}:{}\n".format(self.peer_ip, self.peer_port)

        return str

class OnTheFlyDeferredConnect(ProxySocket):
    def __init__(self, sock1, peer_ip, peer_port):
        self.leg1 = sock1
        self.leg2_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.leg2 = None
        self.peer_ip = peer_ip
        self.peer_port = int(peer_port)

        ProxySocket.__init__(self, self.leg1, self.leg2)

    def confirm_connection(self, is_leg1):
        if is_leg1:
            return None # we do not know how to reconnect the first leg, we do not have its peer endpoint

        connect_err = socket_connect(self.leg2_socket, self.peer_ip, self.peer_port)

        if connect_err != None:
            return None

        self.leg2 = self.leg2_socket

        return self.leg2

    def __str__(self):
        str = ProxySocket.__str__(self)

        str = str + "\nPeer => {}:{}\n".format(self.peer_ip, self.peer_port)

        return str

class HalfPermaConnectedSockets(ProxySocket):
    def __init__(self, perma_sock, other_sock):
        super().__init__(perma_sock, other_sock)

    def on_close(self, sock):
        return None

    def close_connections(self):
        logger.debug("close-connections: Only closing leg2 {}".format(str(self.leg2)))
        self.leg2.close()
        self.leg2 = None

    def __str__(self):
        str = "{} {}: leg1 {}, leg2 {}\n".format(__class__.__name__, hex(id(self)), self.leg1, self.leg2)
        return str

class HalfPerma:
    def __init__(self, sock1, peer_ip, peer_port, gUDPMode):
        self.leg1 = sock1
        self.peer_ip = peer_ip
        self.peer_port = peer_port
        self.proxy = None
        self.gUDPMode = gUDPMode

    def on_data(self, sock, buf):
        if self.leg1 == sock:
            if self.proxy == None:
                self.proxy = OnTheFly(self.leg1, self.peer_ip, self.peer_port, self.gUDPMode)

            logger.debug("Check proxy {}".format(self.proxy))
            return self.proxy.on_data(sock, buf)
        else:
            self.leg1.sendall(buf)
            return self.leg1

    def on_close(self, sock):
        if sock == self.leg1:
            logger.critical("Permanent side closed, should not be handled by this path!")
            sys.exit()
        else:
            self.proxy.on_close(self.leg1)

        return None

    def get_closable_connections(self):
        leg1 = self.leg1
        leg2 = None
        if self.proxy != None:
            leg2 = self.proxy.leg2
        return [leg1, leg2]

    def close_connections(self):
        self.proxy.on_close(self.leg1)

        self.proxy = None

    def __str__(self):
        str = "{} {}: leg1 {}, leg2 {}\npeer_ip {}, peer_port {}\n".format(__class__.__name__, hex(id(self)), self.leg1, self.proxy, self.peer_ip, self.peer_port)
        return str
