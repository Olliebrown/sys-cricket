import debug from 'debug'
import udp from 'node:dgram'

// Debug loggers
const logServer = debug('streamer:server')
const logClient = debug('streamer:client')

// Address and port of the switch
const SERVER_PORT = 3000
const SERVER_ADDRESS = '192.168.50.91'

// Creating a udp socket
const server = udp.createSocket('udp4')

// Emits when any error occurs
server.on('error', (error) => {
  logServer.error(error)
  server.close()
})

// emits on new datagram msg
server.on('message', (msg, info) => {
  logServer(`Message received from ${info.address}:${info.port}`)
  console.log(msg.toString())
})

// Emits when socket is ready and listening for datagram msgs
server.on('listening', () => {
  const address = server.address()
  logServer(`Server is listening at ${address.address}:${address.port}`)

  // Sending connection message
  const client = udp.createSocket('udp4')
  client.send(address.port.toString(), SERVER_PORT, SERVER_ADDRESS, (error) => {
    if (error) {
      logClient.error('Client send error:')
      logClient.error(error)
    } else {
      logClient('Port sent')
    }
    client.close()
  })
})

// Emits after the socket is closed using socket.close();
server.on('close', () => {
  logServer('Socket is closed')
})

// Bind the socket to a random local port
server.bind()
