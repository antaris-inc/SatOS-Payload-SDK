import sys, os, socket
import logging
import pdb
import getopt
import socket_proxy
import select
import hexdump as HEX
import time

# Creating a logger object
logging.basicConfig(format='%(asctime)s %(levelname)-8s %(message)s', level=logging.DEBUG, datefmt='%Y-%m-%d %H:%M:%S')
logger = logging.getLogger(__name__.split('.')[0])

gAgentMode=None
gAgentPublicIp=None
gAgentPublicPort=None
gServerIp=None
gServerPort=None
gInternalPeerIP=None
gInternalPeerPort=None

gPermaSocket=None
gInternalListener=None
gWebListerer=None
gPeerConnectedOnInternalListener=None
gSleepBeforeConnect=5
gFlatSatMode=False
gFlatSatModeHandler=None
gUDPMode=False
g_UDP_Cmd_Size=223
udppacket=b""
g_MODE_USER="user"
g_MODE_ATMOS="atmos"
g_READ_SIZE=8192

g_VALID_MODES = [g_MODE_USER, g_MODE_ATMOS]

# map of action against fd
gActionMap = {}
gKnownSockets = []

def log_sockets(logger_fn, event_msg):
    global gActionMap
    global gKnownSockets
    global gPeerConnectedOnInternalListener

    logger_fn("\n{}\nAction-Map: {}\n".format(event_msg, gActionMap))
    logger_fn("\nKnown-sockets: {}\n\n".format(gKnownSockets))
    logger_fn("\nPeer-connected-on-internal-sock {}".format(gPeerConnectedOnInternalListener))

def print_usage():
    global gAgentMode
    global gAgentPublicIp
    global gAgentPublicPort
    global gServerIp
    global gServerPort
    global gInternalPeerIP
    global gInternalPeerPort

    logger.error("{}: Usage".format(sys.argv[0]))
    logger.error("{} -m/--mode atmos|user -i/--web-public-ip WEB-PUBLIC-IP -p/--web-public-port WEB_PUBLIC_PORT -s/--internal-server-ip SERVER_IP -t/--internal-server-port SERVER_PORT -l/--local-peer-ip LOCAL_PEER_SERVER_IP -o/--local-peer-port LOCAL_PEER_SERVER_PORT [-f/--flat-sat-mode] [-u/--udp-mode] [-h/--help]".format(sys.argv[0], gServerIp))
    logger.error("Flat sat mode is optional and tcp socket will be used by default")
    logger.error("Udp Socket is only available in flat sat mode specify packet size after flag")

def print_params():
    global gAgentMode
    global gAgentPublicIp
    global gAgentPublicPort
    global gServerIp
    global gServerPort
    global gInternalPeerIP
    global gInternalPeerPort
    global gFlatSatMode
    global gUDPMode

    logger.info("\n\n{}: working with parameters".format(sys.argv[0]))
    logger.info("MODE={}".format(gAgentMode))
    logger.info("FlatSat Mode={}".format(gFlatSatMode))
    logger.info("FlatSat Mode UDP={}".format(gUDPMode))
    logger.info("PUBLIC-IP={}".format(gAgentPublicIp))
    logger.info("PUBLIC-PORT={}".format(gAgentPublicPort))
    logger.info("INTERNAL-SERVER-IP={}".format(gServerIp))
    logger.info("INTERNAL-SERVER-PORT={}".format(gServerPort))
    logger.info("LOCAL-PEER-IP:{}".format(gInternalPeerIP))
    logger.info("LOCAL-PEER-PORT:{}\n\n".format(gInternalPeerPort))

