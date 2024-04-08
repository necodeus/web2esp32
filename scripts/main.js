const isDev = window.location.port === "5500"

const HOST = isDev ? "192.168.1.163" : window.location.hostname
const PORT = isDev ? "80" : window.location.port

const websocket = new WebsocketManager(`ws://${HOST}:${PORT}/ws`)
websocket.init()

const terminal = new TerminalManager({
    wsManager: websocket,
    inputSelector: '#terminal-input',
    listSelector: '#terminal-list',
})

const sdcard = new SdCardManager({
    host: HOST,
    port: PORT,
    currentPathSelector: '#current-path',
})
