import debug from 'debug'
import udp from 'node:dgram'

// Debug loggers
const logServer = debug('streamer:server')
const logClient = debug('streamer:client')

// Address and port of the switch
const SERVER_PORT = 3000
const SERVER_ADDRESS = '192.168.50.91'

// Data to request (Current health)
const currentHealthData = {
  nickname: 'currentHealth',
  offsets: [0x45d6750, 0x480, 0x130, 0x1f0, 0x150, 0x198],
  dataType: 'i32',
  dataCount: 1,
  nsInterval: 1e9 // 1 second
}

const currentStaminaData = {
  nickname: 'currentStamina',
  offsets: [0x4649c10, 0x40, 0x934],
  dataType: 'f32',
  dataCount: 1,
  nsInterval: 5e8 // 1/2 second
}

function wait (ms) {
  return new Promise((resolve) => setTimeout(resolve, ms))
}

function sendConfigMessage (message) {
  // Sending connection message
  const client = udp.createSocket('udp4')
  client.send(JSON.stringify(message), SERVER_PORT, SERVER_ADDRESS, (error) => {
    if (error) {
      logClient.error('Client send error:')
      logClient.error(error)
    } else {
      logClient(`Sent message: ${JSON.stringify(message)}`)
    }
    client.close()
  })
}

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
server.on('listening', async () => {
  const address = server.address()
  logServer(`Server is listening at ${address.address}:${address.port}`)

  // Sending connection message
  sendConfigMessage({
    type: 'connect',
    port: address.port
  })
  await wait(500)
  sendConfigMessage({
    type: 'start',
    port: address.port,
    ...currentHealthData
  })
  await wait(500)
  sendConfigMessage({
    type: 'start',
    port: address.port,
    ...currentStaminaData
  })
  await wait(10000)
  sendConfigMessage({
    type: 'stop',
    port: address.port,
    nickname: currentHealthData.nickname
  })
  await wait(5000)
  sendConfigMessage({
    type: 'stop',
    port: address.port,
    nickname: currentStaminaData.nickname
  })
  await wait(100)
  sendConfigMessage({ type: 'disconnect', port: address.port })
  await wait(100)
  process.exit(0)
})

// Emits after the socket is closed using socket.close();
server.on('close', () => {
  logServer('Socket is closed')
})

// Bind the socket to a random local port
server.bind()