def parse_opts():
    global gAgentMode
    global gAgentPublicIp
    global gAgentPublicPort
    global gServerIp
    global gServerPort
    global g_VALID_MODES
    global gInternalPeerIP
    global gInternalPeerPort
    global gFlatSatMode
    global gUDPMode

    argv = sys.argv[1:]
    print("Got args: {}".format(argv))
    try:
      opts, args = getopt.getopt(argv, "hm:i:p:s:t:l:o:fu",["help", "mode=", "web-public-ip=", "web-public-port=", "internal-server-ip=", "internal-server-port", "local-peer-ip", "local-peer-port", "flat-sat-mode", "udp-mode"])
    except getopt.GetoptError:
      logger.critical ('Error parsing arguments')
      print_usage()
      sys.exit(-1)

    for opt, arg in opts:
        logger.info("Parsing option {} with value {}".format(opt, arg))
        if opt in ('-h', "--help"):
            print_usage()
            sys.exit()
        elif opt in ("-m", "--mode"):
            if arg in g_VALID_MODES:
                gAgentMode = arg
            else:
                logger.critical("{} is not a valid mode".format(arg))
                print_usage()
                sys.exit()
        elif opt in ("-i", "--web-public-ip"):
            gAgentPublicIp = arg
        elif opt in ("-p", "--web-public-port"):
            gAgentPublicPort = int(arg)
        elif opt in ("-s", "--internal-server-ip"):
            gServerIp = arg
        elif opt in ("-t", "--internal-server-port"):
            gServerPort = int(arg)
        elif opt in ("-l", "--local-peer-ip"):
            gInternalPeerIP = arg
        elif opt in ("-o", "--local-peer-port"):
            gInternalPeerPort = int(arg)
        elif opt in ("-f", "--flat-sat-mode"):
            gFlatSatMode = True
        elif opt in ("-u", "--udp-mode"):
            gUDPMode = True
            if arg:
                g_UDP_Cmd_Size = int(arg)

    if None == gAgentMode:
        logger.critical("Compulsory parameter mode missing")
        print_usage()
        sys.exit(-1)

    if None == gAgentPublicIp:
        logger.critical("Compulsory parameter Public-IP missing")
        print_usage()
        sys.exit(-1)

    if None == gAgentPublicPort:
        logger.critical("Compulsory parameter Public-port missing")
        print_usage()
        sys.exit(-1)

    if None == gServerIp and gFlatSatMode == False:
        logger.critical("Compulsory parameter Internal-IP missing")
        print_usage()
        sys.exit(-1)

    if None == gServerPort and gFlatSatMode == False:
        logger.critical("Compulsory parameter Internal-port missing")
        print_usage()
        sys.exit(-1)

    if None == gInternalPeerIP:
        logger.critical("Compulsory parameter Local-Peer-IP missing")
        print_usage()
        sys.exit(-1)

    if None == gInternalPeerPort:
        logger.critical("Compulsory parameter Local-Peer-port missing")
        print_usage()
        sys.exit(-1)
    
    if False == gFlatSatMode and True == gUDPMode:
        logger.critical("UDP mode can only be used with flat sat mode")
        print_usage()
        sys.exit(-1)

def setup_permanent_socket(skip_bind = False):
    global gAgentMode
    global gAgentPublicIp
    global gAgentPublicPort
    global gPermaSocket
    global gWebListerer
    global gActionMap
    global gKnownSockets
    global g_MODE_ATMOS
    global g_MODE_USER
    global gInternalPeerIP
    global gInternalPeerPort

    addr = (gAgentPublicIp, gAgentPublicPort)

    logger.info("Setting up permasock: {}".format(addr))

    if gAgentMode == g_MODE_ATMOS:

        if skip_bind == False:
            gWebListerer = socket_proxy.socket_create_listener(gAgentPublicIp, gAgentPublicPort)

            # Add web-listener to known-sockets
            gKnownSockets.append(gWebListerer)

        logger.info("Waiting for user-agent connect")
        gPermaSocket, peer_addr = gWebListerer.accept()

        logger.info("ATMOS-MODE: perma-socket {}, fd {} created with peer {}".format(gPermaSocket, gPermaSocket.fileno(), peer_addr))

        if gFlatSatModeHandler != None:
            gFlatSatModeHandler.leg1 = gPermaSocket
            if gFlatSatModeHandler.leg1 == None:
                gFlatSatModeHandler.leg1 = gPermaSocket
            else:
                gFlatSatModeHandler.leg2 = gPermaSocket

            gActionMap[gPermaSocket.fileno()] = gFlatSatModeHandler
            logger.debug("After Recreating handler {}".format(gFlatSatModeHandler))

    else:
        gPermaSocket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)

        logger.info("Connecting to atmos-agent, will retry till this works")
        while True:
            time.sleep(gSleepBeforeConnect)
            connect_err = socket_proxy.socket_connect(gPermaSocket, gAgentPublicIp, gAgentPublicPort)

            if connect_err == None:
                break

        logger.info("USER-MODE: perma-socket {}, fd {} created with peer {}".format(gPermaSocket, gPermaSocket.fileno(), addr))

    # Add perma-sock to known sockets
    gKnownSockets.append(gPermaSocket)

    # log_sockets(logger.info, "Completed perma-sock setup")

