#pragma once

#include "util.h"
#include "ChainedCallback.h"

#include <WiFiServer.h>
#include <WiFiClient.h>

#include <vector>

class WifiServer: public WiFiServer {

    public:

        static inline constexpr uint8 kMaxBacklog     = 4; //number of connections queued up
        static inline constexpr uint8 kMaxConnections = 2; //number of connections managed by server

        static inline constexpr uint32 kDisconnectedConnectionId = 0;

    public:

        struct Connection {

            struct OnReadArgs;
            using OnReadCallback = void(*)(OnReadArgs& args);

            private:
                friend class WifiServer;
                uint32 id = kDisconnectedConnectionId;

                Connection() {}
                Connection(WiFiClient client) : client(client) {}

            public:

                WiFiClient client;
                ChainedCallback<OnReadCallback> onRead;  
                std::vector<uint8> buffer;

                uint32 Id() { return id; }
        };

        struct Connection::OnReadArgs {
            WifiServer& server;
            Connection& connection;
            size_t bytesRead;
        };


        using ConnectionCallback = void(*)(WifiServer& server, Connection& connection);
        ChainedCallback<ConnectionCallback> onConnect; 
        ChainedCallback<ConnectionCallback> onDisconnect; 

    protected:
        
        Connection connections[kMaxConnections];
        uint8 connectedClientCount = 0;

        uint32 currentConnectionId = kDisconnectedConnectionId;
        uint32 getConnectionId() {
            
            // TODO: runtime on this algorithm is trash in worst case, but should be
            //       O(1) in practice... replace this with algorithm which is fast in worst case too
            for(uint32 id = currentConnectionId+1; id != currentConnectionId; ++id) {

                if(UNLIKELY(id == kDisconnectedConnectionId)) continue;

                bool idInUse = false;
                for(const Connection& connection : connections) {
                    if(id == connection.id) {
                        idInUse = true;
                        break;
                    }
                }

                if(LIKELY(!idInUse)) {
                    currentConnectionId = id;
                    return id;
                }
            }

            return kDisconnectedConnectionId;
        }

    public:

        WifiServer(): WiFiServer(0) {}

        uint8 ConnectedClientCount() const { return connectedClientCount; }

        static const char* TcpStateString(wl_tcp_state state) {
            switch(state) {
                case CLOSED:      return "CLOSED";
                case LISTEN:      return "LISTEN";
                case SYN_SENT:    return "SYN_SENT";
                case SYN_RCVD:    return "SYN_RCVD";
                case ESTABLISHED: return "ESTABLISHED";
                case FIN_WAIT_1:  return "FIN_WAIT_1";
                case FIN_WAIT_2:  return "FIN_WAIT_2";
                case CLOSE_WAIT:  return "CLOSE_WAIT";
                case CLOSING:     return "CLOSING";
                case LAST_ACK:    return "LAST_ACK";
                case TIME_WAIT:   return "TIME_WAIT";
                default:          return "INVALID";
            }
        }

        inline wl_tcp_state TcpState() {
            return wl_tcp_state(status());
        }

        // TODO: replace all PrintStatus functions with more generic 'ToString' 
        void PrintStatus() {
            Log(
                "WifiServer Status {\n"
                "\tIP: %s\n"
                "\tTCP Port: %d\n"
                "\tTCP State: %s\n"
                "\tConnections: %d/%d\n"
                "\tHas Client: %s\n"
                "\tMax Backlog: %d\n"
                "}\n",

                _addr.toString().c_str(),
                _port,
                TcpStateString(TcpState()),
                connectedClientCount, kMaxConnections,
                hasClient() ? "True" : "False",
                kMaxBacklog
            );
        }

        void Init(uint16 port, 
                  ConnectionCallback onConnectCallback = nullptr,
                  ConnectionCallback onDisconnectCallback = nullptr) {

            if(onConnectCallback) onConnect.Append(onConnectCallback);
            if(onDisconnectCallback) onConnect.Append(onDisconnectCallback);

            begin(port, kMaxBacklog);

            PrintStatus();
        }

        void Broadcast(const char* data, size_t bytes) {
            for(Connection& connection : connections) {
                if(connection.client) {
                    connection.client.write(data, bytes);
                }
            }
        }

        void Update() {
            
            for(Connection& connection : connections) {

                if(!connection.client) {
        
                    // Disconnect old client
                    if(connection.id != kDisconnectedConnectionId) {
                        --connectedClientCount;
                        onDisconnect(*this, connection);
                        connection.id = kDisconnectedConnectionId;
                    }

                    //Check for new connection
                    // Note: quick out check for when `available()` yields 1ms when there is no `_unclaimed` 
                    //       connection to to connect to 
                    if(!_unclaimed) continue;
                    
                    connection = Connection(available());

                    // Note: we don't use `client.connected()` just in case connection is still in the 
                    //       initial stages where connected() would return false.
                    if(connection.client.status() == CLOSED) {

                        // No available client, move on
                        Warn("_unclaimed client was closed, why?");
                        continue;
                    }

                    // Get client id
                    connection.id = getConnectionId();
                    if(UNLIKELY(connection.id == kDisconnectedConnectionId)) {

                        Warn("Failed to get connection Id ... disconnecting remoteIp: %s:%d | localIp: %s:%d | ClientStatus: %d",
                            connection.client.remoteIP().toString().c_str(), connection.client.remotePort(),                            
                            connection.client.localIP().toString().c_str(), connection.client.localPort(),                                         
                            connection.client.status()
                        );

                        // Note: abort doesn't wait for TCP ACK from client like stop does... just cuts them off and frees up heap memory
                        connection.client.abort();
                        continue;
                    }

                    //Invoke onConnect callback
                    ++connectedClientCount;
                    onConnect(*this, connection);
                 }

                //check for data
                size_t availableBytes = connection.client.peekAvailable();
                if(!availableBytes) continue;
            
                //allocate the new buffer
                size_t oldBufferSize = connection.buffer.size();
                size_t newBufferBytes = oldBufferSize + availableBytes;
                
                uint32 freeHeapBytes = ESP.getFreeHeap();
                if(newBufferBytes > freeHeapBytes) {

                    Warn("Skipping buffer reallocation of size: %u for connection: %s. Free Heap Bytes: %u", 
                         newBufferBytes,
                         connection.client.remoteIP().toString().c_str(),
                         freeHeapBytes
                    );

                    continue;
                }
                
                // TODO: Even with sanity check above I still don't like using new when exceptions are turned off. 
                //       Think of way to get stdlib to use placement new or no-throw new
                connection.buffer.resize(newBufferBytes);
                
                //read in the buffer
                size_t bytesRead = connection.client.read(&connection.buffer[oldBufferSize], availableBytes);
                connection.buffer.resize(oldBufferSize + bytesRead);

                //invoke onRead callback
                if(bytesRead) {

                    Connection::OnReadArgs args = {
                        .server = *this,
                        .connection = connection,
                        .bytesRead = bytesRead
                    };

                    connection.onRead(args);
                }
            }
        }
};
