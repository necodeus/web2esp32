class WebsocketManager {
    constructor(host) {
        this.ws = null
        this.host = host
    }

    init() {
        console.log('WebSocket connection initializing');
        this.ws = new WebSocket(this.host);

        this.ws.onopen = function () {
            console.log('WebSocket connection opened');
        };

        this.ws.onclose = function () {
            setTimeout(function () {
                this.ws = new WebSocket(this.host);
            }, 500);
        };

        return this.ws
    }

    send(command) {
        if (this.ws.readyState !== WebSocket.OPEN) {
            console.log('WebSocket not open');
            return;
        }

        this.ws.send(command);
    }
}