def setup_internal_listener():
    global gAgentMode
    global gAgentPublicIp
    global gServerIp
    global gServerPort
    global gInternalListener
    global gActionMap
    global gKnownSockets

    if gFlatSatMode == True:
        if g_MODE_ATMOS == gAgentMode:
            gInternalListener = socket_proxy.socket_create_listener(gServerIp, gServerPort)
            logger.info("Set up internal server at {}:{}".format(gServerIp, gServerPort))
            logger.info("Will mimic Flat sat")
            gKnownSockets.append(gInternalListener)
    else:
        gInternalListener = socket_proxy.socket_create_listener(gServerIp, gServerPort)
        logger.info("Set up internal server at {}:{}".format(gServerIp, gServerPort))
        if g_MODE_USER == gAgentMode:
            logger.info("Will mimic PC")
        else:
            logger.info("Will mimic Payload App")
        gKnownSockets.append(gInternalListener)

    # log_sockets(logger.info, "Setup internal-listener completed")

def install_permanent_handler():
    global gServerIp
    global gServerPort
    global gInternalListener
    global gActionMap
    global gKnownSockets
    global g_MODE_ATMOS
    global g_MODE_USER
    global gInternalPeerIP
    global gInternalPeerPort

    # on the user side, make a HalfPerma connecting perma-socket to the app's callback listener
    if g_MODE_USER == gAgentMode:
        logger.debug("USER-MODE: setting up a permanent handler for permasock")
        half_perma = socket_proxy.HalfPerma(gPermaSocket, gInternalPeerIP, gInternalPeerPort, gUDPMode)
        gActionMap[gPermaSocket.fileno()] = half_perma

        logger.debug("Created permanent half-perma handler {}".format(str(half_perma)))
    else:
        logger.debug("ATMOS-MODE: does not require permanent handler for permasock")

    # log_sockets(logger.info, "Setup of permanent handlers completed")

def recover_perma_sock():
    global gAgentMode
    global gAgentPublicIp
    global gAgentPublicPort
    global gPermaSocket
    global gInternalListener
    global gActionMap
    global gInternalPeerIP
    global gInternalPeerPort
    global gKnownSockets

    gKnownSockets.remove(gPermaSocket)

    if gPermaSocket.fileno() in gActionMap:
        del gActionMap[gPermaSocket.fileno()]

    gPermaSocket.close()

    setup_permanent_socket(True)

    install_permanent_handler()

def handle_exceptions(sock):
    global gAgentMode
    global gAgentPublicIp
    global gAgentPublicPort
    global gPermaSocket
    global gInternalListener
    global gActionMap
    global gInternalPeerIP
    global gInternalPeerPort
    global gKnownSockets
    global g_READ_SIZE
    global g_MODE_ATMOS
    global g_MODE_USER
    global gPeerConnectedOnInternalListener
    global gFlatSatMode

    if gFlatSatMode == True and gAgentMode == g_MODE_ATMOS:
        if gPermaSocket == sock:
            logger.warning("Permanent socket closed")

            logger.info("Mode {} gPermaSock closed, cleaning up state and waiting for USER-AGENT reconnect".format(gPermaSocket))
            handler = gActionMap[sock.fileno()]

            logger.info("Connection handled by handler {}".format(str(handler)))

            if handler.leg1 == gPermaSocket:
                handler.leg1 = None
            else:
                handler.leg2 = None

            recover_perma_sock()

            log_sockets(logger.info, "After perma-sock recovery")
            logger.debug("handler after permasock disconnect {}".format(handler))

            return

    if gPermaSocket == sock:
        logger.warning("Permanent socket closed")

        logger.info("Mode {} gPermaSock closed, cleaning up state and waiting for USER-AGENT reconnect".format(gPermaSocket))
        handler = gActionMap[sock.fileno()]

        logger.info("Connection handled by handler {}".format(str(handler)))

        connection_legs = handler.get_closable_connections()

        # close the other leg, not the perma one, which we will eventually recover

        if connection_legs[0] == gPermaSocket:
            other_sock = connection_legs[1]
        else:
            other_sock = connection_legs[0]

        if other_sock == gPeerConnectedOnInternalListener:
            gPeerConnectedOnInternalListener = None

        if None != other_sock:
            logger.info("Cleaning up socket {}".format(other_sock))
            del gActionMap[other_sock.fileno()]
            other_sock.close()
            gKnownSockets.remove(other_sock)
        else:
            logger.info("Other sock is None {}, skipping cleanup".format(other_sock))

        recover_perma_sock()

        log_sockets(logger.info, "After perma-sock recovery")

        return

    handler = gActionMap[sock.fileno()]

    logger.debug("Detected exception on {}".format(sock))

    logger.debug("Handler for this sock is {}".format(str(handler)))

    cleanup_sock_list = handler.get_closable_connections()

    logger.debug("Following sockets require cleanup => {}".format(cleanup_sock_list))

    for s in cleanup_sock_list:
        if gPermaSocket == s:
            logger.debug("Detected Perma-sock {} in cleanup list, will close and recover".format(s))
            recover_perma_sock()
        else:
            del gActionMap[s.fileno()]
            gKnownSockets.remove(s)
            s.close()

        if gPeerConnectedOnInternalListener == s:
            gPeerConnectedOnInternalListener = None

    # pdb.set_trace()

    log_sockets(logger.info, "After exception handling ==> \n")


def handle_udp(sock ,databuf):
    global udppacket
    while len(databuf) > 0:

        if (len(udppacket) + len(databuf)) == g_UDP_Cmd_Size:
            #if total length is equal to command size append and send buffer
            udppacket += databuf
            databuf = b""

        elif (len(udppacket) + len(databuf)) < g_UDP_Cmd_Size:
            #if total length is less than command size append and save buffer
            udppacket += databuf
            return
        
        else:
            #if total length is more then append bytes required and send buffer
            n = g_UDP_Cmd_Size - len(udppacket) 
            udppacket += databuf[:n]
            databuf = databuf[n:]
        
        logger.debug("\n\n >>>>>>>>>>>>> =====> DATA packet {} bytes for udp sock {} ===>\n\n{}\n".format(len(udppacket), sock, str(HEX.hexdump(udppacket))))
        handler = gActionMap[sock.fileno()]
        logger.debug("Handler for this READ is {}".format(str(handler)))
        forwarded_leg_sock = handler.on_data(sock, udppacket)

        if forwarded_leg_sock == None:
            logger.warn("Handler {} forced to drop data without forwarding".format(str(handler)))
            udppacket = []
            return

        logger.debug("Handler has forwarded to socket {}".format(str(forwarded_leg_sock)))
        udppacket = b""

    forwarded_sock_no = forwarded_leg_sock.fileno()
    if forwarded_sock_no not in gActionMap:
        gKnownSockets.append(forwarded_leg_sock)
        gActionMap[forwarded_sock_no] = handler

    return


def handle_readable(sock):
    global gAgentMode
    global gAgentPublicIp
    global gAgentPublicPort
    global gPermaSocket
    global gInternalListener
    global gActionMap
    global gInternalPeerIP
    global gInternalPeerPort
    global g_READ_SIZE
    global g_MODE_ATMOS
    global g_MODE_USER
    global gPeerConnectedOnInternalListener
    global gFlatSatModeHandler

    socket_has_error = False

    logger.debug("{} has readable event".format(sock))

    if sock.fileno() in gActionMap:
        # must be readable for data
        databuf, error = socket_proxy.socket_recv(sock, g_READ_SIZE)

        if None == error and databuf != None:
            logger.info("\n\n >>>>>>>>>>>>> =====> DATA {} bytes from sock {} ===>\n\n{}\n".format(len(databuf), sock, str(HEX.hexdump(databuf))))
        else:
            logger.error("Got error {} on sock {}".format(error, sock))
            socket_has_error = True

        if socket_has_error or None == databuf or 0 == len(databuf):
            handle_exceptions(sock)
        else:

            if sock.fileno() == gPermaSocket.fileno() and gUDPMode == True and gAgentMode == g_MODE_USER:
                #handle udp socket in user mode
                handle_udp(sock, databuf)

            else:
                handler = gActionMap[sock.fileno()]
                logger.debug("Handler for this READ is {}".format(str(handler)))
                forwarded_leg_sock = handler.on_data(sock, databuf)

                if forwarded_leg_sock == None:
                    logger.warn("Handler {} forced to drop data without forwarding".format(str(handler)))
                    return

                logger.debug("Handler has forwarded to socket {}".format(str(forwarded_leg_sock)))

                forwarded_sock_no = forwarded_leg_sock.fileno()

                if forwarded_sock_no not in gActionMap:
                    gKnownSockets.append(forwarded_leg_sock)
                    gActionMap[forwarded_sock_no] = handler

    elif sock.fileno() == gPermaSocket.fileno():
        # should only happen on a perma-disconnect before previous usage, otherwise we should have been in the action-map
        databuf, error = socket_proxy.socket_recv(sock, g_READ_SIZE)

        if None == error and databuf != None:
            logger.info("\n\n >>>>>>>>>>>>> =====> DATA {} bytes from perma-sock {} ===>\n\n{}\n".format(len(databuf), sock, str(HEX.hexdump(databuf))))
        else:
            logger.error("Got error {} on perma-sock {}".format(error, sock))
            socket_has_error = True

        logger.warning("Mode {}: perma-sock received data without handler, potential sync problem ".format(g_MODE_ATMOS))

        logger.warning("Dropping and recovering perma-sock")

        recover_perma_sock()
    else:
        # a new connection?
        if g_MODE_USER == gAgentMode:
            # we can only get connections from the app
            if sock == gInternalListener:
                logger.debug("USER-AGENT: internal-listener {} has readable event".format(sock))

                newsock, addr = gInternalListener.accept()

                new_handler = socket_proxy.OnTheFly(newsock, gAgentPublicIp, gAgentPublicPort)

                other_leg = new_handler.leg2

                gActionMap[newsock.fileno()] = new_handler
                gActionMap[other_leg.fileno()] = new_handler

                gKnownSockets.append(newsock)
                gKnownSockets.append(other_leg)

                logger.debug("USER: got internal connect from {}, created on-the-fly handler {}".format(addr, str(new_handler)))

                log_sockets(logger.info, "USER-MODE, after accepting internal listener connection ====>\n")
            else:
                logger.critical("In USER-MODE, sock {}, fd {} does not have handler and is not the internal server endpoint".format(sock, sock.fileno()))
                sys.exit()
        else:
            # we can get connections from the app or the user-agent
            if sock == gInternalListener:
                logger.debug("ATMOS-AGENT: internal-listener {} has readable event".format(sock))

                newsock, addr = gInternalListener.accept()

                if gPeerConnectedOnInternalListener == None:
                    new_handler = socket_proxy.HalfPermaConnectedSockets(gPermaSocket, newsock)

                    gActionMap[newsock.fileno()] = new_handler
                    gActionMap[gPermaSocket.fileno()] = new_handler
                    gFlatSatModeHandler = new_handler

                    gKnownSockets.append(newsock)
                    # gPermaSock is always in known-sockets
                    # gKnownSockets.append(gPermaSocket)

                    gPeerConnectedOnInternalListener = newsock

                    logger.debug("ATMOS: got internal connect from {}, created half-perma-connected-socks handler {}".format(addr, str(new_handler)))

                    log_sockets(logger.info, "AGENT-MODE, after accepting internal listener connection ====>\n")
                else:
                    logger.error("ATMOS-AGENT: received connect on internal-listener, but we were already connected {}".format(gPeerConnectedOnInternalListener))
                    logger.error("ATMOS-AGENT: second connection on same port came from {}, connect {}".format(addr, newsock))

                    logger.error("ATMOS-AGENT: as recovery attempt, dropping both old and new connections, and recovering permasock")

                    log_sockets(logger.info, "AGENT-MODE, before error-recovery ====>\n")

                    logger.debug("Recovering perma-sock")

                    handle_exceptions(gPermaSocket)

                    if gPeerConnectedOnInternalListener != None:
                        logger.warn("ATMOS-AGENT: recovering perma-sock still did not clear out connected-peer on internal listener {}".format(gPeerConnectedOnInternalListener))
                        handle_exceptions(gPeerConnectedOnInternalListener)
                    else:
                        logger.info("ATMOS-AGENT: peer-connected on internal listener got cleared out as part of perma-sock cleanup. This was expected.")

                    logger.debug("ATMOS-AGENT: dropping the new connection {}".format(newsock))
                    newsock.close()

                    log_sockets(logger.info, "AGENT-MODE, after completed error-recovery for double-connect on internal listener ====>\n")

            elif sock == gWebListerer:
                logger.debug("ATMOS-AGENT: web-listener {} has readable event".format(sock))

                newsock, addr = gWebListerer.accept()
                new_handler = socket_proxy.OnTheFlyDeferredConnect(newsock, gInternalPeerIP, gInternalPeerPort)

                gActionMap[newsock.fileno()] = new_handler
                gKnownSockets.append(newsock)

                # try to obtain the second leg by connecting immediately
                other_leg = new_handler.confirm_connection(False)

                if other_leg != None:
                    gActionMap[other_leg.fileno()] = new_handler
                    gKnownSockets.append(other_leg)
                else:
                    logger.warn("ATMOS: could not connect to local peer's server endpoint on incoming Web connect")

                logger.debug("ATMOS: got internal connect from {}, created on-the-fly-deffered-connect handler {}".format(addr, str(new_handler)))

                log_sockets(logger.info, "AGENT-MODE, after accepting web-connection connection ====>\n")

            else:
                logger.critical("In atmos-mode, sock {}, fd {} does not have handler and is not the internal server endpoint".format(sock, sock.fileno()))
                sys.exit()

def worker():
    global gPermaSocket
    global gInternalListener
    global gActionMap
    global gKnownSockets

    while True:
        reader_fds = gKnownSockets
        except_fds = gKnownSockets
        saw_perma_sock_events = False

        log_sockets(logger.debug, "\n\nSetting up for SELECT ===>\n")

        try:
            rd_set, write_set, except_set = select.select(reader_fds, [], except_fds)
        except select.error as e:
            logger.error("Select exception {}".format(e))
            break
        except socket.error as e:
            logger.error("Socket error {}".format(e))
            break

        logger.debug('READ-SET: {}'.format(rd_set))
        logger.debug('EXCEPT-SET: {}'.format(except_set))

        logger.debug("Handling permasock from both sets if present {}".format(gPermaSocket))

        if gPermaSocket in rd_set:
            logger.debug("Perma-socket has data, handling it first")
            rd_set.remove(gPermaSocket)
            handle_readable(gPermaSocket)
            saw_perma_sock_events = True

        if gPermaSocket in except_set:
            logger.debug("Perma-socket has exception, handling it first ")
            except_set.remove(gPermaSocket)
            handle_exceptions(gPermaSocket)
            saw_perma_sock_events = True

        logger.debug('READ-SET - (after permasock handling): {}'.format(rd_set))
        logger.debug('EXCEPT-SET - (after permasock handling): {}'.format(except_set))

        if saw_perma_sock_events:
            logger.debug("Handled perma-sock event, redoing from select")
            continue

        for r in rd_set:
            logger.debug("Processing readable socket {}".format(r))
            # sometimes, earlier processing may have closed a socket that we are now trying to process in a subsequent iteration of this loop
            if r.fileno() == -1:
                logger.warning("Socket {} probably closed in an earlier iteration".format(r))
            else:
                handle_readable(r)

        for e in except_set:
            logger.debug("Processing exception on socket {}".format(e))
            # sometimes, earlier processing may have closed a socket that we are now trying to process in a subsequent iteration of this loop
            if e.fileno() == -1:
                logger.warning("Socket {} probably closed in an earlier readable processing or exception iteration)".format(e))
            else:
                handle_exceptions(e)

# Main function
if __name__ == '__main__':

    parse_opts()
    print_params()
    setup_permanent_socket(False)
    setup_internal_listener()

    install_permanent_handler()

    worker()
